// 3E+ bankswitching scheme macros
// Copyleft 2023 Bruno STEUX
//
#ifndef __3E_PLUS__
#define __3E_PLUS__

const unsigned char * const cc3eplus_marker = "TJ3E";
unsigned char * const ROM_SELECT = 0x3f; // ROM bank selection. Automatically selected.
unsigned char * const RAM_SELECT = 0x3e; // RAM bank selection

// 3E+ segments are pre associated to banks :
// RAM or ROM Bank0 is mapped at 0x1c00
// RAM or ROM Bank1 is mapped at 0x1800
// RAM or ROM Bank2 is mapped at 0x1400
// RAM or ROM Bank3 is mapped at 0x1000
// RAM or ROM Bank4 is mapped at 0x1c00
// etc.

// Rule 1 :Never select a RAM bank which has the same segment as the current executing ROM bank !
// Rulé 2: RAM bank selection is manual. Use select_ram macro
#define select_ram(x) *RAM_SELECT = (((3 - (x & 3)) << 6) | x);
// Rule 3: ROM selection is automatic, based on the called function definition

#endif // __3E_PLUS__
