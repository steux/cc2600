#include "vcs.h"
#include "vcs_colors.h"

//#define EXTRA_RAM superchip
#define MS_OFFSCREEN_BANK bank0
#define MS_KERNEL_BANK bank1
#define MS_MAX_NB_SPRITES 10 

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

MS_KERNEL_BANK const unsigned char gameover0_gfx[20] = { 0, 0, 0x3c, 0x3e, 0x76, 0x60, 0xe0, 0xc0, 0xc0, 0xcf, 0xc3, 0xc3, 0xc3, 0xc7, 0x66, 0x7e, 0x3c, 0x3c, 0, 0};
MS_KERNEL_BANK const unsigned char gameover1_gfx[20] = { 0, 0, 0x18, 0x18, 0x1c, 0x3e, 0x36, 0x36, 0x36, 0x62, 0x63, 0x63, 0xc3, 0xff, 0xc3, 0xc3, 0xc3, 0xc3, 0, 0};
MS_KERNEL_BANK const unsigned char gameover2_gfx[20] = { 0, 0, 0xfc, 0xde, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0, 0};
MS_KERNEL_BANK const unsigned char gameover3_gfx[20] = { 0, 0, 0x3f, 0x3f, 0x70, 0x60, 0xe0, 0xc0, 0xc0, 0xfc, 0xc0, 0xc0, 0xc0, 0xc0, 0x60, 0x7f, 0x3f, 0x3f, 0, 0};
MS_KERNEL_BANK const unsigned char gameover4_gfx[20] = { 0, 0, 0x3c, 0x3e, 0x76, 0x63, 0xe3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0x66, 0x7e, 0x3c, 0x3c, 0, 0};
MS_KERNEL_BANK const unsigned char gameover5_gfx[20] = { 0, 0, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc7, 0xe6, 0x66, 0x6c, 0x6c, 0x3c, 0x38, 0x18, 0x18, 0, 0};
MS_KERNEL_BANK const unsigned char gameover6_gfx[20] = { 0, 0, 0xfc, 0xc6, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xfe, 0xfc, 0xcc, 0xc6, 0xc6, 0xc3, 0xc3, 0, 0};
#define BLANK 40
#define OVERSCAN 30

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

#define MS_NB_SPRITES_DEF 16 
#define MS_KERNEL_DATA \
MS_KERNEL_BANK const char *ms_grptr[MS_NB_SPRITES_DEF] = {spaceship_gfx, meteorite_gfx, missile_gfx, explosion0_gfx, explosion1_gfx, explosion2_gfx, explosion3_gfx, explosion4_gfx, spaceship_exhaust_gfx, \
    gameover0_gfx, gameover1_gfx, gameover2_gfx, gameover3_gfx, gameover4_gfx, gameover5_gfx, gameover6_gfx }; \
MS_KERNEL_BANK const char *ms_coluptr[MS_NB_SPRITES_DEF] = {spaceship_colors, meteorite_colors, missile_colors, explosion_colors, explosion_colors, explosion_colors, explosion_colors, explosion_colors, spaceship_colors, \
    spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors }; \
MS_KERNEL_BANK const char ms_height[MS_NB_SPRITES_DEF] = {19, 15, 15, 15, 15, 15, 15, 15, 27, 19, 19, 19, 19, 19, 19, 19};

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

#define MS_PLAYFIELD_HEIGHT (192 - 12)
#define MS_SELECT_FAST
#include "multisprite.h"

#define MK_ARMY_FONT
#define MK_BANK bank2
#include "minikernel.h"
MK_BANK const char lives31[7] = {0x02, 0x07, 0x57, 0xf9, 0xf9, 0x20, 0x20};
MK_BANK const char lives32[7] = {0x80, 0xc0, 0xd4, 0x3e, 0x3e, 0x08, 0x08};
MK_BANK const char lives22[7] = {0x80, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00};
MK_BANK const char livesdummy[1] = {0};
MK_BANK const char lives11[7] = {0x00, 0x00, 0x50, 0xf8, 0xf8, 0x20, 0x20};
MK_BANK const char *livesleft[4] = { lives22 + 3, lives11, lives31, lives31 };
MK_BANK const char *livesright[4] = { lives22 + 3, lives22 + 3, lives22, lives32 };

MS_KERNEL_BANK prepare_background(char scrolling)
{
    char j, start = 0;
    scrolling += 12;
    if (scrolling >= 30) start = scrolling - 30;
    *PF2 = 0;
    *COLUBK = 0;
    // Replay background to put the correct colors/regs
    for (Y = start; Y != scrolling;) {
        j = playfield[Y++];
        X = playfield[Y++];
        VSYNC[X] = j;
    }
}

EXTRA_RAM char player_xpos, player_ypos, player_state, player_state2, player_timer, nb_lives;
EXTRA_RAM char button_pressed; 
EXTRA_RAM char missile_sprite;
EXTRA_RAM int score;
EXTRA_RAM char update_score;
    
MK_BANK update_lives_display()
{
    X = nb_lives; 
    mk_s0 = livesleft[X];
    mk_s1 = livesright[X];
}

void game_init()
{
    score = 0;
    update_score = 1;
    player_xpos = 76;
    player_ypos = 170;
    player_state = 0;
    missile_sprite = MS_UNALLOCATED;
    button_pressed = 0;
    player_timer = 1;
    nb_lives = 3;
    update_lives_display();
}

void game_over()
{
    multisprite_new(9, 80 - 19, 50, 0);
    multisprite_new(10, 80 - 9, 50, 0);
    multisprite_new(11, 80 + 1, 50, 0);
    multisprite_new(12, 80 + 11, 50, 0);
    multisprite_new(13, 80 - 19, 100, 0);
    multisprite_new(14, 80 - 9, 100, 0);
    multisprite_new(12, 80 + 1, 100, 0);
    multisprite_new(15, 80 + 11, 100, 0);
}

void lose_one_life()
{
    player_state = 1;
    player_state2 = 0;
    player_timer = 10;
    nb_lives--;
    if (nb_lives == 0) game_over();
    if (nb_lives == -1) nb_lives = 3;
    update_lives_display();
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

    score += 1;
    update_score = 1;
}

void main()
{
    char scrolling = 0;
    multisprite_init(playfield);
    game_init();
    //multisprite_new(0, player_xpos, player_ypos, 0);
    game_over();

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
        //game_logic();

        ms_scenery = playfield - MS_OFFSET + 12;
        ms_scenery += scrolling;

        multisprite_kernel_prep();
        
        while (*INTIM); // Wait for end of blank
 
        multisprite_kernel();
        
        // Overscan
        strobe(WSYNC);
        *COLUBK = VCS_RED;
        *PF0 = 0; *PF1 = 0; *PF2 = 0;
        *COLUP0 = VCS_WHITE; *COLUP1 = VCS_WHITE;
         
        mini_kernel_6_sprites();
        strobe(WSYNC);
        *COLUBK = VCS_RED;
        strobe(WSYNC);
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = ((OVERSCAN) * 76) / 64 + 2;
        // Do some logic here
        multisprite_kernel_post();
        
        prepare_background(scrolling);
        scrolling -= 2;
        if (scrolling < 0) scrolling = 82;

        if (update_score) {
            mini_kernel_update_score_4_digits(score);
            update_score = 0;
        }

        while (*INTIM); // Wait for end of overscan
    } while(1);
}
