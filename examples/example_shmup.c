#include "vcs.h"
#include "vcs_colors.h"

#define EXTRA_RAM superchip
#define MS_OFFSCREEN_BANK bank2
#define MS_OFFSCREEN2_BANK bank3
#define MS_KERNEL_BANK bank1
#define MS_MAX_NB_SPRITES 10 

// Music code
bank3 {
#include "miniblast_tiatracker.h"
#include "miniblast_trackdata.h"
#include "sfx.h"

const char sfx_pewpew[66] = {
	0x10, 0, 0x00, 0x1c, 0x04, 0x0f, 0x1c, 0x04, 0x0f, 0x09, 0x04, 0x0b, 0x03, 0x0c, 0x0a, 0x04,
	0x0c, 0x0e, 0x12, 0x04, 0x0c, 0x19, 0x04, 0x0f, 0x1c, 0x04, 0x0f, 0x07, 0x04, 0x05, 0x09, 0x04,
	0x05, 0x0d, 0x04, 0x06, 0x0c, 0x04, 0x05, 0x18, 0x04, 0x06, 0x1c, 0x04, 0x05, 0x1e, 0x04, 0x03,
	0x07, 0x04, 0x03, 0x09, 0x04, 0x03, 0x0c, 0x04, 0x02, 0x04, 0x0c, 0x02, 0x06, 0x0c, 0x01, 0x00,
	0x00, 0x00
};

const char sfx_bigboom[261] = {
	0x10, 1, 0x00, 0x1d, 0x07, 0x0f, 0x1e, 0x06, 0x0f, 0x00, 0x06, 0x0f, 0x14, 0x07, 0x0f, 0x13,
	0x0f, 0x0f, 0x1b, 0x07, 0x0f, 0x0e, 0x07, 0x0f, 0x1b, 0x07, 0x0f, 0x0f, 0x07, 0x0f, 0x10, 0x07,
	0x0f, 0x10, 0x06, 0x0f, 0x16, 0x07, 0x0f, 0x0d, 0x0f, 0x0f, 0x1e, 0x0c, 0x0f, 0x16, 0x01, 0x0f,
	0x17, 0x01, 0x0f, 0x10, 0x07, 0x0f, 0x10, 0x0f, 0x0f, 0x15, 0x07, 0x0d, 0x1a, 0x07, 0x0f, 0x1a,
	0x01, 0x0f, 0x1a, 0x07, 0x0f, 0x14, 0x0f, 0x0f, 0x16, 0x07, 0x0f, 0x16, 0x07, 0x0f, 0x15, 0x07,
	0x0f, 0x17, 0x07, 0x0f, 0x13, 0x0f, 0x0f, 0x13, 0x0f, 0x0f, 0x19, 0x0f, 0x0f, 0x18, 0x07, 0x0c,
	0x0b, 0x06, 0x0c, 0x1e, 0x01, 0x0d, 0x10, 0x01, 0x0d, 0x14, 0x07, 0x0f, 0x16, 0x06, 0x0c, 0x17,
	0x07, 0x0c, 0x1a, 0x01, 0x0c, 0x12, 0x06, 0x0d, 0x17, 0x07, 0x0c, 0x0b, 0x0f, 0x0c, 0x19, 0x07,
	0x09, 0x19, 0x07, 0x0b, 0x0b, 0x0f, 0x09, 0x0d, 0x0e, 0x0b, 0x0d, 0x0e, 0x0b, 0x19, 0x0f, 0x09,
	0x0e, 0x0f, 0x06, 0x1b, 0x0c, 0x08, 0x18, 0x0f, 0x08, 0x13, 0x07, 0x05, 0x1a, 0x01, 0x05, 0x17,
	0x0f, 0x08, 0x16, 0x06, 0x08, 0x0c, 0x06, 0x05, 0x1c, 0x0f, 0x06, 0x16, 0x06, 0x08, 0x0b, 0x06,
	0x06, 0x12, 0x06, 0x04, 0x0f, 0x0f, 0x05, 0x11, 0x07, 0x06, 0x09, 0x06, 0x05, 0x10, 0x06, 0x05,
	0x10, 0x06, 0x05, 0x10, 0x06, 0x05, 0x11, 0x0f, 0x04, 0x15, 0x0f, 0x04, 0x1e, 0x07, 0x05, 0x16,
	0x01, 0x04, 0x16, 0x01, 0x04, 0x1a, 0x0f, 0x04, 0x19, 0x0f, 0x02, 0x1e, 0x0f, 0x02, 0x1b, 0x0f,
	0x02, 0x1e, 0x0f, 0x02, 0x1c, 0x0f, 0x02, 0x0d, 0x0f, 0x01, 0x0f, 0x06, 0x02, 0x0e, 0x06, 0x01,
	0x18, 0x0f, 0x01, 0x0b, 0x06, 0x02, 0x16, 0x0f, 0x01, 0x17, 0x0f, 0x01, 0x13, 0x06, 0x01, 0x0f,
	0x0e, 0x01, 0x00, 0x00, 0x00
};
}

// Sprites data (generated by sprite2600 shmup.yaml > example_shmup_gfx.c)
#include "example_shmup_gfx.c"

#define BLANK 40
#define OVERSCAN 30

#define BULLET_L    0
#define BULLET_BL   1
#define BULLET_B    2
#define BULLET_BR   3
#define BULLET_R    4
#define BULLET_TR   5
#define BULLET_T    6
#define BULLET_TL   7

MS_OFFSCREEN_BANK {
    const char sprite_width[8] = {8, 24, 40, 40, 72, 16, 72, 32}; 
    const char sprite_is_one_invader[8] = {1, 0, 0, 0, 0, 1, 0, 1}; 
    const char invader_score[2] = {100, 10}; 
    const char sprite_new_nusiz_remove_left[7] = {0, 0, 0, 1, 0, 0, 0}; 
    const char sprite_offset_remove_left[7] = {0, 16, 32, 16, 64, 0, 32}; 
    const char sprite_new_nusiz_remove_right[7] = {0, 0, 0, 1, 0, 0, 2}; 
    const signed char bullet_dx[8] = {-2, -1, 0, 1, 2, 1, 0, -1}; 
    const signed char bullet_dy[8] = {0, 2, 3, 2, 0, -2, -3, -2};
    const char bullet_start_direction[8] = {BULLET_BL, BULLET_B, BULLET_BR, BULLET_B, BULLET_L, BULLET_R, BULLET_TL, BULLET_TR};
}

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

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
#define MK_BANK bank3
#include "minikernel.h"
MK_BANK const char lives31[7] = {0x02, 0x07, 0x57, 0xf9, 0xf9, 0x20, 0x20};
MK_BANK const char lives32[7] = {0x80, 0xc0, 0xd4, 0x3e, 0x3e, 0x08, 0x08};
MK_BANK const char lives22[7] = {0x80, 0xc0, 0xc0, 0x00, 0x00, 0x00, 0x00};
MK_BANK const char livesdummy[1] = {0};
MK_BANK const char lives11[7] = {0x00, 0x00, 0x50, 0xf8, 0xf8, 0x20, 0x20};
MK_BANK const char *livesleft[4] = { lives22 + 3, lives11, lives31, lives31 };
MK_BANK const char *livesright[4] = { lives22 + 3, lives22 + 3, lives22, lives32 };

EXTRA_RAM char player_xpos, player_ypos, player_state, player_state2, player_timer, player_rank, nb_lives;
EXTRA_RAM char button_pressed; 
EXTRA_RAM char missile_sprite, missile_rank;
EXTRA_RAM int score;
EXTRA_RAM char update_score;
EXTRA_RAM char game_counter;
EXTRA_RAM char game_state;
EXTRA_RAM char background_color;
EXTRA_RAM char moving_enemy_counter;
EXTRA_RAM char do_update_live_display;
EXTRA_RAM char do_schedule_sfx;
#define GAME_STARTED    0
#define GAME_OVER       1
#define MAX_NB_ENEMIES  3 
EXTRA_RAM char enemy_sprite[MAX_NB_ENEMIES], enemy_type[MAX_NB_ENEMIES], enemy_state[MAX_NB_ENEMIES], enemy_counter[MAX_NB_ENEMIES], enemy_rank[MAX_NB_ENEMIES], enemy_x[MAX_NB_ENEMIES], enemy_y[MAX_NB_ENEMIES];

#define MAX_NB_BULLETS  3 
EXTRA_RAM char bullet_sprite[MAX_NB_BULLETS], bullet_direction[MAX_NB_BULLETS], bullet_rank[MAX_NB_BULLETS];
  
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
    player_ypos = 192;
    player_state = 0;
    moving_enemy_counter = 0;
    do_update_live_display = 0;
    do_schedule_sfx = 0;
    missile_sprite = MS_UNALLOCATED;
    button_pressed = 1;
    player_timer = 1;
    nb_lives = 3;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        enemy_type[X] = 0;
    }
    for (X = MAX_NB_BULLETS - 1; X >= 0; X--) {
        bullet_sprite[X] = 0;
    }
    update_lives_display();
    game_counter = 0;
    game_state = GAME_STARTED;
    multisprite_new(SPRITE_SPACESHIP, player_xpos, player_ypos, 0);
    player_rank = X;
    background_color = VCS_BLACK;
}

MS_OFFSCREEN_BANK void spawn_new_enemy(char type, char spec)
{
    char i, r, s;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        if (!enemy_type[X]) break;
    }
    if (X == -1) return; // No room for this enemy
    
    enemy_type[X] = type;
    i = X;
    if (type == 1) {
        enemy_state[X] = 0;
        enemy_counter[X] = 0;
        r = multisprite_new(SPRITE_ENEMY1, 60, 22, 3);
        s = X;
        X = i;
        if (r == -1) {
            enemy_type[X] = 0; // No room left for this enemy
        } else {
            enemy_sprite[X] = r;
            enemy_state[X] = spec;
            enemy_rank[X] = s;
            enemy_x[X] = 60;
            enemy_y[X] = 22;
        }
    } else if (type == 128) {
        enemy_counter[X] = 3;
        r = multisprite_new(SPRITE_BIGBOSS, spec, 2, 5);
        Y = X;
        X = i;
        enemy_rank[X] = Y;
        s = multisprite_new(SPRITE_BIGBOSS, spec + 16, 2, 5 | MS_REFLECTED);
        X = i;
        if (r == -1) {
            enemy_type[X] = 0; // No room left for this enemy
        } else {
            enemy_sprite[X] = r;
            enemy_state[X] = s;
            enemy_x[X] = spec;
            enemy_y[X] = 2;
        }
    }
}

MS_OFFSCREEN_BANK void fire_new_bullet(char x, char y, char direction, char nusiz)
{
    char i, r, s;
    for (X = MAX_NB_BULLETS - 1; X >= 0; X--) {
        if (!bullet_sprite[X]) break;
    }
    if (X == -1) return; // No room for this bullet 
    
    i = X;
    r = multisprite_new(SPRITE_BULLET, x, y, nusiz);
    s = X;
    X = i;
    if (r != -1) {
        bullet_sprite[X] = r;
        bullet_direction[X] = direction;
        bullet_rank[X] = s;
    }
}

MS_OFFSCREEN_BANK void check_shot_at_enemy()
{
    char i, j, my = ms_sprite_y[X = missile_sprite];
    char my2 = my - 12;
    char mx = ms_sprite_x[X];
    char mx2 = mx + 7;
    char hit = 0;
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        if (enemy_type[X]) {
            j = enemy_type[X] & 128;
            Y = enemy_sprite[X];
            if (ms_sprite_y[Y] < my && ms_sprite_y[Y] >= my2) {
                // We are at the right height
                i = X;
                char l = sprite_width[X = ms_sprite_nusiz[Y] & 7];
                if (j) l <<= 1;
                if (mx2 >= ms_sprite_x[Y] && mx < ms_sprite_x[Y] + l) {
                    // Let's see if we hit one of these invaders
                    if (sprite_is_one_invader[X]) {
                        hit = 1;
                        X = i;
                        if (j) {
                            enemy_counter[X]--;
                            if (enemy_counter[X] == 0) { 
                                j = Y;
                                multisprite_delete_with_rank(enemy_state[X], enemy_rank[X] + 1);
                                Y = j;
                                j = 0;
                                X = i;
                            }
                        }
                        if (!j) { 
                            X = enemy_type[X] & 0x7f; // Remove the boss byte
                            score += invader_score[X];
                            X = i;
                            enemy_type[X] = 0;
                            multisprite_delete_with_rank(Y, enemy_rank[X]);
                        }
                    } else {
                        // Let's see if we hit the left side
                        if (mx < ms_sprite_x[Y] + 8) {
                            // Yes !
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_sprite_nusiz[Y] = sprite_new_nusiz_remove_left[X];
                            ms_sprite_x[Y] += sprite_offset_remove_left[X];
                        } else if (mx2 >= ms_sprite_x[Y] + l - 8) {
                            // Yes. Wi hit the right side
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_sprite_nusiz[Y] = sprite_new_nusiz_remove_right[X];
                        } else if (X == 3 && mx2 >= ms_sprite_x[Y] + 16 && mx < ms_sprite_x[Y] + 24) {
                            // Yes. Wi hit the right side
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_sprite_nusiz[Y] = 2;
                        } else if (X == 6 && mx2 >= ms_sprite_x[Y] + 32 && mx < ms_sprite_x[Y] + 40) {
                            // Yes. Wi hit the right side
                            hit = 1;
                            // Let's reduce the size of this invader
                            ms_sprite_nusiz[Y] = 4;
                        }
                    }
                    if (hit) {
                        do_schedule_sfx = 2; 
                        multisprite_delete_with_rank(missile_sprite, missile_rank);
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

MS_OFFSCREEN_BANK void game_move_enemies()
{
    char i, j;
    // "cheap" part of movement first
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        if (enemy_type[X] == 1) {
            Y = enemy_sprite[X];
            if (enemy_counter[X] & 1) {
                ms_sprite_nusiz[Y] |= MS_REFLECTED;
            } else {
                ms_sprite_nusiz[Y] &= ~MS_REFLECTED;
            }
            enemy_counter[X]++;
            if (enemy_counter[X] < 100) {
                enemy_y[X]++;
                if (enemy_counter[X] >= 60) {
                    enemy_x[X]++;
                    if (enemy_counter[X] == 60) {
                        j = ms_sprite_nusiz[Y];
                        fire_new_bullet(enemy_x[X], enemy_y[X] + 12, bullet_start_direction[Y = enemy_state[X]], j);
                    }
                }
            } else {
                enemy_y[X]--;
                if (enemy_y[X] == 21) {
                    i = X;
                    multisprite_delete_with_rank(Y, enemy_rank[X]);
                    X = i;
                    enemy_type[X] = 0;
                }    
            }
        } else if (enemy_type[X] == 128) {
            enemy_y[X]++;
            if (enemy_y[X] == 70) {
                fire_new_bullet(enemy_x[X] + 6, enemy_y[X] + 24, BULLET_B, 1);
            } else if (enemy_y[X] == 200) {
                i = X;
                multisprite_delete_with_rank(enemy_sprite[X], enemy_rank[X]);
                X = i;
                multisprite_delete_with_rank(enemy_state[X], enemy_rank[X]);
                X = i;
                enemy_type[X] = 0;
            }
        }
    }
    
    // And actually move the sprites if there is still some resources available
    while (*INTIM >= 23) {
        X = moving_enemy_counter;
        if (X == 0) {
            X = MAX_NB_ENEMIES;
        }
        X--;
        moving_enemy_counter = X;

        if (enemy_type[X] == 1) {
            multisprite_move_with_rank(enemy_sprite[X], enemy_x[X], enemy_y[X], enemy_rank[X]); 
            X = moving_enemy_counter;
            enemy_rank[X] = Y;
        } else if (enemy_type[X] == 128) {
            // Move the right one before to save the order
            multisprite_move_with_rank(enemy_state[X], -1, enemy_y[X], enemy_rank[X] + 1); 
            X = moving_enemy_counter;
            multisprite_move_with_rank(enemy_sprite[X], -1, enemy_y[X], enemy_rank[X]); 
            X = moving_enemy_counter;
            enemy_rank[X] = Y;
        }
    }
}

MS_OFFSCREEN_BANK void game_move_bullets()
{
    char i, nx, ny, destroy;
    for (X = MAX_NB_BULLETS - 1; X >= 0; X--) {
        Y = bullet_sprite[X];
        if (Y) {
            destroy = 0;
            i = X;
            X = bullet_direction[X];
            nx = ms_sprite_x[Y] + bullet_dx[X];
            if (nx < 3) destroy = 1;
            if (nx >= 150) destroy = 1;
            ny = ms_sprite_y[Y] + bullet_dy[X];
            if (ny < MS_OFFSET) destroy = 1;
            if (ny >= MS_PLAYFIELD_HEIGHT + MS_OFFSET - 2) destroy = 1;
            if (destroy) {
                X = i;
                multisprite_delete_with_rank(Y, bullet_rank[X]);
                X = i;
                bullet_sprite[X] = 0;
            } else {
                X = i;
                multisprite_move_with_rank(Y, nx, ny, bullet_rank[X]);
                X = i;
                bullet_rank[X] = Y;
            }    
        }
    }
}

void game_scenario()
{
    if (!(game_counter & 1)) {
        if ((game_counter & 7) == 0) {
            spawn_new_enemy(128, 60);
        } else {
            spawn_new_enemy(1, (game_counter >> 2) & 3);
        }
    }
}

MS_OFFSCREEN_BANK game_over()
{
    char i;
    // Destroy all enemies & bullets
    for (X = MAX_NB_ENEMIES - 1; X >= 0; X--) {
        enemy_type[X] = 0;
    }
    for (X = MAX_NB_BULLETS - 1; X >= 0; X--) {
        bullet_sprite[X] = 0;
    }
    // Destroy missile
    missile_sprite = MS_UNALLOCATED;

    multisprite_clear();
    multisprite_new(SPRITE_LETTER_G, 80 - 19, 80, 0);
    multisprite_new(SPRITE_LETTER_A, 80 - 9, 80, 0);
    multisprite_new(SPRITE_LETTER_M, 80 + 1, 80, 0);
    multisprite_new(SPRITE_LETTER_E, 80 + 11, 80, 0);
    multisprite_new(SPRITE_LETTER_O, 80 - 19, 130, 0);
    multisprite_new(SPRITE_LETTER_V, 80 - 9, 130, 0);
    multisprite_new(SPRITE_LETTER_E, 80 + 1, 130, 0);
    multisprite_new(SPRITE_LETTER_R, 80 + 11, 130, 0);

    game_state = GAME_OVER;
    game_counter = 0;
    background_color = VCS_RED;
}

MS_OFFSCREEN_BANK lose_one_life()
{
    do_schedule_sfx = 2; 
    player_state = 1;
    player_state2 = 0;
    player_timer = 10;
    nb_lives--;
    do_update_live_display = 1; 
}

MS_OFFSCREEN_BANK game_logic()
{
    X = 0; Y = 0;
    if (!(*SWCHA & 0x80) && player_xpos < 153) { player_xpos++; Y = 1; } // Right
    else if (!(*SWCHA & 0x40) && player_xpos >= 1) { player_xpos--; Y = 1; } // Left
    if (!(*SWCHA & 0x20) && player_ypos < 200) { player_ypos++; Y = 1;} // Down
    else if (!(*SWCHA & 0x10) && player_ypos >= 32) { player_ypos--; ms_sprite_model[X] = SPRITE_SPACESHIP_EXHAUST; Y = 1;} // Up
    else { ms_sprite_model[X] = SPRITE_SPACESHIP; } 
    if (Y) player_rank = multisprite_move_with_rank(0, player_xpos, player_ypos, player_rank);

    // Missile management
    if (missile_sprite != MS_UNALLOCATED) {
        X = missile_sprite;
        // Check for collision 
        if (ms_sprite_nusiz[X] & MS_PF_COLLISION) {
            if (ms_sprite_x[X] < 12 || ms_sprite_x[X] >= 153 - 12) {
                multisprite_delete_with_rank(missile_sprite, missile_rank);
                missile_sprite = MS_UNALLOCATED;
            }
        }
        // Check if an enemy was destroyed
        check_shot_at_enemy();
    }
    if (missile_sprite != MS_UNALLOCATED) {
        X = missile_sprite;
        char y = ms_sprite_y[X] - 6;
        if (y < 32) {
            multisprite_delete_with_rank(missile_sprite, missile_rank);
            missile_sprite = MS_UNALLOCATED;
        } else {
            missile_rank = multisprite_move_with_rank(missile_sprite, -1 /* Go straight */, y, missile_rank);
        }
    }

    if (!(*INPT4 & 0x80)) {
        if (!button_pressed) {
            button_pressed = 1;
            if (missile_sprite == MS_UNALLOCATED) {
                do_schedule_sfx = 1; 
                missile_sprite = multisprite_new(SPRITE_FIRE, player_xpos, player_ypos - 8, 0);
                missile_rank = X;
            }
        }
    } else button_pressed = 0;
    
    // Player management
    if (player_state == 0) {
        // Check collision with playfield
        if (ms_sprite_nusiz[X = 0] & MS_PF_COLLISION) {
            if (ms_sprite_x[X] < 12 || ms_sprite_x[X] >= 153 - 12) {
                lose_one_life();
            }
        }
        if (ms_sprite_nusiz[X = 0] & MS_COLLISION) {
            lose_one_life();
        }
    }
    if (player_state == 0) {
        if (player_timer >= 1) {
            player_timer = 0;
            ms_sprite_nusiz[X] = 0;
        } else {
            player_timer = 1;
            ms_sprite_nusiz[X] = MS_REFLECTED;
        }
    } else {
        // Player explosion
        if (player_state == 1) {
            ms_sprite_model[X = 0] = SPRITE_EXPLOSION1 + player_state2;
            player_timer--;
            if (player_timer == 0) {
                player_timer = 10;
                player_state2++;
                if (player_state2 == 5) {
                    if (nb_lives == 0) game_over();
                    else {
                        player_state = 0;
                        ms_sprite_model[X] = SPRITE_SPACESHIP;
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
                game_init();
            }
        } else button_pressed = 0;
    }
}

void main()
{
    tia_tracker_init();

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
        game_move_enemies();

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
        } else {
            // This is executed only if the game scenario is not evaluated
            // So that balances the execution time 
            game_move_bullets();
            if (do_schedule_sfx) {
                if (do_schedule_sfx == 1) {
                    sfx_schedule(sfx_pewpew);
                } else {
                    sfx_schedule(sfx_bigboom);
                }
                do_schedule_sfx = 0;
            }
            if (do_update_live_display) {
                do_update_live_display = 0;
                update_lives_display();
            }
            if (update_score) {
                mini_kernel_update_score_4_digits(score);
                update_score = 0;
            }
        }

        tia_tracker_play();
        sfx_play();
#ifdef DEBUG
        if (*INTIM < min_timer_overscan) min_timer_overscan = *INTIM;
#endif 
        while (*INTIM); // Wait for end of overscan
    } while(1);
}
