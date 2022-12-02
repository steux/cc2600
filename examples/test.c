#include "vcs.h"

unsigned char X, Y;

#ifdef PAL
const unsigned char RED = 0x64;
const unsigned char BLUE = 0xB4;
const unsigned char LBLUE = 0xBA;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x2e;
const unsigned char ORANGE = 0x4a;
#define BLANK 48
#define KERNAL 228
#define OVERSCAN 36
#else
const unsigned char RED = 0x36;
const unsigned char BLUE = 0x8e;
const unsigned char LBLUE = 0x9e;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x1e;
const unsigned char ORANGE = 0xfa;
#define BLANK 40
#define KERNAL 192
#define OVERSCAN 30
#endif

#define SPRITE_HEIGHT 16 
#define WAIT for (Y = 5; Y != 0; Y--);

void draw_sprite()
{
        *COLUP0 = RED; 
        *COLUP1 = WHITE; 
        *GRP0 = 0x1C;
        *HMBL = 0xD0; // +3 BALL
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x38;
        *GRP1 = 0x04;
        *ENABL = 2;
        *HMBL = 0xF0; // + 1 BALL
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
        *HMBL = 0xF0; // +1 BALL
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
        *HMBL = 0x40; // -4 BALL
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0xe0;
        *COLUP0 = YELLOW;
        *GRP1 = 0x0b; 
        *HMBL = 0xF0; // +1 BALL
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
        *HMBL = 0xe0; // +2 BALL
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0x70;
        *GRP1 = 0x1f;
        *COLUP1 = ORANGE;
        *HMP1 = 0xD0; // P1 : + 3
        *HMBL = 0xf0; // + 1 BALL
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMP1 = 0x00;
        *HMBL = 0x10; // -1 BALL
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
        *HMBL = 0x10; // -1 BALL
        *GRP1 = 0;
        *GRP0 = 0x3e;
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMBL = 0x30; // -3 BALL
        *ENABL = 0;
        *GRP0 = 0x3f;
        *HMP1 = 0x30; // P1: - 3
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *HMBL = 0;
        *GRP0 = 0x1e;
        *HMP1 = 0;
        *CTRLPF = 0x20; // Ball back to 4 pixels
        strobe(WSYNC);
        strobe(HMOVE);
      WAIT;
        *GRP0 = 0;
}

void init_sprites_pos()
{
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
    strobe(HMOVE);
}

char first_time;

void main()
{
  first_time = 1;

  do {
    *VBLANK = 2; // Enable VBLANK
    *VSYNC = 2; // Set VSYNC
    strobe(WSYNC); // Hold it for 3 scanlines
    strobe(WSYNC);
    strobe(WSYNC);
    *VSYNC = 0; // Turn VSYNC Off

    if (first_time) {
        for (X = BLANK - 7; X != 0; X--) strobe(WSYNC);
        init_sprites_pos();
        first_time = 0;
    } else {
        for (X = BLANK - 3; X != 0; X--) strobe(WSYNC);
    }
    
    // Renable output (disable VBLANK)
    *VBLANK = 0;

    for (X = KERNAL - SPRITE_HEIGHT; X != 0; X--) {
      strobe(WSYNC);
      strobe(HMOVE);
      WAIT;
      if (X == 32) draw_sprite(); 
    }

    *VBLANK = 2; // Enable VBLANK again
    // Now we have 30 lines of VBLANK
    //strobe(HMCLR);
    for (X = OVERSCAN; X != 0; X--) strobe(WSYNC);
  } while(1);
}
