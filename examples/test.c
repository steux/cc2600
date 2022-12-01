#include "vcs.h"

unsigned char X, Y;

const unsigned char RED = 0x36;
const unsigned char BLUE = 0x8e;
const unsigned char LBLUE = 0x9e;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x1e;
const unsigned char ORANGE = 0xfa;

#define SPRITE_HEIGHT 15 
#define WAIT for (Y = 5; Y != 0; Y--);

void draw_sprite()
{
        *GRP0 = 0x1C;
        *HMBL = 0xD0;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x38;
        *GRP1 = 0x04;
        *ENABL = 2;
        *HMBL = 0xF0;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x70;
        *GRP1 = 0x0c;
        *HMBL = 0x00;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP1 = 0x0e;
        *HMBL = 0xF0; // One pixel to the right
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP1 = 0x0a;
        *HMBL = 0x00;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x10;
        *CTRLPF = 0x30;
        *HMBL = 0x40;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0xe0;
        *COLUP0 = YELLOW;
        *GRP1 = 0x0b; 
        *HMBL = 0xF0;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMBL = 0x00;
        *GRP1 = 0x0f;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0xF0;
        *GRP1 = 0x07;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *COLUP1 = RED;
        *GRP1 = 0x04;
        *HMBL = 0xe0;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x70;
        *GRP1 = 0x1f;
        *COLUP1 = ORANGE;
        *HMP1 = 0xD0;
        *HMBL = 0xf0;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMP1 = 0x00;
        *HMBL = 0x10;
        *GRP0 = 0x78;
        *GRP1 = 0x10;
        *COLUP0 = RED;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMBL = 0x00;
        *GRP0 = 0x7c;
        *GRP1 = 0x0e;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMBL = 0x10;
        *GRP1 = 0;
        *GRP0 = 0x3e;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMBL = 0;
        *ENABL = 0;
        *GRP0 = 0x3f;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x1e;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0;
}

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
    for (X = 32; X != 0; X--) strobe(WSYNC);

    strobe(WSYNC);

    *COLUP0 = RED; 
    *HMP0 = 0;
    *GRP0 = 0;
    *GRP0 = 0;
    *GRP0 = 0;
    strobe(RESP0);
    strobe(WSYNC);
    
    *COLUP1 = WHITE; 
    *HMP1 = 0;
    *GRP1 = 0;
    *GRP1 = 0;
    *GRP1 = 0;
    strobe(RESP1);
    strobe(WSYNC);
    
    *COLUBK = BLUE;
    *COLUPF = BLACK;
    *GRP1 = 0;
    *GRP1 = 0;
    *CTRLPF = 0x20;
    strobe(RESBL);
    strobe(WSYNC);
    
    // Renable output (disable VBLANK)
    *VBLANK = 0;

    for (X = 192 - SPRITE_HEIGHT; X != 0; X--) {
      strobe(HMOVE);
      WAIT;
      if (X == 32) draw_sprite(); 
      strobe(WSYNC);
    }

    *VBLANK = 2; // Enable VBLANK again
    // Now we have 30 lines of VBLANK
    strobe(HMCLR);
    for (X = 30; X != 0; X--) strobe(WSYNC);
  } while(1);
}
