#include "vcs.h"
#include "vcs_colors.h"

// TODO: Add sounds

unsigned char X, Y;

#define BLANK 40
#define KERNAL 192 
#define OVERSCAN 30

#define ROTATION_DELAY 5
#define MAX_DISTANCE_MISSILE_1 100 
#define MAX_DISTANCE_MISSILE_2 50 

char i, j, k, r;
char rand_counter;
char *sprite_ptr0, *sprite_ptr1;
char *mask_ptr0, *mask_ptr1;
char *second_tank_mask0, *second_tank_mask1;
char *playfield_valreg_ptr;
char joystick[2]; // Joystick inputs (bit7 is button)
char switches; // Console switches
char odd; // Odd or even frame ?
char sound_iterator[2];
char sound_counter[2];

// Game state
char game_state; // 0: not started, 1: running, 2: explosion
char playfield_select;
char counter; // For explosion
unsigned short xpos[2]; // Position of sprites. 16-bits to account for diagonal movements
unsigned short ypos[2];
signed char direction[2]; // Between 0 and 23
char rotation_counter[2]; // Rotation rotation_counter for both players
unsigned short xshell[2]; // Position of shells. 16-bits to account for diagonal movements
unsigned short yshell[2];
signed char direction_shell[2]; // Between 0 and 23. -1 if shell not fired
char distance_shell[2]; // Distance counter for shells
char firing_range[2]; // Max distance for shells, depending on player difficulty switch
char button_pressed[2]; // Debouncing buttons
char tank_switch_counter[2];
char lives[2]; // Remaining lives for all tanks / all players
char xpos_second_tank[2]; // xpos for second tank
char ypos_second_tank[2]; // ypos for second tank
char direction_second_tank[2]; // Direction for second tank, between 0 and 23

// Used for masking GRPX register for player tank display
const char tank_mask[KERNAL + 12 + KERNAL] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
// Used for NUSIZX register setting + Bit 1 for ENAMX (tank shells displayed using the missile sprite)
const unsigned char shell_mask[4 + KERNAL] = { 
    0x12, 0x12, 0x12, 0x12,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
}; 
// Used for NUSIZX register setting + Bit 1 for ENAMX (second tank displayed using the missile sprite)
const unsigned char second_tank_mask[KERNAL] = { 
    0x20, 0x20, 0x22, 0x22, 0x22, 0x22, 0x32, 0x32, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  
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
const unsigned char explosion0[12] = { 0x00, 0x00, 0x00, 0x08, 0x34, 0x24, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00};
const unsigned char explosion1[12] = { 0x00, 0x00, 0x08, 0x30, 0x2e, 0x6c, 0x26, 0x12, 0x3c, 0x08, 0x00, 0x00};
const unsigned char explosion2[12] = { 0x1e, 0x6a, 0x48, 0xf5, 0x41, 0xd3, 0x93, 0xa3, 0xcf, 0x77, 0x68, 0x06};
const unsigned char explosion3[12] = { 0x52, 0x76, 0xf7, 0xa3, 0x86, 0x00, 0x40, 0x47, 0x84, 0xaa, 0xe5, 0x23};
const unsigned char explosion4[12] = { 0x43, 0x64, 0x82, 0xc1, 0x00, 0x00, 0x00, 0x00, 0x81, 0x41, 0xc2, 0x25};

const char *tank_models[29] = {tank0, tank1, tank2, tank3, tank4, tank5, 
                               tank6, tank5, tank4, tank3, tank2, tank1, 
                               tank0, tank7, tank8, tank9, tank10, tank11,
                               tank12, tank11, tank10, tank9, tank8, tank7,
                               explosion0, explosion1, explosion2, explosion3, explosion4
};

const char sprite_reflect[24] = {0, 0, 0, 0, 0, 0, 
                                 0, 8, 8, 8, 8, 8, 
                                 8, 8, 8, 8, 8, 8,
                                 0, 0, 0, 0, 0, 0};

#ifdef PAL
const char explosion_colors[5] = { 0x4E, 0x4C, 0x4A, 0x48, 0x46 };
#else
const char explosion_colors[5] = { 0xFE, 0xFC, 0xFA, 0xF8, 0xF6 };
#endif

// Generated by build_directions.c. Takes into account the 1.6 pixel ratio between vertical and horizontal
const signed char dx[24] = {40, 38, 34, 28, 19, 10, 0, -10, -20, -28, -34, -38, -40, -38, -34, -28, -19, -10, 0, 10, 19, 28, 34, 38};
const signed char dy[24] = {0, 16, 32, 45, 55, 61, 64, 61, 55, 45, 32, 16, 0, -16, -31, -45, -55, -61, -64, -61, -55, -45, -32, -16};

// Generated by build_missile_directions.c. Takes into account a 1.6 pixel ratio between vertical and horizontal
const signed short dx_shell[24] = {300, 289, 259, 212, 149, 77, 0, -77, -150, -212, -259, -289, -300, -289, -259, -212, -149, -77, 0, 77, 149, 212, 259, 289};
const signed short dy_shell[24] = {0, 124, 240, 339, 415, 463, 480, 463, 415, 339, 240, 124, 0, -124, -239, -339, -415, -463, -480, -463, -415, -339, -240, -124};

// Generated by hmgen misc utility from cc2600 (using offset 9, matching the the 3 cycles of strobe(HMOVE) * 3 CPU cycle per color cycle)
const char sprite_wait[153] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
const char sprite_hm[153] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0};

const char shell_pingpong[24] = { 12, 11, 10, 9, 20, 19, 18, 17, 16, 3, 2, 1, 0, 23, 22, 21, 8, 7, 6, 5, 4, 15, 14, 13};

#define REG_COLUPF  0x08
#define REG_COLUBK  0x09
#define REG_CTRLPF  0x0a
#define REG_PF0     0x0d
#define REG_PF1     0x0e
#define REG_PF2     0x0f

const char playfield_valregs[384] = {
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN,     
    // The lake
    REG_PF2, 0x0, REG_PF1, 0x0, REG_PF0, 0x00, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_PF0, 0x70, REG_CTRLPF, 0x00, REG_PF0, 0x30, REG_PF1, 0x0f, REG_PF2, 0xff, REG_PF2, 0xf0, REG_COLUPF, VCS_BLUE, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN,
    // The trees
    REG_CTRLPF, 0x01 /* Reflective and not priority */, REG_PF0, 0x0, REG_PF1, 0x0, REG_PF2, 0x0, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_BLUE, REG_COLUPF, VCS_BROWN, REG_PF2, 0x44,  
    REG_PF1, 0x22, REG_PF0, 0xA0, REG_PF1, 0x77, REG_PF1, 0x55, REG_PF1, 0x22, REG_PF1, 0x77, REG_PF1, 0x22, REG_PF2, 0xee,
    // The road
    REG_PF2, 0x44, REG_COLUPF, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_CTRLPF, 0x04, REG_PF0, 0, REG_PF1, 0, REG_PF2, 0, REG_COLUPF, VCS_BROWN, 
    REG_COLUPF, VCS_WHITE, REG_PF2, 0xaa, REG_PF1, 0x55, REG_PF0, 0xa0, REG_CTRLPF, 0x00, REG_COLUBK, VCS_BROWN, REG_COLUPF, VCS_BROWN,  
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, 
    REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN, REG_COLUBK, VCS_GREEN, REG_COLUBK, VCS_LGREEN 
};

// Bit 0: Fast tank
// Bit 1: Slow tank
// Bit 2: Stop tank
// Bit 3: Stop shell
// Bit 4: Ping pong shell
const char playfield_special[48] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0xc, 0xc, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 
};

// Sound code

#define GAMEOVER_SOUND  0
#define SCORE_SOUND     1
#define HIGHSCORE_SOUND 2
#define GREAT_SOUND     3

const unsigned char sound[4] = { 0x38, 0x48, 0x48, 0x48 };
const unsigned char sound_index[4] = { 0, 9, 13, 17 };
const unsigned char sound_pitch[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
const unsigned char sound_duration[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

// X is the sound to play
// Y is the player number (matching voice number)
// X is used by the function (beware)
void play_sound()
{
    AUDV0[Y] = sound[X];
    AUDC0[Y] = sound[X] >> 4;
    X = sound_index[X];
    sound_iterator[Y] = X;
    sound_counter[Y] = sound_duration[X];
    AUDF0[Y] = sound_pitch[X];
}

// X is the player number (matching voice number)
// Y is used (beware)
void play_sound_iteration()
{
    if (sound_counter[X]) {
        sound_counter[X]--;
        if (sound_counter[X] == 0) {
            sound_iterator[X]++;
            Y = sound_iterator[X];
            sound_counter[X] = sound_duration[Y];
            if (sound_counter[X]) {
                AUDF0[X] = sound_pitch[Y];
            } else {
                AUDV0[X] = 0;
                AUDC0[X] = 0;
                AUDF0[X] = 0;
            }
        } 
    }
}

void sprites_hpos_set()
{
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
    if (!odd && direction_shell[0] != - 1) {
        X = xshell[0] >> 8;
    } else {
        X = xpos_second_tank[0];
    }
    Y = sprite_wait[X];
    *HMP0 = 0; // Stop motion of player 0
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESM0);
    *HMM0 = sprite_hm[X];
    if (odd && direction_shell[1] != - 1) {
        X = xshell[1] >> 8;
    } else {
        X = xpos_second_tank[1];
    }
    Y = sprite_wait[X];
    *HMP1 = 0; // Stop motion of player 1
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESM1);
    *HMM1 = sprite_hm[X];
    Y = sprite_wait[X];
    csleep(10); // Necessary, otherwise if the ball is on the left (tank bullets), this would occur too early (in HBLANK)
    *HMM0 = 0; // Stop motion of shell 0 (second tank of player 0)
    strobe(WSYNC);
    strobe(HMOVE);
    strobe(WSYNC);
    strobe(HMCLR); // Reset all horizontal motions
}

void init()
{
    // Init graphics
    *COLUP0 = VCS_DGRAY;
    *COLUP1 = VCS_RED;
    *REFP1 = 0x08; // Second player looks left
    *VDELP1 = 1; // Delay P0 update
    *COLUPF = VCS_BLACK; 
    
    // Init game state
    ypos[0] = ypos[1] = (KERNAL / 2 - 20) * 256;
    xpos[0] = 30 * 256;
    xpos[1] = (160 - 30 - 8) * 256;
    direction[0] = 0; 
    direction[1] = 12;
    direction_shell[0] = -1;
    direction_shell[1] = -1;
    xshell[0] = 0;
    xshell[1] = 0;
    xpos_second_tank[0] = 30 + 4;
    xpos_second_tank[1] = (160 - 30 - 4);
    for (X = 1; X >= 0; X--) {
        ypos_second_tank[X] = KERNAL / 2 + 8 + (1 - X);
        direction_second_tank[X] = direction[X];
        lives[X] = 3; 
    }

    playfield_select = 0;
    playfield_valreg_ptr = playfield_valregs - 1;

    game_state = 0;
    // Reset collision detection
    strobe(CXCLR);
}

void player_was_hit()
{
    // Explosion sound
    for (X = 1; X >= 0; X--) {
        if (lives[X] == 0) {
            // Explosion player
            game_state = 2;
        }
    }
}

void rand()
{
    i = X; // Save X register
    r = tank0[X = rand_counter++];
    X = i;
}

void go_forward()
{
    xpos[Y] += dx[X];
    ypos[Y] -= dy[X];
}

void switch_to_second_tank()
{
    i = xpos[X] >> 8;
    xpos[X] = xpos_second_tank[X] << 8;
    xpos_second_tank[X] = i;
    i = ypos[X] >> 8;
    ypos[X] = ypos_second_tank[X] << 8;
    ypos_second_tank[X] = (i & 0xFE) | X;
}

inline void hide_second_tank()
{
    ypos_second_tank[X] = KERNAL;
}

void game_logic()
{
    if (switches & 0x80) {
        firing_range[1] = MAX_DISTANCE_MISSILE_1;
    } else {
        firing_range[1] = MAX_DISTANCE_MISSILE_2;
    }
    if (switches & 0x40) {
        firing_range[0] = MAX_DISTANCE_MISSILE_1;
    } else {
        firing_range[0] = MAX_DISTANCE_MISSILE_2;
    }
    
    // Execute action for both players
    for (Y = 1; Y >= 0; Y--) {

        // Collisions management 
        if (CXM0P[Y] & 0x80) {
            // Y = 0 => M0 hit Player 1 tank
            // Y = 1 => M1 hit Player 0 tank
            X = 1;
            if (Y) X = 0;
            lives[X]--;
            if (lives[X] == 1) {
                switch_to_second_tank();
                hide_second_tank();
            }
            player_was_hit();
            direction_shell[Y] = -1;
        }

        // Collision between missiles and playfield
        if ((CXM0FB[Y] & 0x80) && direction_shell[Y] != -1 && Y != odd) {
            // Missile hit playfield
            X = ((yshell[Y] >> 8) >> 3) + (playfield_select >> 3);
            j = playfield_special[X];
            if (j & 8) {
                direction_shell[Y] = -1;
            } else if (j & 16) {
                // Ping pong
                direction_shell[Y] = shell_pingpong[X = direction_shell[Y]]; 
            }
        }

        X = ((ypos[Y] >> 8) >> 3) + (playfield_select >> 3);
        j = playfield_special[X];

        if (!(joystick[Y] & 0x04)) { // Left
            rotation_counter[Y]++;
            if (rotation_counter[Y] == ROTATION_DELAY) {
                rotation_counter[Y] = 0;
                direction[Y]--;
                if (direction[Y] < 0) direction[Y] = 23;
            }
        } else if (!(joystick[Y] & 0x08)) { // Right 
            rotation_counter[Y]++;
            if (rotation_counter[Y] == ROTATION_DELAY) {
                rotation_counter[Y] = 0;
                direction[Y]++;
                if (direction[Y] == 24) direction[Y] = 0;
            } 
        } else rotation_counter[Y] = ROTATION_DELAY - 1;
        if (!(joystick[Y] & 0x01)) { // Up/Forward
            X = direction[Y];
            i = 0;
            if (CXP0FB[Y] & 0x80) { // Collide with playfield
                if (j & 4) {
                    // Tank should be stopped there
                    X += 12;
                    if (X >= 24) X -= 24;
                }
                i = j & 2; // Test for slow down
            }
            go_forward();
            if (!i) {
                go_forward();
                if (j & 1) {
                    go_forward();
                }
            }
            if ((xpos[Y] >> 8) == 0) xpos[Y] = 256;
            else if ((xpos[Y] >> 8) >= 150) xpos[Y] = 149 * 256;
            if ((ypos[Y] >> 8) == 0) ypos[Y] = 256;
            else if ((ypos[Y] >> 8) >= 181) ypos[Y] = 180 * 256;
        }
        if (!(joystick[Y] & 0x02) && lives[Y] != 1) { // Backward : switch tank
            if (tank_switch_counter[Y] == 0) {
                X = Y;
                switch_to_second_tank();
            } 
            tank_switch_counter[Y] = 60; // 1 second delay between two tank switches
        }
        if (tank_switch_counter[Y] > 0) tank_switch_counter[Y]--;

        // Joystick button
        if (!(joystick[Y] & 0x80)) { // Button pressed
            if (!button_pressed[Y]) {
                button_pressed[Y] = 1;
                if (direction_shell[Y] == -1) {
                    // Start a shell
                    direction_shell[Y] = direction[Y];
                    xshell[Y] = xpos[Y] + 3 * 256;
                    yshell[Y] = ypos[Y] + 3 * 256;
                    distance_shell[Y] = 0;
                    // And go left/right randomly
                    rand();
                    if (r & 1) {
                        direction[Y]++;
                        if (direction[Y] == 24) direction[Y] = 0;
                    } else {
                        direction[Y]--;
                        if (direction[Y] < 0) direction[Y] = 23;
                    }
                }
            }
        } else button_pressed[Y] = 0; 

        // Make the shell progress
        if (direction_shell[Y] != -1) {
            X = direction_shell[Y];
            xshell[Y] += dx_shell[X];
            yshell[Y] -= dy_shell[X];
            distance_shell[Y]++;
            if ((xshell[Y] >> 8) == 0 || (xshell[Y] >> 8) >= 153 || (yshell[Y] >> 8) == 0 || (yshell[Y] >> 8) >= 189 || distance_shell[Y] > firing_range[Y]) {
                direction_shell[Y] = -1;
                xshell[Y] = 0;
            }
        }
    }

    // Collisions between missiles and second tanks
    {
        if (*CXPPMM & 0x40) {
            // This is most likely a hit of shell on the second tank
            if (!odd && direction_shell[1] != - 1) {
                // This is player 1 that hit player 0 second tank
                lives[0]--;
                if (lives[0] == 1) {
                    X = 0;
                    hide_second_tank();
                }
                player_was_hit();
                direction_shell[1] = -1;
            } else if (odd && direction_shell[0] != -1) {
                // This is player 0 that hit player 1 second tank
                lives[1]--;
                if (lives[1] == 1) {
                    X = 1;
                    hide_second_tank();
                }
                player_was_hit();
                direction_shell[0] = -1;
            } else {
                // Shells have hit each other
                direction_shell[0] = -1;
                direction_shell[1] = -1;
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
    sprite_ptr1 = tank10;
    Y = lives[1];
    if (Y == 0) {
        sprite_ptr1 = tank_models[Y = direction[1]]; //explosion2;
        Y = 1;
    }
    X = 160 + 4 - (Y << 4); // 4 pixels margin on the right
    Y = sprite_wait[X];
    strobe(WSYNC);
    strobe(HMOVE);
    do { Y--; } while (Y);
    strobe(RESP1);
    *HMP1 = sprite_hm[X];
    *HMP0 = 0; // Stop motion of player 0
    sprite_ptr0 = tank10;
    X = lives[0];
    if (X == 0) {
        sprite_ptr0 = tank_models[Y = direction[0]]; //explosion2;
    }
    strobe(WSYNC);
    strobe(HMOVE);
    *NUSIZ0 = nusiz_for_lives[X];
    *NUSIZ1 = nusiz_for_lives[X = lives[1]];
    *REFP1 = 8;
    *REFP0 = 0;
    strobe(HMCLR);
    *VDELP1 = 0;
    for (Y = sizeof(tank0) - 1; Y >= 0; Y--) {
        strobe(WSYNC);
        *GRP0 = sprite_ptr0[Y]; 
        *GRP1 = sprite_ptr1[Y];
    }
    strobe(WSYNC);
    *VDELP1 = 1;
    *NUSIZ0 = 0;
    *NUSIZ1 = 0;
}

void prepare_drawing()
{
    // Setup colors
    *COLUBK = VCS_LGREEN;

    for (Y = 1; Y >= 0; Y--) {
        // Set up players sprites
        X = direction[Y];
        i = (ypos[Y] >> 8) + 1; // +1 offset because lower position (ypos = 0) matches sprite_ptr[Y = 1]
        if (direction_shell[Y] != -1) {
            j = yshell[Y] >> 8;
        } else j = 0;
        // Set up sprite pointer
        if (Y) {
            sprite_ptr1 = tank_models[X] - i;
            mask_ptr1 = tank_mask + KERNAL - i; // Same offset as sprite_ptr
            if (odd && j) {
                second_tank_mask1 = shell_mask - 1 - j;
                *NUSIZ1 = 0x10;
            } else {
                second_tank_mask1 = second_tank_mask - ypos_second_tank[1];
                *NUSIZ1 = 0x20;
            } 
        } else {
            sprite_ptr0 = tank_models[X] - i; // -1 offset because lower position (ypos = 0) matches sprite_ptr[Y = 1]
            mask_ptr0 = tank_mask + KERNAL - i; // Same offset as sprite_ptr
            if (!odd && j) {
                second_tank_mask0 = shell_mask - 1 - j;
                *NUSIZ0 = 0x10;
            } else {
                second_tank_mask0 = second_tank_mask - ypos_second_tank[0];
                *NUSIZ0 = 0x20;
            } 
        }
        REFP0[Y] = sprite_reflect[X];
    }
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

        // Get joystick and switches input 
        joystick[0] = (*INPT4 & 0x80) | (*SWCHA) >> 4;
        joystick[1] = (*INPT5 & 0x80) | (*SWCHA & 0x0f);
        switches = *SWCHB;

        odd ^= 1; 

        if (game_state == 0) {
            if (!((joystick[0] | joystick[1]) & 0x80)) {
                game_state = 1; // Start game
                button_pressed[0] = 1;
                button_pressed[1] = 1;
            }
            if (!(switches & 2)) {
                if (counter == 0) {
                    playfield_select += 32;
                    if (playfield_select > sizeof(playfield_valregs) - 192) playfield_select = 0;
                    playfield_valreg_ptr = playfield_valregs - 1 + playfield_select;
                    counter = 1; // Anti bouncing for select
                }
            } else counter = 0; // Anti bouncing for select
        } else if (game_state == 2) {
            // Game over
            for (Y = 1; Y >= 0; Y--) {
                if (lives[Y] == 0) {
                    rotation_counter[Y]++;
                    if (rotation_counter[Y] == ROTATION_DELAY) {
                        rotation_counter[Y] = 0;
                        X = counter++;
                        direction[Y] = 24 + X;
                        COLUP0[Y] = explosion_colors[X];
                        if (X == 4) counter = 0;
                    }
                }
            }
            if (!(switches & 1)) { // Reset
                init();
            }
        } else game_logic();

        // Reset collision detection
        strobe(CXCLR);

        // Drawing
        prepare_drawing();

        sprites_hpos_set();

        Y = KERNAL;
        // Load TIA registers for the first line
        *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y];
        *GRP0 = sprite_ptr0[Y] & mask_ptr0[Y];
        i = playfield_valreg_ptr[Y]; // The first value for playfield is for index KERNAL (192)
        Y--;
        // Load TIA registers for the second line
        *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y]; // This is not active until GRP0 is loaded
        j = sprite_ptr0[Y] & mask_ptr0[Y];
        X = playfield_valreg_ptr[Y]; // The first register for playfield if for index KERNAL - 1 (191), applied for the first 2 lines
        VSYNC[X] = i; // Change one of the TIA registers, programatically
        Y--;
    
        *ENAM0 = 0;
        *ENAM1 = 0; // Just to make sure
        *PF0 = 0;
        *PF1 = 0;
        *PF2 = 0;

        while (*INTIM); // Wait for the end of Vertical blank
        
        strobe(WSYNC);
        // First line. A bit special. It is correctly displayed because we have already fetched GRP0 and GRP1
        *VBLANK = 0;
        load(j); // Load for the second line
        // Now, we will draw the image
        do {
            // We're using a "hybrid" 2 lines kernel (GRPX & ENAMX are set every line, so maximize player and shells
            // resolution. NUSIZX is set one line out of two, for the display of the second tank. A programmable TIA register is also set every second line) 
            strobe(WSYNC);
            store(*GRP0);    // loads GRP0 and GRP1
            *ENAM1 = second_tank_mask1[Y];
            *NUSIZ0 = (*ENAM0 = second_tank_mask0[Y]) & 0xf0;
            *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y];
            i = playfield_valreg_ptr[Y];
            X = sprite_ptr0[Y] & mask_ptr0[Y];
            Y--;
            load(playfield_valreg_ptr[Y]); // The last register for playfield is for Y = 1 (and is applied for the last two lines)
            // 8 reads can go over a 256 bytes boundaries per line (all masks and playfield reads) 
            // so timing of first kernel line is between 67 and 75 cycles (<76, so OK)
            strobe(WSYNC);
            *GRP0 = X;
            store(X); // Transfer accumulator to X (TAX)
            VSYNC[X] = i; // Change one of the TIA registers, programatically
            *ENAM0 = second_tank_mask0[Y];
            *NUSIZ1 = (*ENAM1 = second_tank_mask1[Y]) & 0xf0;
            *GRP1 = sprite_ptr1[Y] & mask_ptr1[Y];
            load(sprite_ptr0[Y] & mask_ptr0[Y]);
            Y--;
            // timing of second kernel line is between 64 and 70 cycles (<76, so OK)
        } while (Y); 

        // Last line
        strobe(WSYNC);
        store(*GRP0);

        strobe(WSYNC);
        *TIM64T = (OVERSCAN * 76) / 64 + 2;
        
        *COLUBK = VCS_BLACK;
        // Display remaining lives
        if (game_state) display_remaining_lives();

        // Overscan
        *VBLANK = 2; // Enable VBLANK
        // Do some logic here
        for (X = 1; X >= 0; X--)
            play_sound_iteration();
        
        while (*INTIM);
        strobe(WSYNC);
    }
}
