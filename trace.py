#!/usr/bin/gdb -x
# Extract an instruction pointer trace from GDB in the same format
# the 386 emulator produces.
gdb.execute("target exec tinyboot1")
gdb.execute("set pagination off")
gdb.execute("break *0x20dec")
outfile = open("gdb-trace.out", "w")
gdb.execute("run < hello42.tbf1 > tmp.out")
while True:
    try:
        eip = hex(int(gdb.parse_and_eval("$eip")))
    except gdb.error:   # supposing this is because the program exited
        outfile.close()
        sys.exit(0)

    mem = hex(int(gdb.parse_and_eval("*(unsigned char*)$eip")))
    outfile.write("{eip: %s, [eip]: %s}\n" % (eip, mem))
    gdb.execute("stepi")
