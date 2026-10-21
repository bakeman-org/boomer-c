set -xe

clang-22 -std=c23 boomer-c.c $(pkg-config --cflags --libs x11 xrandr gl glew libpng xft lua5.4) -o boomer-c -lm -lwayland-client