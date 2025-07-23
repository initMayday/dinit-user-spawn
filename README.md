dinit-user-spawn

Dinit-user-spawn is a program which runs a dinit process, under each user that logs in (and removes it when they log out).
This means that users have a dinit instance to manage their own user dinit services, automatically.
The program checks whether a new user has logged in, by checking for a new file within /run/user/. On startup, it will spawn a dinit instance for each of these users, and then if any more are added to that folder.
Users who are logged out are removed from /run/user, and the program sees this and sends a kill to that dinit process, of which is then handled.

Find all configuration options within config.h, where there is an example toml demonstrating everything (within a raw string).

Please report any issues under the github issues. If there are any feature requests, you can also post them there, however, the scope of this program is limited to what is mentioned above.

Copyright (C) 2025 initMayday
This program is available under AGPL-v3-or-later. See LICENSE.md for further information.
