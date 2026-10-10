# ports/bcc: the C compiler for XENIX

The compiler, assembler and linker of this Development System are **BCC**, from
Dev86 0.16.17: Bruce Evans's C compiler, assembler (as86) and linker (ld86),
with Robert de Bath's preprocessor and the work of the Dev86 contributors.
BCC is a K&R C compiler for the 8086 that was written to run on small 16-bit
systems, and the linker's source still carries a work-around for "a bug in
Xenix 2.5 cc".

This directory holds no BCC source. `fetch.sh` downloads the Dev86 0.16.17
tarball (the one Debian and Ubuntu ship as `linux86_0.16.17.orig.tar.gz`),
checks its SHA-256, unpacks it under `build/ports` and applies the patches here.
`build.sh` then compiles the four programs as XENIX executables, against this
project's own `libc.a` and headers.

| Program  | Role                     | Code size (64K limit) |
|----------|--------------------------|-----------------------|
| bcc-cpp  | preprocessor             | about 25K             |
| bcc-cc1  | compiler                 | about 64K             |
| as86     | assembler                | about 33K             |
| ld86     | linker                   | about 29K             |

The `cc` command that runs them, and `aout2x`, which turns ld86's output into
a XENIX x.out, are this project's own (`cmd/`).

## The patches

BCC is normally built on 32- and 64-bit hosts. Built as a 16-bit program for
XENIX it showed three problems, each found by comparing its output with the
host's BCC, file by file:

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

## Building and testing

    ./dist.sh                     # everything, laid out under build/xenix/root
    python3 ports/bcc/test.py     # about a minute

`test.py` runs each XENIX program on an emulated 8086 (`tools/xsim.py`) next to
the host's BCC: preprocessor, compiler and assembler output, and linked
programs, must be byte-identical across this project's library, tests and
BCC's own sources. Then the native `cc` must build every test program exactly
as the cross-compiler does, and the native toolchain must rebuild `bcc-cc1`,
`cc` and `aout2x` byte for byte. One difference is expected and checked:
the host's 64-bit build of bcc-cc1 folds `(unsigned long)-1 >> 1` wrongly;
the 16-bit build gets it right.

## Not yet

No floating point: `float` and `double` are rejected at compile time, and
`cc` predefines `__HAS_NO_FLOATS__`. It comes with the 8087 work.

## License

Dev86 is distributed under the GNU General Public License, version 2. These
patches and scripts are under the same terms as the rest of this project
(GPL version 2 or later), so the combination is GPL version 2.
