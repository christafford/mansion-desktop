#!/bin/bash
cd /home/deck/code/elsewhere
rm -f /tmp/gdb4s-out.txt

cat > /tmp/gdb4s-cmds << 'EOF'
set pagination off
set height 0
handle SIGSEGV stop
run --headless --socket elsewhere-gdb4s --exit-after-ms 15000
bt 30
info frame
list *$pc
quit
EOF

gdb -x /tmp/gdb4s-cmds --args ./build/elsewhere --headless --socket elsewhere-gdb4s --exit-after-ms 15000 > /tmp/gdb4s-out.txt 2>&1 &
GDBPID=$!
sleep 3
./build/elsewhere-test-client --socket elsewhere-gdb4s --toplevel --buffer 200x100 --color 00ff00 --exit-after-ms 4000 2>/dev/null
sleep 10
wait $GDBPID 2>/dev/null
cat /tmp/gdb4s-out.txt | grep -A30 "Thread"
