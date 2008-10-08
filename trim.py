#!/usr/bin/python
"Remove comments and most extra whitespace from a tbf1 program."
import sys
wsp = False
comment = False
while True:
    byte = sys.stdin.read(1)
    if not byte: break
    elif byte == '(': comment = True
    elif byte == ')': comment = False
    elif not comment and byte in ' \n':
        if not wsp: sys.stdout.write(byte)
        wsp = True
    elif not comment:
        sys.stdout.write(byte)
        wsp = False
