#!/usr/bin/python
"Remove comments and most extra whitespace from a tbf1 program."
import sys
wsp = comment = newline = False
while True:
    byte = sys.stdin.read(1)
    if not byte: break
    elif byte == '(': comment = True
    elif byte == ')': comment = False
    elif not comment:
        if byte == '\n': newline = True
        elif byte == ' ': wsp = True
        else:
            if newline: sys.stdout.write('\n')
            elif wsp: sys.stdout.write(' ')
            sys.stdout.write(byte)
            wsp = newline = False
sys.stdout.write('\n')
