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
wrongminus:
        pop %ecx
        sub %ecx, %eax
minus:  sub %eax, (%esp)
        pop %eax
lessthan:
        sub (%esp), %eax
        pop %eax
        setge %al
        setle %al
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
init:   mov %esp, %ebp
        sub $2048, %ebp
getchar:
	## first load the buffer address, then
        mov %eax, %ecx
	mov $3, %eax
	xor %ebx, %ebx
        mov $1, %edx
        int $0x80
        test %eax, %eax
        jnz okay
        mov $-1, %eax
        ret
okay:   ## load buffer address again, then
        ## fetch byte
        movzbl (%eax), %eax
xchg:   xchg %eax, (%esp)       # hex 87 04 24, decimal 135 4 36
        xchg (%esp), %eax       # syntactic variant of the above
add:    add (%esp), %eax
        pop %ecx
newstuff:
        stosl
        sub -4(%edi), %eax
        add $-4, %edi
	sub (%edi), %eax
        sub $4, %edi
        lea -4(%edi), %edi
        mov (%edi), %eax
newstore:
        sub $8, %edi
        mov 4(%edi), %ecx
        mov %ecx, (%eax)
        mov (%edi), %eax
newsubtract:
        sub $4, %edi
        sub %eax, (%edi)
        mov (%edi), %eax
newbytestore:
        sub $8, %edi
        mov 4(%edi), %ecx
        mov (%ebx,%edi,1), %ecx # alternately if %ebx==4; no shorter tho
        movb %cl, (%eax)
        mov (%edi), %eax
newentrypoint:
        mov %esp, %edi
        sub $2048, %edi