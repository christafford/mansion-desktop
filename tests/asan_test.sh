#!/bin/bash
cd /home/deck/code/mansion-desktop
rm -f /tmp/asan-out.txt /tmp/asan-err.txt
ASAN_OPTIONS='detect_leaks=0:halt_on_error=1:abort_on_error=1:print_summary=1:print_stacktrace=1' ./build-asan/mansion-desktop --headless --socket mansion-asan --exit-after-ms 15000 > /tmp/asan-out.txt 2> /tmp/asan-err.txt &
ASPID=$!
sleep 2
./build-asan/mansion-test-client --socket mansion-asan --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 1000 2>/dev/null
sleep 1
./build-asan/mansion-test-client --socket mansion-asan --toplevel --buffer 100x100 --color ff0000 --report-input --exit-after-ms 1000 2>/dev/null
sleep 3
if kill -0 $ASPID 2>/dev/null; then
    kill $ASPID 2>/dev/null
    wait $ASPID 2>/dev/null
    echo "ASAN: SURVIVED"
else
    wait $ASPID 2>/dev/null
    echo "ASAN: CRASHED"
fi
echo "=== ASAN ERR ==="
cat /tmp/asan-err.txt
