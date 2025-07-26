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

const std::string example_toml = R"(# The options provided here are not checked for validity, they are just ran for your user. If you cook them, your personal dinit process will be cooked.
# These configs are unique to each user, one config does not affect another user.

# Add arguments one by one, for example ["--user", "--services-dir", "/home/myuser/.config/dinit.d"].
dinit_arguments = ["--user"] 

# Disables running the 'env' command, to get your other environment variables (of which it will transfer to the new dinit process).
# This means that only your XDG_RUNTIME_DIR will be set, the minimum required for the dinit child processes to run correctly.
# XDG_RUNTIME_DIR is not set by looking at the 'env' commands output, but manually constructed so it is a steady fallback.
minimal_environment_handling = false

# Enables verbose debugging options. It will flood logs, in catlog.
verbose_debug = false
)";

struct configuration {
    std::vector<std::string> arguments;
    bool minimum_environment_handling = false;
    bool verbose_debug = false;
};

bool ensure_config(std::string home);
std::optional<configuration> get_config(std::string home);
