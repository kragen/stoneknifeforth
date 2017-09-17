StoneKnifeForth
===============

This is StoneKnifeForth, a very simple language inspired by Forth.  It
is not expected to be useful; instead, its purpose is to show how
simple a compiler can be.  The compiler is a bit under two pages of
code when the comments are removed.

This package includes a “metacircular compiler” which is written in
StoneKnifeForth and compiles StoneKnifeForth to an x86 Linux ELF
executable.

There is also a StoneKnifeForth interpreter written in Python (tested
with Python 2.4).  It seems to be about 100× slower than code emitted
by the compiler.

(All of the measurements below may be a bit out of date.)

On my 700MHz laptop, measuring wall-clock time:

- compiling the compiler, using the compiler running in the interpreter,
  takes about 10 seconds;
- compiling the compiler, using the compiler compiled with the compiler,
  takes about 0.1 seconds;
- compiling a version of the compiler from which comments and extra
  whitespace have been “trimmed”, using the compiler compiled with the
  compiler, takes about 0.02 seconds.

So this is a programming language implementation that can recompile
itself from source twice per 24fps movie frame.  The entire “trimmed”
source code is 1902 bytes, which is less than half the size of the
nearest comparable project that I’m aware of, `otccelf`, which is 4748
bytes.

As demonstrated by the interpreter written in Python, the programming
language itself is essentially machine-independent, with very few
x86 quirks:

- items on the stack are 32 bits in size;
- arithmetic is 32-bit;
- stack items stored in memory not only take up 4 bytes, but they are
  little-endian.

(It would be fairly easy to make a tiny “compiler” if the source
language were, say, x86 machine code.)

The output executable is 4063 bytes, containing about 1400
instructions.  `valgrind` reports that it takes 1,813,395 instructions
to compile itself.  (So you would think that it could compile itself
in 2.6 ms.  The long runtimes are a result of reading its input one
byte at at time.)

Why?  To Know What I’m Doing
----------------------------

A year and a half ago, I wrote a [metacircular Bicicleta-language
interpreter][0], and I said:

> Alan Kay frequently expresses enthusiasm over the metacircular Lisp
> interpreter in the Lisp 1.5 Programmer’s Manual.  For example, in
> <http://acmqueue.com/modules.php?name=Content&pa=showpage&pid=273&page=4>
> he writes:
> 
> >     Yes, that was the big revelation to me when I was in graduate
> >     school — when I finally understood that the half page of code on
> >     the bottom of page 13 of the Lisp 1.5 manual was Lisp in
> >     itself. These were “Maxwell’s Equations of Software!” This is the
> >     whole world of programming in a few lines that I can put my hand
> >     over.
> > 
> >     I realized that anytime I want to know what I’m doing, I can just
> >     write down the kernel of this thing in a half page and it’s not
> >     going to lose any power. In fact, it’s going to gain power by
> >     being able to reenter itself much more readily than most systems
> >     done the other way can possibly do.

But if you try to implement a Lisp interpreter in a low-level language
by translating that metacircular interpreter into it (as [I did later
that year][1]) you run into a problem.  The metacircular interpreter
glosses over a large number of things that turn out to be nontrivial
to implement — indeed, about half of the code is devoted to things
outside of the scope of the metacircular interpreter.  Here’s a list
of issues that the Lisp 1.5 metacircular interpreter neglects, some
semantic and some merely implementational:

- memory management
- argument evaluation order
- laziness vs. strictness
- other aspects of control flow
- representation and comparison of atoms
- representation of pairs
- parsing
- type checking and type testing (atom)
- recursive function call and return
- tail-calls
- lexical vs. dynamic scoping

In John C. Reynolds’s paper, “[Definitional Interpreters
Revisited][2]”, Higher-Order and Symbolic Computation, 11, 355–361
(1998), he says:

> In the fourth and third paragraphs before the end of Section 5, I
> should have emphasized the fact that a metacircular interpreter is
> not really a deﬁnition, since it is trivial when the deﬁning
> language is understood, and otherwise it is ambiguous. In
> particular, Interpreters I and II say nothing about order of
> application, while Interpreters I and III say little about
> higher-order functions.  Jim Morris put the matter more strongly:
> 
> >  The activity of deﬁning features in terms of themselves is highly
> >  suspect, especially when they are as subtle as functional
> >  objects. It is a fad that should be debunked, in my opinion. A
> >  real signiﬁcance of [a self-deﬁned] interpreter . . . is that it
> >  displays a simple universal function for the language in
> >  question.
> 
> On the other hand, I clearly remember that John McCarthy’s
> deﬁnition of LISP [1DI], which is a deﬁnitional interpreter in the
> style of II, was a great help when I ﬁrst learned that language. But
> it was not the sole support of my understanding.

(For what it’s worth, it may not even the case that self-defined
interpreters are necessarily Turing-complete; it might be possible to
write a non-Turing-complete metacircular interpreter for a
non-Turing-complete language, such as David Turner’s Total Functional
Programming systems.)

A metacircular compiler forces you to confront this extra complexity.
Moreover, metacircular compilers are self-sustaining in a way that
interpreters aren’t; once you have the compiler running, you are free
to add features to the language it supports, then take advantage of
those features in the compiler.

So this is a “stone knife” programming tool: bootstrapped out of
almost nothing as quickly as possible.

[0]: http://lists.canonical.org/pipermail/kragen-hacks/2007-February/000450.html
[1]: http://lists.canonical.org/pipermail/kragen-hacks/2007-September/000464.html
[2]: http://www.brics.dk/~hosc/local/HOSC-11-4-pp355-361.pdf

Why? To Develop a Compiler Incrementally
----------------------------------------

When I wrote Ur-Scheme, my thought was to see if I could figure out
how to develop a compiler incrementally, starting by building a small
working metacircular compiler in less than a day, and adding features
from there.  I pretty much failed; it took me two and a half weeks to
get it to compile itself successfully.

Part of the problem is that a minimal subset of R5RS Scheme powerful
enough to write a compiler in — without making the compiler even
larger due to writing it in a low-level language — is still a
relatively large language.  Ur-Scheme doesn’t have much arithmetic,
but it does have integers, dynamic typing, closures, characters,
strings, lists, recursion, booleans, variadic functions, `let` to
introduce local variables, character and string literals, a sort of
crude macro system, five different conditional forms (if, cond, case,
and, or), quotation, tail-call optimization, function argument count
verification, bounds checking, symbols, buffered input to keep it from
taking multiple seconds to compile itself, and a library of functions
for processing lists, strings, and characters.  And each of those
things was added because it was necessary to get the compiler to be
able to compile itself.  The end result was that the compiler is 90
kilobytes of source code, about 1600 lines if you leave out the
comments.

Now, maybe you can write 1600 lines of working Scheme in a day, but I
sure as hell can’t.  It’s still not a very large compiler, as
compilers go, but it’s a lot bigger than `otccelf`.  So I hypothesized
that maybe a simpler language, without a requirement for compatibility
with something else, would enable me to get a compiler bootstrapped
more easily.

So StoneKnifeForth was born.  It’s inspired by Forth, so it inherits
most of Forth’s traditional simplifications:
- no expressions;
- no statements;
- no types, dynamic or static;
- no floating-point (although Ur-Scheme doesn’t have floating-point either);
- no scoping, lexical or dynamic;
- no dynamic memory allocation;

And it added a few of its own:
- no names of more than one byte;
- no user-defined IMMEDIATE words (the Forth equivalent of Lisp macros);
- no interpretation state, so no compile-time evaluation at all;
- no interactive REPL;
- no `do` loops;
- no `else` on `if` statements;
- no recursion;
- no access to the return stack;
- no access to the filesystem, just stdin and stdout;
- no multithreading.

Surprisingly, the language that results is still almost bearable to
write a compiler in, although it definitely has the flavor of an
assembler.

Unfortunately, I still totally failed to get it done in a day.  It was
15 days from when I first started scribbling about it in my notebook
until it was able to compile itself successfully, although `git` only
shows active development happening on six of those days (including
some help from my friend Aristotle).  So that’s an improvement, but
not as much of an improvement as I would like.  At that point, it was
13k of source, 114 non-comment lines of code, which is definitely a
lot smaller than Ur-Scheme’s 90k and 1600 lines.  (Although there are
another 181 lines of Python for the bootstrap interpreter.)

It’s possible to imagine writing and debugging 114 lines of code in a
day, or even 300 lines.  It’s still maybe a bit optimistic to think I could
do that in a day, so maybe I need to find a way to increase
incrementality further.

My theory was that once I had a working compiler, I could add features
to the language incrementally and test them as I went.  So far I
haven’t gotten to that part.

Why? Wirth envy
---------------

[Michael Franz writes][3]:

> In order to find the optimal cost/benefit ratio, Wirth used a highly
> intuitive metric, the origin of which is unknown to me but that may
> very well be Wirth’s own invention. He used the compiler’s
> self-compilation speed as a measure of the compiler’s
> quality. Considering that Wirth’s compilers were written in the
> languages they compiled, and that compilers are substantial and
> non-trivial pieces of software in their own right, this introduced a
> highly practical benchmark that directly contested a compiler’s
> complexity against its performance. Under the self-compilation speed
> benchmark, only those optimizations were allowed to be incorporated
> into a compiler that accelerated it by so much that the intrinsic
> cost of the new code addition was fully compensated.

Wirth is clearly one of the great apostles of simplicity in
programming, together with with Edsger Dijkstra and Chuck Moore.  But
I doubt very much that the Oberon compiler could ever compile itself
in 2 million instructions, given the complexity of the Oberon
language.

R. Kent Dybvig used the same criterion; speaking of the 1985–1987
development of Chez Scheme, [he writes][dybvig]:

> At some point we actually instituted the following rule to keep a
> lid on compilation overhead: if an optimization doesn’t make the
> compiler itself enough faster to make up for the cost of doing the
> optimization, the optimization is discarded. This ruled out several
> optimizations we tried, including an early attempt at a source
> optimizer.


[3]: http://www.ics.uci.edu/~franz/Site/pubs-pdf/BC03.pdf "Oberon — the overlooked jewel"
[dybvig]: http://www.cs.indiana.edu/~dyb/pubs/hocs.pdf "The Development of Chez Scheme"

Far-Fetched Ways This Code Could Actually be Useful
---------------------------------------------------

The obvious way that it could be useful is that you could read it and
learn things from it, then put them to use in actually useful
software.  This section is about the far-fetched ways instead.

If you want to counter Ken Thompson’s “Trusting Trust” attack, you
would want to start with a minimal compiler on a minimal chip;
StoneKnifeForth might be a good approach.

Blind Alleys
------------

Here are some things I thought about but didn’t do.

### Making More Things Into Primitives ###

There are straightforward changes to reduce the executable size
further, but they would make the compiler more complicated, not
simpler.  Some of the most-referenced routines should be open-coded,
which should also speed it up, as well as making them available to
other programs compiled with the same compiler.  Here are the routines
that were called in more than 10 places some time ago:

     11 0x169  xchg
     13 0xc20  Lit
     22 0x190  -  (now replaced by +, which is only used in 25 places)
     25 0x15b  pop
     26 0x1bc  =
     35 0x13d  dup
     60 0x286  .

Of these, `xchg`, `pop`, `-`, `=`, and `dup` could all be open-coded
at zero or negative cost at the call sites, and then their definitions
and temporary variables could be removed.

I tried out open-coding `xchg`, `pop`, `dup`, and `+`.  The executable
shrank by 346 bytes (from 4223 bytes to 3877 bytes, an 18% reduction;
it also executed 42% fewer instructions to compile itself, from
1,492,993 down to 870,863 on the “trimmed” version of itself), and the
source code stayed almost exactly the same size, at 119 non-comment
lines; the machine-code definitions were one line each.  They look
like this:

    dup 'd = [ pop 80 . ; ]               ( dup is `push %eax` )
    dup 'p = [ pop 88 . ; ]               ( pop is `pop %eax` )
    dup 'x = [ pop 135 . 4 . 36 . ; ]     ( xchg is xchg %eax, (%esp)
    dup '+ = [ pop 3 . 4 . 36 . 89 . ; ]  ( `add [%esp], %eax; pop %ecx` )

However, I decided not to do this.  The current compiler already
contains 58 bytes of machine code, and this would add another 9 bytes
to that.  The high-level Forth definitions (`: dup X ! ;` and the
like) are, I think, easier to understand and verify the correctness
of; and they don’t depend on lower-level details like what
architecture we’re compiling for, or how we represent the stacks.
Additionally, it adds another step to the bootstrap process.

### Putting the Operand Stack on `%edi` ###

Forth uses two stacks: one for procedure nesting (the “return stack”)
and one for parameter passing (the “data stack” or “operand stack”).
This arrangement is shared by other Forth-like languages such as
PostScript and HP calculator programs.  Normally, in Forth, unlike in
these other languages, the “return stack” is directly accessible to
the user.

Right now, StoneKnifeForth stores these two stacks mostly in memory,
although it keeps the top item of the operand stack in `%eax`.  The
registers `%esp` and `%ebp` point to the locations of the stacks in
memory; the one that’s currently being used is in `%esp`, and the
other one is in `%ebp`.  So the compiler has to emit an `xchg %esp,
%ebp` instruction whenever it switches between the two stacks.  As a
result, when compiling itself, something like 30% of the instructions
it emits are `xchg %esp, %ebp`.

Inspired, I think, by colorForth, I considered just keeping the
operand stack pointer in `%edi` and using it directly from there,
rather than swapping it into `%esp`.  The x86 has a `stosd`
instruction (GCC calls it `stosl`) which will write a 32-bit value in
`%eax` into memory at `%edi` and increment (or decrement) `%edi` by 4,
which is ideal for pushing values from `%eax` (as in `Lit`, to make
room for the new value).  Popping values off the stack, though,
becomes somewhat hairier.  The `lodsd` or `lodsl` instruction that
corresponds to `stosl` uses `%esi` instead of `%edi`, you have to set
“DF”, the direction flag, to get it to decrement instead of
incrementing, and like `stosl`, it accesses memory *before* updating
the index register, not after.

So, although we would eliminate a lot of redundant and ugly `xchg`
instructions in the output, as well as the `Restack`, `u`, `U`, and
`%flush` functions, a bunch of the relatively simple instruction
sequences currently emitted by the compiler would become hairier.
I think the changes are more or less as follows:

- `!` is currently `pop (%eax); pop %eax`, which is three bytes; this
  occurs 17 times in the output.  The new code would be:
    sub $8, %edi
    mov 4(%edi), %ecx
    mov %ecx, (%eax)
    mov (%edi), %eax
  This is ten bytes.  `store`, the byte version of `!`, is similar.
- `-` is currently `sub %eax, (%esp); pop %eax`, which is four bytes;
  this occurs 23 times in the output.  The new code would be seven
  bytes:
    sub $4, %edi
    sub %eax, (%edi)
    mov (%edi), %eax
  There's something analogous in `<`, although it only occurs three
  times.
- in `JZ`, `jnz`, and `Getchar`, there are occurrences of `pop %eax`,
  which is one byte (88).  The new code would be five bytes:
    sub $4, %edi
    mov (%edi), %eax
  `JZ` occurs 38 times in the output, `jnz` occurs 5 times, and
  Getchar occurs once.
- `Msyscall` would change a bit.

There are 193 occurrences of `push %eax` in the output code at the
moment, each of which is followed by a move-immediate into %eax.
These would just change to `stosd`, which is also one byte.

So this change would increase the amount of machine code in the
compiler source by 10 - 3 + 7 - 4 + 5 - 1 + 5 - 1 + 5 - 1 = 22 bytes,
which is a lot given that there’s only 58 bytes there now; I think
that would make the compiler harder to follow, although `Restack` does
too.  It would also increase the size of the compiler output by (10 -
3) * 17 + (7 - 4) * 23 + (5 - 1) * (38 + 5 + 1) = 364 bytes, although
it would eliminate 430 `xchg %esp, %ebp` instructions, two bytes each,
for a total of 2 * 430 - 364 = 496 bytes less; and the resulting
program would gain (4 - 2) * 17 + (3 - 2) * 23 + (2 - 1) * (38 + 5 +
1) = 101 instructions, then lose the 430 `xchg` instructions.

My initial thought was that it would be silly to space-optimize
popping at the expense of pushing; although they happen the same
number of times during the execution of the program, generally more
data is passed to callees than is returned to callers, so the number
of push sites is greater than the number of pop sites.  (In this
program, it’s about a factor of 2, according to the above numbers.)
Also, consumers of values from the stack often want to do something
interesting with the top two values on the stack, not just discard the
top-of-stack: `-` subtracts, `<` compares, `!` and `store` send it to
memory.  Only `JZ` and `jnz` (and `pop`) just want to discard
top-of-stack — but to my surprise, they make up half of the pops.

However, I wasn’t thinking about the number of places in the compiler
where machine code would be added.  What if I used `%esi` instead of
`%edi`, to get a single-byte single-instruction pop (in the form of
`lodsl`) instead of a single-byte push?  This would make `Lit` (the
only thing that increases the depth of the operand stack) uglier, and
each of the 193 occurrencies of `push %eax` that result from the 193
calls to `Lit` in the compilation process would get four bytes bigger
(totaling 772 extra bytes) but the seven or so primitives that
*decrease* the depth of the operand stack would gain less extra
complexity.  And we’d still lose the `xchg %esp, %ebp` crap, including
the code to avoid emitting them.

What’s Next
-----------

Maybe putting the operand stack on `%esi`.

Maybe factor some commonality out of the implementation of `-` and
`<`.

If we move the creation of the ELF header and `Msyscall` and `/buf` to
run-time instead of compile-time, we could eliminate the `#` and
`byte` compile-time directives, both from the compiler and the
interpreter; the output would be simpler; `Msyscall` and `/buf`
wouldn't need two separate names and tricky code to poke them into the
output; the tricky code wouldn’t need a nine-line comment explaining
it; the characters ‘E’, ‘L’, and ‘F’ wouldn’t need to be magic
numbers.

Maybe factor out "0 1 -"!

Maybe pull the interpreter and compiler code into a literate document
that explains them.

Maybe building a compiler for a slightly bigger and better language on
top of this one.  Maybe something like Lua, Scheme, or Smalltalk.  A
first step toward that would be something that makes parsing a little
more convenient.  Another step might be to establish some kind of
intermediate representation in the compiler, and perhaps some kind of
pattern-matching facility to make it easier to specify rewrites on the
intermediate representation (either for optimizations or for code
generation).

Certainly the system as it exists is not that convenient to program
in, the code is pretty hard to read, and when it fails, it is hard to
debug — especially in the compiler, which doesn’t have any way to emit
error messages.  Garbage collection, arrays with bounds-checking,
finite maps (associative arrays), strong typing (any typing, really),
dynamic dispatch, some thing that saves you from incorrect stack
effects, metaprogramming, and so on, these would all help; an
interactive REPL would be a useful non-linguistic feature.

Related work
------------

Andre Adrian’s 2008 BASICO: <http://www.andreadrian.de/tbng/>

> - is a small imperative programming language that is just powerful
>   enough to compile itself (compiler bootstrapping).
> - has no GOTO, but has while-break-wend and multiple return
> - has C-like string handling.
> - is implemented in less then 1000 source code lines for the compiler.
> - produces real binary programs for x86 processors, not P-code or
>   Byte-Code.
> - uses the C compiler toolchain (assembler, linker)
> - uses C library functions like printf(), getchar(), strcpy(),
>   isdigit(), rand() for run-time support.

Actually it produces assembly, not executables.

Version 0.9 was released 15 Jul 2006.  The 1000 source lines include a
recursive-descent parser and a hand-coded lexer.

Sample code:

    // return 1 if ch is in s, 0 else
    func in(ch: char, s: array char): int
    var i: int
    begin
      i = 0
      while s[i] # 0 do
        if ch = s[i] then
          return 1
        endif
        i = i + 1
      wend
      return 0
    end

FIRST and THIRD, from [the IOCCC entry][5].

[Ian Piumarta’s COLA][6] system.

Oberon.

Fabrice Bellard’s [OTCC][7].

[F-83][8].

eForth, especially the ITC [eForth][9].

Jack Crenshaw’s [Let’s Build a Compiler][4].  This is a how-to book
that walks you through an incrementally-constructed compiler for a toy
language, written in Pascal, in about 340 pages of text.  The text is
really easy to read, but it will still take at least three to ten
hours to read.  It uses recursive-descent parsing, no intermediate
representation, and it emits 68000 assembly code.

Ikarus.

[Ur-Scheme][10].

PyPy.

Bootstrapping a simple compiler from nothing: 
Edmund GRIMLEY EVANS
2001
<http://web.archive.org/web/20061108010907/http://www.rano.org/bcompiler.html>

[4]: http://compilers.iecc.com/crenshaw/
[5]: http://www.ioccc.org/1992/buzzard.2.design
[6]: http://piumarta.com/software/cola/
[7]: https://bellard.org/otcc/
[8]: https://github.com/ForthHub/F83
[9]: http://www.exemark.com/FORTH/eForthOverviewv5.pdf
[10]: http://canonical.org/~kragen/sw/urscheme/
