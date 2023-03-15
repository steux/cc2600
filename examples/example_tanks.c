#include "vcs.h"
#include "vcs_colors.h"

unsigned char X, Y;

#define BLANK 40
#define KERNAL 192 
#define OVERSCAN 30

#define ROTATION_DELAY 5
#define MAX_DISTANCE_MISSILE 100 

#define OUT_OF_SCREEN KERNAL 

char i, j;
char *sprite_ptr0, *sprite_ptr1;
char *mask_ptr0, *mask_ptr1;
char *missile_mask_ptr;
char *second_tank_mask0, *second_tank_mask1;
char *second_tank_nusiz0, *second_tank_nusiz1;
char joystick[2]; // Joystick inputs (bit7 is button)
char odd; // Odd or even frame ?

// Game state
unsigned short xpos[2]; // Position of sprites. 16-bits to account for diagonal movements
unsigned short ypos[2];
signed char direction[2]; // Between 0 and 23
char counter[2]; // Rotation counter for both players
unsigned short xmissile[2]; // Position of missiles. 16-bits to account for diagonal movements
unsigned short ymissile[2];
signed char direction_missile[2]; // Between 0 and 23. -1 is missile not fired
char distance_missile[2]; // Distance counter for missiles.
char button_pressed[2]; // Debouncing buttons
char tank_switch_counter[2];
char lives[2]; // Remaining lives for all tanks / all players
char xpos_second_tank[2]; // xpos for second tank
char ypos_second_tank[2]; // ypos for second tank
char direction_second_tank[2]; // direction for second tank, between 0 and 23

const char tank_mask[KERNAL + 12 + KERNAL] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
const unsigned char second_tank_nusiz[12 + KERNAL] = { 
    0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x30, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
}; 
const char missile_mask[4 + KERNAL] = {
    0xff, 0xff, 0xff, 0xff, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Generated from build_sprites.c & PlayerPal & back to C with spritegen
const unsigned char tank0[12] = { 0x00, 0xfe, 0xfe, 0x64, 0x58, 0x5f, 0x5f, 0x58, 0x64, 0xfe, 0xfe, 0x00 };
const unsigned char tank1[12] = { 0x0c, 0x3c, 0xfc, 0xe3, 0xdf, 0x5e, 0x5c, 0x5b, 0x27, 0x7e, 0x78, 0x20 };
const unsigned char tank2[12] = { 0x18, 0x38, 0x7b, 0xe7, 0xde, 0xdc, 0x5b, 0x53, 0x26, 0x3c, 0x38, 0x10 };
const unsigned char tank3[12] = { 0x18, 0x32, 0x76, 0x4c, 0xd9, 0xdb, 0xdb, 0x46, 0x24, 0x3c, 0x18, 0x10 };
const unsigned char tank4[12] = { 0x34, 0x2c, 0x6c, 0x5b, 0xdb, 0xda, 0xda, 0xc6, 0x6c, 0x3c, 0x18, 0x08 };
const unsigned char tank5[12] = { 0x08, 0x6b, 0x6b, 0x5b, 0x5b, 0xdb, 0xda, 0xd6, 0xe6, 0xfe, 0xc6, 0x00 };
const unsigned char tank6[12] = { 0x10, 0xd6, 0xd6, 0xd6, 0xba, 0xba, 0xba, 0xba, 0xc6, 0xfe, 0xc6, 0x00 };
const unsigned char tank7[12] = { 0x20, 0x78, 0x7e, 0x27, 0x5b, 0x5c, 0x5e, 0xdf, 0xe3, 0xfc, 0x3c, 0x0c };
const unsigned char tank8[12] = { 0x10, 0x38, 0x3c, 0x26, 0x53, 0x5b, 0xdc, 0xde, 0xe7, 0x7b, 0x38, 0x18 };
const unsigned char tank9[12] = { 0x10, 0x18, 0x3c, 0x24, 0x46, 0xdb, 0xdb, 0xd9, 0x4c, 0x76, 0x32, 0x18 };
const unsigned char tank10[12] ={ 0x08, 0x18, 0x3c, 0x6c, 0xc6, 0xda, 0xda, 0xdb, 0x5b, 0x6c, 0x2c, 0x34 };
const unsigned char tank11[12] ={ 0x00, 0xc6, 0xfe, 0xe6, 0xd6, 0xda, 0xdb, 0x5b, 0x5b, 0x6b, 0x6b, 0x08 };
const unsigned char tank12[12] ={ 0x00, 0xc6, 0xfe, 0xc6, 0xba, 0xba, 0xba, 0xba, 0xd6, 0xd6, 0xd6, 0x10 };

const char *tank_models[24] = {tank0, tank1, tank2, tank3, tank4, tank5, 
                               tank6, tank5, tank4, tank3, tank2, tank1, 
                               tank0, tank7, tank8, tank9, tank10, tank11,
                               tank12, tank11, tank10, tank9, tank8, tank7 };
const char sprite_reflect[24] = {0, 0, 0, 0, 0, 0, 
                                 0, 8, 8, 8, 8, 8, 
                                 8, 8, 8, 8, 8, 8,
                                 0, 0, 0, 0, 0, 0};
// Generated by build_directions.c. Takes into account the 1.6 pixel ratio between vertical and horizontal
const signed short dx[24] = {100, 96, 86, 70, 49, 25, 0, -25, -50, -70, -86, -96, -100, -96, -86, -70, -49, -25, 0, 25, 49, 70, 86, 96};
const signed short dy[24] = {0, 41, 80, 113, 138, 154, 160, 154, 138, 113, 80, 41, 0, -41, -79, -113, -138, -154, -160, -154, -138, -113, -80, -41};

// Generated by build_missile_directions.c. Takes into account a 1.6 pixel ratio between vertical and horizontal
const signed short dx_missile[24] = {300, 289, 259, 212, 149, 77, 0, -77, -150, -212, -259, -289, -300, -289, -259, -212, -149, -77, 0, 77, 149, 212, 259, 289};
const signed short dy_missile[24] = {0, 124, 240, 339, 415, 463, 480, 463, 415, 339, 240, 124, 0, -124, -239, -339, -415, -463, -480, -463, -415, -339, -240, -124};

// Generated by hmgen misc utility from cc2600 (using offset 9, matching the the 3 cycles of strobe(HMOVE) * 3 CPU cycle per color cycle)
const char sprite_wait[153] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
const char sprite_hm[153] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0};

void sprites_hpos_set()
{
    i = (odd || direction_missile[0] == -1) && direction_missile[1] != -1; // Condition for second missile is precomputed. Withing the code it is a bit too long and causes weird results
    X = xpos[0] >> 8;
    Y = sprite_wait[X];
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESP0);
    *HMP0 = sprite_hm[X];
    X = xpos[1] >> 8;
    Y = sprite_wait[X];
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESP1);
    *HMP1 = sprite_hm[X];
    X = xpos_second_tank[0];
    Y = sprite_wait[X];
    *HMP0 = 0; // Stop motion of player 0
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESM0);
    *HMM0 = sprite_hm[X];
    X = xpos_second_tank[1];
    Y = sprite_wait[X];
    *HMP1 = 0; // Stop motion of player 1
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESM1);
    *HMM1 = sprite_hm[X];
    if (i) {
        X = xmissile[1] >> 8;
    } else {
        X = xmissile[0] >> 8;
    }
    Y = sprite_wait[X];
    *HMM0 = 0; // Stop motion of missile 0 (second tank of player 0)
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESBL);
    *HMBL = sprite_hm[X];
    csleep(10); // Necessary, otherwise if the ball is on the left (tank bullets), this would occur too early (in HBLANK)
    *HMM1 = 0; // Stop motion of missile 1 (second tank of player 1)
    strobe(WSYNC);
    strobe(HMOVE);
    strobe(WSYNC);
    strobe(HMCLR); // Reset all horizontal motions
}

void init()
{
    // Init graphics
    *COLUP0 = VCS_DGRAY;
    *COLUP1 = VCS_BLUE;
    *REFP1 = 0x08; // Second player looks left
    *VDELP1 = 1; // Delay P0 update
    *VDELBL = 1; // Delay ball update
    *CTRLPF = 0x11; // 2 pixels wide ball + Reflective playfield, for this tutorial
    *COLUPF = VCS_GREEN; 
    
    // Init game state
    ypos[0] = ypos[1] = (KERNAL / 2 - 20) * 256;
    xpos[0] = 30 * 256;
    xpos[1] = (160 - 30 - 8) * 256;
    direction[0] = 6; 
    direction[1] = 12;
    direction_missile[0] = -1;
    direction_missile[1] = -1;
    xmissile[0] = 0;
    xmissile[1] = 0;
    xpos_second_tank[0] = 30 + 4;
    xpos_second_tank[1] = (160 - 30 - 4);
    for (X = 1; X >= 0; X--) {
        ypos_second_tank[X] = KERNAL / 2 + 8;
        direction_second_tank[X] = direction[X];
        lives[X] = 3; 
    }
}

void game_logic()
{
    // Execute action for both players
    for (Y = 1; Y >= 0; Y--) {
        if (!(joystick[Y] & 0x04)) { // Left
            counter[Y]++;
            if (counter[Y] == ROTATION_DELAY) {
                counter[Y] = 0;
                direction[Y]--;
                if (direction[Y] < 0) direction[Y] = 23;
            }
        } else if (!(joystick[Y] & 0x08)) { // Right 
            counter[Y]++;
            if (counter[Y] == ROTATION_DELAY) {
                counter[Y] = 0;
                direction[Y]++;
                if (direction[Y] == 24) direction[Y] = 0;
            } 
        } else counter[Y] = ROTATION_DELAY - 1;
        if (!(joystick[Y] & 0x01)) { // Up/Forward
            X = direction[Y];
            xpos[Y] += dx[X];
            ypos[Y] -= dy[X];
            if ((xpos[Y] >> 8) == 0) xpos[Y] = 256;
            else if ((xpos[Y] >> 8) > 151) xpos[Y] = 151 * 256;
            if ((ypos[Y] >> 8) == 0) ypos[Y] = 256;
            else if ((ypos[Y] >> 8) > 180) ypos[Y] = 180 * 256;
        }
        if (!(joystick[Y] & 0x02)) { // Backward : switch tank
            if (tank_switch_counter[Y] == 0) {
                i = xpos[Y] >> 8;
                xpos[Y] = xpos_second_tank[Y] << 8;
                xpos_second_tank[Y] = i;
                i = ypos[Y] >> 8;
                ypos[Y] = ypos_second_tank[Y] << 8;
                ypos_second_tank[Y] = i;
            } 
            tank_switch_counter[Y] = 60; // 1 second delay between two tank switches
        }
        if (tank_switch_counter[Y] > 0) tank_switch_counter[Y]--;
        
        // Joystick button
        if (!(joystick[Y] & 0x80)) { // Button pressed
            if (!button_pressed[Y]) {
                button_pressed[Y] = 1;
                if (direction_missile[Y] == -1) {
                    // Start a missile
                    direction_missile[Y] = direction[Y];
                    xmissile[Y] = xpos[Y] + 3 * 256;
                    ymissile[Y] = ypos[Y] + 3 * 256;
                    distance_missile[Y] = 0;
                }
            }
        } else button_pressed[Y] = 0; 

        // Make the missile progress
        if (direction_missile[Y] != -1) {
            X = direction_missile[Y];
            xmissile[Y] += dx_missile[X];
            ymissile[Y] -= dy_missile[X];
            distance_missile[Y]++;
            if ((xmissile[Y] >> 8) == 0 || (xmissile[Y] >> 8) > 152 || (ymissile[Y] >> 8) == 0 || (ymissile[Y] >> 8) > 181 || distance_missile[Y] > MAX_DISTANCE_MISSILE) {
                direction_missile[Y] = -1;
                xmissile[Y] = 0;
            }
        }
    }
}

const char nusiz_for_lives[4] = {0, 0, 1, 3};

void display_remaining_lives()
{
    // Set up left lives positions 
    Y = sprite_wait[4]; // 4 pixels to the left
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESP0);
    *HMP0 = sprite_hm[4];
    // Right lives display
    X = 160 + 4 - (lives[1] << 4); // 4 pixels margin on the right
    Y = sprite_wait[X];
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESP1);
    *HMP1 = sprite_hm[X];
    *HMP0 = 0; // Stop motion of player 0
    strobe(WSYNC);
    strobe(HMOVE);
    *NUSIZ0 = nusiz_for_lives[X = lives[0]];
    *NUSIZ1 = nusiz_for_lives[X = lives[1]];
    *REFP1 = 8;
    *REFP0 = 0;
    strobe(HMCLR);
    *VDELP1 = 0;
    for (Y = sizeof(tank0) - 1; Y >= 0; Y--) {
        strobe(WSYNC);
        *GRP0 = tank10[Y]; 
        *GRP1 = tank10[Y];
    }
    strobe(WSYNC);
    *VDELP1 = 1;
}

void prepare_drawing()
{
    // Setup colors
    *COLUBK = VCS_LGREEN;

    for (Y = 1; Y >= 0; Y--) {
        // Set up players sprites
        X = direction[Y];
        i = ypos[Y] >> 8;
        // Set up sprite pointer
        if (Y) {
            sprite_ptr1 = tank_models[X] - 1 - i; // -1 offset because lower position (ypos = 0) matches sprite_ptr[Y = 1]
            mask_ptr1 = tank_mask + KERNAL - 1 - i; // Same offset as sprite_ptr
            second_tank_mask1 = tank_mask + KERNAL - 1 - ypos_second_tank[Y];
            second_tank_nusiz1 = second_tank_nusiz - ypos_second_tank[Y];
        } else {
            sprite_ptr0 = tank_models[X] - 1 - i; // -1 offset because lower position (ypos = 0) matches sprite_ptr[Y = 1]
            mask_ptr0 = tank_mask + KERNAL - 1 - i; // Same offset as sprite_ptr
            second_tank_mask0 = tank_mask + KERNAL - 1 - ypos_second_tank[Y];
            second_tank_nusiz0 = second_tank_nusiz - ypos_second_tank[Y];
        }
        REFP0[Y] = sprite_reflect[X];
    }
    i = OUT_OF_SCREEN; 
    // Set up missiles display
    if ((odd || direction_missile[0] == -1) && direction_missile[1] != -1) {
        i = ymissile[1] >> 8;
    } else if (direction_missile[0] != -1) {
        i = ymissile[0] >> 8;
    }
    missile_mask_ptr = missile_mask - 1 - i;
}
    
void main()
{
    init();

    while(1) {
        // Blank
        *VBLANK = 2; // Enable VBLANK
        *VSYNC = 2; // Set VSYNC
        strobe(WSYNC); // Hold it for 3 scanlines
        strobe(WSYNC);
        strobe(WSYNC);
        *VSYNC = 0; // Turn VSYNC Off
        
        *TIM64T = ((BLANK - 3) * 76) / 64;

        // Get joystick input 
        joystick[0] = (*INPT4 & 0x80) | (*SWCHA) >> 4;
        joystick[1] = (*INPT5 & 0x80) | (*SWCHA & 0x0f);

        odd ^= 1; 

        game_logic();

        // Drawing
        prepare_drawing();

        sprites_hpos_set();

        Y = KERNAL;
        // Load TIA registers for the first line
        *ENABL = missile_mask_ptr[Y];
        *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y];
        *GRP0 = sprite_ptr0[Y] & mask_ptr0[Y];
        Y--;
        // Load TIA registers for the second line
        *ENABL = missile_mask_ptr[Y]; // This is not active until GRP0 is loaded
        *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y]; // This is not active until GRP0 is loaded
        j = sprite_ptr0[Y] & mask_ptr0[Y];
        *PF2 = Y; // To check
        Y--;

        while (*INTIM); // Wait for the end of Vertical blank
        
        strobe(WSYNC);
        // First line. A bit special. It is correctly displayed because we have already fetched GRP0 and GRP1
        *VBLANK = 0;
        load(j); // Load for the second line
        // Now, we will draw the image
        do {
            strobe(WSYNC);
            store(*GRP0);    // loads GRP0, GRP1 and ENABL1 (due to VDEL registers) 
            *ENABL = missile_mask_ptr[Y];
            *PF2 = Y; // To check
            *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y];
            *NUSIZ0 = second_tank_nusiz0[Y];
            *ENAM0 = second_tank_mask0[Y];
            load(sprite_ptr0[Y] & mask_ptr0[Y]);
            Y--;
            strobe(WSYNC);
            store(*GRP0);
            *ENABL = missile_mask_ptr[Y];
            *PF2 = Y; // To check
            *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y];
            *NUSIZ1 = second_tank_nusiz1[Y];
            *ENAM1 = second_tank_mask1[Y];
            load(sprite_ptr0[Y] & mask_ptr0[Y]);
            Y--;
        } while (Y); 

        // Last line
        strobe(WSYNC);
        store(*GRP0);
        *PF2 = Y; // To check

        strobe(WSYNC);
        *TIM64T = (OVERSCAN * 76) / 64 + 2;
        
        *COLUBK = VCS_BLACK;
        // Display remaining lives
        display_remaining_lives();

        // Overscan
        *VBLANK = 2; // Enable VBLANK
        // Do some logic here
        while (*INTIM);
        strobe(WSYNC);
    }
}
