#include "vcs_colors.h"

#define BLANK 40
#define OVERSCAN 30

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

#define MS_NB_SPRITES_DEF 1
#define MS_KERNEL_DATA \
const char garfield[24] = { 0, 0, 0x12, 0x36, 0x4a, 0x33, 0x55, 0x33, 0xcb, 0xb6, 0x48, 0x3e, 0x5e, 0x6e, 0x76, 0x36, 0x84, 0xbc, 0x3a, 0x76, 0x66, 0x66, 0, 0}; \
const char garfield_colors[22] = { 0, 0, 0x3c, 0x3c, 0x3c, 0x0e, 0x0e, 0x0e, 0x3c, 0x3c, 0x3c, 0x3c, 0x38, 0x2c, 0x3c, 0x3c, 0x38, 0x38, 0x2c, 0x2c, 0x12, 0x12}; \
const char *ms_grptr[MS_NB_SPRITES_DEF] = {garfield}; \
const char *ms_coluptr[MS_NB_SPRITES_DEF] = {garfield_colors}; \
const char ms_height[MS_NB_SPRITES_DEF] = {24};

const char playfield[192 + 32] = {
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK,
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK,
    VCS_RED, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK
};

#define MS_SELECT_FAST
#include "multisprite.h"

void main()
{
    char xpos = 50, ypos = 50, scrolling = 0;
    multisprite_init(playfield);
    multisprite_new(0, xpos, ypos, 0);
    multisprite_new(0, 50, 50, 3);
    multisprite_new(0, 50, 70, 3);
    multisprite_new(0, 100, 50, 3);
    multisprite_new(0, 30, -10, 3);
    multisprite_new(0, 30, 170, 3);
    multisprite_new(0, 50, 180, 3);
    
    do {
        *VBLANK = 2; // Enable VBLANK
        *VSYNC = 2; // Set VSYNC
        strobe(WSYNC); // Hold it for 3 scanlines
        strobe(WSYNC);
        strobe(WSYNC);
        *VSYNC = 0; // Turn VSYNC Off
        
        // Blank
        *TIM64T = ((BLANK - 3) * 76) / 64 - 3;
        // Do some logic here
        if (!(*SWCHA & 0x80) && xpos < 158) { xpos++; ms_nusiz[0] = 0; } // Right
        if (!(*SWCHA & 0x40) && xpos > 0) { xpos--; ms_nusiz[0] = 8; } // Left
        if (!(*SWCHA & 0x20)) { ypos++; } // Down
        if (!(*SWCHA & 0x10)) { ypos--; }// Up

        multisprite_move(0, xpos, ypos);

        ms_scenery = playfield - MS_OFFSET + scrolling;
        scrolling -= 2;
        if (scrolling < 0) scrolling = 32;

        multisprite_kernel_prep();
        while (*INTIM); // Wait for end of blank
 
        multisprite_kernel();
        
        // Overscan
        strobe(WSYNC);
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = ((OVERSCAN) * 76) / 64 + 2;
        // Do some logic here
        multisprite_kernel_post();  
        while (*INTIM); // Wait for end of overscan
    } while(1);
}
