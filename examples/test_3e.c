#include "vcs.h"
#include "3e.h"

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

// bank1 tells that this is a variable stored on cartridge in first RAM bank (selected by select(0))
// With 3E bankswtiching scheme, each RAM bank is 1kB large
bank1 unsigned char color;

// Just to test bankswitching
bank1 void init()
{
}

void main()
{
    init();
    init(); // 2 calls are necessary to trigger the 3E cart auto detection on stella
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
        select(0); // Select the RAM bank so that color variable gets accessible
        asm("LDA #0"); // Makes sure detection of 3E is OK
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
