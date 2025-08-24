// Copyright (C) 2025 initMayday (initMayday@protonmail.com). This is available under AGPLv3-or-later. See LICENSE

#pragma once
#include <optional>
#include <string>
#include <vector>

const std::string dinitd_dir = "/.config/dinit.d/";
const std::string config_dir = "/.config/dinit.d/config/";

const std::string boot_service = R"(
type = internal
waits-for.d: ./boot.d/
)";

struct configuration {
    std::string binary = "/usr/bin/dinit";
    std::vector<std::string> arguments;
    bool verbose_debug = false;
};

bool ensure_config(std::string home);
bool check_config_exists(std::string home);
std::optional<configuration> get_config(std::string home);
