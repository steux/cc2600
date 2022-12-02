#ifdef __ATARI2600__
#include "vcs.h"
unsigned char X, Y;

const unsigned char *const s0_PF0[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xc0,0xe0,0xf0,0xf0,0x70,0x30,0x10};
const unsigned char *const s0_PF1[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0f,0x1f,0x3e,0x7c,0xf8,0xf0,0xe0,0xc0,0x80,0x00,0x00,0x00,0x00};
const unsigned char *const s0_PF2[24]={0x80,0xc0,0xe0,0xf0,0xf8,0x7c,0x3e,0x1f,0x0f,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char *const s1_PF0[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xc0,0xe0,0x70,0x30,0x10,0x00};
const unsigned char *const s1_PF1[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0e,0x1c,0x38,0x70,0xe0,0xc0,0x80,0x00,0x00,0x00,0x00,0x00};
const unsigned char *const s1_PF2[24]={0x00,0x80,0xc0,0xe0,0x70,0x38,0x1c,0x0e,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

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
        for (X = BLANK - 8; X != 0; X--) strobe(WSYNC);
        init_sprites_pos();
        first_time = 0;
    } else {
#ifdef PAL
        *TIM64T = 53; // ((48-4) * 76) / 64
#else
        *TIM64T = 44;
#endif
        while (*INTIM);
        //for (X = BLANK - 4; X != 0; X--) strobe(WSYNC);
    }
    
    // Renable output (disable VBLANK)
    strobe(WSYNC);
    strobe(HMOVE);
    *VBLANK = 0;

    for (X = KERNAL - 1 - SPRITE_HEIGHT; X != 0; X--) {
      strobe(WSYNC);
      strobe(HMOVE);
      WAIT;
      if (X == 32) draw_sprite(); 
    }

    strobe(WSYNC);
    *VBLANK = 2; // Enable VBLANK again
    // Now we have 30 lines of VBLANK
    //strobe(HMCLR);
    for (X = OVERSCAN; X != 0; X--) strobe(WSYNC);
  } while(1);
}
#else

#include <stdio.h>

unsigned char reverse(unsigned char input)
{
    unsigned char output = 0;
    if (input & 1) output |= 128;
    if (input & 2) output |= 64;
    if (input & 4) output |= 32;
    if (input & 8) output |= 16;
    if (input & 16) output |= 8;
    if (input & 32) output |= 4;
    if (input & 64) output |= 2;
    if (input & 128) output |= 1;
    return output;
}

void main()
{
    int i, j, c, d;
#define WIDTH 24
    unsigned char PF[3][WIDTH];
    for (j = 0; j < 2; j++) {
        unsigned int mask = (j)?0x0e:0x1f;
        for (i = 0; i < WIDTH; i++) {
            unsigned int val = (mask << i) >> 4;
            PF[0][i] = reverse((val >> 16) & 0x0f);
            PF[1][i] = (val >> 8) & 0xff;
            PF[2][i] = reverse(val & 0xff);
        }
        for (d = 0; d < 3; d++) {
            printf("const unsigned char *const s%d_PF%d[%d]={", j, d, WIDTH);
            for (c = 0; c < WIDTH - 1; c++) {
                printf("0x%02x,", PF[d][c]);
            }
            printf("0x%02x};\n", PF[d][c]);
        }
    }
}
#endif
