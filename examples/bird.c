#define MAX_RAINBOW_OFFSET 16 

#ifdef __ATARI2600__
#include "vcs.h"
unsigned char X, Y;

#ifdef PAL
const unsigned char RED = 0x64;
const unsigned char BLUE = 0xB2;
const unsigned char LBLUE = 0xBA;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x2c;
const unsigned char ORANGE = 0x4c;
const unsigned char GREY = 0x04;
const unsigned char LGREEN = 0x5c;
const unsigned char GREEN = 0x58;
const unsigned char DGREEN = 0x54;
const unsigned char BROWN = 0x20;
#else
const unsigned char RED = 0x32;
const unsigned char BLUE = 0x84;
const unsigned char LBLUE = 0x8e;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x1e;
const unsigned char ORANGE = 0xfa;
const unsigned char GREY = 0x04;
const unsigned char LGREEN = 0xcc;
const unsigned char GREEN = 0xc6;
const unsigned char DGREEN = 0xc2;
const unsigned char BROWN = 0xF0;
#endif

#define KERNAL 192
#ifdef PAL
#define BLANK 48
#define PALBOTTOM 36
#define OVERSCAN 36
#else
#define BLANK 40
#define OVERSCAN 30
#endif

const unsigned char s0_PF0[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xc0,0xe0,0xf0,0xf0,0x70,0x30,0x10};
const unsigned char s0_PF1[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0f,0x1f,0x3e,0x7c,0xf8,0xf0,0xe0,0xc0,0x80,0x00,0x00,0x00,0x00};
const unsigned char s0_PF2[24]={0x80,0xc0,0xe0,0xf0,0xf8,0x7c,0x3e,0x1f,0x0f,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
const unsigned char s1_PF0[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xc0,0xe0,0x70,0x30,0x10,0x00};
const unsigned char s1_PF1[24]={0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0e,0x1c,0x38,0x70,0xe0,0xc0,0x80,0x00,0x00,0x00,0x00,0x00};
const unsigned char s1_PF2[24]={0x00,0x80,0xc0,0xe0,0x70,0x38,0x1c,0x0e,0x07,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

char first_time;
unsigned char i, j, k, l;
unsigned char scroll_sequence;
unsigned char scroll_counter;
unsigned char lPFx[12];
unsigned char rPFx[12];
unsigned char lPFy[12];
unsigned char rPFy[12];
unsigned char left_window, right_window;
unsigned short ybird;
signed short yspeed;
unsigned char button_pressed;
unsigned char bird_type;
unsigned char bird_animation_counter;
unsigned char score_low;
unsigned char score_high;
unsigned char highscore_low;
unsigned char highscore_high;
unsigned char *background_ptr1;
unsigned char *background_ptr2;
unsigned char rainbow_offset;
unsigned char difficulty;

// TIATracker variables
// =====================================================================
// Permanent variables. These are states needed by the player.
// =====================================================================
char tt_timer;                // current music timer value
char tt_cur_pat_index_c0;     // current pattern index into tt_SequenceTable
char tt_cur_pat_index_c1;     
char tt_cur_note_index_c0;    // note index into current pattern
char tt_cur_note_index_c1;    
char tt_envelope_index_c0;    // index into ADSR envelope
char tt_envelope_index_c1;    
char tt_cur_ins_c0;           // current instrument
char tt_cur_ins_c1;     

// =====================================================================
// Temporary variables. These will be overwritten during a call to the
// player routine, but can be used between calls for other things.
// =====================================================================
char *tt_ptr; 

#define SPRITE_HEIGHT 16 
#define RAINBOW_SIZE 16

#define BIRD1
#define BEFORE X = i;   
#define WAIT i = Y >> 4; Y--; 
#define BEFORE2 X = i; *COLUBK = j;  
#define WAIT2 i = right_shift4[Y]; Y--; 

#define kernel kernel11
#define LEFT_PLAYFIELD *PF1 = lPFx[X]; *PF2 = lPFy[X]
#define RIGHT_PLAYFIELD *PF1 = rPFx[X]; *PF2 = rPFy[X]
#define START BEFORE; LEFT_PLAYFIELD; WAIT; RIGHT_PLAYFIELD; 
#define START2 BEFORE2; LEFT_PLAYFIELD; WAIT2; RIGHT_PLAYFIELD;
#include "bird_kernel.c"

#undef kernel
#undef START
#undef START2
#undef LEFT_PLAYFIELD
#undef RIGHT_PLAYFIELD

#define kernel kernel21
#define LEFT_PLAYFIELD *PF0 = lPFx[X]; *PF1 = lPFy[X]
#define RIGHT_PLAYFIELD *PF0 = rPFx[X]; *PF1 = rPFy[X]
#define START BEFORE; LEFT_PLAYFIELD; WAIT; RIGHT_PLAYFIELD; 
#define START2 BEFORE2; LEFT_PLAYFIELD; WAIT2; RIGHT_PLAYFIELD;
#include "bird_kernel.c"

#undef kernel
#undef START
#undef START2
#undef LEFT_PLAYFIELD
#undef RIGHT_PLAYFIELD

#define kernel kernel31
#define LEFT_PLAYFIELD *PF0 = lPFx[X]; *PF2 = lPFy[X]
#define RIGHT_PLAYFIELD *PF0 = rPFx[X]; *PF2 = rPFy[X]
#define START BEFORE; LEFT_PLAYFIELD; WAIT; RIGHT_PLAYFIELD; 
#define START2 BEFORE2; LEFT_PLAYFIELD; WAIT2; RIGHT_PLAYFIELD;
#include "bird_kernel.c"

#undef kernel
#undef START
#undef START2
#undef LEFT_PLAYFIELD
#undef RIGHT_PLAYFIELD

#undef BIRD1
#define bank1 bank2
#define right_shift4 right_shift4_bank2
#define rainbow rainbow_bank2
#undef WAIT2
#define WAIT2 i = right_shift4[Y]; Y--; 

#define kernel kernel12
#define LEFT_PLAYFIELD *PF1 = lPFx[X]; *PF2 = lPFy[X]
#define RIGHT_PLAYFIELD *PF1 = rPFx[X]; *PF2 = rPFy[X]
#define START BEFORE; LEFT_PLAYFIELD; WAIT; RIGHT_PLAYFIELD; 
#define START2 BEFORE2; LEFT_PLAYFIELD; WAIT2; RIGHT_PLAYFIELD;
#include "bird_kernel.c"

#undef kernel
#undef START
#undef START2
#undef LEFT_PLAYFIELD
#undef RIGHT_PLAYFIELD

#define kernel kernel22
#define LEFT_PLAYFIELD *PF0 = lPFx[X]; *PF1 = lPFy[X]
#define RIGHT_PLAYFIELD *PF0 = rPFx[X]; *PF1 = rPFy[X]
#define START BEFORE; LEFT_PLAYFIELD; WAIT; RIGHT_PLAYFIELD; 
#define START2 BEFORE2; LEFT_PLAYFIELD; WAIT2; RIGHT_PLAYFIELD;
#include "bird_kernel.c"

#undef kernel
#undef START
#undef START2
#undef LEFT_PLAYFIELD
#undef RIGHT_PLAYFIELD

#define kernel kernel32
#define LEFT_PLAYFIELD *PF0 = lPFx[X]; *PF2 = lPFy[X]
#define RIGHT_PLAYFIELD *PF0 = rPFx[X]; *PF2 = rPFy[X]
#define START BEFORE; LEFT_PLAYFIELD; WAIT; RIGHT_PLAYFIELD; 
#define START2 BEFORE2; LEFT_PLAYFIELD; WAIT2; RIGHT_PLAYFIELD;
#include "bird_kernel.c"

void init_bird_sprite_pos()
{
    strobe(WSYNC);
    *COLUP0 = RED; 
    *COLUP1 = WHITE; 
    *NUSIZ0 = 0x30;
    *NUSIZ1 = 0x20;
    asm("pha");
    asm("pla");
    strobe(RESP0);
    strobe(RESP1);
    strobe(RESBL);
    *HMBL = 0x70; 
    strobe(WSYNC);
    strobe(HMOVE);
    *HMP0 = 0xE0; 
    *HMP1 = 0x70;
    *HMBL = 0x40; 
    strobe(WSYNC);
    strobe(HMOVE);
    *HMP0 = 0x00;
    *HMP1 = 0x00;
    *HMBL = 0x00;
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
    init_bird_sprite_pos();
    first_time = 0;
    ybird = 100 * 256;
    yspeed = 0;
    score_low = 00;
    score_high = 99;
#ifdef PAL
    difficulty = 8;
#else
    difficulty = 10;
#endif
}

void gameover()
{
    init();
}

void game_logic()
{
    if (scroll_counter == difficulty) {
        scroll_counter = 0;
        scroll_sequence++;
        if (scroll_sequence == 20) left_window = right_window;
        if (scroll_sequence == 16) {
            score_low++;
            if (score_low == 100) {
                score_low = 0;
                score_high++;
            }
            if (score_high > highscore_high || (score_high == highscore_high && score_low > highscore_low)) {
                highscore_high = score_high;
                highscore_low = score_low;
            }
        }
        if (scroll_sequence == 24) {
            right_window = right_window + 1;
            if (right_window == 8) right_window = 0;
            scroll_sequence = 0;
        }
    }
    *HMM0 = 0x10;
    if (scroll_counter & 3) *HMM1 = 0x10;
    yspeed -= 10;
    if (yspeed >> 8 < 0) bird_type = 0;
    if ((*INPT4 & 0x80) != 0) {
        if (button_pressed == 0) {
            button_pressed = 1;
            yspeed = 350;
            if (bird_type == 0) 
                bird_type = 1;
            else {
                bird_type = 0;
                bird_animation_counter = 5;
            }
        }
    } else button_pressed = 0;

    if (bird_animation_counter != 0) {
        bird_animation_counter--;
        if (bird_animation_counter == 0) bird_type = 1;
    }

    ybird = ybird + yspeed;
    if (ybird >> 8 < 22) {
        ybird = 22 * 256;
        //gameover();
    }
    if (ybird >> 8 > 189) {
        ybird = 189 * 256;
        //gameover();
    }
    if ((*CXP0FB & 0x80) != 0) gameover();
    if ((*CXP1FB & 0x80) != 0) gameover();
    if ((*CXBLPF & 0x80) != 0) gameover();
    strobe(CXCLR);

    load_scroll_sequence();
    scroll_counter++;

    rainbow_offset++;
    if (rainbow_offset == MAX_RAINBOW_OFFSET + 16) rainbow_offset = 0;
}

const unsigned char gameover0[13] = { 0x38, 0x79, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xc1, 0xc0, 0xf8, 0x78, 0x38};
const unsigned char gameover1[13] = { 0xed, 0xed, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xef, 0xef, 0x00, 0x00, 0x00};
const unsigned char gameover2[13] = { 0x44, 0x4c, 0x5c, 0x5c, 0x5f, 0x5b, 0x5b, 0x5b, 0xdb, 0x9f, 0x0e, 0x00, 0x00};
const unsigned char gameover3[13] = { 0x08, 0x1c, 0x3e, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3e, 0x1c, 0x08};
const unsigned char gameover4[13] = { 0xc0, 0xe1, 0xf3, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0x03, 0x01, 0x00, 0x00};
const unsigned char gameover5[13] = { 0x8c, 0x8c, 0x8c, 0x8c, 0xec, 0x6c, 0x6f, 0x6f, 0x67, 0xe3, 0xc0, 0x00, 0x00};

void display_gameover()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUBK = BLACK;
    *GRP0 = 0;
    *GRP1 = 0;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *COLUP0 = WHITE;
    *COLUP1 = WHITE;
    strobe(RESP0);
    strobe(RESP1);
    *VDELP0 = 1;
    *VDELP1 = 1;
    *HMP1 = 0x20;
    *HMP0 = 0x10;
    strobe(WSYNC);
    strobe(HMOVE);
    for (Y = 12; Y != 255; Y--) {
        strobe(WSYNC);
        i = Y;
        *GRP0 = gameover0[Y];
        *GRP1 = gameover1[Y];
        *GRP0 = gameover2[Y];
        X = gameover4[Y];
        j = gameover5[Y];
        asm("lda gameover3,Y");
        Y = j;
        asm("sta GRP1");
        *GRP0 = X;
        *GRP1 = Y;
        strobe(GRP0);
        Y = i;
    }
    strobe(WSYNC);
    strobe(HMOVE);
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
}

const unsigned char flappybird0[16] = { 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xf6, 0xf6, 0xf6, 0xc6, 0xf6, 0xf6, 0x76 };
const unsigned char flappybird1[16] = { 0x03, 0x03, 0x03, 0x3b, 0x7b, 0xda, 0xda, 0xda, 0xda, 0xda, 0xda, 0x7b, 0x3b, 0x00, 0x00, 0x00 };
const unsigned char flappybird2[16] = { 0x18, 0x18, 0x18, 0x9c, 0xde, 0xd6, 0xd6, 0xd6, 0xd6, 0xd6, 0xd6, 0xde, 0x9c, 0x00, 0x00, 0x00 };
const unsigned char flappybird3[16] = { 0x30, 0x38, 0x18, 0x3b, 0x7b, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0x03, 0x03, 0x03 };
const unsigned char flappybird4[16] = { 0x00, 0x00, 0x00, 0x9b, 0xdb, 0x5b, 0x5b, 0xdb, 0x9b, 0xdb, 0x43, 0x59, 0x58, 0x40, 0xc0, 0x80 };
const unsigned char flappybird5[16] = { 0x00, 0x00, 0x00, 0x0f, 0x1f, 0x1b, 0x1b, 0x1b, 0x1b, 0xdb, 0xdb, 0xdf, 0xcf, 0x03, 0x03, 0x03 };

void display_flappybird()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUBK = BLACK;
    *GRP0 = 0;
    *GRP1 = 0;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *COLUP0 = WHITE;
    *COLUP1 = WHITE;
    strobe(RESP0);
    strobe(RESP1);
    *VDELP0 = 1;
    *VDELP1 = 1;
    *HMP1 = 0x20;
    *HMP0 = 0x10;
    strobe(WSYNC);
    strobe(HMOVE);
    for (Y = 15; Y != 255; Y--) {
        strobe(WSYNC);
        i = Y;
        *GRP0 = flappybird0[Y];
        *GRP1 = flappybird1[Y];
        *GRP0 = flappybird2[Y];
        X = flappybird4[Y];
        j = flappybird5[Y];
        asm("lda flappybird3,Y");
        Y = j;
        asm("sta GRP1");
        *GRP0 = X;
        *GRP1 = Y;
        strobe(GRP0);
        Y = i;
    }
    strobe(WSYNC);
    strobe(HMOVE);
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
}

const unsigned char getready0[16] = { 0x00, 0x00, 0x00, 0x38, 0x79, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xc3, 0xc3, 0xf9, 0x78, 0x38 };
const unsigned char getready1[16] = { 0x00, 0x00, 0x00, 0x83, 0x87, 0x86, 0x86, 0xe6, 0x66, 0x66, 0x6f, 0x6f, 0xef, 0xc6, 0x06, 0x06 };
const unsigned char getready2[16] = { 0x00, 0x00, 0x00, 0x36, 0x36, 0x3c, 0x3c, 0x3c, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3e, 0x3c };
const unsigned char getready3[16] = { 0x00, 0x00, 0x00, 0x20, 0x61, 0xe3, 0xe3, 0xfb, 0xdb, 0xdb, 0xdb, 0xd9, 0xf8, 0x70, 0x00, 0x00 };
const unsigned char getready4[16] = { 0x00, 0x00, 0x00, 0xe7, 0xef, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0xef, 0xe7, 0x03, 0x03, 0x03 };
const unsigned char getready5[16] = { 0x70, 0x78, 0x18, 0x1b, 0x3b, 0x78, 0x7b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x03, 0x03, 0x03 };

void display_getready()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUBK = BLACK;
    *GRP0 = 0;
    *GRP1 = 0;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *COLUP0 = WHITE;
    *COLUP1 = WHITE;
    strobe(RESP0);
    strobe(RESP1);
    *VDELP0 = 1;
    *VDELP1 = 1;
    *HMP1 = 0x20;
    *HMP0 = 0x10;
    strobe(WSYNC);
    strobe(HMOVE);
    for (Y = 15; Y != 255; Y--) {
        strobe(WSYNC);
        i = Y;
        *GRP0 = getready0[Y];
        *GRP1 = getready1[Y];
        *GRP0 = getready2[Y];
        X = getready4[Y];
        j = getready5[Y];
        asm("lda getready3,Y");
        Y = j;
        asm("sta GRP1");
        *GRP0 = X;
        *GRP1 = Y;
        strobe(GRP0);
        Y = i;
    }
    strobe(WSYNC);
    strobe(HMOVE);
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
}

#include "bird_score.c"

// 20 pixels high
bank3 void display_score()
{
    *PF0 = 0;
    *COLUP0 = WHITE;
    *COLUP1 = WHITE;
    strobe(WSYNC);
    strobe(HMOVE);
    strobe(RESP0);
    strobe(RESP1);
    strobe(WSYNC);
    strobe(HMOVE);
    asm("pha"); asm("pla");
    asm("pha"); asm("pla");
    *HMP1 = 0xF0;
    *HMP0 = 0x50;
    strobe(WSYNC);
    strobe(HMOVE);
    strobe(WSYNC);
    *GRP1 = 0x00;
    
    strobe(WSYNC);
    *GRP0 = 0x04;
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line1_PF1[X];
    X = score_low;
    *PF2 = score_line1_PF2[X];
    *GRP0 = 0x0d;
    X = highscore_high;
    *PF1 = score_line1_PF1[X];
    *COLUPF = YELLOW;
    X = highscore_low;
    *PF2 = score_line1_PF2[X];
    *GRP1 = 0xae;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line2_PF1[X];
    X = score_low;
    *PF2 = score_line2_PF2[X];
    *GRP0 = 0x1d;
    *GRP1 = 0xad;
    X = highscore_high;
    *PF1 = score_line2_PF1[X];
    X = highscore_low;
    *COLUPF = YELLOW;
    *PF2 = score_line2_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line3_PF1[X];
    X = score_low;
    *PF2 = score_line3_PF2[X];
    *GRP0 = 0x0d;
    X = highscore_high;
    *PF1 = score_line3_PF1[X];
    X = highscore_low;
    *COLUPF = YELLOW;
    *PF2 = score_line3_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line4_PF1[X];
    X = score_low;
    *PF2 = score_line4_PF2[X];
    *GRP1 = 0xae;
    X = highscore_high;
    *PF1 = score_line4_PF1[X];
    X = highscore_low;
    *COLUPF = YELLOW;
    *PF2 = score_line4_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line5_PF1[X];
    X = score_low;
    *PF2 = score_line5_PF2[X];
    *GRP1 = 0xac;
    X = highscore_high;
    *PF1 = score_line5_PF1[X];
    X = highscore_low;
    *COLUPF = YELLOW;
    *PF2 = score_line5_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line6_PF1[X];
    X = score_low;
    *PF2 = score_line6_PF2[X];
    *GRP0 = 0x0c;
    *GRP1 = 0xcc;
    X = highscore_high;
    *PF1 = score_line6_PF1[X];
    *COLUPF = YELLOW;
    X = highscore_low;
    *PF2 = score_line6_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line7_PF1[X];
    X = score_low;
    *PF2 = score_line7_PF2[X];
    *GRP0 = 0x1e;
    *GRP1 = 0x00;
    X = highscore_high;
    *PF1 = score_line7_PF1[X];
    *COLUPF = YELLOW;
    X = highscore_low;
    *PF2 = score_line7_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line8_PF1[X];
    X = score_low;
    *PF2 = score_line8_PF2[X];
    *GRP0 = 0x00;
    X = highscore_high;
    *PF1 = score_line8_PF1[X];
    X = highscore_low;
    *COLUPF = YELLOW;
    *PF2 = score_line8_PF2[X];
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line9_PF1[X];
    X = score_low;
    *PF2 = score_line9_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line9_PF1[X];
    X = highscore_low;
    *PF2 = score_line9_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line10_PF1[X];
    X = score_low;
    *PF2 = score_line10_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line10_PF1[X];
    X = highscore_low;
    *PF2 = score_line10_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line11_PF1[X];
    X = score_low;
    *PF2 = score_line11_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line11_PF1[X];
    X = highscore_low;
    *PF2 = score_line11_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line12_PF1[X];
    X = score_low;
    *PF2 = score_line12_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line12_PF1[X];
    X = highscore_low;
    *PF2 = score_line12_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line13_PF1[X];
    X = score_low;
    *PF2 = score_line13_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line13_PF1[X];
    X = highscore_low;
    *PF2 = score_line13_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line14_PF1[X];
    X = score_low;
    *PF2 = score_line14_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line14_PF1[X];
    X = highscore_low;
    *PF2 = score_line14_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line15_PF1[X];
    X = score_low;
    *PF2 = score_line15_PF2[X];
    asm("nop");
    X = highscore_high;
    *PF1 = score_line15_PF1[X];
    X = highscore_low;
    *PF2 = score_line15_PF2[X];
    *COLUPF = YELLOW;
    
    strobe(WSYNC);
    *CTRLPF = 0;
    *PF0 = 0;
    *PF1 = 0;
    *PF2 = 0;
}

void bottom()
{
    strobe(WSYNC);
    strobe(HMOVE);
#ifdef PAL
    display_score();
    for (X = PALBOTTOM - 20; X != 0; X--) strobe(WSYNC);
#endif

    *VBLANK = 2; // Enable VBLANK again
                 // Now we have 30 lines of VBLANK
                 //strobe(HMCLR);
    init_bird_sprite_pos(); // 4 lines
    for (X = OVERSCAN - 4; X != 0; X--) strobe(WSYNC);
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
        *TIM64T = 49; 
#else
        *TIM64T = 39;
#endif
        // The game logic
        game_logic();

        *ENAM0 = 0;
        *ENAM1 = 0;
        *COLUPF = BROWN;
        *GRP0 = 0;
        *GRP1 = 0;
        while (*INTIM);
    }
    strobe(WSYNC);
    strobe(HMOVE);
    Y = KERNAL - 1;
    i = (KERNAL - 1) / 16; 
    *COLUBK = YELLOW;
    *HMM0 = 0x00;
    *HMM1 = 0x00;
   
    if (bird_type == 0) {
        if (scroll_sequence < 12) {
            kernel11();
        } else if (scroll_sequence < 20) {
            kernel21();
        } else kernel31();
    } else {
        if (scroll_sequence < 12) {
            kernel12();
        } else if (scroll_sequence < 20) {
            kernel22();
        } else kernel32();
    }

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
            printf("const unsigned char s%d_PF%d[%d]={", j, d, WIDTH);
            for (c = 0; c < WIDTH - 1; c++) {
                printf("0x%02x,", PF[d][c]);
            }
            printf("0x%02x};\n", PF[d][c]);
        }
    }
    printf("const unsigned char right_shift4[192]={\n\t");
    for (i = 0; i < 192; i++) {
        printf("0x%02x", i >> 4);
        if (i != 191) printf(",");
        if (!((i + 1) % 16)) printf("\n\t");
    }
    printf("};\n");
    const int sprite_height = 18;
    const int rainbow_height = 16;
    const int blue_pal = 0xb2;
    const int blue_ntsc = 0x84;
    unsigned char rainbow[rainbow_height];

    printf("#ifdef PAL\n");
    for (j = 1; j >= 0; j--) {
        switch (j) {
            case 0: // NTSC
                for (i = 0; i < 8; i++) {
                    rainbow[i] = 0x76 + 0x10 * i;
                }
                for (i = 0; i < 8; i++) {
                    rainbow[i + 8] = 0x06 + 0x10 * i;
                }
                break;
            case 1: // PAL
                for (i = 0; i < 6; i++) {
                    rainbow[i] = 0xd6 - 0x20 * i;
                }
                rainbow[i + 6] = 0x38;
                rainbow[i + 7] = 0x3e;
                rainbow[i + 8] = 0x2e;
                rainbow[i + 9] = 0x26;
                for (i = 0; i < 6; i++) {
                    rainbow[i + 10] = 0x26 + 0x20 * i;
                }
                break;
        }
        printf("const bank1 unsigned char rainbow[%d]={\n\t", 2 * MAX_RAINBOW_OFFSET + (192 - sprite_height) + 2 * rainbow_height);
        for (i = 0; i < MAX_RAINBOW_OFFSET; i++) {
            printf("0x%02x, ", (j)?blue_pal:blue_ntsc);
            if (!((i + 1) % 16)) printf("\n\t");
        }
        printf("\n\t");
        for (i = 0; i< rainbow_height; i++) {
            printf("0x%02x, ", rainbow[i]);
        }
        printf("\n\t");
        for (i = 0; i < 192 - sprite_height; i++) {
            printf("0x%02x, ", (j)?blue_pal:blue_ntsc);
            if (!((i + 1) % 16)) printf("\n\t");
        }
        printf("\n\t");
        for (i = 0; i < rainbow_height; i++) {
            printf("0x%02x, ", rainbow[rainbow_height - 1 - i]);
        }
        printf("\n\t");
        for (i = 0; i < MAX_RAINBOW_OFFSET - 1; i++) {
            printf("0x%02x, ", (j)?blue_pal:blue_ntsc);
            if (!((i + 1) % 16)) printf("\n\t");
        }
        printf("0x%02x};\n", (j)?blue_pal:blue_ntsc);
        if (j) printf("#else\n");
    }
    printf("#endif\n");
}
#endif
