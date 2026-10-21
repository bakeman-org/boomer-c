
set -xe

clang-22 one.c $(pkg-config --cflags --libs x11 xrandr gl glew) -o boomer -lm -lwayland-client -std=c23 