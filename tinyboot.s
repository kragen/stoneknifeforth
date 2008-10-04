	## sample code snippets for the tiny bootstrap forth compiler
return: xchg %ebp, %esp
        ret
        ##  subroutine call
call:   xchg %ebp, %esp
        call foo
        xchg %ebp, %esp
        ##  push a constant
push:   push %eax
        mov $1234567, %eax
fetch:  mov (%eax), %eax
store:  pop (%eax)
        pop %eax
syscall:
        pop %eax
        pop %edx
        pop %ecx
        pop %ebx
        int $0x80
plus:   pop %ecx
foo:    add %ecx, %eax
lessthan:
        sub (%esp), %eax
        pop %eax
        setge %al
        dec %al
        movsbl %al, %eax
not:    not %eax
bytefetch:
        movzbl (%eax), %eax
bytestore:
        pop %ecx
        movb %cl, (%eax)
        pop %eax
jump:   test %eax, %eax
        pop %eax
        jnz foo
        jnz bar
        jz bar
bar:    nop
rshift: pop %ecx
        sar %cl, %eax
