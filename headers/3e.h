// 3E bankswitching scheme macros
// Copyleft 2023 Bruno STEUX
//
#ifndef __3E__
#define __3E__

unsigned char * const ROM_SELECT = 0x3f; // ROM bank selection
unsigned char * const RAM_SELECT = 0x3e; // RAM bank selection

#define select(x) *RAM_SELECT = x

#endif // __3E__
