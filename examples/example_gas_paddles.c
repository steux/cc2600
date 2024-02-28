#include "vcs_colors.h"

#define MS_OFFSCREEN_BANK bank0
#define MS_KERNEL_BANK bank1
// Generated with sprites2600 cars.yaml
#include "example_gas_paddles_gfx.c"
// Generated with sprites2600 --raw car2.yaml
#include "example_gas_paddles_gfx2.c"

#define BLANK 40
#define OVERSCAN 30 

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a // LSB: Playfield priority / Score mode / Reflective playfield
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

#define MS_PLAYFIELD_HEIGHT (192 - 12)
MS_KERNEL_BANK const char playfield[MS_PLAYFIELD_HEIGHT] = {
    VCS_BLACK, REG_COLUBK, 0xc0, REG_PF1, 0xfc, REG_PF2, 0x80, REG_PF1, 0xf8, REG_PF2, 0x00, REG_PF1, VCS_DGREEN, REG_COLUPF, 0xf0, REG_PF2, 
    VCS_GREEN, REG_COLUPF, 0x70, REG_PF0, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0xe0, REG_PF2, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x30, REG_PF0, 
    VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x04, REG_PF1, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0xc0, REG_PF2,
    VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x06, REG_PF1, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, 
    VCS_GREEN, REG_COLUPF, 0x80, REG_PF2, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x07, REG_PF1,
    0x10, REG_PF0, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, 
    VCS_GREEN, REG_COLUPF, 0x00, REG_PF2,  VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x0f, REG_PF1,
    VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x01, REG_PF2,VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF,
    VCS_GREEN, REG_COLUPF, 0x03, REG_PF2, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x07, REG_PF2, VCS_DGREEN, REG_COLUPF, 0x0f, REG_PF2, 0xff, REG_PF2,
    VCS_GREEN, REG_COLUPF, 0x07, REG_PF1, VCS_DGREEN, REG_COLUPF, 0x03, REG_PF1, 0x01, REG_PF1, 0x00, REG_PF1, 0x00, REG_PF2, 
    VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, VCS_GREEN, REG_COLUPF, 0x30, REG_PF0,
    VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, 0x70, REG_PF0, VCS_GREEN, REG_COLUPF, VCS_DGREEN, REG_COLUPF, 0xf0, REG_PF0, VCS_GREEN, REG_COLUPF, 0x80, REG_PF1, 
    VCS_DGREEN, REG_COLUPF, 0xC0, REG_PF1, 0xf0, REG_PF1, 0xff, REG_PF1, 0xff, REG_PF2   
};

#define MS_ONE_COLOR_SPRITES
#define MS_SELECT_FAST
#define MS_MAX_NB_SPRITES 4
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
   if (!(*INPT3 & 0x80)) paddle[3] = X; \
   if (Y >= 163) *ENABL = 2;

#include "multisprite.h"

#define MK_ARMY_FONT
#include "minikernel.h"

const int dx[24] = {40, 38, 34, 28, 19, 10, 0, -10, -20, -28, -34, -38, -40, -38, -34, -28, -19, -10, 0, 10, 19, 28, 34, 38};
const int dy[24] = {0, 16, 32, 45, 55, 61, 64, 61, 55, 45, 32, 16, 0, -16, -31, -45, -55, -61, -64, -61, -55, -45, -32, -16};

unsigned int xpos[4], ypos[4], direction[4];
char speed[4], race_laps[4], race_step[4];
char steering[4], pstate[4], pstate_counter[4];
char counter;
#define STATE_READY_SET_GO  0
#define STATE_FIRST         1
#define STATE_SECOND        2
#define STATE_THIRD         3
#define STATE_FOURTH        4
#define STATE_OUT_OF_GAME   5  
#define STATE_OK            6  
#define STATE_LAST_LAP      7  
char ranked[4];
char game_state;
#define GAME_STATE_STARTING 0
#define GAME_STATE_RUNNING  1

const char car_model[24] = {6, 7, 8, 9, 10, 11, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5}; 
const char car_offset[24] = {2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2}; 
const char car_reflect[24] = {0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0, 0, 0, 0, 0, 0};
const char player_color[4] = {VCS_RED, VCS_BLUE, VCS_LGREEN, VCS_YELLOW };
const char paddle_trigger_flag[4] = {0x80, 0x40, 0x08, 0x04};
const char steering_offset[4] = {6, 1, 2, 0};

const char *state_sprite[8] = {ready_set_go_gfx, first_gfx, second_gfx, third_gfx, fourth_gfx, nok_gfx, ok_gfx, last_lap_gfx};
#define NB_WAYPOINTS 9
const char waypoint_xy[NB_WAYPOINTS] = {90, 128, 40 + MS_OFFSET, 108, 90 + MS_OFFSET, 60, 40 + MS_OFFSET, 32, 160 + MS_OFFSET};
#define WPT_UP      0
#define WPT_LEFT    1
#define WPT_DOWN    2
#define WPT_RIGHT   3
const char waypoint_dir[NB_WAYPOINTS] = {WPT_RIGHT, WPT_RIGHT, WPT_UP, WPT_LEFT, WPT_DOWN, WPT_LEFT, WPT_UP, WPT_LEFT, WPT_DOWN};

#ifdef DEBUG
char min_timer_vblank, min_timer_overscan;
#endif

void game_init() 
{
#define YSTART 32 //180
    const char xinit[4] = {80, 80, 68, 68};
    const char yinit[4] = {YSTART, YSTART + 12, YSTART, YSTART + 12};
    
    // Initialize ball (start line)
    X = 90;
    strobe(WSYNC);                  

    *HMBL = ms_sprite_hm_offscreen[X];
    X = ms_sprite_wait_offscreen[X];
    csleep(4);                      
    // Critical loop. Must not cross page boundary. 
    if (X) do { X--; } while (X);   // 3 for X = 0. 1 + X * 5 cycles. 
    strobe(RESBL);                  // 3. Minimum = 23 cycles
    strobe(WSYNC);                  // 3
    strobe(HMOVE);

    char i;
    for (X = 0; X < 4; X++) {
        xpos[X] = xinit[X] << 8;
        ypos[X] = yinit[X] << 8;
        direction[X] = 256 + 128;
        speed[X] = 0;
        steering[X] = 0;
        race_laps[X] = 0xf0;
        race_step[X] = 0x00;
        ranked[X] = -1;
        pstate[X] = STATE_OUT_OF_GAME; 
        pstate_counter[X] = 0;
        paddle[X] = 100;
        i = X;
        multisprite_new(6, xpos[X] >> 8, ypos[X] >> 8, 0, player_color[X]);
        X = i;
    } 
    counter = 0;
    game_state = GAME_STATE_STARTING;

    strobe(HMCLR);
}

inline void car_forward()
{
    xpos[X] += dx[Y];
    ypos[X] += dy[Y];
}

#define DEADZONE 32 
void game_logic(char player)
{
    signed char psteering;
    X = player;
    Y = direction[X] >> 8;
    Y--;
    psteering = paddle[X] - 128;
    if (psteering >= 0) {
        if (psteering >= DEADZONE) psteering -= DEADZONE;
        else psteering = 0;
    } else {
        if (psteering < -DEADZONE) psteering += DEADZONE;
        else psteering = 0;
    }
    psteering >>= 1;
    steering[X] = (psteering >> 2) + steering_offset[X] + 9;

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
    if (ms_sprite_nusiz[X] & MS_COLLISION) {
        speed[X] = 0; // Collision with playfield
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
                    psteering -= (psteering >> 2);
                }
            } else {
                psteering -= (psteering >> 1);
            }
        } else {
            signed char s = psteering >> 1;
            psteering -= s - (s >> 1);
        }
    } else psteering = 0; 
    direction[X] += psteering;
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
    if (ms_sprite_nusiz[X] & MS_PF_COLLISION) {
        speed[X] = 0; // Collision with playfield
    }
    ms_sprite_nusiz[X] = car_reflect[Y];
    multisprite_move(X, xpos[X] >> 8, (ypos[X] >> 8) + car_offset[Y]);

    if (pstate[X] == STATE_READY_SET_GO) {
        if ((counter & 1) == 0 && pstate_counter[X] < 126 - 9) pstate_counter[X]++;
    }

#define tmp psteering
    // Compute progress on the track
    tmp = waypoint_dir[Y = race_laps[X] & 0x0f];
    if (tmp == WPT_RIGHT) {
        race_step[X] = (xpos[X] >> 8) - waypoint_xy[Y];
    } else if (tmp == WPT_UP) {
        race_step[X] = (ypos[X] >> 8) - waypoint_xy[Y];
    } else if (tmp == WPT_DOWN) {
        race_step[X] = waypoint_xy[Y] - (ypos[X] >> 8);
    } else {
        race_step[X] = waypoint_xy[Y] - (ypos[X] >> 8);
    }
    if (race_step[X] < 0) {
        race_laps[X]++;
        if ((race_laps[X] & 0x0f) == NB_WAYPOINTS) {
            race_laps[X] = race_laps[X] & 0x0f;
        } else if ((race_laps[X] & 0x0f) == 1) {
            race_laps[X] = (race_laps[X] & 0xf0) + 0x11;
        }
        tmp = waypoint_dir[Y = race_laps[X] & 0x0f];
        if (tmp == WPT_RIGHT) {
            race_step[X] = (xpos[X] >> 8) - waypoint_xy[Y];
        } else if (tmp == WPT_UP) {
            race_step[X] = (ypos[X] >> 8) - waypoint_xy[Y];
        } else if (tmp == WPT_DOWN) {
            race_step[X] = waypoint_xy[Y] - (ypos[X] >> 8);
        } else {
            race_step[X] = waypoint_xy[Y] - (ypos[X] >> 8);
        }
    }
    // Update race ranking
    Y = pstate[X];
    if (Y == STATE_READY_SET_GO) {
        for (Y = 0; Y != 4; Y++) {
            if (ranked[Y] == -1) {
                ranked[Y] = X; 
                pstate[X] = Y;
                break;
            }
        }
    } else if (Y >= STATE_SECOND) {
        Y = ranked[--Y]; // Y is the car that is potentially overtaken by car X
        if (race_laps[Y] < race_laps[X] || (race_laps[Y] == race_laps[X] && race_step[Y] < race_step[X])) {
            tmp = Y;
            Y = pstate[X]; // pstate[X] is the former position of X
            ranked[Y] = tmp; // The overtaken car is ranked there
            ranked[--Y] = X; // We put car X at the previous position        
            pstate[X]--; // And we update the position of each X and Y cars
            pstate[Y = tmp]++;
        }
    }
}

void main()
{
    char o0, o1;

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
        if (pstate[0] != STATE_OUT_OF_GAME) game_logic(0);
        if (pstate[1] != STATE_OUT_OF_GAME) game_logic(1);
#ifdef DEBUG
        if (*INTIM < min_timer_vblank) min_timer_vblank = *INTIM;
#endif
        *PF0 = 255; *PF1 = 255; *PF2 = 255;
        *CTRLPF = 1; // Reflective playfield. Ball size is 1
        *COLUPF = VCS_GREEN; // Grass
        if (counter & 1) {
            o0 = pstate_counter[2];
            o1 = pstate_counter[1];
        } else {
            o0 = pstate_counter[3];
            o1 = pstate_counter[0];
        }
        multisprite_kernel_prep();
        *HMBL = 0x80; // Set due to early HMOVE
        
        while (*INTIM); // Wait for end of blank
        *TIM64T = 255;

        multisprite_kernel();
        
        // Overscan
        strobe(WSYNC);
        *COLUPF = VCS_BROWN;
        *CTRLPF = 4; // Playfield priority over players
        *ENABL = 0;
        *COLUBK = VCS_BLACK;
        *VBLANK = 0x80; // Keep video on. Dump to ground 
        if (counter & 1) {
            // Player 2 & 1
            X = steering[2];
            *HMM0 = ms_sprite_hm_offscreen[X];            // 7
            Y = ms_sprite_wait_offscreen[X];              // 6 [19/76]
            strobe(RESP0);
            if (Y) do { Y--; } while (Y);       // 3 for X = 0. 1 + X * 5 cycles. 
            strobe(RESM0);                      // 3. Minimum = 26 cycles
            *HMP0 = 0xe0;
            strobe(WSYNC);
            *HMP1 = 0xf0;
            *COLUP0 = VCS_LGREEN;
            *COLUP1 = VCS_BLUE;
            X = steering[1];
            *HMM1 = ms_sprite_hm_offscreen[X];            // 7
            X = ms_sprite_wait_offscreen[X];              // 6 [19/76]
            strobe(RESP1);
            if (X) do { X--; } while (X);       // 3 for X = 0. 1 + X * 5 cycles. 
            strobe(RESM1);                      // 3. Minimum = 26 cycles
            X = pstate[2]; Y = pstate[1];
        } else {
            // Player 4 & 0
            *COLUP0 = VCS_YELLOW;
            *COLUP1 = VCS_RED;
            X = steering[3];
            *HMM0 = ms_sprite_hm_offscreen[X];            // 7
            Y = ms_sprite_wait_offscreen[X];              // 6 [19/76]
            csleep(3);
            strobe(RESP0);
            if (Y) do { Y--; } while (Y);       // 3 for X = 0. 1 + X * 5 cycles. 
            strobe(RESM0);                      // 3. Minimum = 26 cycles
            strobe(WSYNC);
            X = steering[0];
            *HMM1 = ms_sprite_hm_offscreen[X];            // 7
            X = ms_sprite_wait_offscreen[X];              // 6 [19/76]
            strobe(RESP1);
            if (X) do { X--; } while (X);       // 3 for X = 0. 1 + X * 5 cycles. 
            strobe(RESM1);                      // 3. Minimum = 26 cycles
            *HMP1 = 0x30;
            *HMP0 = 0x00;
            X = pstate[3]; Y = pstate[0];
        }
        *GRP0 = 0x00; 
        *GRP1 = 0x00;
        *HMBL = 0x00;
        *ENAM1 = 0x02;
        *ENAM0 = 0x02;
        strobe(WSYNC); // Line 1
        strobe(HMOVE);
        *PF0 = 0xc0; *PF1 = 0x1c; *PF2 = 0xe3; *NUSIZ0 = 0x20; *NUSIZ1 = 0x20;
        ms_grp0ptr = state_sprite[X] | o0; ms_grp1ptr = state_sprite[Y] | o1; Y = 0; 
        strobe(WSYNC); // Line 2
        *PF0 = 0x40; *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y]; *REFP0 = 0; *REFP1 = 0; *PF1 = 0x0c; *PF2 = 0xc1; Y++;
        strobe(WSYNC); // Line 3
        *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++]; multisprite_kernel_post();
        strobe(WSYNC); // Line 4
        *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++]; strobe(HMCLR);
        strobe(WSYNC); // Line 5
        *PF0 = 0x00; *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++]; *PF1 = 0x44; *PF2 = 0x88;
        strobe(WSYNC); // Line 6
        *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++]; *PF1 = 0xe4, *PF2 = 0x9c;
        strobe(WSYNC); // Line 7
        *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++];
        strobe(WSYNC); // Line 8
        *PF0 = 0x80; *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++]; *PF1 = 0xf4; *PF2 = 0xbe;
        strobe(WSYNC); // Line 10 
        *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++];
        strobe(WSYNC); // Line 9
        *GRP1 = ms_grp1ptr[Y]; *GRP0 = ms_grp0ptr[Y++];
        strobe(WSYNC); // Line 11
        *ENAM0 = 0; *ENAM1 = 0;
        *VBLANK = 0x02; // Turn off video. Don't dump to ground 
        *TIM64T = ((OVERSCAN) * 76) / 64 + 2;
        // Do some logic here
#ifdef DEBUG
        if (*INTIM < min_timer_overscan) min_timer_overscan = *INTIM;
#endif
        if (pstate[2] != STATE_OUT_OF_GAME) game_logic(2);
        if (pstate[3] != STATE_OUT_OF_GAME) game_logic(3);
        counter++;
    
        while (*INTIM); // Wait for end of overscan
    } while(1);
}
