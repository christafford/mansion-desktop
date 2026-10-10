#!/bin/bash
cd /home/deck/code/elsewhere

rm -f /tmp/elsewhere-gdb-mp41 /tmp/gdb-out.txt

cat > /tmp/gdb-cmds << 'GDBEOF'
set pagination off
set height 0
handle SIGSEGV nostop noprint
run --headless --socket elsewhere-gdb-mp41 --exit-after-ms 15000
GDBEOF

gdb -batch -x /tmp/gdb-cmds --args ./build/elsewhere --headless --socket elsewhere-gdb-mp41 --exit-after-ms 15000 > /tmp/gdb-out.txt 2>&1 &
GDBPID=$!
sleep 3
MALLOC_PERTURB_=41 ./build/elsewhere-test-client --socket elsewhere-gdb-mp41 --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 4000 2>/dev/null
sleep 1
MALLOC_PERTURB_=41 ./build/elsewhere-test-client --socket elsewhere-gdb-mp41 --toplevel --buffer 100x100 --color ff0000 --report-input --exit-after-ms 3000 2>/dev/null
sleep 5
if kill -0 $GDBPID 2>/dev/null; then
    kill $GDBPID 2>/dev/null
fi
wait $GDBPID 2>/dev/null
echo "=== GDB OUTPUT ==="
cat /tmp/gdb-out.txt | tail -60
