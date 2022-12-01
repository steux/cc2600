#include "vcs.h"

unsigned char X, Y;

const unsigned char RED = 0x34;
const unsigned char BLUE = 0x84;
const unsigned char BLACK = 0x00;

void main()
{
  do {
    *VBLANK = 2; // Enable VBLANK
    *VSYNC = 2; // Set VSYNC
    strobe(WSYNC); // Hold it for 3 scanlines
    strobe(WSYNC);
    strobe(WSYNC);
    *VSYNC = 0; // Turn VSYNC Off

    // Now we have 37 lines of VBLANK
    for (X = 37; X != 0; X--) strobe(WSYNC);
    // Renable output (disable VBLANK)
    *VBLANK = 0;
  
    *COLUBK = BLUE;
    *COLUPF = BLACK;

    for (X = 192; X != 0; X--) {
      (*COLUBK)++; 
      strobe(WSYNC);
    }

    *VBLANK = 2; // Enable VBLANK again
    // Now we have 30 lines of VBLANK
    for (X = 30; X != 0; X--) strobe(WSYNC);
  } while(1);
}
