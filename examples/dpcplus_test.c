#include "vcs.h"
#include "dpcplus.h"

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
unsigned char ypos;

const display char sprite1[] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7};
const display char sprite2[] = { 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7};

void init()
{
    ypos = 100;
    // Position sprite
    strobe(WSYNC);
    X = 6;
    do { X--; } while (X >= 0);
    strobe(RESP0);
    strobe(RESP1);
    *COLUP0 = 0x64;
    *COLUP1 = 0x44;
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
        sprite_ptr = ypos - sprite1;
        *DF0HI = sprite_ptr >> 8;
        *DF0LOW = sprite_ptr;
        *DF0TOP = -sprite1 - 1;
        *DF0BOT = -sprite1 - 9;
        
        // Set up second sprite
        sprite_ptr = ypos - sprite2;
        *DF1HI = sprite_ptr >> 8;
        *DF1LOW = sprite_ptr;
        *DF1TOP = -sprite2 - 1;
        *DF1BOT = -sprite2 - 9;
       
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
        *HMP1 = 0;
        
        // Apply movement
        strobe(WSYNC);
        strobe(HMOVE);

        while (*INTIM);
        strobe(WSYNC);
        *VBLANK = 0;

        // Image
        // Do some logic here
        for (Y = KERNAL + 1; Y != 0; Y--) {
            strobe(WSYNC);
            *GRP0 = *DF0DATAW;
            *GRP1 = *DF1DATAW;
        }
        
        // Overscan
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = (OVERSCAN * 76 + 13) / 64;
        // Do some logic here
        while (*INTIM);
    }
}
