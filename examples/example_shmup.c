#include "vcs_colors.h"

#define MS_OFFSCREEN_BANK bank0
#define MS_KERNEL_BANK bank1
#define MS_EXTRA_RAM superchip
#define MS_MAX_NB_SPRITES 16 

MS_KERNEL_BANK const unsigned char spaceship_gfx[20] = { 0, 0, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x18, 0x18, 0x3c, 0xbd, 0xff, 0xdb, 0xdb, 0xdb, 0x66, 0x66, 0, 0};
MS_KERNEL_BANK const unsigned char meteorite_gfx[16] = { 0, 0, 0x1c, 0x36, 0x7a, 0x7f, 0xfd, 0xfd, 0xfd, 0xff, 0xfe, 0x7e, 0x7c, 0x38, 0, 0};
MS_KERNEL_BANK const unsigned char meteorite_colors[14] = { 0, 0, 0x1e, 0x2a, 0x2a, 0x36, 0x36, 0x42, 0x42, 0x42, 0x40, 0x40, 0x40, 0x40};
MS_KERNEL_BANK const unsigned char missile_gfx[16] = { 0, 0, 0x42, 0xe7, 0xe7, 0xc3, 0x42, 0x42, 0x42, 0x00, 0x42, 0x42, 0x00, 0x42, 0, 0};
MS_KERNEL_BANK const unsigned char missile_colors[14] = { 0, 0, 0x42, 0x44, 0x36, 0x36, 0x18, 0x18, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c};
MS_KERNEL_BANK const unsigned char explosion_colors[14] = { 0, 0, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c};
MS_KERNEL_BANK const unsigned char explosion0_gfx[16] = { 0, 0, 0x00, 0x00, 0x00, 0x08, 0x34, 0x24, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0, 0};
MS_KERNEL_BANK const unsigned char explosion1_gfx[16] = { 0, 0, 0x00, 0x00, 0x08, 0x30, 0x2e, 0x6c, 0x26, 0x12, 0x3c, 0x08, 0x00, 0x00, 0, 0};
MS_KERNEL_BANK const unsigned char explosion2_gfx[16] = { 0, 0, 0x1e, 0x6a, 0x48, 0xf5, 0x41, 0xd3, 0x93, 0xa3, 0xcf, 0x77, 0x68, 0x06, 0, 0};
MS_KERNEL_BANK const unsigned char explosion3_gfx[16] = { 0, 0, 0x52, 0x76, 0xf7, 0xa3, 0x86, 0x00, 0x40, 0x47, 0x84, 0xaa, 0xe5, 0x23, 0, 0};
MS_KERNEL_BANK const unsigned char explosion4_gfx[16] = { 0, 0, 0x43, 0x64, 0x82, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x81, 0x41, 0xc2, 0x25, 0, 0};
MS_KERNEL_BANK const unsigned char spaceship_exhaust_gfx[28] = { 0, 0, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x18, 0x18, 0x3c, 0xbd, 0xff, 0xdb, 0xdb, 0xdb, 0x66, 0x7e, 0xfe, 0x7f, 0xff, 0x76, 0x6e, 0x76, 0x66, 0x64, 0, 0};
MS_KERNEL_BANK const unsigned char spaceship_colors[26] = { 0, 0, 0x04, 0x04, 0x84, 0x80, 0x90, 0x06, 0x08, 0x08, 0x0a, 0x0a, 0x0a, 0x0c, 0x0c, 0x0e, 0x0e, 0x30, 0x32, 0x34, 0x36, 0x28, 0x1a, 0x1c, 0x1e, 0x1e};
#define BLANK 40
#define OVERSCAN 30

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

#define MS_NB_SPRITES_DEF 9
#define MS_KERNEL_DATA \
MS_KERNEL_BANK const char *ms_grptr[MS_NB_SPRITES_DEF] = {spaceship_gfx, meteorite_gfx, missile_gfx, explosion0_gfx, explosion1_gfx, explosion2_gfx, explosion3_gfx, explosion4_gfx, spaceship_exhaust_gfx}; \
MS_KERNEL_BANK const char *ms_coluptr[MS_NB_SPRITES_DEF] = {spaceship_colors, meteorite_colors, missile_colors, explosion_colors, explosion_colors, explosion_colors, explosion_colors, explosion_colors, spaceship_colors}; \
MS_KERNEL_BANK const char ms_height[MS_NB_SPRITES_DEF] = {19, 15, 15, 15, 15, 15, 15, 15, 27};

MS_KERNEL_BANK const char playfield[] = {
    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLACK, REG_COLUBK, VCS_GREEN, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2,

    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLACK, REG_COLUBK, VCS_GREEN, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2,

    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLACK, REG_COLUBK, VCS_GREEN, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2,

    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLACK, REG_COLUBK, VCS_GREEN, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2
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

MS_KERNEL_BANK prepare_background(char scrolling)
{
    char j, start = 0;
    scrolling += 12;
    if (scrolling >= 30) start = scrolling - 30;
    *PF2 = 0;
    // Replay background to put the correct colors/regs
    for (Y = start; Y != scrolling;) {
        j = playfield[Y++];
        X = playfield[Y++];
        VSYNC[X] = j;
    }
}

char player_xpos, player_ypos, player_state, player_state2, player_timer;
char button_pressed; 
char missile_sprite;

void game_init()
{
    player_xpos = 76;
    player_ypos = 170;
    player_state = 0;
    missile_sprite = MS_UNALLOCATED;
    button_pressed = 0;
    player_timer = 1;
}

void lose_one_life()
{
    player_state = 1;
    player_state2 = 0;
    player_timer = 10;
}

void game_logic()
{
    X = 0;
    if (!(*SWCHA & 0x80) && player_xpos < 153) { player_xpos++; } // Right
    if (!(*SWCHA & 0x40) && player_xpos > 0) { player_xpos--; } // Left
    if (!(*SWCHA & 0x20) && player_ypos < 180) { player_ypos++; } // Down
    if (!(*SWCHA & 0x10) && player_ypos > 0) { player_ypos--; ms_sprite_model[X] = 8;} // Up
    else { ms_sprite_model[X] = 0; } 
    multisprite_move(0, player_xpos, player_ypos);

    if (player_state == 0) {
        // Check collision with playfield
        if (ms_nusiz[X = 0] & MS_PF_COLLISION) {
            if (ms_sprite_x[X] < 12 || ms_sprite_x[X] >= 153 - 12) {
                lose_one_life();
            }
        }
    }
    if (player_state == 0) {
        if (player_timer >= 1) {
            player_timer = 0;
            ms_nusiz[X] = 0;
        } else {
            player_timer = 1;
            ms_nusiz[X] = MS_REFLECTED;
        }
    } else {
        if (player_state == 1) {
            ms_sprite_model[X = 0] = 3 + player_state2;
            player_timer--;
            if (player_timer == 0) {
                player_timer = 10;
                player_state2++;
                if (player_state2 == 5) {
                    player_state = 0;
                    ms_sprite_model[X] = 0;
                }
            } 
        }
    }

    if (missile_sprite != MS_UNALLOCATED) {
        X = missile_sprite;
        // Check for collision 
        if (ms_nusiz[X] & MS_PF_COLLISION) {
            if (ms_sprite_x[X] < 12 || ms_sprite_x[X] >= 153 - 12) {
                multisprite_delete(missile_sprite);
                missile_sprite = MS_UNALLOCATED;
            }
        }
    }
    if (missile_sprite != MS_UNALLOCATED) {
        X = missile_sprite;
        char y = ms_sprite_y[X] - (MS_OFFSET + 6);
        if (y < 0) {
            multisprite_delete(missile_sprite);
            missile_sprite = MS_UNALLOCATED;
        } else {
            multisprite_move(missile_sprite, -1, y);
        }
    }

    if (!(*INPT4 & 0x80)) {
        if (!button_pressed) {
            button_pressed = 1;
            if (missile_sprite == MS_UNALLOCATED) {
                missile_sprite = multisprite_new(2, player_xpos, player_ypos - 8, 0);
            }
        }
    } else button_pressed = 0;
}

void main()
{
    char scrolling = 0;
    multisprite_init(playfield);
    game_init();
    multisprite_new(0, player_xpos, player_ypos, 0);

    //init_sprites();
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
        game_logic();

        ms_scenery = playfield - MS_OFFSET + 12;
        ms_scenery += scrolling;

        multisprite_kernel_prep();
        
        while (*INTIM); // Wait for end of blank
 
        multisprite_kernel();
        
        // Overscan
        strobe(WSYNC);
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = ((OVERSCAN) * 76) / 64 + 2;
        // Do some logic here
        multisprite_kernel_post();
        prepare_background(scrolling);
        scrolling -= 2;
        if (scrolling < 0) scrolling = 82;

        while (*INTIM); // Wait for end of overscan
    } while(1);
}
