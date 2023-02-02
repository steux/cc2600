# cc2600

cc2600 implements a subset of C compiler for Atari 2600. The main goal of cc2600 is to be able to make games for the Atari 2600
using C language, including writing kernels, not to provide full C support for the 6502 
(have a look at cc65 if this is what you are looking for). Any code written for cc2600 can be compiled with gcc, but not
the other way round... The 6502 processor is famous for being an inefficient target for C compilers, due to its poor
stack support, its infamous indexing modes and its lack of registers. In addition the limitations of the Atari 2600 (128
bytes of RAM, strong reliance on bankswitching, low speed in general), the use of pure C on this platform is limited.
cc2600 tries to cope with these limitations by not strictly implementing all C features but mapping C syntax to the specifics of 6502,
in particular indexing modes.

Note that this compiler is for writing "old school" code for ATARI 2600. It's not meant to be used for CDFJ (custom ARM
code on the Melody/Harmony cart) development, where the 6507 code is reduced to the minimum. On the contrary, it was
designed to write code the classical atari way, possibly with DPC or DPC+ accelerators or a superchip for more RAM.

cc2600 should not be a starting point for writing ATARI 2600. You'll first have to learn writing games
in assembler (I definitely recommand reading "Making Games For The Atari 2600", by Steven Hugg, see http://8bitworkshop.com/docs/book).
On the other hand, if you're an experienced ASM code writer, you may gain a lot of time using cc2600 for your
next game developement, since cc2600 will enable you to leverage the use of structural code writing.


## Main features


- Produces DASM compatible code
- Native Atari F4, F6 and F8, 3E (lots of RAM!), DPC and DPC+ bankswitching schemes support
- Superchip (128 bytes of additional RAM!) support
- Uses only 1 byte of RAM
- load/store/strobe intrinsics allow the writing of efficient kernels.
- X and Y registers are directly usable, just like if they ware declared as unsigned char global variables.
- All C constructs are implemented (for, if, while, goto, etc).
- Clean bootstrap/bankswitching code is automatically generated
- PlusROM support for Wifi communication with PlusCART

## Known limitations


- The only data types supported are char (8-bit), short (8-bit) and char pointers (16-bits), and one dimensional arrays of chars.
- Only global variables are supported, not local variables (no use of stack. It's so slow on 6502 and so dangerous due
    to the lack of RAM that it'd be a bad idea anyway)
- Functions can't have arguments and return values (no use of stack). Everything must go through global variables.
- Array subscripts are limited to constants, X and Y.
- 16-bits arithmetics is severly constrained
- No 32-bits operations, no floating point.

## How to install

Installing from source is quite straightforward when Rust is available on your platform. If this is not the case, please
use [rustup](https://www.rust-lang.org/tools/install) to install it, then use `cargo install --path .` in the root
directoy to compile and install cc2600 locally. `cargo test` launches the unit tests of cc2600.

## TODO

- [ ] Windows installer
