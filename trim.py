#!/usr/bin/python
"Remove comments and most extra whitespace from a tbf1 program."
# Wow this program has gotten ugly.
import sys
deindent = ('-i' in sys.argv)
wsp = comment = newline = False
firstline = startline = True
while True:
    byte = sys.stdin.read(1)
    if not byte: break
    elif byte == '(': comment = True
    elif byte == ')': comment = False
    elif not comment:
        if byte == '\n':
            startline = True
            newline = True
        elif (deindent or not startline) and byte == ' ':
            wsp = True
        else:
            startline = (byte == ' ')
            if newline:
                if not firstline: sys.stdout.write('\n')
            elif wsp: sys.stdout.write(' ')
            sys.stdout.write(byte)
            if byte == "'": sys.stdout.write(sys.stdin.read(1))
            wsp = newline = firstline = False
sys.stdout.write('\n')
