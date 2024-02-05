#include "vcs.h"
#include "vcs_colors.h"

#define EXTRA_RAM superchip
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

MS_KERNEL_BANK const unsigned char invader1_gfx[16] = { 0, 0, 0x18, 0x3c, 0x7e, 0x99, 0xff, 0xff, 0xff, 0x7e, 0x66, 0x42, 0x81, 0x42, 0, 0};
MS_KERNEL_BANK const unsigned char invader1_colors[14] = { 0, 0, 0xd2, 0xd2, 0xd2, 0xd2, 0xc4, 0xc4, 0xd6, 0xd6, 0xdc, 0x1c, 0x1c, 0x1c};

#define BLANK 40
#define OVERSCAN 30

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

#define MS_NB_SPRITES_DEF 17 
#define MS_KERNEL_DATA \
MS_KERNEL_BANK const char *ms_grptr[MS_NB_SPRITES_DEF] = {spaceship_gfx, meteorite_gfx, missile_gfx, explosion0_gfx, explosion1_gfx, explosion2_gfx, explosion3_gfx, explosion4_gfx, spaceship_exhaust_gfx, \
    gameover0_gfx, gameover1_gfx, gameover2_gfx, gameover3_gfx, gameover4_gfx, gameover5_gfx, gameover6_gfx, invader1_gfx }; \
MS_KERNEL_BANK const char *ms_coluptr[MS_NB_SPRITES_DEF] = {spaceship_colors, meteorite_colors, missile_colors, explosion_colors, explosion_colors, explosion_colors, explosion_colors, explosion_colors, spaceship_colors, \
    spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, spaceship_colors, invader1_colors }; \
MS_KERNEL_BANK const char ms_height[MS_NB_SPRITES_DEF] = {19, 15, 15, 15, 15, 15, 15, 15, 27, 19, 19, 19, 19, 19, 19, 19, 15};

#define MS_OFFSCREEN_DATA \
const char sprite_width[8] = {8, 24, 40, 40, 72, 16, 72, 32}; \
const char sprite_is_one_invader[8] = {1, 0, 0, 0, 0, 1, 0, 1}; \
const char invader_score[2] = {0, 10}; \
const char sprite_new_nusiz_remove_left[7] = {0, 0, 0, 1, 0, 0, 0}; \
const char sprite_offset_remove_left[7] = {0, 16, 32, 16, 64, 0, 32}; \
const char sprite_new_nusiz_remove_right[7] = {0, 0, 0, 1, 0, 0, 2};

MS_KERNEL_BANK const char playfield[] = {
    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLUE, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2,

    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLUE, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2,

    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLUE, REG_COLUPF,
    0xfc, REG_PF2, 0xff, REG_PF2, 0x03, REG_PF1, 0x0f, REG_PF1, 0x1f, REG_PF1, 0x3f, REG_PF1, 0x7f, REG_PF1, 0xff, REG_PF1,
    0x80, REG_PF0, 0, REG_PF2, 0xc0, REG_PF0, 0xf0, REG_PF1, 0xe0, REG_PF0, 0xe0, REG_PF1, 0xc0, REG_PF1,
    0xf0, REG_PF0, 0x80, REG_PF1, VCS_WHITE, REG_COLUPF, 0x00, REG_PF1, 1, REG_CTRLPF, 0x70, REG_PF0, VCS_LGREY, REG_COLUPF,
    0xf0, REG_PF0, 0xe0, REG_PF0, 0xc0, REG_PF1, VCS_GREY, REG_COLUPF, 0xf0, REG_PF1, 0xc0, REG_PF0, 0xff, REG_PF1, 0x0, REG_PF0, 0xff, REG_PF2,
    0x0f, REG_PF1, 0x04, REG_COLUPF, 0, REG_PF1, 0, REG_PF2,

    5, REG_CTRLPF, 0, REG_PF0, 0, REG_PF1, 0, REG_PF2, VCS_BLUE, REG_COLUPF,
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

EXTRA_RAM char player_xpos, player_ypos, player_state, player_state2, player_timer, nb_lives;
EXTRA_RAM char button_pressed; 
EXTRA_RAM char missile_sprite;
EXTRA_RAM int score;
EXTRA_RAM char update_score;
EXTRA_RAM unsigned int game_counter;
EXTRA_RAM char game_state;
EXTRA_RAM char background_color;
#define GAME_STARTED    0
#define GAME_OVER       1
#define MAX_NB_ENEMIES  3 
EXTRA_RAM char enemy_sprite[MAX_NB_ENEMIES], enemy_type[MAX_NB_ENEMIES], enemy_state[MAX_NB_ENEMIES], enemy_counter[MAX_NB_ENEMIES];
  
#ifdef DEBUG
char min_timer_vblank;
char min_timer_overscan;
#endif

MS_KERNEL_BANK prepare_background(char scrolling)
{
    char j, start = 0;
    scrolling += 12;
    if (scrolling >= 30) start = scrolling - 30;
    *PF2 = 0;
    *COLUBK = background_color;
    // Replay background to put the correct colors/regs
    for (Y = start; Y != scrolling;) {
        j = playfield[Y++];
        X = playfield[Y++];
        VSYNC[X] = j;
    }
}

MK_BANK update_lives_display()
{
    X = nb_lives; 
    mk_s0 = livesleft[X];
    mk_s1 = livesright[X];
}

void game_init()
{
    multisprite_init(playfield);
    score = 0;
    update_score = 1;
    player_xpos = 76;
    player_ypos = 160;
    player_state = 0;
    missile_sprite = MS_UNALLOCATED;
    button_pressed = 0;
    player_timer = 1;
    nb_lives = 3;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        enemy_type[X] = 0;
    }
    update_lives_display();
    game_counter = 0;
    game_state = GAME_STARTED;
    multisprite_new(0, player_xpos, player_ypos, 0);
    background_color = VCS_BLACK;
}

void spawn_new_enemy(char type, char spec)
{
    char i, r;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        if (!enemy_type[X]) break;
    }
    if (X < 0) return; // No room for this enemy
    
    if (type == 1) {
        enemy_type[X] = 1;
        enemy_state[X] = 0;
        enemy_counter[X] = 0;
        i = X;
        r = multisprite_new(16, spec, -12, 3);
        X = i;
        if (r == -1) {
            enemy_type[X] = 0; // No room left for this enemy
        } else {
            enemy_sprite[X] = r;
        }
    }
}

void check_shot_at_enemy()
{
    char i, my = ms_sprite_y[X = missile_sprite];
    char my2 = my - 12;
    char mx = ms_sprite_x[X];
    char mx2 = mx + 7;
    char hit = 0;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        if (enemy_type[X]) {
            Y = enemy_sprite[X];
            if (ms_sprite_y[Y] < my && ms_sprite_y[Y] >= my2) {
                // We are at the right height
                i = X;
                char l = sprite_width[X = ms_nusiz[Y] & 7];
                if (mx2 >= ms_sprite_x[Y] && mx < ms_sprite_x[Y] + l) {
                    // Let's see if we hit one of these invaders
                    if (sprite_is_one_invader[X]) {
                        hit = 1;
                        X = i;
                        X = enemy_type[X];
                        score += invader_score[X];
                        X = i;
                        enemy_type[X] = 0;
                        multisprite_delete(Y);
                    } else {
                        // Let's see if we hit the left side
                        if (mx < ms_sprite_x[Y] + 8) {
                            // Yes !
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_nusiz[Y] = sprite_new_nusiz_remove_left[X];
                            ms_sprite_x[Y] += sprite_offset_remove_left[X];
                        } else if (mx2 >= ms_sprite_x[Y] + l - 8) {
                            // Yes. Wi hit the right side
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_nusiz[Y] = sprite_new_nusiz_remove_right[X];
                        } else if (X == 3 && mx2 >= ms_sprite_x[Y] + 16 && mx < ms_sprite_x[Y] + 24) {
                            // Yes. Wi hit the right side
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_nusiz[Y] = 2;
                        } else if (X == 6 && mx2 >= ms_sprite_x[Y] + 32 && mx < ms_sprite_x[Y] + 40) {
                            // Yes. Wi hit the right side
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_nusiz[Y] = 4;
                        }
                    }
                    if (hit) { 
                        multisprite_delete(missile_sprite);
                        missile_sprite = MS_UNALLOCATED;
                        score += 1;
                        update_score = 1;
                        break;
                    }
                }
                X = i;
            }
        }
    }
}

void game_move_enemies()
{
    char i, ny;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        if (enemy_type[X] == 1) {
            Y = enemy_sprite[X];
            ny = ms_sprite_y[Y] + (1 - MS_OFFSET);
            if (ny == 170) {
                i = X;
                multisprite_delete(Y);
                X = i;
                enemy_type[X] = 0;
            } else {
                i = X;
                multisprite_move(Y, -1, ny); 
                X = i;
            }    
        }
    }
}

void game_scenario()
{
    if (!(game_counter & 1)) {
        spawn_new_enemy(1, 60);
    }
}

void game_over()
{
    char i;
    // Destroy all enemies
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        enemy_type[X] = 0;
    }
    // Destroy missile
    missile_sprite = MS_UNALLOCATED;

    multisprite_clear();
    multisprite_new(9, 80 - 19, 50, 0);
    multisprite_new(10, 80 - 9, 50, 0);
    multisprite_new(11, 80 + 1, 50, 0);
    multisprite_new(12, 80 + 11, 50, 0);
    multisprite_new(13, 80 - 19, 100, 0);
    multisprite_new(14, 80 - 9, 100, 0);
    multisprite_new(12, 80 + 1, 100, 0);
    multisprite_new(15, 80 + 11, 100, 0);

    game_state = GAME_OVER;
    game_counter = 0;
    background_color = VCS_RED;
}

void lose_one_life()
{
    player_state = 1;
    player_state2 = 0;
    player_timer = 10;
    nb_lives--;
    update_lives_display();
}

void game_logic()
{
    X = 0;
    if (!(*SWCHA & 0x80) && player_xpos < 153) { player_xpos++; } // Right
    if (!(*SWCHA & 0x40) && player_xpos > 0) { player_xpos--; } // Left
    if (!(*SWCHA & 0x20) && player_ypos < 170) { player_ypos++; } // Down
    if (!(*SWCHA & 0x10) && player_ypos > 0) { player_ypos--; ms_sprite_model[X] = 8;} // Up
    else { ms_sprite_model[X] = 0; } 
    multisprite_move(0, player_xpos, player_ypos);

    // Missile management
    if (missile_sprite != MS_UNALLOCATED) {
        X = missile_sprite;
        // Check for collision 
        if (ms_nusiz[X] & MS_PF_COLLISION) {
            if (ms_sprite_x[X] < 12 || ms_sprite_x[X] >= 153 - 12) {
                multisprite_delete(missile_sprite);
                missile_sprite = MS_UNALLOCATED;
            }
        }
        // Check if an enemy was destroyed
        check_shot_at_enemy();
    }
    if (missile_sprite != MS_UNALLOCATED) {
        X = missile_sprite;
        char y = ms_sprite_y[X] - (MS_OFFSET + 6);
        if (y < 0) {
            multisprite_delete(missile_sprite);
            missile_sprite = MS_UNALLOCATED;
        } else {
            multisprite_move(missile_sprite, -1 /* Go straight */, y);
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
    
    // Player management
    if (player_state == 0) {
        // Check collision with playfield
        if (ms_nusiz[X = 0] & MS_PF_COLLISION) {
            if (ms_sprite_x[X] < 12 || ms_sprite_x[X] >= 153 - 12) {
                lose_one_life();
            }
        }
        if (ms_nusiz[X = 0] & MS_COLLISION) {
            lose_one_life();
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
        // Player explosion
        if (player_state == 1) {
            ms_sprite_model[X = 0] = 3 + player_state2;
            player_timer--;
            if (player_timer == 0) {
                player_timer = 10;
                player_state2++;
                if (player_state2 == 5) {
                    if (nb_lives == 0) game_over();
                    else {
                        player_state = 0;
                        ms_sprite_model[X] = 0;
                    }
                }
            } 
        }
    }
}

void game_wait_for_restart()
{
    if (game_counter >= 3) {
        game_counter = 3;
        if (!(*INPT4 & 0x80)) {
            if (!button_pressed) {
                button_pressed = 1;
                game_init();
            }
        } else button_pressed = 0;
    }
}

void main()
{
#ifdef DEBUG
    min_timer_overscan = 255;
    min_timer_vblank = 255;
#endif

    char scrolling = 0;
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
        if (game_state == GAME_STARTED) 
            game_logic();
        else game_wait_for_restart();

        ms_scenery = playfield - MS_OFFSET + 12;
        ms_scenery += scrolling;

        multisprite_kernel_prep();
       
#ifdef DEBUG
        if (*INTIM < min_timer_vblank) min_timer_vblank = *INTIM;
#endif 
        while (*INTIM); // Wait for end of blank
 
        multisprite_kernel();
        
        // Overscan
        strobe(WSYNC);
        *COLUBK = VCS_RED;
        *GRP0 = 0; *GRP1 = 0;
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
        if (scrolling < 0) {
            scrolling = 80;
            game_counter++;
            if (game_state == GAME_STARTED) 
                game_scenario();
        }
        game_move_enemies();

        if (update_score) {
            mini_kernel_update_score_4_digits(score);
            update_score = 0;
        }

#ifdef DEBUG
        if (*INTIM < min_timer_overscan) min_timer_overscan = *INTIM;
#endif 
        while (*INTIM); // Wait for end of overscan
    } while(1);
}
