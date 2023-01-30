// This file defines hardware registers and memory mapping for the
// Atari 2600. It is distributed as a companion to cc2600.

//------------------------------------------------------------------------------
//
// TIA REGISTERS MEMORY MAP

#ifndef __VCS_H__
#define __VCS_H__

unsigned char * const VSYNC       = 0x00; //  0000 00x0   Vertical Sync Set-Clear
unsigned char * const VBLANK	  = 0x01; //  xx00 00x0   Vertical Blank Set-Clear
unsigned char * const WSYNC       = 0x02; //  ---- ----   Wait for Horizontal Blank
unsigned char * const RSYNC       = 0x03; //  ---- ----   Reset Horizontal Sync Counter
unsigned char * const NUSIZ0	  = 0x04; //  00xx 0xxx   Number-Size player/missle 0
unsigned char * const NUSIZ1	  = 0x05; //  00xx 0xxx   Number-Size player/missle 1
unsigned char * const COLUP0	  = 0x06; //  xxxx xxx0   Color-Luminance Player 0
unsigned char * const COLUP1      = 0x07; //  xxxx xxx0   Color-Luminance Player 1
unsigned char * const COLUPF      = 0x08; //  xxxx xxx0   Color-Luminance Playfield
unsigned char * const COLUBK      = 0x09; //  xxxx xxx0   Color-Luminance Background
unsigned char * const CTRLPF      = 0x0A; //  00xx 0xxx   Control Playfield, Ball, Collisions
unsigned char * const REFP0       = 0x0B; //  0000 x000   Reflection Player 0
unsigned char * const REFP1       = 0x0C; //  0000 x000   Reflection Player 1
unsigned char * const PF0         = 0x0D; //  xxxx 0000   Playfield Register Byte 0
unsigned char * const PF1         = 0x0E; //  xxxx xxxx   Playfield Register Byte 1
unsigned char * const PF2         = 0x0F; //  xxxx xxxx   Playfield Register Byte 2
unsigned char * const RESP0       = 0x10; //  ---- ----   Reset Player 0
unsigned char * const RESP1       = 0x11; //  ---- ----   Reset Player 1
unsigned char * const RESM0       = 0x12; //  ---- ----   Reset Missle 0
unsigned char * const RESM1       = 0x13; //  ---- ----   Reset Missle 1
unsigned char * const RESBL       = 0x14; //  ---- ----   Reset Ball
unsigned char * const AUDC0       = 0x15; //  0000 xxxx   Audio Control 0
unsigned char * const AUDC1       = 0x16; //  0000 xxxx   Audio Control 1
unsigned char * const AUDF0       = 0x17; //  000x xxxx   Audio Frequency 0
unsigned char * const AUDF1       = 0x18; //  000x xxxx   Audio Frequency 1
unsigned char * const AUDV0       = 0x19; //  0000 xxxx   Audio Volume 0
unsigned char * const AUDV1       = 0x1A; //  0000 xxxx   Audio Volume 1
unsigned char * const GRP0        = 0x1B; //  xxxx xxxx   Graphics Register Player 0
unsigned char * const GRP1        = 0x1C; //  xxxx xxxx   Graphics Register Player 1
unsigned char * const ENAM0       = 0x1D; //  0000 00x0   Graphics Enable Missle 0
unsigned char * const ENAM1       = 0x1E; //  0000 00x0   Graphics Enable Missle 1
unsigned char * const ENABL       = 0x1F; //  0000 00x0   Graphics Enable Ball
unsigned char * const HMP0        = 0x20; //  xxxx 0000   Horizontal Motion Player 0
unsigned char * const HMP1        = 0x21; //  xxxx 0000   Horizontal Motion Player 1
unsigned char * const HMM0        = 0x22; //  xxxx 0000   Horizontal Motion Missle 0
unsigned char * const HMM1        = 0x23; //  xxxx 0000   Horizontal Motion Missle 1
unsigned char * const HMBL        = 0x24; //  xxxx 0000   Horizontal Motion Ball
unsigned char * const VDELP0      = 0x25; //  0000 000x   Vertical Delay Player 0
unsigned char * const VDELP1      = 0x26; //  0000 000x   Vertical Delay Player 1
unsigned char * const VDELBL      = 0x27; //  0000 000x   Vertical Delay Ball
unsigned char * const RESMP0      = 0x28; //  0000 00x0   Reset Missle 0 to Player 0
unsigned char * const RESMP1      = 0x29; //  0000 00x0   Reset Missle 1 to Player 1
unsigned char * const HMOVE       = 0x2A; //  ---- ----   Apply Horizontal Motion
unsigned char * const HMCLR       = 0x2B; //  ---- ----   Clear Horizontal Move Registers
unsigned char * const CXCLR       = 0x2C; //  ---- ----   Clear Collision Latches
 
//-------------------------------------------------------------------------------

unsigned char * const CXM0P       = 0x30; // xx00 0000       Read Collision  M0-P1   M0-P0
unsigned char * const CXM1P       = 0x31; // xx00 0000                       M1-P0   M1-P1
unsigned char * const CXP0FB      = 0x32; // xx00 0000                       P0-PF   P0-BL
unsigned char * const CXP1FB      = 0x33; // xx00 0000                       P1-PF   P1-BL
unsigned char * const CXM0FB      = 0x34; // xx00 0000                       M0-PF   M0-BL
unsigned char * const CXM1FB      = 0x35; // xx00 0000                       M1-PF   M1-BL
unsigned char * const CXBLPF      = 0x36; // x000 0000                       BL-PF   -----
unsigned char * const CXPPMM      = 0x37; // xx00 0000                       P0-P1   M0-M1
unsigned char * const INPT0       = 0x38; // x000 0000       Read Pot Port 0
unsigned char * const INPT1       = 0x39; // x000 0000       Read Pot Port 1
unsigned char * const INPT2       = 0x3A; // x000 0000       Read Pot Port 2
unsigned char * const INPT3       = 0x3B; // x000 0000       Read Pot Port 3
unsigned char * const INPT4       = 0x3C; // x000 0000       Read Input (Trigger) 0
unsigned char * const INPT5       = 0x3D; // x000 0000       Read Input (Trigger) 1

//------------------------------------------------------------------------------
//
// RIOT MEMORY MAP

unsigned char * const SWCHA       = 0x280; //   Port A data register for joysticks:

unsigned char * const SWACNT      = 0x281; //   Port A data direction register (DDR)
unsigned char * const SWCHB       = 0x282; //		Port B data (console switches)
unsigned char * const SWBCNT      = 0x283; //   Port B DDR
unsigned char * const INTIM       = 0x284; //		Timer output

unsigned char * const TIMINT  	  = 0x285;

unsigned char * const TIM1T       = 0x294; //		set 1 clock interval
unsigned char * const TIM8T       = 0x295; //   set 8 clock interval
unsigned char * const TIM64T      = 0x296; //   set 64 clock interval
unsigned char * const T1024T      = 0x297; //   set 1024 clock interval
                                           //
#endif // __VCS_H__
