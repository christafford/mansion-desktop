#!/bin/bash
cd /home/deck/code/elsewhere

# Use base code (no changes)
git stash --include-untracked > /dev/null 2>&1
meson compile -C build > /dev/null 2>&1

rm -f /tmp/elsewhere-base-gdb /tmp/gdb-base-out.txt

cat > /tmp/gdb-base-cmds << 'GDBEOF'
set pagination off
set height 0
catch signal SIGSEGV
run --headless --socket elsewhere-base-gdb --exit-after-ms 15000
bt 20
quit
GDBEOF

gdb -batch -x /tmp/gdb-base-cmds --args ./build/elsewhere --headless --socket elsewhere-base-gdb --exit-after-ms 15000 > /tmp/gdb-base-out.txt 2>&1 &
GDBPID=$!
sleep 3
MALLOC_PERTURB_=41 ./build/elsewhere-test-client --socket elsewhere-base-gdb --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 4000 2>/dev/null
sleep 1
MALLOC_PERTURB_=41 ./build/elsewhere-test-client --socket elsewhere-base-gdb --toplevel --buffer 100x100 --color ff0000 --report-input --exit-after-ms 3000 2>/dev/null
sleep 10
if kill -0 $GDBPID 2>/dev/null; then
    kill $GDBPID 2>/dev/null
fi
wait $GDBPID 2>/dev/null
echo "=== BASE GDB OUTPUT ==="
cat /tmp/gdb-base-out.txt | grep -A30 "SIGSEGV\|bt 20\|#0" | head -40
