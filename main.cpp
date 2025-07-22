#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>
#include <sys/inotify.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <security/pam_appl.h>

const std::filesystem::path monitored_path = "/run/user";

void handle_user(int uid) {
    std::cout << "[LOG] Handling: " << uid << std::endl;

    // Check UID is valid
    passwd* pw = getpwuid(uid);
    if (pw == nullptr) {
        std::cerr << "[ERROR] UID of " << uid << " is invalid!" << std::endl;
        return;
    }

    // UID is valid
    pid_t pid = fork();
    std::string process_id = std::to_string(getpid());

    if (pid == -1) {
        std::cerr << process_id << " [ERROR] Failed to fork!" << std::endl;
        return;
    }

    // Only continue if we are the child process - We must use exit from now on
    if (pid != 0) {
        return;
    }
    
    // Clear the old environment, to avoid conflicting variables
    clearenv();
    // Set just XDG_RUNTIME_DIR, as it is all dinit needs. Note that the proper way to do this I believe
    // is via PAM, and getting the user environment via it, which will be done in future.
    setenv("XDG_RUNTIME_DIR", std::string("/run/user/" + std::to_string(pw->pw_uid)).c_str(), 1);

    // Swap to the user
    if (initgroups(pw->pw_name, pw->pw_gid) != 0) {
        perror(std::string(process_id + " [ERROR] initgroups failed").c_str());
        exit(EXIT_FAILURE);
    }
    if (setgid(pw->pw_gid) != 0) {
        perror(std::string(process_id + " [ERROR] setgid failed").c_str());
        exit(EXIT_FAILURE);
    }
    if (setuid(uid) != 0) {
        perror(std::string(process_id + " [ERROR] setuid failed").c_str());
        exit(EXIT_FAILURE);
    }

    // Check we have fully swapped, to prevent privilege escalation issues
    if (geteuid() != pw->pw_uid || getegid() != pw->pw_gid) {
        perror(std::string(process_id + " [ERROR] Failed to drop privileges").c_str());
        exit(EXIT_FAILURE);
    }

    // system("env"); --> Prints current environment vars

    std::cout << process_id << " [LOG] Running as: " << getpwuid(getuid())->pw_name << std::endl;

    // Confirm there is no dinit process running
    std::string command = "pgrep -x -u " + std::to_string(uid) + " dinit > /dev/null"; 
    int execute_success = system(command.c_str());
    if (execute_success == -1) {
        std::cerr << process_id << " [ERROR] System call failed, return code: " << execute_success << std::endl;
        exit(EXIT_FAILURE);
    }
    int exit_code = WEXITSTATUS(execute_success);
    if (exit_code == 0) {
        std::cerr << process_id << " [ERROR] There was already a dinit process running!" << std::endl;
        exit(EXIT_FAILURE);
    } else if (exit_code > 1) {
        std::cerr << process_id << " [ERROR] pgrep returned exit_code: " << exit_code << std::endl;
        exit(EXIT_FAILURE);
    }

    // Run the dinit process
    execl("/usr/bin/dinit", "--user", (void*)NULL);
    //execl("/bin/sh", "sh", "-l", "-c", "dinit --user --services-dir /etc/dinit.d/user", (char*)NULL);
    std::cerr << process_id << " [ERROR] Somehow made it past the dinit excel!" << std::endl;
    exit(EXIT_FAILURE); // if we somehow make it past the execl
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

int main() {
    
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
    int watch_descriptor = inotify_add_watch(inotify_file_descriptor, monitored_path.string().c_str(), IN_CREATE);
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
            perror("[ERROR] Failed to read inotify_file_descriptor");
            break;
        }

        for (char* ptr = buffer; ptr < buffer + length; ) {
            inotify_event* event = static_cast<inotify_event*>(static_cast<void*>(ptr));

            if (event->len > 0) {
                if (event->mask & IN_CREATE) {
                    if (event->mask & IN_ISDIR) {
                        auto parsed_name = get_int_from_name(event->name);
                        if (parsed_name.has_value()) {
                            handle_user(parsed_name.value());
                        }
                    } else {
                        // Only directories should be added!
                        std::cerr << "[ERROR] Found entry: " << event->name << " in " << monitored_path
                            << ", but it was not a directory!" << std::endl;
                    }
                } else {
                    // We specified IN_CREATE before, and hence should only receive these events
                    std::cerr << "[ERROR] Received event in monitored path, but it was not a creation event!" << std::endl; 
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
