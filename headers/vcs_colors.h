// This file defines standard colors for the Atari 2600 
// It is distributed as a companion to cc2600.

#ifndef __VCS_COLORS_H__
#define __VCS_COLORS_H__

#define VCS_BLACK           0x00
#define VCS_WHITE           0x0E
#ifdef PAL
// Main PAL colors are defined to be compatible with SECAM
#define VCS_BLUE      0xD2
#define VCS_RED       0x64
#define VCS_PURPLE    0xA6
#define VCS_LGREEN    0x58
#define VCS_LBLUE     0xBA
#define VCS_YELLOW    0x2C
// Others are PAL only. Beware with SECAM systems
#define VCS_ORANGE    0x4A
#define VCS_GREEN     0x54
#define VCS_BROWN     0x24
#define VCS_DGRAY     0x04
#else // NTSC colors
#define VCS_BLUE      0x72
#define VCS_RED       0x32
#define VCS_PURPLE    0x56
#define VCS_LGREEN    0xC8
#define VCS_LBLUE     0x9A
#define VCS_YELLOW    0x1C
#define VCS_ORANGE    0x3A
#define VCS_GREEN     0xC4
#define VCS_BROWN     0x14
#define VCS_DGRAY     0x02
#endif

#define VCS_GRAY      0x06
#define VCS_LGRAY     0x0C
#define VCS_GREY      0x06
#define VCS_LGREY     0x0C

#endif // __VCS_COLORS_H__
