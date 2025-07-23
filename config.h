// Copyright (C) 2025 initMayday. This is available under GPLv3-or-later. See LICENSE.md

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

const std::string example_toml = R"(
# The options provided here are not checked, they are just ran for your user. If you cook them, your dinit process will be cooked.

dinit_arguments = ["--user"] # Add arguments one by one, for example ["--user", "--services-dir", "/home/myuser/.config/dinit.d"].
)";

struct configuration {
    std::vector<std::string> arguments;
};

bool ensure_config(std::string home);
std::optional<configuration> get_config(std::string home);
