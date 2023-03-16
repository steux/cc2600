// 3E+ bankswitching scheme macros
// Copyleft 2023 Bruno STEUX
//
#ifndef __3E_PLUS__
#define __3E_PLUS__

// Use shadow registers
#define TIA_BASE_ADDRESS 0x40

const unsigned char * const cc3eplus_marker = "TJ3E";
unsigned char * const ROM_SELECT = 0x3f; // ROM bank selection. Automatically selected.
unsigned char * const RAM_SELECT = 0x3e; // RAM bank selection

// 3E+ segments are pre associated to banks :
// RAM or ROM Bank0 is mapped at 0x1c00 (segment 3)
// RAM or ROM Bank1 is mapped at 0x1800 (segment 2)
// RAM or ROM Bank2 is mapped at 0x1400 (segment 1)
// RAM or ROM Bank3 is mapped at 0x1000 (segment 0)
// RAM or ROM Bank4 is mapped at 0x1c00 (segment 3)
// etc.

// Rule 1: Never select a RAM bank which has the same segment as the current executing ROM bank !
// Rulé 2: RAM bank selection is manual. Use select_ram macro
#define select_ram(x) *RAM_SELECT = (((3 - (x & 3)) << 6) | x);
// Rule 3: ROM selection is also manual. Use select_rom macro. 
#define select_rom(x) *ROM_SELECT = (((3 - (x & 3)) << 6) | x);
// On startup, bank0 to bank3 ROM are selected for their respective segment, covering all the 4kb adressable by the
// Atari. No RAM bank is selected by default.

#endif // __3E_PLUS__
