all: try-build-static

clang-build:
	clang main-embed.c -lX11 -lGL -lGLEW -lm -lXrandr -std=c23

try-build-static:
	clang -o boomer-c main-embed.c -std=c23 \
	-l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libXext.a \
	-l:libX11.a \
	-l:libxcb-shm.a \
	-l:libxcb.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libfreetype.a \
	-l:libpng.a \
	-l:libz.a \
	-l:libbz2.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libxcb.a -Wl,--no-whole-archive \
	-lGLX -lGL \
	-l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libXext.a \
	-l:libX11.a \
	-l:libxcb-shm.a \
	-l:libX11-xcb.a \
	-l:libxcb.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libfreetype.a \
	-l:libpng.a \
	-l:libz.a \
	-l:libbz2.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libxcb.a -Wl,--no-whole-archive \
	-lGLX -lGL \
	-Wl,--as-needed \
	-l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libXext.a \
	-l:libX11.a \
	-l:libxcb-shm.a \
	-l:libxcb.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libfreetype.a \
	-l:libpng.a \
	-l:libz.a \
	-l:libbz2.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libxcb.a -Wl,--no-whole-archive \
	-lGLX -lGL \
	-l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -l:libX11.a \
	-l:libxcb.a -l:libXau.a -l:libXdmcp.a -l:libXrandr.a \
	-l:libGLEW.a -l:libXext.a -l:libXrender.a -lm \
	-l:libXext.a \
	-l:libX11.a \
	-l:libxcb-shm.a \
	-l:libX11-xcb.a \
	-l:libxcb.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libfreetype.a \
	-l:libpng.a \
	-l:libz.a \
	-l:libbz2.a \
	-l:libXau.a \
	-l:libXdmcp.a \
	-l:libxcb.a -Wl,--no-whole-archive \
	-lGLX -lGL \
	-Wl,--as-needed \
	-Wl,-z,relro,-z,now

install:
	make -B
	cp boomer-c /usr/local/bin/boomer-c

uninstall:
	test -f /usr/local/bin/boomer-c && rm /usr/local/bin/boomer-c