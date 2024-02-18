#include "vcs_colors.h"

#define MS_OFFSCREEN_BANK bank0
#define MS_KERNEL_BANK bank1

#include "example_gas_paddles_gfx.c"

#define BLANK 40
#define OVERSCAN 30

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

MS_KERNEL_BANK const char playfield[192 + 32] = {
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

const signed char dx[24] = {40, 38, 34, 28, 19, 10, 0, -10, -20, -28, -34, -38, -40, -38, -34, -28, -19, -10, 0, 10, 19, 28, 34, 38};
const signed char dy[24] = {0, 16, 32, 45, 55, 61, 64, 61, 55, 45, 32, 16, 0, -16, -31, -45, -55, -61, -64, -61, -55, -45, -32, -16};

int xpos[4], ypos[4];
char direction[4];
const char car_model[24] = {6, 7, 8, 9, 10, 11, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5}; 
const char car_reflect[24] = {0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0};


void game_init() 
{
    char i = 100, j;
    for (X = 0; X < 4; X++) {
        xpos[X] = 80 * 256;
        ypos[X] = i << 8;
        i += 20;
        direction[X] = 0;
        j = X;
        multisprite_new(6, xpos[X] >> 8, ypos[X] >> 8, 0);
        X = j;
    } 
}

void main()
{
    game_init();

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
