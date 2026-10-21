#!/bin/bash
# Test each testdata shader: launch boomer-c with it, press 's' to save, kill.
set -u
cd /home/etcix/code/own/boomer
mkdir -p /tmp/shader-test
rm -f /tmp/shader-test/*.png

for path in testdata/*.frag; do
    f=$(basename "$path" .frag)
    out="/tmp/shader-test/$f.png"
    ./boomer-c -o "$out" testdata/vertex.glsl testdata/$f.frag >/tmp/shader-test/$f.log 2>&1 &
    pid=$!
    sleep 1.2
    xdotool key s 2>/dev/null
    sleep 0.6
    kill $pid 2>/dev/null
    wait $pid 2>/dev/null
    if [ -s "$out" ]; then
        echo "PASS $f ($(stat -c%s "$out") bytes)"
    else
        echo "FAIL $f"
        cat /tmp/shader-test/$f.log
    fi
done
