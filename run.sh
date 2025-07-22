set -e
meson compile -C build
./build/dinit-user-spawn
