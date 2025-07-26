// Copyright (C) 2025 initMayday (initMayday@protonmail.com). This is available under AGPLv3-or-later. See LICENSE

#include <string>

// Here, you can see the configuration options in example_toml. Ideally, this would be in its own toml file, but for ease of syncing between the program's definition and the visible definition, this was decided to be the best compromise, having it in its own header file. The only relevant part is the string.
// You can find a copy of this in your config directory, found at ~/.config/dinit.d/config/dinit-user-spawn.toml

const std::string example_toml = R"(
# The options provided here are not checked for validity, they are just ran for your user. If you cook them, your personal dinit process will be cooked.
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
