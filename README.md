# cc2600
cc2600 implements a subset of C compiler for Atari 2600. The main goal of cc2600 is to be able to make games for the Atari 2600
using C language, including writing kernels, not to provide full C support for the 6502 
(have a look at cc65 if this is what you are looking for). Any code written for cc2600 can be compiled with gcc, but not
the other way round... 

## Main features

- Produces DASM compatible code
- Native Atari F4, F6 and F8 bankswitching schemes support
- Superchip (128 bytes of additional RAM!) support
- Uses only 1 byte of RAM
- load/store/strobe intrinsics allow the writing of efficient kernels.
- X and Y registers are directly usable, just like if they ware declared as unsigned char global variables.
- All C constructs are implemented (for, if, while, goto, etc).
- Clean bootstrap/bankswitching code is automatically generated

## Known limitations

- The only data types supported are char (8-bit), short (8-bit) and char pointers (16-bits), and one dimensional arrays of chars.
- Only global variables are supported, not local variables (no use of stack. It's so slow on 6502 and so dangerous due
    to the lack of RAM that it'd be a bad idea anyway)
- Functions can't have arguments and return values (no use of stack). Everything must go through global variables.
- Array subscripts are limited to constants, X and Y.
- 16-bits arithmetics is severly constrained
- No 32-bits operations, no floating point.
- Only basic bankswitching schemes are supported
