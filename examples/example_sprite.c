#include "vcs.h"
#include "vcs_colors.h"

unsigned char X, Y;

#define BLANK 40
#define KERNAL 192
#define OVERSCAN 30

char ypos;
unsigned char *sprite_ptr; // Pointer to the sprite data to be drawn
unsigned char *mask_ptr;   // Pointer to the mask for drawing
unsigned char *color_ptr;  // Pointer to the color table of the sprite

// Generated for sprite size 20 with maskgen.c (in cc2600/misc directory)
const char sprite_mask[364] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Drawn with PlayerPal and converted to C code by spritegen.c (in cc2600/misc directory)
const unsigned char sprite0[20] = { 0x66, 0x66, 0x76, 0x3a, 0xbc, 0x84, 0x36, 0x76, 0x6e, 0x5e, 0x3e, 0x48, 0xb6, 0xcb, 0x33, 0x55, 0x33, 0x4a, 0x36, 0x12};
const unsigned char colors0[20] = { 0x12, 0x12, 0x2c, 0x2c, 0x38, 0x38, 0x3c, 0x3c, 0x2c, 0x38, 0x3c, 0x3c, 0x3c, 0x3c, 0x0e, 0x0e, 0x0e, 0x3c, 0x3c, 0x3c};

void init()
{
    *COLUBK = VCS_GREEN;
    *COLUPF = VCS_LGREEN;
    ypos = 100;
    // Position sprite horizontally
    strobe(WSYNC);
    X = 6;
    do { X--; } while (X >= 0);
    strobe(RESP0);
    strobe(WSYNC);
}

void main()
{
    init();

    while(1) {
        *VBLANK = 2; // Disable VBLANK
        *VSYNC = 2; // Set VSYNC
        strobe(WSYNC); // Hold it for 3 scanlines
        strobe(WSYNC);
        strobe(WSYNC);
        *VSYNC = 0; // Turn VSYNC Off
        
        // Blank
        *TIM64T = ((BLANK - 3) * 76) / 64 + 1;
        // Do some logic here

        // Set up sprite pointer
        sprite_ptr = sprite0 - 1 - ypos; // -1 offset because lower position (ypos = 0) matches sprite_ptr[Y = 1] (line 96)
        mask_ptr = sprite_mask + KERNAL - sizeof(sprite0) - 1 - ypos; // Same offset as speite_ptr
        // Set up color pointer 
        color_ptr = colors0 - ypos; // 0 offset baecause lower position (ypos = 0) matches color_ptr[Y = 0] (lint 103)
       
        // Joystick input 
        if (!(*SWCHA & 0x80)) { *HMP0 = 0xF0; *REFP0 = 0; } // Right
        if (!(*SWCHA & 0x40)) { *HMP0 = 0x10; *REFP0 = 8; } // Left
        if (!(*SWCHA & 0x20) && ypos > 0) ypos--; // Down
        if (!(*SWCHA & 0x10) && ypos < 192 - sizeof(sprite0)) ypos++; // Up
       
        // Apply movement 
        strobe(WSYNC);
        strobe(HMOVE);
        
        // And stop any movement
        strobe(WSYNC);
        strobe(HMCLR);

        Y = KERNAL; // Initialize line counter
        X = sprite_ptr[Y] & mask_ptr[Y]; // Preload sprite data for the first line
        Y--;
        // Load TIA registers for first line
        *GRP0 = X;
        *COLUP0 = color_ptr[Y];
        *PF2 = Y; // Marker. Just to check that what is displayed is correct

        // Do some extra logic
        while (*INTIM);
        strobe(WSYNC);
        *VBLANK = 0;
        load(sprite_ptr[Y] & mask_ptr[Y]);
        Y--;

        // Image drawing
        do {
            strobe(WSYNC);
            store(*GRP0); // Apply preloaded sprite data
            *COLUP0 = color_ptr[Y]; // Load current line sprite color
            *PF2 = Y; // Marker. Just to check that what is displayed is correct
            load(sprite_ptr[Y] & mask_ptr[Y]); // Load next line sprite data
            Y--;
        } while (Y);
        
        // Last line is out of loop and is simpler
        strobe(WSYNC);
        store(*GRP0); // Apply preloaded sprite data
        *COLUP0 = color_ptr[Y]; // Load current line sprite color
        *PF2 = Y; // Marker. Just to check that what is displayed is correct
        
        strobe(WSYNC);
        // Overscan
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = (OVERSCAN * 76) / 64 + 2;
        // Do some logic here
        while (*INTIM);
    }
}
