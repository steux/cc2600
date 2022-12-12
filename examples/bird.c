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
const unsigned char BLUE = 0xB2;
const unsigned char LBLUE = 0xBA;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x2c;
const unsigned char ORANGE = 0x4c;
const unsigned char GREY = 0x04;
const unsigned char GREEN = 0x58;
#define BLANK 48
#define KERNAL 192 
#define PALBOTTOM 36
#define OVERSCAN 36
#else
const unsigned char RED = 0x36;
const unsigned char BLUE = 0x8e;
const unsigned char LBLUE = 0x9e;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x1e;
const unsigned char ORANGE = 0xfa;
const unsigned char GREY = 0x04;
const unsigned char GREEN = 0xC6;
#define BLANK 40
#define KERNAL 192
#define OVERSCAN 30
#endif

char first_time;
unsigned char i, j, k;
unsigned char scroll_sequence;
unsigned char scroll_counter;
unsigned char lPFx[12];
unsigned char rPFx[12];
unsigned char lPFy[12];
unsigned char rPFy[12];
unsigned char left_window, right_window;
unsigned short ybird;
unsigned short yspeed;
unsigned char button_pressed;

#define SPRITE_HEIGHT 17 

#define WAIT i = Y >> 4; Y--; 
#define BEFORE X = i  

#define kernel kernel1
#define draw_bird1 draw_bird11
#define START BEFORE; *PF1 = lPFx[X]; *PF2 = lPFy[X]; WAIT; *PF1= rPFx[X]; *PF2 = rPFy[X];
#include "bird_kernel.c"

#undef kernel
#undef draw_bird1
#undef START

#define kernel kernel2
#define draw_bird1 draw_bird12
#define START BEFORE; *PF0 = lPFx[X]; *PF1 = lPFy[X]; WAIT; *PF0 = rPFx[X]; *PF1 = rPFy[X];
#include "bird_kernel.c"

#undef kernel
#undef draw_bird1
#undef START

#define kernel kernel3
#define draw_bird1 draw_bird13
#define START BEFORE; *PF0 = lPFx[X]; *PF2 = lPFy[X]; WAIT; *PF0 = rPFx[X]; *PF2 = rPFy[X];
#include "bird_kernel.c"

void init_sprites_pos()
{
    strobe(WSYNC);

    *COLUP0 = RED; 
    *HMP0 = 0;
    *GRP0 = 0;
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
    *GRP0 = 0;
    strobe(RESP1);
    strobe(WSYNC);

    *COLUBK = BLUE;
    *COLUPF = BLACK;
    *GRP1 = 0;
    *GRP1 = 0;
    *CTRLPF = 0x20;
    *GRP0 = 0;
    strobe(RESBL);
    strobe(WSYNC);
    strobe(HMOVE);
}

void load_scroll_sequence()
{
    if (scroll_sequence < 12) {
        i = 0;
    } else if (scroll_sequence < 20) {
        i = 1;
    } else {
        i = 2;
    }

    Y = scroll_sequence;
    if (i == 0) {
        j = s1_PF1[Y];
        k = s1_PF2[Y]; 
    } else if (i == 1) {
        j = s1_PF0[Y];
        k = s1_PF1[Y]; 
    } else if (i == 2) {
        j = s1_PF0[Y];
        k = s1_PF2[Y]; 
    }

    for (X = 0; X != right_window; X++) {
        rPFx[X] = j;
        rPFy[X] = k;
    }

    if (i == 0) {
        j = s0_PF1[Y];
        k = s0_PF2[Y]; 
    } else if (i == 1) {
        j = s0_PF0[Y];
        k = s0_PF1[Y]; 
    } else if (i == 2) {
        j = s0_PF0[Y];
        k = s0_PF2[Y]; 
    }
    rPFx[X] = j;
    rPFy[X] = k;
    X++;

    for (; X != right_window + 4; X++) {
        rPFx[X] = 0;
        rPFy[X] = 0;
    }

    if (i == 0) {
        j = s0_PF1[Y];
        k = s0_PF2[Y]; 
    } else if (i == 1) {
        j = s0_PF0[Y];
        k = s0_PF1[Y]; 
    } else if (i == 2) {
        j = s0_PF0[Y];
        k = s0_PF2[Y]; 
    }
    rPFx[X] = j;
    rPFy[X] = k;
    X++;

    if (i == 0) {
        j = s1_PF1[Y];
        k = s1_PF2[Y]; 
    } else if (i == 1) {
        j = s1_PF0[Y];
        k = s1_PF1[Y]; 
    } else if (i == 2) {
        j = s1_PF0[Y];
        k = s1_PF2[Y]; 
    }

    for (; X != 12; X++) {
        rPFx[X] = j;
        rPFy[X] = k;
    }
    
    Y = scroll_sequence + 4;
    if (Y >= 24) Y = Y - 24;
    if (i == 0) {
        j = s1_PF1[Y];
        k = s1_PF2[Y]; 
    } else if (i == 1) {
        j = s1_PF0[Y];
        k = s1_PF1[Y]; 
    } else if (i == 2) {
        j = s1_PF0[Y];
        k = s1_PF2[Y]; 
    }

    for (X = 0; X != left_window; X++) {
        lPFx[X] = j;
        lPFy[X] = k;
    }

    if (i == 0) {
        j = s0_PF1[Y];
        k = s0_PF2[Y]; 
    } else if (i == 1) {
        j = s0_PF0[Y];
        k = s0_PF1[Y]; 
    } else if (i == 2) {
        j = s0_PF0[Y];
        k = s0_PF2[Y]; 
    }
    lPFx[X] = j;
    lPFy[X] = k;
    X++;

    for (; X != left_window + 4; X++) {
        lPFx[X] = 0;
        lPFy[X] = 0;
    }

    if (i == 0) {
        j = s0_PF1[Y];
        k = s0_PF2[Y]; 
    } else if (i == 1) {
        j = s0_PF0[Y];
        k = s0_PF1[Y]; 
    } else if (i == 2) {
        j = s0_PF0[Y];
        k = s0_PF2[Y]; 
    }
    lPFx[X] = j;
    lPFy[X] = k;
    X++;

    if (i == 0) {
        j = s1_PF1[Y];
        k = s1_PF2[Y]; 
    } else if (i == 1) {
        j = s1_PF0[Y];
        k = s1_PF1[Y]; 
    } else if (i == 2) {
        j = s1_PF0[Y];
        k = s1_PF2[Y]; 
    }

    for (; X != 12; X++) {
        lPFx[X] = j;
        lPFy[X] = k;
    }
}
    
void init()
{
    for (X = BLANK - 8; X != 0; X--) strobe(WSYNC);
    init_sprites_pos();
    scroll_sequence = 0;
    scroll_counter = 0;
    first_time = 0;
    j = 0;
    left_window = 0;
    right_window = 0;
    ybird = 100 * 256;
    yspeed = 0;
    button_pressed = 0;
}

void game_logic()
{
    if (scroll_counter == 10) {
        scroll_counter = 0;
        scroll_sequence++;
        if (scroll_sequence == 20) left_window = right_window;
        if (scroll_sequence == 24) {
            right_window = right_window + 1;
            if (right_window == 8) right_window = 0;
            scroll_sequence = 0;
        }
    }
    yspeed -= 10;
    if (ybird >> 8 < SPRITE_HEIGHT + 8) {
        ybird = 100 * 256;
        yspeed = 0;
    }
    if (ybird >> 8 > 190) {
        ybird = 100 * 256;
        yspeed = 0;
    }
    if ((*INPT4 & 0x80) != 0) {
        if (button_pressed == 0) {
            button_pressed = 1;
            yspeed = 350;
        }
    } else button_pressed = 0;
    ybird = ybird + yspeed;

    load_scroll_sequence();
    scroll_counter++;
}

void bottom()
{
    strobe(WSYNC);
#ifdef PAL
    for (X = PALBOTTOM; X != 0; X--) strobe(WSYNC);
#endif

    *VBLANK = 2; // Enable VBLANK again
                 // Now we have 30 lines of VBLANK
                 //strobe(HMCLR);
    for (X = OVERSCAN; X != 0; X--) strobe(WSYNC);
}

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
        init();
    } else {
#ifdef PAL
        *TIM64T = 51; 
#else
        *TIM64T = 41;
#endif
        // The game logic
        game_logic();
        *COLUBK = BLUE;
        while (*INTIM);
    }
    strobe(WSYNC);
    
    if (scroll_sequence < 12) {
        kernel1();
    } else if (scroll_sequence < 20) {
        kernel2();
    } else kernel3();

    bottom();
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
