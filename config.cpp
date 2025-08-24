// Copyright (C) 2025 initMayday (initMayday@protonmail.com). This is available under AGPLv3-or-later. See LICENSE

#include "config.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <toml++/toml.hpp>
#include "configuration_example.h"

bool ensure_path(std::filesystem::path path) {
 if (!std::filesystem::exists(path)) {
        // Create the path if it doesn't exist
        try {
            std::filesystem::create_directories(path);
            std::cout << "[LOG] Created dir: " << path.c_str() << std::endl;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[ERROR] Failed to create directory: " << e.what() << " Error Code: " << e.code() << std::endl;
            return false;
        }
    }
    return true;
}

bool ensure_file(std::filesystem::path path, std::string content) {
    if (!std::filesystem::exists(path)) {
        // create the boot service - it doesn't exist
        std::ofstream output_file(path);
        if (!output_file) {
            std::cerr << "[ERROR] Failed to open boot file in " << path.c_str() << std::endl;
            return false;
        }
        output_file << content;
        output_file.close();
        std::cout << "[LOG] Wrote file: " << path.c_str() << std::endl;
    }
    return true;
}

bool check_path(std::filesystem::path path) {
    if (std::filesystem::exists(path)) {
        return true;
    }
    std::cout << "[LOG] Path did not exist: " << path.string() << std::endl;
    return false;
}

bool check_file(std::filesystem::path path) {
    if (std::filesystem::exists(path)) {
        return true;
    }
    std::cout << "[LOG] File did not exist: " << path.string() << std::endl;
    return false;
}

bool check_config_exists(std::string home) {
    std::filesystem::path dinitd_path = home + dinitd_dir;
    std::filesystem::path config_path = home + config_dir;
    
    std::filesystem::path boot_dir = dinitd_path / "boot";
    std::filesystem::path boot_d_dir = dinitd_path / "boot.d";
    std::filesystem::path spawn_dir = config_path / "dinit-user-spawn.toml";

    // Ensure the directories actually exist
    if (check_path(dinitd_path) == false) { return false; }
    if (check_path(config_path) == false) { return false; }

    // Technically, it would be beneficial to also check if it is a directory / file, but
    // that should never occur granted they haven't messed around to largely intentionally cause this
    if (check_file(boot_dir) == false) { return false; }

    if (check_path(boot_d_dir) == false) { return false; }
    if (check_file(spawn_dir) == false) { return false; }

    return true;
}

bool ensure_config(std::string home) {
    std::filesystem::path dinitd_path = home + dinitd_dir;
    std::filesystem::path config_path = home + config_dir;
    
    std::filesystem::path boot_dir = dinitd_path / "boot";
    std::filesystem::path boot_d_dir = dinitd_path / "boot.d";
    std::filesystem::path spawn_dir = config_path / "dinit-user-spawn.toml";

    // Ensure the directories actually exist
    if (ensure_path(dinitd_path) == false) { return false; }
    if (ensure_path(config_path) == false) { return false; }

    // Technically, it would be beneficial to also check if it is a directory / file, but
    // that should never occur granted they haven't messed around to largely intentionally cause this
    if (ensure_file(boot_dir, boot_service) == false) { return false; }

    if (ensure_path(boot_d_dir) == false) { return false; }
    if (ensure_file(spawn_dir, example_toml) == false) { return false; }

    return true;
}

void did_parse(bool parsed, std::string arg_name) {
    if (!parsed) {
        std::cout << "[LOG] Did not parse: " << arg_name << std::endl;
    }
}

void parse_boolean(std::shared_ptr<toml::table> config, bool& target_bool, std::string arg_name) {
    bool parsed = false;
    auto min_env = config->get(arg_name);
    if (min_env) {
        if (min_env->is_boolean()) {
            auto opt = min_env->as_boolean();
            parsed = true;
            target_bool = opt->get();
        } else {
            std::cerr << "[ERROR] Value of " << arg_name << " was not a bool" << std::endl;
        }
    }
    did_parse(parsed, arg_name);
}

std::optional<configuration> get_config(std::string home) {
    std::filesystem::path config_path = home + config_dir;
    std::filesystem::path spawn_dir = config_path / "dinit-user-spawn.toml";

    std::shared_ptr<toml::table> config;
    configuration ret = {};

    try {
        config = std::make_shared<toml::table>(toml::parse_file(spawn_dir.c_str()));
    } catch (const toml::parse_error& e) {
        std::cerr << "[ERROR] TOML parse error: " << e.description() << " at " << e.source().begin << std::endl;
        return std::nullopt;
    }

    bool parsed = false;
    
    auto binary = config->get("binary");
    if (binary) {
        if (binary->is_string()) {
            auto opt = binary->as_string();
            parsed = true;
            ret.binary = opt->get();
        } else {
            std::cerr << "[ERROR] Value of binary was not a string" << std::endl;
        }
    }
    did_parse(parsed, "binary");
    parsed = false;

    auto dinit_args = config->get("dinit_arguments");
    if (dinit_args) {
        if (dinit_args->is_array()) {
            auto array = dinit_args->as_array();
            parsed = true;
            for (const auto& element : *array) {
                if (auto val = element.value<std::string>()) {  // try to get int value
                    ret.arguments.push_back(*val);
                } else {
                    std::cerr << "[ERROR] Arg in dinit_arguments was not a string" << std::endl;
                }
            }
        }
    }
    did_parse(parsed, "dinit_arguments");
    parsed = false;

    parse_boolean(config, ret.verbose_debug, "verbose_debug");

    return ret;
}
