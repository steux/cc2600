#include "vcs.h"
#include "3eplus.h"

unsigned char Y;

#ifdef PAL
#define BLANK 48
#define KERNAL (192 + 36)
#define OVERSCAN 36
#else
#define BLANK 40
#define KERNAL 192
#define OVERSCAN 30
#endif

// bank1 tells that this is a variable stored on cartridge in second RAM bank
// With 3E+ bankswitching scheme, each RAM bank is 512 bytes large
bank1 unsigned char color;

// Just to test bankswitching
bank2 void init()
{
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
        select_ram(1); // Select the RAM bank so that color variable gets accessible
        color = 0;
        while (*INTIM);
        strobe(WSYNC);
        *VBLANK = 0;

        // Image
        // Do some logic here
        for (Y = KERNAL + 1; Y != 0; Y--) {
            strobe(WSYNC);
            *COLUBK = ++color;
        }
        
        // Overscan
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = (OVERSCAN * 76 + 13) / 64;
        // Do some logic here
        while (*INTIM);
    }
}
