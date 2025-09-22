# dinit-user-spawn

Dinit-user-spawn spawns a dinit process for each user that logs onto the system, automagically. This is useful by, for example, allowing each user to run their own pipewire processes under their own user.

Upon user logout, the user dinit process is cleaned up. This is the entire scope of the program.

## Installation
Choose your ideal package, and then run dinitctl enable dinit-user-spawn to enable the program. The included service file can be seen by inspecting dinit-user-spawn.service.

*The master branch is the stable branch, and is intended for use by end users. The dev branch is only intended for testing, and thus may break. Hence, you should (unless you otherwise know what you are doing) use the master branch. The master branch is periodically synced to the dev branch, when it is believed to be stable.*

**Master Branch (Stable)**:  
[Arch User Repository](https://aur.archlinux.org/packages/dinit-user-spawn-master-git) (first-party)  
[Artix System Repository](https://packages.artixlinux.org/packages/system/x86_64/dinit-user-spawn/) (third-party)

**Dev Branch (Testing)**:  
[Arch User Repository](https://aur.archlinux.org/packages/dinit-user-spawn-dev-git) (first-party)

## The logic behind the program
The program finds logged-in users by monitoring /run/user/, where a directory (with the name of the user's UID), will be created when they log in. For example, for most people, when they log onto their main account a new folder "1000" (their user's UID), will be created in /run/user/.

For each of these users, dinit-user-spawn creates a new dinit process that is ran under that user. The dinit process PID is kept in a dictionary, with the user's UID as the key.

Upon user logout, dinit-user-spawn kills the associated dinit process (by looking it up in the dictionary), and then handles the cleanup.

## Documentation / Configuration
Find all configuration options within [configuration_example](configuration_example.h), where there is an example toml demonstrating everything. You can also find the same toml as your default configuration, which you can find at ~/.config/dinit.d/config/dinit-user-spawn.toml.

## Important Notes
dinit-user-spawn will not start another dinit instance for your user if you are already running one (via another method).

## Misc
Please report any issues under the github issues. If there are any feature requests, you can also post them there, however, the scope of this program is limited to what is mentioned above. Alternatively, I can often be found on the artix telegram, so feel free to ask there.

Copyright (C) 2025 initMayday (initMayday@protonmail.com)

AGPL-v3-or-later Licensed
