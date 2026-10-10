#!/bin/bash
cd /home/deck/code/elsewhere

rm -f /tmp/elsewhere-gdb2-mp41 /tmp/gdb2-out.txt

cat > /tmp/gdb2-cmds << 'GDBEOF'
set pagination off
set height 0
catch signal SIGSEGV
run --headless --socket elsewhere-gdb2-mp41 --exit-after-ms 15000
bt 50
info registers
quit
GDBEOF

gdb -batch -x /tmp/gdb2-cmds --args ./build/elsewhere --headless --socket elsewhere-gdb2-mp41 --exit-after-ms 15000 > /tmp/gdb2-out.txt 2>&1 &
GDBPID=$!
sleep 3
MALLOC_PERTURB_=41 ./build/elsewhere-test-client --socket elsewhere-gdb2-mp41 --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 4000 2>/dev/null
sleep 1
MALLOC_PERTURB_=41 ./build/elsewhere-test-client --socket elsewhere-gdb2-mp41 --toplevel --buffer 100x100 --color ff0000 --report-input --exit-after-ms 3000 2>/dev/null
sleep 10
if kill -0 $GDBPID 2>/dev/null; then
    kill $GDBPID 2>/dev/null
fi
wait $GDBPID 2>/dev/null
echo "=== GDB OUTPUT ==="
cat /tmp/gdb2-out.txt | grep -A50 "SIGSEGV\|bt\|registers" | head -80
