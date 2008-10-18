StoneKnifeForth
===============

This is StoneKnifeForth, a very simple language inspired by Forth.
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
source code is 1979 bytes, which is less than half the size of the
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

The output executable is 6268 bytes, containing about 2550
instructions.  `valgrind` reports that it takes 1,901,062 instructions
to compile itself.  (The long runtimes are a result of reading its
input one byte at at time.)

Reducing Executable Size
------------------------

There are straightforward changes to reduce it to below 4096 bytes,
but they will make the compiler more complicated, not simpler.  The
tables T and A should be moved past the end of the executable text,
and some of the most-referenced routines should be open-coded, which
should also speed it up.  Here are the routines that are called in
more than 10 places:

     11 0x169  xchg
     13 0xc20  Literal
     22 0x190  -  (note that + is only used in 25 places)
     25 0x15b  pop
     26 0x1bc  =
     35 0x13d  dup
     60 0x286  .

Of these, xchg, pop, -, =, and dup could all be open-coded at zero or
negative cost at the call sites, and then their definitions and
temporary variables could be removed.

Improving Clarity
-----------------

If routine names could be more than one character, it would help a
lot; this should be possible with minimal extra implementation
complexity if only the first character is significant for comparisons.
Also, if we could name input characters by example, as '*' or ?*
instead of 42, it would make the compiler considerably clearer and
probably a bit faster.

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
sure as hell can’t.  It's still not a very large compiler, as
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
lot smaller than Ur-Scheme's 90k and 1600 lines.  (Although there are
another 181 lines of Python for the bootstrap interpreter.)

It’s possible to imagine writing and debugging 114 lines of code in a
day, or even 300.  It’s still maybe a bit optimistic to think I could
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
> highly practical benchmark that directly contested a compiler's
> complexity against its performance. Under the self-compilation speed
> benchmark, only those optimizations were allowed to be incorporated
> into a compiler that accelerated it by so much that the intrinsic
> cost of the new code addition was fully compensated.

Wirth is clearly one of the great apostles of simplicity in
programming, together with with Edsger Dijkstra and Chuck Moore.  But
I doubt very much that the Oberon compiler could ever compile itself
in 2 million instructions, given the complexity of the Oberon
language.

[3]: http://www.ics.uci.edu/~franz/Site/pubs-pdf/BC03.pdf "Oberon — the overlooked jewel"