#define MAX_RAINBOW_OFFSET 16 

#ifdef __ATARI2600__
#include "vcs.h"
unsigned char X, Y;

#ifdef PLUSROM
const char *PLUSROM_API = "a\0h.firmaplus.de";
#endif

#ifdef PAL
const unsigned char RED = 0x64;
const unsigned char BLUE = 0xB2;
const unsigned char DBLUE = 0xB0;
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
const unsigned char DBLUE = 0x80;
const unsigned char LBLUE = 0x8e;
const unsigned char BLACK = 0x00;
const unsigned char WHITE = 0x0e;
const unsigned char YELLOW = 0x1e;
const unsigned char ORANGE = 0xfa;
const unsigned char GREY = 0x04;
const unsigned char LGREEN = 0xcc;
const unsigned char GREEN = 0xc6;
const unsigned char DGREEN = 0xc2;
const unsigned char BROWN = 0x10;
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

char state;
unsigned char i, j, k;
unsigned char scroll_sequence;
unsigned char scroll_counter;
unsigned char lPFx[12];
unsigned char rPFx[12];
unsigned char lPFy[12];
unsigned char rPFy[12];
char left_window, right_window;
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
unsigned char *grass1;
unsigned char *grass2;
unsigned char rainbow_offset;
unsigned char rainbow_sequence;
unsigned char difficulty;
unsigned char game_mode;
unsigned char random;
unsigned char do_display_score;
unsigned char counter;
unsigned char blinking_if_high_score;
unsigned char blinking_counter;
unsigned char sound_iterator;
unsigned char sound_counter;

// Sound code

#define GAMEOVER_SOUND  0
#define SCORE_SOUND     1
#define HIGHSCORE_SOUND 2
#define GREAT_SOUND     3

const unsigned char sound[4] = { 0x38, 0x48, 0x48, 0x48 };
const unsigned char sound_index[4] = { 0, 9, 13, 17 };
const unsigned char sound_pitch[26] = { 11, 12, 13, 14, 15, 16, 17, 18, 0, 25, 26, 23, 0, 13, 11, 12, 0, 19, 16, 15, 12, 0, 12, 0, 12, 0 };
const unsigned char sound_duration[26] = { 7, 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 14, 0, 7, 7, 10, 0, 7, 7, 7, 14, 7, 7, 7, 21, 0 };

// X is the sound to play
void play_sound()
{
    *AUDV1 = sound[X];
    *AUDC1 = sound[X] >> 4;
    Y = sound_index[X];
    sound_iterator = Y;
    sound_counter = sound_duration[Y];
    *AUDF1 = sound_pitch[Y];
}

void play_sound_iteration()
{
    if (sound_counter) {
        sound_counter--;
        if (sound_counter == 0) {
            sound_iterator++;
            Y = sound_iterator;
            sound_counter = sound_duration[Y];
            if (sound_counter) {
                *AUDF1 = sound_pitch[Y];
            } else {
                *AUDV1 = 0;
                *AUDC1 = 0;
                *AUDF1 = 0;
            }
        } 
    }
}

// Savekey code

#include "i2c.inc"

=== ASSEMBLER BEGIN ===
    I2C_SUBS i
==== ASSEMBLER END ====

void save_highscore()
{
    asm("jsr i2c_startwrite");
    asm("bcs .eeprom_error");
    load(0x01);
    asm("jsr i2c_txbyte");
    load(0x40);
    asm("jsr i2c_txbyte");
    load(highscore_high);
    asm("jsr i2c_txbyte");
    load(highscore_low);
    asm("jsr i2c_txbyte");
eeprom_error:
    asm("jsr i2c_stopwrite");
}

void load_highscore()
{
    asm("jsr i2c_startwrite	; Start signal and $a0 command byte");
    asm("bcs .eeprom_error	; exit if command byte not acknowledged");
    load(0x01);
    asm("jsr i2c_txbyte");
    load(0x40);
    asm("jsr i2c_txbyte");
    asm("jsr i2c_stopwrite");
    asm("jsr i2c_startread");
    asm("jsr i2c_rxbyte");
    store(highscore_high);
    asm("jsr i2c_rxbyte");
    store(highscore_low);
    if (highscore_high == 0xff || highscore_low == 0xff) {
        highscore_high = 0;
        highscore_low = 0;
    }
eeprom_error:
    asm("jsr i2c_stopread");
}

// Display kernel code

const bank1 unsigned char grass_style1[16] = {
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0xE0
};
const bank1 unsigned char grass_style2[16] = {
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x00, 
    0x00, 0x00, 0x10, 0x00,
    0x00, 0x10, 0x00, 0xD0
};
const bank1 unsigned char grass_style3[16] = {
    0x00, 0x10, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00,
    0x10, 0x00, 0x10, 0xB0
};

const bank2 unsigned char grass_style1_bank2[16] = {
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00,
    0x10, 0x00, 0x00, 0xE0
};
const bank2 unsigned char grass_style2_bank2[16] = {
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x10, 0x00, 
    0x00, 0x00, 0x10, 0x00,
    0x00, 0x10, 0x00, 0xD0
};
const bank2 unsigned char grass_style3_bank2[16] = {
    0x00, 0x10, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00, 
    0x00, 0x10, 0x00, 0x00,
    0x10, 0x00, 0x10, 0xB0
};

const bank1 aligned(128) unsigned char gameover0[13] = { 0x38, 0x79, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xc1, 0xc0, 0xf8, 0x78, 0x38};
const bank1 unsigned char gameover1[13] = { 0xed, 0xed, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xef, 0xef, 0x00, 0x00, 0x00};
const bank1 unsigned char gameover2[13] = { 0x44, 0x4c, 0x5c, 0x5c, 0x5f, 0x5b, 0x5b, 0x5b, 0xdb, 0x9f, 0x0e, 0x00, 0x00};
const bank1 unsigned char gameover3[13] = { 0x08, 0x1c, 0x3e, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3e, 0x1c, 0x08};
const bank1 unsigned char gameover4[13] = { 0xc0, 0xe1, 0xf3, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0x03, 0x01, 0x00, 0x00};
const bank1 unsigned char gameover5[13] = { 0x8c, 0x8c, 0x8c, 0x8c, 0xec, 0x6c, 0x6f, 0x6f, 0x67, 0xe3, 0xc0, 0x00, 0x00};

const bank1 unsigned char getready0[16] = { 0x00, 0x00, 0x00, 0x38, 0x79, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xc3, 0xc3, 0xf9, 0x78, 0x38 };
const bank1 unsigned char getready1[16] = { 0x00, 0x00, 0x00, 0x83, 0x87, 0x86, 0x86, 0xe6, 0x66, 0x66, 0x6f, 0x6f, 0xef, 0xc6, 0x06, 0x06 };
const bank1 unsigned char getready2[16] = { 0x00, 0x00, 0x00, 0x36, 0x36, 0x3c, 0x3c, 0x3c, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x3e, 0x3c };
const bank1 unsigned char getready3[16] = { 0x00, 0x00, 0x00, 0x20, 0x61, 0xe3, 0xe3, 0xfb, 0xdb, 0xdb, 0xdb, 0xd9, 0xf8, 0x70, 0x00, 0x00 };
const bank1 unsigned char getready4[16] = { 0x00, 0x00, 0x00, 0xe7, 0xef, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0xef, 0xe7, 0x03, 0x03, 0x03 };
const bank1 unsigned char getready5[16] = { 0x70, 0x78, 0x18, 0x1b, 0x3b, 0x78, 0x7b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x5b, 0x03, 0x03, 0x03 };

#define SPRITE_HEIGHT 16 
#define RAINBOW_SIZE 16

#define FIRST_TIME

#define BIRD1
#define BEFORE X = i;   
#define WAIT Y--; i = Y >> 4;  
#define BEFORE2 X = i; *COLUBK = j;  
#define WAIT2 Y--; i = right_shift4[Y];  

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
#define FIRST_TIME

#define bank1 bank2
#define right_shift4 right_shift4_bank2
#define rainbow rainbow_bank2
#define grass_style1 grass_style1_bank2
#define grass_style2 grass_style2_bank2
#define grass_style3 grass_style3_bank2
#undef WAIT2
#define WAIT2 Y--; i = right_shift4[Y];

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
#undef bank1
#undef rainbow
#undef grass_style1 
#undef grass_style2 
#undef grass_style3 

#undef kernel
#undef START
#undef START2
#undef LEFT_PLAYFIELD
#undef RIGHT_PLAYFIELD

// Init code

void set_rainbow_and_grass()
{
    i = (counter >> 2) & 3;
    if (bird_type) {
        background_ptr1 = rainbow_bank2;
        if (i == 0) {
            grass1 = grass_style1_bank2;
            grass2 = grass_style2_bank2;
        } else if (i == 1) {
            grass1 = grass_style2_bank2;
            grass2 = grass_style1_bank2;
        } else if (i == 2) {
            grass1 = grass_style3_bank2;
            grass2 = grass_style2_bank2;
        } else {
            grass1 = grass_style2_bank2;
            grass2 = grass_style3_bank2;
        }
    } else {
        background_ptr1 = rainbow;
        if (i == 0) {
            grass1 = grass_style1;
            grass2 = grass_style2;
        } else if (i == 1) {
            grass1 = grass_style2;
            grass2 = grass_style1;
        } else if (i == 2) {
            grass1 = grass_style3;
            grass2 = grass_style2;
        } else {
            grass1 = grass_style2;
            grass2 = grass_style3;
        }
    }
    background_ptr1 += MAX_RAINBOW_OFFSET + RAINBOW_SIZE - 1;
    background_ptr1 -= (ybird >> 8);
    background_ptr2 = background_ptr1;
    background_ptr2 += 193;
    background_ptr1 -= rainbow_offset;
    background_ptr2 += rainbow_offset;
}

void init_bird_sprite_pos()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUP1 = WHITE; 
    *NUSIZ0 = 0x10;
    *NUSIZ1 = 0x00;
    if (state == 0) {
        csleep(7);
        csleep(7);
    }
    strobe(RESP0);
    strobe(RESP1);
    strobe(RESBL);
    *HMP0 = 0x80; 
    *HMP1 = 0x00; 
    *HMBL = 0x70; 
    strobe(WSYNC);
    strobe(HMOVE);
    *CTRLPF = 0x20;
    csleep(7);
    *HMP0 = 0xF0;
    *HMBL = 0x10;
    strobe(WSYNC);
    strobe(HMOVE);
    csleep(7);
    *HMP0 = 0x00;
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

    if (left_window == -1) {
        for (X = 0; X != 12; X++) {
            lPFx[X] = 0;
            lPFy[X] = 0;
        }
    } else {
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
}
   
void init()
{
    init_bird_sprite_pos();
    state = 0;
    ybird = 70 * 256;
    yspeed = 0;
    score_low = 00;
    score_high = 00;
    left_window = 2;
    right_window = 2;
    difficulty = 8;
    button_pressed = 1;
    random = 0xaa;
    blinking_if_high_score = BROWN;
    load_highscore();
}

void rand()
{
    asm("lda random");
    asm("asl");
    asm("eor random");
    asm("asl");
    asm("eor random");
    asm("asl");
    asm("asl");
    asm("eor random");
    asm("asl");
    asm("rol random");
}

void next_sequence()
{
    rand();
    right_window = random & 0x07;
    /* Test for difficulty
    if (right_window == 7)
        right_window = 0;
    else 
        right_window = 7;
        */
}

void getready()
{
    *AUDV0 = 0; // Cut bird sound
    button_pressed = 1;
    ybird = 100 * 256;
    left_window = -1;
    next_sequence();
    scroll_sequence = 12;
    state = 1;
    score_low = 00;
    score_high = 00;
    load_scroll_sequence();
    blinking_if_high_score = BROWN;
    blinking_counter = 0;
    rainbow_offset = 0;
    rainbow_sequence = 0;
}

void gameover()
{
    *AUDV0 = 0; // Cut bird sound
    button_pressed = 1;
    state = 3;
    yspeed = 0;
    blinking_counter = 0;
#ifdef PAL 
    if (ybird >> 8 > KERNAL - 30) {
       ybird = (KERNAL - 60) * 256;
    } 
#else
    if (ybird >> 8 > KERNAL - 60) {
       ybird = (KERNAL - 60) * 256;
    }
#endif 
    X = GAMEOVER_SOUND;
    play_sound();
    if (score_low == highscore_low && score_high == highscore_high) {
        save_highscore();
    }
}

void increment_score()
{
    score_low++;
    if (rainbow_sequence == 1 && rainbow_offset < 16) {
        rainbow_offset++;
    }

    if (score_low == 100) {
        score_low = 0;
        score_high++;
        rainbow_sequence++;
        if (rainbow_sequence == 5) rainbow_sequence = 0;
        if (score_high == 100) score_high = 0;
    }
    if (score_high > highscore_high || (score_high == highscore_high && score_low > highscore_low)) {
        highscore_high = score_high;
        highscore_low = score_low;
        blinking_if_high_score = 0;
        blinking_counter = 0;
        X = HIGHSCORE_SOUND;
    } else {
        X = SCORE_SOUND;
    }
    if (!score_low) X = GREAT_SOUND;
    play_sound();
}

void scrolling()
{
    if (scroll_counter >= difficulty) {
        scroll_counter = 0;
        scroll_sequence++;
        if (scroll_sequence == 20) left_window = right_window;
        if (state == 2 && scroll_sequence == 12 && left_window != -1) {
            increment_score();
        }
        if (scroll_sequence == 24) {
            if (state == 2) next_sequence();
            scroll_sequence = 0;
        }
    }
    load_scroll_sequence();
    scroll_counter++;
    if (scroll_sequence & 3) *HMM1 = 0x10;
    *HMM0 = 0x10;
}

void flap()
{
    yspeed = 420;
    if (bird_type == 0) 
        bird_type = 1;
    else {
        bird_type = 0;
    }
    bird_animation_counter = 5;
    *AUDV0 = 8;
    *AUDC0 = 4;
    *AUDF0 = 4;
}

void game_logic()
{
    if ((*INPT4 & 0x80) != 0) {
        if (button_pressed == 0) {
            button_pressed = 1;
            flap();
        }
    } else button_pressed = 0;

    if ((*CXP0FB & 0x80) != 0) gameover();
    if ((*CXP1FB & 0x80) != 0) gameover();
    if ((*CXBLPF & 0x80) != 0) gameover();

    if (ybird >> 8 < 100) do_display_score = 1;
    else if (ybird >> 8 > 150) do_display_score = 0;
            
    if (rainbow_sequence == 2) {
        rainbow_offset++;
        if (rainbow_offset == MAX_RAINBOW_OFFSET + 16) rainbow_offset = 0;
    } else if (rainbow_sequence == 3) {
        if (counter & 16) {
            rainbow_offset = 16 + (counter & 0x0f);
        } else {
            rainbow_offset = 24 - (counter & 0x0f);
        }
    } else if (rainbow_sequence == 4) {
        rainbow_offset--;
        if (rainbow_offset <= 0) rainbow_offset = MAX_RAINBOW_OFFSET + 15;
    }
}

bank1 void display_gameover()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *GRP1 = 0;
    *GRP0 = 0;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *VDELP0 = 1;
    *VDELP1 = 1;
    *HMP1 = 0xA0; 
    *HMP0 = 0x90;
    strobe(RESP0);
    strobe(RESP1);
    strobe(WSYNC);
    strobe(HMOVE);
    csleep(7);
    *HMP1 = 0x0;
    *HMP0 = 0x0;
    if (i == 0) {
        for (Y = 12; Y >= 0; Y--) {
            strobe(WSYNC);
            strobe(HMOVE);
            i = Y;
            *GRP0 = gameover0[Y];
            *GRP1 = gameover1[Y];
            *GRP0 = gameover2[Y];
            X = gameover4[Y];
            j = gameover5[Y];
            load(gameover3[Y]);
            Y = j;
            store(*GRP1);
            *GRP0 = X;
            *GRP1 = Y;
            strobe(GRP0);
            Y = i;
        }
    } else {
        for (Y = 15; Y >= 0; Y--) {
            strobe(WSYNC);
            strobe(HMOVE);
            i = Y;
            *GRP0 = getready0[Y];
            *GRP1 = getready1[Y];
            *GRP0 = getready2[Y];
            X = getready4[Y];
            j = getready5[Y];
            load(getready3[Y]);
            Y = j;
            store(*GRP1);
            *GRP0 = X;
            *GRP1 = Y;
            strobe(GRP0);
            Y = i;
        }
    }
    strobe(WSYNC);
    strobe(HMOVE);
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
}

#ifdef FLAPPYBIRD
const aligned(128) unsigned char flappybird0[16] = { 0x00, 0x00, 0x00, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xc6, 0xf6, 0xf6, 0xf6, 0xc6, 0xf6, 0xf6, 0x76 };
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
    *VDELP0 = 1;
    *VDELP1 = 1;
    strobe(RESP0);
    strobe(RESP1);
    *HMP1 = 0x20;
    *HMP0 = 0x10;
    strobe(WSYNC);
    strobe(HMOVE);
    for (Y = 15; Y >= 0; Y--) {
        strobe(WSYNC);
        i = Y;
        *GRP0 = flappybird0[Y];
        *GRP1 = flappybird1[Y];
        *GRP0 = flappybird2[Y];
        X = flappybird4[Y];
        j = flappybird5[Y];
        load(flappybird3[y]);
        Y = j;
        store(*GRP1);
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
#else
const aligned(256) unsigned char happybird0[32] = { 0x1e, 0x21, 0x4c, 0x42, 0x42, 0x4c, 0x21, 0x1e, 0x00, 0x00, 0x00, 0xd8, 0xd9, 0xdb, 0xdb, 0xdb, 0xfb, 0xfb, 0xfb, 0xd9, 0xd8, 0xd8, 0xd8, 0xd8, 0x00, 0x86, 0x8a, 0xca, 0xaa, 0xa6, 0xa0, 0xc0 };
const unsigned char happybird1[32] = { 0x38, 0x05, 0x85, 0x99, 0xa1, 0xa1, 0x1d, 0x00, 0x0c, 0x0c, 0x0c, 0xef, 0xef, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xef, 0xef, 0x00, 0x00, 0x00, 0x00, 0x68, 0xa8, 0xa8, 0xa8, 0xaa, 0x0a, 0x08 };
const unsigned char happybird2[32] = { 0x99, 0x22, 0x3a, 0x2a, 0x92, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x3c, 0xbe, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xb6, 0xbe, 0x3c, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x20, 0x40, 0x80, 0x60, 0x00, 0x00 };
const unsigned char happybird3[32] = { 0xa8, 0xa8, 0x90, 0xa8, 0xa8, 0x00, 0x00, 0x00, 0x30, 0x38, 0x18, 0x3b, 0x7b, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char happybird4[32] = { 0xe6, 0x89, 0xc9, 0x69, 0x29, 0xc6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9b, 0xdb, 0x5b, 0x5b, 0xdb, 0x9b, 0xdb, 0x43, 0x59, 0x58, 0x40, 0xc0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const unsigned char happybird5[32] = { 0x76, 0x41, 0x61, 0x36, 0x11, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x1f, 0x1b, 0x1b, 0x1b, 0x1b, 0xdb, 0xdb, 0xdf, 0xcf, 0x03, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void display_happybird()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUBK = RED;
    *VDELP0 = 1;
    *VDELP1 = 1;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *COLUP0 = WHITE;
    *COLUP1 = WHITE;
    *HMP1 = 0xA0; 
    strobe(RESP0);
    strobe(RESP1);
    *HMP0 = 0x90;
    strobe(WSYNC);
    strobe(HMOVE);
    csleep(7);
    *HMP1 = 0x0;
    *HMP0 = 0x0;
    for (Y = 31; Y >= 0; Y--) {
        strobe(WSYNC);
        strobe(HMOVE);
        i = Y;
        *GRP0 = happybird0[Y];
        *GRP1 = happybird1[Y];
        *GRP0 = happybird2[Y];
        X = happybird4[Y];
        j = happybird5[Y];
        load(happybird3[Y]);
        Y = j;
        store(*GRP1);
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
#endif

const bank2 aligned(128) unsigned char normal0[7] = { 0x24, 0x25, 0x2d, 0x2d, 0x34, 0x34, 0x24 };
const bank2 unsigned char normal1[7] = { 0x92, 0x52, 0x52, 0x52, 0x8b, 0x00, 0x00 };
const bank2 unsigned char normal2[7] = { 0xa6, 0xaa, 0xaa, 0xaa, 0xc6, 0x00, 0x00 };
const bank2 unsigned char normal3[7] = { 0x8a, 0x8a, 0x8a, 0x8a, 0x8f, 0x80, 0x80 };
const bank2 unsigned char normal4[7] = { 0x91, 0xaa, 0xaa, 0xaa, 0x11, 0x00, 0x00 };
const bank2 unsigned char normal5[7] = { 0x98, 0xa0, 0xb8, 0xa8, 0x90, 0x80, 0x80 };

const bank2 unsigned char pro0[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const bank2 unsigned char pro1[7] = { 0x21, 0x21, 0x39, 0x25, 0x24, 0x24, 0x38 };
const bank2 unsigned char pro2[7] = { 0x10, 0x28, 0x28, 0x28, 0x90, 0x00, 0x00 };
const bank2 unsigned char pro3[7] = { 0xa9, 0xaa, 0xaa, 0xaa, 0xf1, 0x00, 0x00 };
const bank2 unsigned char pro4[7] = { 0x19, 0xaa, 0xab, 0xaa, 0x19, 0x08, 0x08 };
const bank2 unsigned char pro5[7] = { 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00 };

const bank2 unsigned char pause0[7] = { 0x01, 0x00, 0x07, 0x00, 0x1f, 0x00, 0x00 };
const bank2 unsigned char pause1[7] = { 0xfa, 0x02, 0xfb, 0x02, 0xfa, 0x02, 0x03 };
const bank2 unsigned char pause2[7] = { 0x0c, 0x15, 0x8d, 0x45, 0x59, 0x40, 0x80 };
const bank2 unsigned char pause3[7] = { 0xd8, 0x45, 0x49, 0x51, 0x4c, 0x00, 0x00 };
const bank2 unsigned char pause4[7] = { 0xdf, 0x00, 0xdf, 0x40, 0x9f, 0x00, 0x00 };
const bank2 unsigned char pause5[7] = { 0x80, 0x00, 0xe0, 0x00, 0xf8, 0x00, 0x00 };

bank2 void display_mode()
{
    strobe(WSYNC);
    strobe(HMOVE);
    *GRP0 = 0;
    *GRP1 = 0;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *VDELP0 = 1;
    *VDELP1 = 1;
    *HMP1 = 0xA0; 
    *HMP0 = 0x90;
    strobe(RESP0);
    strobe(RESP1);
    strobe(WSYNC);
    strobe(HMOVE);
    csleep(7);
    Y = 6;
    *HMP1 = 0x0;
    *HMP0 = 0x0;
    if (state == 4) {
        do {
            strobe(WSYNC);
            strobe(HMOVE);
            i = Y;
            *GRP0 = pause0[Y];
            *GRP1 = pause1[Y];
            *GRP0 = pause2[Y];
            X = pause4[Y];
            j = pause5[Y];
            load(pause3[Y]);
            Y = j;
            store(*GRP1);
            *GRP0 = X;
            *GRP1 = Y;
            strobe(GRP0);
            Y = i;
            Y--;
        } while (Y >= 0);
    } else if (game_mode == 0) {
        do {
            strobe(WSYNC);
            strobe(HMOVE);
            i = Y;
            *GRP0 = normal0[Y];
            *GRP1 = normal1[Y];
            *GRP0 = normal2[Y];
            X = normal4[Y];
            j = normal5[Y];
            load(normal3[Y]);
            Y = j;
            store(*GRP1);
            *GRP0 = X;
            *GRP1 = Y;
            strobe(GRP0);
            Y = i;
            Y--;
        } while (Y >= 0);
    } else {
        do {
            strobe(WSYNC);
            strobe(HMOVE);
            i = Y;
            *GRP0 = pro0[Y];
            *GRP1 = pro1[Y];
            *GRP0 = pro2[Y];
            X = pro4[Y];
            j = pro5[Y];
            load(pro3[Y]);
            Y = j;
            store(*GRP1);
            *GRP0 = X;
            *GRP1 = Y;
            strobe(GRP0);
            Y = i;
            Y--;
        } while (Y >= 0);
    }
    strobe(WSYNC);
    strobe(HMOVE);
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
}

#include "bird_score.c"

// 18 pixels high
bank3 void display_score()
{
    *PF0 = 0;
    *COLUP0 = WHITE;
    *COLUP1 = WHITE;
    strobe(WSYNC);
    strobe(HMOVE);
    strobe(RESP1);
    strobe(RESP0);
    if (score_high < 10) {
        strobe(RESP0);
        strobe(RESP1);
        *HMP1 = 0x80;
    } else {
#ifdef PAL
        *COLUP0 = GREEN;
        *COLUP1 = GREEN;
#else
        *COLUP0 = BLACK;
        *COLUP1 = BLACK;
#endif
    }
    strobe(WSYNC);
    strobe(HMOVE);
    csleep(7);
    *HMP0 = 0x00;
    *HMP1 = 0x00;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *GRP0 = 0x04;
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line1_PF1[X];
    X = score_low;
    *PF2 = score_line1_PF2[X];
    X = highscore_high;
    *PF1 = score_line1_PF1[X];
    X = highscore_low;
    *COLUPF = YELLOW;
    *PF2 = score_line1_PF2[X];
    *GRP0 = 0x0d;
    *GRP1 = 0xae;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line2_PF1[X];
    X = score_low;
    *PF2 = score_line2_PF2[X];
    X = highscore_high;
    *PF1 = score_line2_PF1[X];
    X = highscore_low;
    *PF2 = score_line2_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x1d;
    *GRP1 = 0xad;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line3_PF1[X];
    X = score_low;
    *PF2 = score_line3_PF2[X];
    X = highscore_high;
    *PF1 = score_line3_PF1[X];
    X = highscore_low;
    *PF2 = score_line3_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x0d;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line4_PF1[X];
    X = score_low;
    *PF2 = score_line4_PF2[X];
    X = highscore_high;
    *PF1 = score_line4_PF1[X];
    X = highscore_low;
    *PF2 = score_line4_PF2[X];
    *COLUPF = YELLOW;
    *GRP1 = 0xae;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line5_PF1[X];
    X = score_low;
    *PF2 = score_line5_PF2[X];
    X = highscore_high;
    *PF1 = score_line5_PF1[X];
    X = highscore_low;
    *PF2 = score_line5_PF2[X];
    *COLUPF = YELLOW;
    *GRP1 = 0xac;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line6_PF1[X];
    X = score_low;
    *PF2 = score_line6_PF2[X];
    X = highscore_high;
    *PF1 = score_line6_PF1[X];
    X = highscore_low;
    *PF2 = score_line6_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x0c;
    *GRP1 = 0xcc;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line7_PF1[X];
    X = score_low;
    *PF2 = score_line7_PF2[X];
    X = highscore_high;
    *PF1 = score_line7_PF1[X];
    X = highscore_low;
    *PF2 = score_line7_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x1e;
    *GRP1 = 0x00;
    *COLUP1 = YELLOW;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line8_PF1[X];
    X = score_low;
    *PF2 = score_line8_PF2[X];
    X = highscore_high;
    *PF1 = score_line8_PF1[X];
    X = highscore_low;
    strobe(RESP0);
    *PF2 = score_line8_PF2[X];
    *COLUPF = YELLOW;
    *HMP0 = 0x80;
    *COLUP0 = YELLOW;
    *GRP0 = 0;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line9_PF1[X];
    X = score_low;
    *PF2 = score_line9_PF2[X];
    X = highscore_high;
    *PF1 = score_line9_PF1[X];
    X = highscore_low;
    *PF2 = score_line9_PF2[X];
    strobe(RESP1);
    *COLUPF = YELLOW;
    *HMP1 = 0x50;
    *HMP0 = 0;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line10_PF1[X];
    X = score_low;
    *PF2 = score_line10_PF2[X];
    X = highscore_high;
    *PF1 = score_line10_PF1[X];
    X = highscore_low;
    *PF2 = score_line10_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x2A;
    *GRP1 = 0x65;
    *HMP1 = 0;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line11_PF1[X];
    X = score_low;
    *PF2 = score_line11_PF2[X];
    X = highscore_high;
    *PF1 = score_line11_PF1[X];
    X = highscore_low;
    *PF2 = score_line11_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x28;
    *GRP1 = 0x85;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line12_PF1[X];
    X = score_low;
    *PF2 = score_line12_PF2[X];
    X = highscore_high;
    *PF1 = score_line12_PF1[X];
    X = highscore_low;
    *PF2 = score_line12_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x3A;
    *GRP1 = 0xB7;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line13_PF1[X];
    X = score_low;
    *PF2 = score_line13_PF2[X];
    X = highscore_high;
    *PF1 = score_line13_PF1[X];
    X = highscore_low;
    *PF2 = score_line13_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0x2A;
    *GRP1 = 0x95;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line14_PF1[X];
    X = score_low;
    *PF2 = score_line14_PF2[X];
    X = highscore_high;
    *PF1 = score_line14_PF1[X];
    X = highscore_low;
    *PF2 = score_line14_PF2[X];
    *COLUPF = YELLOW;
    *GRP1 = 0x65;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *COLUPF = WHITE;
    X = score_high;
    *PF1 = score_line15_PF1[X];
    X = score_low;
    *PF2 = score_line15_PF2[X];
    X = highscore_high;
    *PF1 = score_line15_PF1[X];
    X = highscore_low;
    *PF2 = score_line15_PF2[X];
    *COLUPF = YELLOW;
    *GRP0 = 0;
    *GRP1 = 0;
    
    strobe(WSYNC);
    strobe(HMOVE);
    *PF1 = 0;
    *PF2 = 0;
    *COLUPF = BROWN;
}

void bottom()
{
    strobe(WSYNC);
    strobe(HMOVE);
#ifdef PAL
    display_score();
    for (X = PALBOTTOM - 20; X >= 0; X--) {
        strobe(WSYNC);
        strobe(HMOVE);
    }
#endif

    strobe(WSYNC);
    strobe(HMOVE);
    *VBLANK = 2; // Enable VBLANK again
                 // Now we have 30 lines of VBLANK
    *TIM64T = (OVERSCAN * 76 + 13) / 64;
    init_bird_sprite_pos(); // 4 lines
    play_sound_iteration();
    while (*INTIM);
}

void main()
{
  init();

  do {
    *VBLANK = 2; // Enable VBLANK
    *VSYNC = 2; // Set VSYNC
    strobe(WSYNC); // Hold it for 3 scanlines
    strobe(WSYNC);
    strobe(WSYNC);
    *VSYNC = 0; // Turn VSYNC Off

#ifdef PAL
    *TIM64T = 50; 
#else
    *TIM64T = 39;
#endif
    
    // The game logic
    if (state == 0 || state == 2) {
        scrolling();
        yspeed -= 15;
        if (yspeed >> 8 < 0) bird_type = 0;
        if (bird_animation_counter != 0) {
            *AUDF0 = bird_animation_counter;
            bird_animation_counter--;
            if (bird_animation_counter == 0) {
                *AUDV0 = 0;
                bird_type = 1;
            }
        }
        ybird = ybird + yspeed;
        if (ybird >> 8 < 20) {
            ybird = 20 * 256;
            if (state == 2) gameover();
        }
        if (ybird >> 8 > 186) {
            ybird = 186 * 256;
        }
    } else if (state == 3) {
        yspeed -= 10;
        ybird = ybird + yspeed;
        if (ybird >> 8 < 20) {
            ybird = 20 * 256;
            yspeed = 0;
        }
    }
   
    if (state == 0 || state == 1 || state == 3) {
        if (*SWCHB & 0x40) {
            game_mode = 1;
            difficulty = 4;
        } else {
            game_mode = 0;
            difficulty = 8;
        }
        // Select to reset high score
        if (!(*SWCHB & 0x02)) {
            blinking_counter++;
            if (blinking_counter == 250) {
                highscore_low = 0;
                highscore_high = 0;
            }
        }
    }

    if (state == 0) {
        if (ybird >> 8 < 70) {
            flap();
        } 
        if ((*INPT4 & 0x80) != 0) {
            if (button_pressed == 0) {
                getready();
            }
        } else button_pressed = 0;
    } else if (state == 1) {
        *COLUBK = RED;
        *COLUP0 = WHITE;
        *COLUP1 = WHITE;
        if (bird_type == 0) bird_type = 1;
        else bird_type = 0;
        if ((*INPT4 & 0x80) != 0) {
            if (button_pressed == 0) {
                button_pressed = 1;
                yspeed = 0;
                do_display_score = 1;
                state = 2;
            }
        } else button_pressed = 0;
    } else if (state == 2) {
        *COLUBK = DBLUE;
        game_logic();
        if (!(*SWCHB & 0x08)) state = 4;
    } else if (state == 3) {
        *COLUBK = BLACK;
        *COLUP0 = WHITE;
        *COLUP1 = WHITE;
        if (bird_type == 0) bird_type = 1;
        else bird_type = 0;
        if ((*INPT4 & 0x80) != 0) {
            if (button_pressed == 0) {
                getready();
            }
        } else button_pressed = 0;
    } else if (state == 4) {
        if (bird_type == 0) bird_type = 1;
        else bird_type = 0;
        if (*SWCHB & 0x08) state = 2;
    }

    if (blinking_if_high_score != BROWN) {
#ifdef PAL
        if (blinking_counter & 16) {
            blinking_if_high_score = 0x20 + (blinking_counter & 0x0f);
        } else {
            blinking_if_high_score = 0x2f - (blinking_counter & 0x0f);
        }
#else
        if (blinking_counter & 16) {
            blinking_if_high_score = 0x10 + (blinking_counter & 0x0f);
        } else {
            blinking_if_high_score = 0x1f - (blinking_counter & 0x0f);
        }
#endif
        blinking_counter++;
    }

    *ENAM0 = 0;
    *ENAM1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
    *PF0 = 0;
    *PF1 = 0;
    *PF2 = 0;
    *CXCLR = 0;
    strobe(WSYNC);
    strobe(HMOVE);
    *HMM0 = 0;
    *HMM1 = 0;
    if (!(*SWCHB & 0x01)) init(); 
        
    set_rainbow_and_grass();
    counter++; 
    while (*INTIM);

    strobe(WSYNC);
    strobe(HMOVE);
    *VBLANK = 0;
    *COLUPF = blinking_if_high_score;

    if (state == 2) {
#ifdef PAL
        Y = KERNAL;
        i = (KERNAL -1) / 16;
        strobe(WSYNC);
        strobe(HMOVE);
#else
        if (do_display_score) {
            i = (KERNAL - 21) / 16;
            display_score();
            Y = KERNAL - 21;
            *COLUPF = blinking_if_high_score;
            init_bird_sprite_pos(); // 4 lines
        } else {
            Y = KERNAL;
            i = (KERNAL -1) / 16;
            strobe(WSYNC);
            strobe(HMOVE);
        }
#endif
    } else if (state == 0) {
#ifdef FLAPPYBIRD
        display_flappybird();
        i = (KERNAL - 22) / 16;
        init_bird_sprite_pos(); // 4 lines
        Y = KERNAL - 22;
#else
        display_happybird();
        strobe(WSYNC);
        strobe(HMOVE);
        *COLUBK = BLUE;
        display_mode();
        init_bird_sprite_pos(); // 4 lines
        i = (KERNAL - 49) / 16;
        Y = KERNAL - 49;
#endif
    } else if (state == 1) {
        i = 1;
        display_gameover();
        strobe(WSYNC);
        strobe(HMOVE);
        *COLUBK = BLUE;
        display_mode();
        i = (KERNAL - 33) / 16;
        init_bird_sprite_pos(); // 4 lines
        Y = KERNAL - 33;
    } else if (state == 4) {
        strobe(WSYNC);
        strobe(HMOVE);
        if (ybird >> 8 < 170) {
            *COLUP0 = WHITE;
            *COLUP1 = WHITE;
            display_mode();
            i = (KERNAL - 14) / 16;
            init_bird_sprite_pos(); // 4 lines
            Y = KERNAL - 14;
        } else {
            Y = KERNAL;
            i = (KERNAL - 1) / 16;
        }
    } else {
#ifdef PAL
        i = 0;
        display_gameover();
        i = (KERNAL - 19) / 16;
        init_bird_sprite_pos(); // 4 lines
        Y = KERNAL - 19;
#else
        display_score();
        i = 0;
        *COLUP0 = WHITE;
        *COLUP1 = WHITE;
        display_gameover();
        i = (KERNAL - 38) / 16;
        init_bird_sprite_pos(); // 4 lines
        Y = KERNAL - 38;
#endif
    }

    *COLUBK = YELLOW;
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
