#include "vcs_colors.h"

#define EXTRA_RAM superchip
#define MS_OFFSCREEN_BANK bank0
#define MS_KERNEL_BANK bank1
#define MS_MAX_NB_SPRITES 16 

MS_KERNEL_BANK const unsigned char spaceship_gfx[20] = { 0, 0, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x18, 0x18, 0x3c, 0xbd, 0xff, 0xdb, 0xdb, 0xdb, 0x66, 0x66, 0, 0};
MS_KERNEL_BANK const unsigned char spaceship_colors[18] = { 0, 0, 0x04, 0x04, 0x84, 0x80, 0x90, 0x06, 0x08, 0x08, 0x0a, 0x0a, 0x0a, 0x0c, 0x0c, 0x0e, 0x0e, 0x44};
MS_KERNEL_BANK const unsigned char meteorite_gfx[16] = { 0, 0, 0x1c, 0x36, 0x7a, 0x7f, 0xfd, 0xfd, 0xfd, 0xff, 0xfe, 0x7e, 0x7c, 0x38, 0, 0};
MS_KERNEL_BANK const unsigned char meteorite_colors[14] = { 0, 0, 0x1e, 0x2a, 0x2a, 0x36, 0x36, 0x42, 0x42, 0x42, 0x40, 0x40, 0x40, 0x40};

#define BLANK 40
#define OVERSCAN 30

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

#define MS_NB_SPRITES_DEF 2
#define MS_KERNEL_DATA \
MS_KERNEL_BANK const char *ms_grptr[MS_NB_SPRITES_DEF] = {spaceship_gfx, meteorite_gfx}; \
MS_KERNEL_BANK const char *ms_coluptr[MS_NB_SPRITES_DEF] = {spaceship_colors, meteorite_colors}; \
MS_KERNEL_BANK const char ms_height[MS_NB_SPRITES_DEF] = {19, 15};

MS_KERNEL_BANK const char playfield[192 + 32] = {
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK,
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK,
    VCS_RED, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_YELLOW, REG_COLUBK, 
    VCS_RED, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_YELLOW, REG_COLUBK
};

#define MS_SELECT_FAST
#include "multisprite.h"

const signed char dx[8] = {-2, -1, 0, 1, 2, 1, 0, -1};
const signed char dy[8] = {0, 2, 3, 2, 0, -2, -3, -2};

void init_sprites()
{
    char i;
    char x = 20, y = 0;
    for (i = 0; i != 8; i++) {
        multisprite_new(1, x, y, 3);
        x += 10;
        y += 20;
        i++;
        multisprite_new(1, x, y, 3 | MS_REFLECTED);
        x += 10;
        y += 20;
    }
}

void move_sprite(char i) {
    char x, y;
    x = ms_sprite_x[X = i] + dx[Y = i & 7];
    if (x < 2) x = 150; else if (x >= 151) x = 2;
    y = ms_sprite_y[X] - MS_OFFSET + dy[Y];
    if (y < 3) y = 179; else if (y >= 180) y = 3;
    multisprite_move(i, x, y);
}

void main()
{
    char i = 1, xpos = 76, ypos = 170, scrolling = 0;
    multisprite_init(playfield);
    multisprite_new(0, xpos, ypos, 0);

    init_sprites();
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
        
        move_sprite(i);
        i++; if (i == 9) i = 1;
        move_sprite(i);
        i++; if (i == 9) i = 1;

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
