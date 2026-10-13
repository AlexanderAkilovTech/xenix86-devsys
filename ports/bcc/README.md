# ports/bcc: the C compiler for XENIX

The compiler, assembler and linker of this Development System are **BCC**, from
Dev86 0.16.17: Bruce Evans's C compiler, assembler (as86) and linker (ld86),
with Robert de Bath's preprocessor and the work of the Dev86 contributors.
The archiver, ar86, is the GNU ar (1988, 1990) that Dev86 ships, and `cc -O`
runs Dev86's peephole optimizer, copt (Gero Kuhlmann's, after an idea of
Christopher W. Fraser's).
BCC is a K&R C compiler for the 8086 that was written to run on small 16-bit
systems, and the linker's source still carries a work-around for "a bug in
Xenix 2.5 cc".

This directory holds no BCC source. `fetch.sh` downloads the Dev86 0.16.17
tarball (the one Debian and Ubuntu ship as `linux86_0.16.17.orig.tar.gz`),
checks its SHA-256, unpacks it under `build/ports` and applies the patches here.
`host.sh` builds BCC's compiler pass and copt for the host from the same patched
source: the project cross-compiles with them (`bcc -B build/ports/host/`), never
with the host's packaged ones, so cross-compiled and native programs come from
identical compiler code. `build.sh` then compiles the programs as XENIX executables,
against this project's own `libc.a` and headers.

| Program  | Role                     | Code size (64K limit) |
|----------|--------------------------|-----------------------|
| bcc-cpp  | preprocessor             | about 30K             |
| bcc-cc1  | compiler                 | 65,184 bytes          |
| copt     | peephole optimizer       | about 15K             |
| as86     | assembler                | about 33K             |
| ld86     | linker                   | about 30K             |
| ar86     | archiver                 | about 24K             |

bcc-cc1 is the tight one: with floating point it fits only as "callersaves"
code (`bcc-cc1 -c`, which BCC's driver selects with `-Mc`: a function keeps
no registers for its caller), passed through copt with patch 11's rules and
`rules.mc`, and assembled with `as86 -O`, which makes each jump as short as it
can be. `build.sh` builds it that way, and the native `cc -O -Mc` rebuilds it
byte for byte.

The `cc` command that runs them, and `aout2x`, which turns ld86's output into
a XENIX x.out, are this project's own (`cmd/`).

## The patches

BCC is normally built on 32- and 64-bit hosts. Built as a 16-bit program for
XENIX it showed these problems, each found by comparing its output with the
host's BCC, file by file, or by a test program failing:

1. `01-int-to-long-args.patch`: 15 places pass an `int` where the function
   takes a `long` offset. That is harmless where `int` and `long` are the same
   size, and wrong in 16-bit K&R C (frame offsets came out as `$40004`).
   Found with `gcc -Wtraditional-conversion`.
2. `02-no386-long-return.patch`: without 386 support (needed to fit in 64K), a
   dangling `else` inside `#ifdef I80386` overwrote the register of every
   `long` a function returned.
3. `03-as86-xenix-heap.patch`: as86's 16-bit memory setup assumed the ELKS and
   Minix layout, with the stack above the heap. XENIX puts the stack below the
   data break; on XENIX (`M_XENIX`) as86 now takes one large block from malloc.
4. `04-ar-16bit.patch`: GNU ar kept archive offsets in `int` and passed them to
   `fseek` and `lseek`, which take a `long`. Offsets are now `long`, so archives
   can pass 64K, member dates are read with `atol`, and member sizes up to 64K
   are handled as unsigned.
5. `05-struct-argument.patch`: passing a struct by value copied it to the wrong
   address. The compiler put the destination in a register, then loaded the
   length into the same one, so `memcpy` copied the struct to address 6 or so.
   This one is in the host's BCC too; it was found because GNU ar passes a
   struct to its per-member functions and could not extract anything.
6. `06-xenix-headers.patch`: XENIX's headers are System V's of 1986, from before
   ANSI C and POSIX. `time.h` does not define `time_t` (it is in `sys/types.h`)
   and there is no `mode_t`. bcc-cpp now includes `sys/types.h`, and ld86
   defines `mode_t` on XENIX, as it already did for MS-DOS. Found when the tools
   were built against SCO's own `sys/types.h`, the one they meet once installed.

The floating point work found four bugs in BCC's code generation, all of them
in the host's BCC too, found by the floating point tests (`tests/unit`):

7. `07-float-char.patch`: a `char` converted to `double` was converted as
   unsigned whether it was signed or not: the helper chosen could not tell a
   plain `char` from a `signed char`. Chars and shorts are now widened to `int`
   first, as assignment does.
8. `08-double-init.patch`: a `double` in an initializer was written as two
   halves the size of a `long`; on a 64-bit host the second half lay past the
   double, so the cross-compiler wrote garbage into static doubles. The halves
   are now built from the bytes.
9. `09-condop-stack.patch`: in `c ? a : b`, the two arms could leave the stack
   at different depths (a `double` arm leaves temporaries on it), so the code
   after it cleaned up the wrong amount and the function returned through
   garbage; `tan()` crashed on it.
10. `10-float-const-reg.patch`: in an operation on a `double` held in registers
    (a function's result) and a constant, as in `0.5 * exp(x)`, the constant was
    pushed through AX, which held a word of the other operand, and that word
    was lost.

And five more to fit floating point into the 8086 programs:

11. `11-copt-rules.patch`: Dev86's rules for copt miscompile bcc-cc1 ("compiler
    bug? - bad address"). Every rule was checked against the code BCC makes;
    those that can change what a program does are removed or narrowed (a rule
    that drops a load into BX, SI or DI that later code still uses; one whose
    operand may be the register it loads; a stack adjustment moved across a
    pop or a ret; std ... cld; a label renamed or moved). New `rules.mc`, for
    callersaves code, pops a C function's arguments into SI, a byte shorter
    than adjusting SP.
12. `12-copt-xenix.patch`: copt in K&R C, so that BCC compiles it, reading its
    input a piece at a time (up to the next global label, or 1000 lines) so it
    runs in a 16-bit program's 64K of data; Dev86's kept every line it read for
    good. Its output is the same as Dev86's copt gives on all 185 files tried.
    It is also nearly twice as fast on an 8086, skipping a rule early when its first
    line cannot match.
13. `13-cc1-hex-floats.patch`: there is no room in bcc-cc1 for `atof` (a
    correctly rounding one needs big integer arithmetic). It now reads floating
    constants written in hexadecimal, in one fixed form, `0x1.921fb54442d18p1`,
    building the double's bits directly; built with `-DNO_ATOF`, as for XENIX,
    it has no decimal reader at all.
14. `14-cpp-hex-floats.patch`: so `bcc-cpp -H`, as `cc` runs it, writes every
    decimal floating constant as that hexadecimal form, using this project's
    `strtod`, which rounds correctly: the compiler gets the same double as from
    the host's `atof`. The tokeniser also learns that `.5`, `0.5` and `017e2`
    are floating constants.
15. `15-ld-archive-name.patch`: ld86 kept a copy of an archive's path for each
    member it read; with the floating point routines in `libc.a` that ran it out
    of memory linking bcc-cc1 on XENIX. It keeps one now, and `build.sh` gives it
    a 4K stack instead of 8K (it uses under 300 bytes), leaving its tables room
    to spare.

## Building and testing

    ./dist.sh                     # everything, laid out under build/xenix/root
    python3 ports/bcc/test.py     # about four minutes

`test.py` runs each XENIX program on an emulated 8086 (`tools/xsim.py`) next to
the host build of the patched compiler and the host's preprocessor, assembler,
linker and archiver: preprocessor, compiler and assembler output, and linked
programs, must be byte-identical across this project's library, tests and
BCC's own sources (the XENIX bcc-cc1 reads the native bcc-cpp's output with
floating constants in hexadecimal, the host's the decimal ones). Then the native `cc` must build every test program exactly
as the cross-compiler does, the native ar86 must make the same archives as the
host's (including one over 64K), and the native toolchain must rebuild
`bcc-cc1`, `cc` and `aout2x` byte for byte. One difference is expected and checked:
a 64-bit host build of bcc-cc1 folds `(unsigned long)-1 >> 1` wrongly;
the 16-bit build gets it right.

## Floating point

`float` and `double` work, in software: the C library's routines do the
arithmetic with the 8086's integer instructions, so programs need no 8087 and
give the same results on any XT. A version that uses the 8087 is not done:
XENIX 2.1.3 does not save the 8087's registers when it switches between
processes (measured on 86Box, `tests/fpu87.c`), so two programs using it at
once would corrupt each other's results.

## License

Dev86 is distributed under the GNU General Public License, version 2. These
patches and scripts are under the same terms as the rest of this project
(GPL version 2 or later), so the combination is GPL version 2.
