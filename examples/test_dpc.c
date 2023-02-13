#include "vcs.h"
#include "dpc.h"

unsigned char X, Y;

#ifdef PAL
#define BLANK 48
#define KERNAL (192 + 36)
#define OVERSCAN 36
#else
#define BLANK 40
#define KERNAL 192
#define OVERSCAN 30
#endif

unsigned char *sprite_ptr;
unsigned char *color_ptr;
char ypos;

const display unsigned char sprite0[20] = { 0x12, 0x36, 0x4a, 0x33, 0x55, 0x33, 0xcb, 0xb6, 0x48, 0x3e, 0x5e, 0x6e, 0x76, 0x36, 0x84, 0xbc, 0x3a, 0x76, 0x66, 0x66};
const display unsigned char colors0[20] = { 0x3c, 0x3c, 0x3c, 0x0e, 0x0e, 0x0e, 0x3c, 0x3c, 0x3c, 0x3c, 0x38, 0x2c, 0x3c, 0x3c, 0x38, 0x38, 0x2c, 0x2c, 0x12, 0x12};

// This function is put to bank1 (instead of default bank0) just to check bankswitching
bank1 void init()
{
    ypos = 100;
    // Position sprite
    strobe(WSYNC);
    X = 6;
    do { X--; } while (X >= 0);
    strobe(RESP0);
    strobe(WSYNC);
}

void main()
{
    init();

    while(1) {
        *VBLANK = 2; // Enable VBLANK
        *VSYNC = 2; // Set VSYNC
        strobe(WSYNC); // Hold it for 3 scanlines
        strobe(WSYNC);
        strobe(WSYNC);
        *VSYNC = 0; // Turn VSYNC Off
        
        // Blank
        *TIM64T = ((BLANK - 3) * 76 + 13) / 64;
        // Do some logic here

        // Prepare DPC sprites for drawing
        // Set up first sprite
        sprite_ptr = ypos - sprite0;
        *DF0HI = sprite_ptr >> 8;
        *DF0LOW = sprite_ptr;
        *DF0TOP = -sprite0 - 1;
        *DF0BOT = -sprite0 - 21;
        
        // Set up color sprite
        color_ptr = ypos - colors0;
        *DF1HI = color_ptr >> 8;
        *DF1LOW = color_ptr;
        *DF1TOP = -colors0 - 1;
        *DF1BOT = -colors0 - 21;
       
        // Joystick input 
        if (!(*SWCHA & 0x80)) *HMP0 = 0xF0; // Right
        if (!(*SWCHA & 0x40)) *HMP0 = 0x10; // Left
        if (!(*SWCHA & 0x20)) ypos++; // Down
        if (!(*SWCHA & 0x10)) ypos--; // Up
       
        // Apply movement 
        strobe(WSYNC);
        strobe(HMOVE);
        csleep(10);

        // Stop moving
        *HMP0 = 0;
        
        // Apply movement
        strobe(WSYNC);
        strobe(HMOVE);

        while (*INTIM);
        strobe(WSYNC);
        *VBLANK = 0;

        // Image
        // Do some logic here
        for (Y = KERNAL + 1; Y != 0; Y--) {
            *GRP0 = *DF0DATAW;
            *COLUP0 = *DF1DATAW;
            strobe(WSYNC);
        }
        
        // Overscan
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = (OVERSCAN * 76 + 13) / 64;
        // Do some logic here
        while (*INTIM);
    }
}
