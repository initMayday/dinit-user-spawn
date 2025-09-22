// Copyright (C) 2025 initMayday (initMayday@protonmail.com). This is available under AGPLv3-or-later. See LICENSE

#include <string>

// Here, you can see the configuration options in example_toml. Ideally, this would be in its own toml file, but for ease of syncing between the program's definition and the visible definition, this was decided to be the best compromise, having it in its own header file. The only relevant part is the string.
// You can find a copy of this in your config directory, found at ~/.config/dinit.d/config/dinit-user-spawn.toml

const std::string example_toml = R"(
# The options provided here are not checked for validity, they are just ran for your user. If you cook them, your personal dinit process will be cooked.
# These configs are unique to each user, one config does not affect another user.

# Binary to execute. Change if in a different location.
binary = "/usr/bin/dinit"

# Add arguments one by one, for example ["--user", "--services-dir", "/home/myuser/.config/dinit.d"].
dinit_arguments = ["--user"] 

# Enables verbose debugging options. It will flood logs, in catlog.
verbose_debug = false

# There used to be a minimal_environment_handling variable, but this was removed and now is always enabled. This is because the previous method of trying to 'inherit' environment variables, didn't actually inherit many useful env vars. Therefore, if you are not doing what will be latter metioned, ensure your services do no depend on env vars beyond (SHELL, PWD, LOGNAME, HOME, SHLVL, XDG_RUNTIME_DIR, and PATH)  For those seeking to set environment variables, please use dinit_arguments to specify an environment file, or utilise the environment file option in dinit services.
)";
