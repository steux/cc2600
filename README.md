<p align="center">
  <img width="50%" src="https://github.com/steux/cc2600/raw/main/misc/cc2600.png" />
</p>

# cc2600

cc2600 implements a subset of C compiler for Atari 2600. The main goal of cc2600 is to enable making games for the Atari 2600
using C language, including writing kernels, not to provide full C support for the 6502 
(have a look at cc65 if this is what you are looking for). Any code written for cc2600 can be compiled with gcc, but not
the other way round... The 6502 processor is famous for being an inefficient target for C compilers, due to its poor
stack support, its infamous indexing modes and its lack of registers. In addition to the limitations of the Atari 2600 (128
bytes of RAM, strong reliance on bankswitching, low speed in general), the use of pure C on this platform is limited.
cc2600 tries to cope with these limitations by not strictly implementing all C features but mapping C syntax to the specifics of 6502,
in particular indexing modes.

Note that this compiler is for writing "old school" code for ATARI 2600. It's not meant to be used for CDFJ (custom ARM
code on the Melody/Harmony cart) development, where the 6507 code is reduced to the minimum. On the contrary, it was
designed to write code the classical atari way, possibly with DPC or DPC+ accelerators or a superchip for more RAM.

cc2600 should not be a starting point for writing ATARI 2600. You'll first have to learn writing games
in assembler (I definitely recommand reading "Making Games For The Atari 2600", by Steven Hugg, see https://8bitworkshop.com/docs/books).
On the other hand, if you're an experienced ASM code writer, you may gain a lot of time using cc2600 for your
next game developement, since cc2600 will enable you to leverage the use of structural code writing.

cc2600 is implemented in the Rust programming language, a touch of modernity for a 45 years old console... The C language grammar has been handwritten from scratch as a PEG grammar, so don't expect any ANSI or ISO C compliance.  

## Main features


- Produces DASM compatible code ([DASM](https://github.com/dasm-assembler/dasm) is required as a second stage compiler) 
- Native Atari F4, F6 and F8, 3E (lots of RAM!), DPC and DPC+ bankswitching schemes support
- Superchip (128 bytes of additional RAM!) support
- Uses only 1 byte of RAM
- load/store/strobe intrinsics allow the writing of efficient kernels.
- X and Y registers are directly mapped to X and Y variables, just like if they were declared as unsigned char global variables.
- All C constructs are implemented (for, if, while, goto, etc).
- Clean bootstrap/bankswitching code is automatically generated
- PlusROM support for Wifi communication with PlusCART

## Known limitations


- The only data types supported are char (8-bit), short (16-bit) and char pointers (16-bits), and one dimensional arrays of these types.
- Only global variables are supported, not local variables (no use of stack. It's so slow on 6502 and so dangerous due
    to the lack of RAM that it'd be a bad idea anyway)
- Functions can't have arguments and return values (no use of stack). Everything must go through global variables.
- Array subscripts are limited to constants, X and Y variables / registers.
- 16-bits arithmetics is severly constrained. Generated code may not work if too complex (carry propagation is not ensured).
- No 32-bits operations, no floating point.
- Works with one C file. No linking provided. Use `#include "other_file.c"` to cope with this.

## How to install

Installing from source is quite straightforward when Rust Cargo is available on your platform. If this is not the case, please
use [rustup](https://www.rust-lang.org/tools/install) to install it, then use `cargo install --path .` in the root
directory to compile and install cc2600 locally. `cargo test` launches the unit tests of cc2600.

You can install the binary directly using Cargo by typing `cargo install cc2600`

If you definitely don't want to install Rust (quite a shame), you can use the Windows installer provided.

## Examples of code using cc2600

A rather complete example of what is possible with cc2600 is the [HappyBird](https://github.com/steux/happybird) game, freely available for download. This example demonstrates a lot of different features : use of inlined assembler (for savekey i2c communication), 48 pixels wide graphics display, ROMplus access, bankswitching, indirect addressing via pointers, etc. 

A few examples are also available in the `examples` directory. There is a Makefile in the folder, but it should only work on Linux. If you want to build yourself the magnificient DPC (David Patrick Crane coprocessor) example featuring Garfield, type :

`cc2600 -Iheaders examples/test_dpc.c`

This will produce `out.a`, which is a DASM compatible source code.

Type `dasm out.a -f3 -v4 -oout.bin -lout.lst -sout.sym` to make the cartridge.

You can then use the stella emulator to run the binary `out.bin`, or copy it on a Harmony ou PlusCart cartridge.

## Technical details

### Bankswitching

Bankswitching is hidden under the carpet by cc2600. Just specify `bank1` to `bank*n*` before the actual definition to locate either the data or the code into the given bank. cc2600 will compute the number of banks at compile time and will generate the cartridge according to this. Not specifying anything puts the data into bank0, which is the default starting bank. Function calls from bank to bank are allowed only from bank0 to bank*n* and from bank*n* to bank*n*. Bankswitching code is automatically inserted if necessary.

#### 3E bankswitching

For heavy professionals, 3E bankswitching is possible with cc2600. Just include the "3e.h" header and it will be selected. Declare the variables with bank1 to bank*n* and they will be put in RAM banks (1kB each. Max number is limited by the cart implementation and thus unknown). ROM banks are 2KB size instead of 4KB for Atari classical bankswitching methods. Bank0 is put last in ROM, since with 3E bankswitching method, the last bank is always active in the last 2KB of accessible memory. Ah yes! Don't forget to use the `select(x)` macro to manually select the x*th* RAM bank in the first part of memory. Start from 0 for `select()`, while start from `bank1` for variables allocation. Have a look at `test_3e.c` if you want to start from something working.

#### DPC bankswitching

DPC coprocessor support is implmented. Use "dpc.h" header to activate it. 2kB display ROM is filled using the keywork `display`, and ROM size for the code is fixed to 8kB. It's time to implement your own pitfall 3.

#### DPC+ bankswitching

DPC+ coprocessor is also supported, and opens up to 4KB of display RAM, 2B of music/frequency ROM (prefilled using `dpcplus_frequencies.h`), and 24KB of data for your game. Think big. DPC+ generate code work as it on Stella, but you'll have to prepend the ARM code "driver" (DPC+.arm) to make it work on Harmony cart or CartPlus (though the latter doesn't uses it). Cry for help on AtariAge forum if you don't understand a word about that.

### Superchip

Superchip support is automatically activated if you use the keywork `superchip` before a variable declaration. It yields 128 bytes of additionnal RAM. Note that this is not compatible with 3E, DPC and DPC+ bankswitching schemes (but 3E and DPC+ provide some RAM by other ways).

### Intrinsics

cc2600 supports a few intrinsics to help making ASM-like tuned code :

- `load(expr)` loads the 6502 accumulator with `expr`. It implements a `LDA` isntruction.

- `store(expr)` stores `expr` into the accumulator. It implements a `STA` isntruction.

- `strobe(pointer)` implements a `STA` instruction also. It's just the same as store, but accepts only pointers. Typically used for your numerous `strobe(WSYNC)` instructions in your kernel...

- `asm(string)` inlines the given assembler instruction into the C code. Particularly useful to call ASM function (use `asm("jsr asm_function")`).

- `csleep(int)` stands for cycle sleep. Helps to insert nops in the code. Implemented for 2 to 10 cycles.

### Assembly code insertion

You can insert assembly code using the `#include`. If the filename provided ends with ".a" or ".inc", it will be considered as assembler and inserted in the DASM generated code. You can also inline code using the following special tag `=== ASSEMBLER BEGIN ===`. For instance, to activate the i2c code from "i2c.inc" (savekey code) and tell the macros to use the `i` temporary variable, just type :
```
=== ASSEMBLER BEGIN ===
    I2C_SUBS i
==== ASSEMBLER END ====
```

### 16-bits arithmetics support

16-bits arithmetics is supported, BUT beware to use only simple expressions (like a simple addition, or `+=`, not multiple additions on the same line of code), since carry propagation is not ensured (maybe will it be in the future). In particular 16-bits operations are not supported in comparisons. Use `short` to declare a 16-bits variable. `char *` are also 16-bits variables, since address space on 6502 is 16-bits wide.

In order to convert from 16-bits to 8-bits, use the `>> 8` special operation to get the higher byte of a `short`, and use nothing to get the lower byte.

### Optimizations

X and Y are `unsigned char` typed, BUT in order to optimize the loops, they are considered `signed char` when compared to 0. Hence the code `do { something; Y-- } while (Y >= 0);` will be implemented with a `BPL` (branch if plus) instruction, just like you would do in assembler. Beware then that if Y > 128, due to the complement-to-2 binary representation, it will be considered negative number and the loop will exit immediately (i.e. don't use this for your 192 lines kernel loop. Use `Y > 0` comparison which uses the carry flag).

## TODO

- [ ] Provide more examples
- [ ] Fix 16 bits arithmetics so that it becomes more usable...
- [X] Implement sign extend (for 8 bit to 16 bits variable assignment)
- [ ] DWARF data output for debugging with Gopher2600
- [X] Add 3E+ bankswitching scheme support 

<p align="center">
  In Memoriam</br>
  <img src="https://github.com/steux/cc2600/raw/main/misc/Chuck Peddle.png" /></br>
  Chuck Peddle 1937 - 2019
</p>
