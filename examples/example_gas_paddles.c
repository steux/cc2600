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

MS_KERNEL_BANK const char playfield[192] = {
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK,
    VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK, VCS_BLACK, REG_COLUBK
};

#define MS_ONE_COLOR_SPRITES
#define MS_SELECT_FAST
#define MS_MAX_NB_SPRITES 4
#define MS_PLAYFIELD_HEIGHT (192 - 12)
char paddle[4];
#define kernel_short_macro \
   X = *INTIM; \
   if (!(*INPT0 & 0x80)) paddle[0] = X; \
   if (!(*INPT1 & 0x80)) paddle[1] = X; \
   if (!(*INPT2 & 0x80)) paddle[2] = X; \
   if (!(*INPT3 & 0x80)) paddle[3] = X;
#define kernel_medium_macro kernel_short_macro
#define kernel_long_macro \
   X = *INTIM; \
   if (!(*INPT0 & 0x80)) paddle[0] = X; \
   if (!(*INPT1 & 0x80)) paddle[1] = X; \
   strobe(WSYNC); \
   if (!(*INPT2 & 0x80)) paddle[2] = X; \
   if (!(*INPT3 & 0x80)) paddle[3] = X;

#include "multisprite.h"

#define MK_ARMY_FONT
#include "minikernel.h"

const int dx[24] = {40, 38, 34, 28, 19, 10, 0, -10, -20, -28, -34, -38, -40, -38, -34, -28, -19, -10, 0, 10, 19, 28, 34, 38};
const int dy[24] = {0, 16, 32, 45, 55, 61, 64, 61, 55, 45, 32, 16, 0, -16, -31, -45, -55, -61, -64, -61, -55, -45, -32, -16};

unsigned int xpos[4], ypos[4], direction[4];
char speed[4];
const char car_model[24] = {6, 7, 8, 9, 10, 11, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5}; 
const char car_offset[24] = {2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2}; 
const char car_reflect[24] = {0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0};
const char player_color[4] = {VCS_RED, VCS_YELLOW, VCS_BLUE, VCS_LGREEN};
const char paddle_trigger_flag[4] = {0x80, 0x40, 0x08, 0x04};

#ifdef DEBUG
char min_timer_vblank, min_timer_overscan;
#endif

void game_init() 
{
    char i = 100, j;
    for (X = 0; X < 4; X++) {
        xpos[X] = 80 * 256;
        ypos[X] = i << 8;
        i += 12;
        direction[X] = 256 + 128;
        speed[X] = 0;
        paddle[X] = 100;
        j = X;
        multisprite_new(6, xpos[X] >> 8, ypos[X] >> 8, 0, player_color[X]);
        X = j;
    } 
    mini_kernel_6_sprites_init();
}

inline void car_forward()
{
    xpos[X] += dx[Y];
    ypos[X] += dy[Y];
}

#define DEADZONE 16
void game_logic(char player)
{
    signed char steering;
    X = player;
    Y = direction[X] >> 8;
    Y--;
    steering = paddle[X] - 128;
    if (steering >= 0) {
        if (steering >= DEADZONE) steering -= DEADZONE;
        else steering = 0;
    } else {
        if (steering < -DEADZONE) steering += DEADZONE;
        else steering = 0;
    }

    if ((*SWCHA) & paddle_trigger_flag[X]) {
        if (speed[X] >= 5) speed[X] -= 4;
        else speed[X] = 0;
    } else {
        if (paddle[X] > 240) {
            xpos[X] -= dx[Y];
            ypos[X] -= dy[Y];
            speed[X] = 0;
        } else if (speed[X] != 255) speed[X]++;
    }
    if (speed[X] != 0) {
        car_forward();
        if (speed[X] >= 64) {
            car_forward();
            if (speed[X] >= 128) {
                car_forward();
                if (speed[X] >= 192) {
                    car_forward();
                } else {
                    steering -= (steering >> 2);
                }
            } else {
                steering -= (steering >> 1);
            }
        } else {
            signed char s = steering >> 1;
            steering -= s - (s >> 1);
        }
    } else steering = 0; 
    direction[X] += steering;
    Y = (direction[X] >> 8);
    if (Y == 0) {
        direction[X] += 24 * 256;
        Y = 24;
    } else if (Y == 25) {
        direction[X] -= 24 * 256;
        Y = 1;
    }
    Y--;
    if ((xpos[X] >> 8) < 2) { xpos[X] = 2 * 256; speed[X] = 0; }
    else if ((xpos[X] >> 8) >= 152) { xpos[X] = 151 * 256; speed[X] = 0; }
    if ((ypos[X] >> 8) < 22) { ypos[X] = 22 * 256; speed[X] = 0; }
    else if ((ypos[X] >> 8) >= 200) { ypos[X] = 199 * 256; speed[X] = 0; }

    ms_sprite_model[X] = car_model[Y];
    ms_sprite_nusiz[X] = car_reflect[Y];
    multisprite_move(X, xpos[X] >> 8, (ypos[X] >> 8) + car_offset[Y]);
}

void main()
{
    char i;
#ifdef DEBUG
    min_timer_vblank = 255;
    min_timer_overscan = 255;
#endif
    multisprite_init(playfield);
    game_init();

    do {
        *VBLANK = 0x02; // Turn off video for vblank period
        *VSYNC = 2; // Set VSYNC
        strobe(WSYNC); // Hold it for 3 scanlines
        strobe(WSYNC);
        strobe(WSYNC);
        *VSYNC = 0; // Turn VSYNC Off
        
        // Blank
        *TIM64T = ((BLANK - 3) * 76) / 64 - 3;
        // Do some logic here
        game_logic(0);
        game_logic(1);
#ifdef DEBUG
        if (*INTIM < min_timer_vblank) min_timer_vblank = *INTIM;
#endif
        mini_kernel_update_3_digits(paddle[0]);

        multisprite_kernel_prep();
        while (*INTIM); // Wait for end of blank
        *TIM64T = 255;

        multisprite_kernel();
        
        // Overscan
        strobe(WSYNC);
        *COLUBK = VCS_RED;
        *VBLANK = 0x80; // Keep video on. Dump to ground 
        *GRP0 = 0; *GRP1 = 0;
        *PF0 = 0; *PF1 = 0; *PF2 = 0;
        *COLUP0 = VCS_WHITE; *COLUP1 = VCS_WHITE;
        mini_kernel_6_sprites();
        strobe(WSYNC);
        *COLUBK = VCS_RED;
        strobe(WSYNC);
        *VBLANK = 0x02; // Turn off video. Don't dump to ground 
        *TIM64T = ((OVERSCAN) * 76) / 64 + 2;
        // Do some logic here
        multisprite_kernel_post();
#ifdef DEBUG
        if (*INTIM < min_timer_overscan) min_timer_overscan = *INTIM;
#endif
        game_logic(2);
        game_logic(3);
    
        while (*INTIM); // Wait for end of overscan
    } while(1);
}
