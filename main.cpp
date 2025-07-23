// Copyright (C) 2025 initMayday. This is available under GPLv3-or-later. See LICENSE.md

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <sys/inotify.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/wait.h>
#include <csignal>

#include "config.h"

const std::filesystem::path monitored_path = "/run/user";
std::map<int, int> uid_pid_map;

std::string get_env_var(const std::string& var) {
    const char* val = std::getenv(var.c_str());
    return val ? std::string(val) : std::string{};
}

// MUST RUN AS ROOT, GETS ENV VARS FOR SPECIFIED USER
std::unordered_map<std::string, std::string> get_env_vars(passwd* pw) {
    std::unordered_map<std::string, std::string> env_vars;
    env_vars.insert({"XDG_RUNTIME_DIR", std::string("/run/user/" + std::to_string(pw->pw_uid))}); // Arbitrary env var we need

    std::string command = "su - " + std::string(pw->pw_name) + " -c 'env'";
    //std::cout << "COMMAND: " << command << std::endl;
    std::array<char, 4096> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Couldn't start command." << std::endl;
        return env_vars;
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != NULL) {
        result += buffer.data();
    }

    int return_code = pclose(pipe);
    if (return_code == -1) {
        // If pclose fails with "no child processes", this is expected behaviour, as our reap_child function handles it before
        // pclose can, and therefore, pclose is unable to. This log is kept here, incase pclose fails for a real reason.
        perror("[WARNING] Pclose failed, but this is likely because we reaped the process before it could, so it is a non-issue");
    } else if (return_code != 0) {
        // ERROR OCCURED! This should satisfice, but unideal.
        std::cerr << "[ERROR] Failed to get env vars for: " << pw->pw_name << std::endl;
        return env_vars;
    }

    std::istringstream iss(result);
    std::string line;
    while (std::getline(iss, line)) {  // getline() removes the \n
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1); // Go one past to skip the '='
            env_vars.insert_or_assign(key, value);
        }
    }
    return env_vars;
}

void handle_user(int uid) {
    std::cout << "[LOG] Handling: " << uid << std::endl;

    // Check UID is valid
    passwd* pw = getpwuid(uid);
    if (pw == nullptr) {
        std::cerr << "[ERROR] UID of " << uid << " is invalid!" << std::endl;
        return;
    }

    // Confirm there is no dinit process running
    std::string command = "pgrep -x -u " + std::to_string(uid) + " dinit > /dev/null"; 
    int execute_success = system(command.c_str());
    if (execute_success == -1) {
        std::cerr << uid << " [ERROR] System call failed, return code: " << execute_success << std::endl;
        return;
    }
    int exit_code = WEXITSTATUS(execute_success);
    if (exit_code == 0) {
        std::cerr << uid << " [ERROR] There was already a dinit process running!" << std::endl;
        return;
    } else if (exit_code > 1) {
        std::cerr << uid << " [ERROR] pgrep returned exit_code: " << exit_code << std::endl;
        return;
    }

    std::unordered_map<std::string, std::string> env_vars = get_env_vars(pw);

    // UID is valid
    pid_t pid = fork(); // Returns 0 to child process, returns child's PID to parent process
    std::string process_id = std::to_string(getpid());
    std::string struid = std::to_string(uid);
    if (pid == -1) {
        std::cerr << struid << " [ERROR] Failed to fork!" << std::endl;
        return;
    }

    // Only continue if we are the child process - We must use exit from now on beyond this if
    if (pid != 0) {
        // Add pid to UID map
        uid_pid_map.insert({uid, pid});
        return;
    }
    
    // Clear the old environment, to avoid conflicting variables
    clearenv();

    // Swap to the user
    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        perror(std::string(struid + " [ERROR] initgroups failed").c_str());
        exit(EXIT_FAILURE);
    }
    if (setgid(pw->pw_gid) != 0) {
        perror(std::string(struid + " [ERROR] setgid failed").c_str());
        exit(EXIT_FAILURE);
    }
    if (setuid(uid) != 0) {
        perror(std::string(struid + " [ERROR] setuid failed").c_str());
        exit(EXIT_FAILURE);
    }

    // Check we have fully swapped, to prevent privilege escalation issues
    if (geteuid() != pw->pw_uid || getegid() != pw->pw_gid) {
        perror(std::string(struid + " [ERROR] Failed to drop privileges").c_str());
        exit(EXIT_FAILURE);
    }

    std::cout << process_id << " [LOG] Running as: " << getpwuid(getuid())->pw_name << std::endl;

    // Set new environment
    for (const auto& pair: env_vars) {
        setenv(pair.first.c_str(), pair.second.c_str(), 0); // Don't overwrite existing env vars
    }

    //system("env"); // Prints current environment vars

    std::string home = get_env_var("HOME");
    if (home.empty()) {
        std::cerr << uid << " [ERROR] HOME environment variable not set!" << std::endl;
        exit(EXIT_FAILURE);
    }
    ensure_config(home);
    std::optional<configuration> user_config = get_config(home);
    if (!user_config.has_value()) {
        std::cerr << uid << " [ERROR] User's configuration was not valid! Giving them an empty one!";
        user_config = {};
    }

    // Run the dinit process
    std::string program = "/usr/bin/dinit";
    std::vector<char*> argv;

    argv.push_back(const_cast<char*>(program.c_str())); // First arg must be prgoram name
    for (auto& arg : user_config.value().arguments) {
        argv.push_back(const_cast<char*>(arg.c_str()));
        //std::cout << uid << "[LOG] Added dinit arg: " << arg << std::endl;
    }
    argv.push_back(nullptr); // Null-terminate argv

    execv(program.c_str(), argv.data());
    perror("Execv failed!");
    //execl("/usr/bin/dinit", "--user", (void*)NULL);
    exit(EXIT_FAILURE); // if we somehow make it past the execv
}

// Get the integer from a UID string, if it fails, then nullopt instead
std::optional<int> get_int_from_name(std::string name) {
    try {
        int parsed_name = std::stoi(name);
        return parsed_name;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Found entry: " << name << " in " << monitored_path.string() << ", but it was not an integer!" << std::endl;
        return std::nullopt;
    }
}

void reap_child(int sig) {
    (void)sig;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        std::cout << "[LOG] Reaped child PID: " << pid << std::endl;
    }
}

int main() {
    if (geteuid() != 0) {
        std::cerr << "[ERROR] Program must be ran with root privileges!" << std::endl;
        exit(EXIT_FAILURE);
    }

    //signal(SIGCHLD, reap_child); // Register listener that repsonds to kernel requests to reap zombie processes.
    // Set up proper signal handling
    struct sigaction sa = {};
    //memset(&sa, 0, sizeof(sa));
    sa.sa_handler = reap_child;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        perror("[ERROR] sigaction failed");
        exit(EXIT_FAILURE);
    }
    
    // Incase we are started after some users have already logged in (should not occur in normal scenarios)
    std::vector<int> queued_users;
    try {
        for (const auto& entry: std::filesystem::directory_iterator(monitored_path)) {
            std::string name = entry.path().filename().string();
            if (entry.is_directory()) {
                auto parsed_name = get_int_from_name(name);
                if (parsed_name.has_value()) {
                    queued_users.push_back(parsed_name.value());
                }
            } else {
                std::cerr << "[ERROR] Found entry: " << name << " in " << monitored_path
                    << ", but it was not a directory!" << std::endl;
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[EXIT] Failed to access " << monitored_path.string() << ": " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Handle these users, if any
    for (int uid: queued_users) {
        handle_user(uid);
    }

    // Create inotify instance (blocking)
    int inotify_file_descriptor = inotify_init();
    if (inotify_file_descriptor == -1) {
        perror("[ERROR] inotify_init failed");
        return -1;
    }

    // Add a watch for creation and deletion
    int watch_descriptor = inotify_add_watch(inotify_file_descriptor, monitored_path.string().c_str(), IN_CREATE | IN_DELETE); // TODO ADD KILL COMMAND FOR USERS WHO LOGOUT! USE A MAP - STORE PIDs upon fork, and then kill(pid, SIGTERM), on removal!
    if (watch_descriptor == -1) {
        perror("[ERROR] inotify_add_watch failed");
        close(inotify_file_descriptor);
        return -1;
    }

    std::cout << "[LOG] Monitoring directory: " << monitored_path.string() << std::endl;

    const size_t buf_len = 4096;
    char buffer[buf_len];

    while (true) {
        ssize_t length = read(inotify_file_descriptor, buffer, buf_len);
        if (length == -1) {
            if (errno == EINTR) continue; // Added this to handle interrupts
            perror("[ERROR] Failed to read inotify_file_descriptor");
            break;
        }

        for (char* ptr = buffer; ptr < buffer + length; ) {
            inotify_event* event = static_cast<inotify_event*>(static_cast<void*>(ptr));

            if (event->len > 0) {
                if (event->mask & IN_ISDIR) {
                    auto parsed_name = get_int_from_name(event->name);
                    if (!parsed_name.has_value()) {
                        std::cerr << "[ERROR] Tried to get name from directory event, it was not a pure int!" << std::endl;
                        continue;
                    }
                    if (event->mask & IN_CREATE) {
                        handle_user(parsed_name.value());
                    } else if (event->mask & IN_DELETE) {
                        if (uid_pid_map.find(parsed_name.value()) != uid_pid_map.end()) {
                            kill(uid_pid_map.at(parsed_name.value()), SIGTERM);
                            std::cout << "[LOG] Cleaning up UID: " << parsed_name.value() << std::endl;
                        } else {
                            std::cerr << "[ERROR] Tried to clean up after UID: " << parsed_name.value() << " but failed to find matching PID!" << std::endl; 
                        }
                    } else {
                    // We specified IN_CREATE before, and hence should only receive these events
                    std::cerr << "[ERROR] Received event in monitored path, but it was not a creation nor deletion event!" << std::endl;
                    }
                } else {
                    // Only directories should be added!
                    std::cerr << "[ERROR] Found entry: " << event->name << " in " << monitored_path
                        << ", but it was not a directory!" << std::endl;
                }
            }
            ptr += sizeof(inotify_event) + event->len;
        }
    }

    // Cleanup
    close(watch_descriptor);
    close(inotify_file_descriptor);
    return 0;
}
