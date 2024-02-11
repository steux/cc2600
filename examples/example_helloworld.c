#include "vcs.h"
#include "vcs_colors.h"

#include "minikernel.h"

unsigned char X, Y;

#define BLANK 40
#define KERNAL 192
#define OVERSCAN 30

// Helloworld 48 pixels wide generated by tools2600/text2rom2600
aligned(256) const char line0[42] = {
	0x53, 0x54, 0x57, 0x75, 0x52, 0x50, 0x50, 
	0x77, 0x22, 0x22, 0x22, 0x22, 0x22, 0x66, 
	0x20, 0x50, 0x50, 0x50, 0x20, 0x00, 0x00, 
	0x52, 0x75, 0x75, 0x75, 0x52, 0x50, 0x50, 
	0x47, 0x42, 0x42, 0x52, 0x62, 0x02, 0x06, 
	0x32, 0x50, 0x52, 0x52, 0x32, 0x12, 0x12};

void main()
{
    char i;
    mini_kernel_position_sprites_center();
    
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

        Y = KERNAL; // Initialize line counter

        // Do some extra logic
        
        while (*INTIM);
        
        // Image drawing
        strobe(WSYNC);
        *VBLANK = 0;
        Y--;
        do {
            strobe(WSYNC);
            Y--;
        } while (Y >= 110);

        i = Y;
        *COLUP0 = VCS_BLUE;
        *COLUP1 = VCS_BLUE;
        mini_kernel_display_text(line0, 7);
        Y = i - 11;

        i = Y;
        *COLUP0 = VCS_WHITE;
        *COLUP1 = VCS_WHITE;
        mini_kernel_display_text(line0, 7);
        Y = i - 11;

        i = Y;
        *COLUP0 = VCS_RED;
        *COLUP1 = VCS_RED;
        mini_kernel_display_text(line0, 7);
        Y = i - 11;

        do {
            strobe(WSYNC);
            Y--;
        } while (Y);

        // Last line is out of loop and is generally simpler
        strobe(WSYNC);
        
        strobe(WSYNC);
        // Overscan
        *VBLANK = 2; // Enable VBLANK
        *TIM64T = (OVERSCAN * 76) / 64;
        // Do some logic here
        while (*INTIM);
    }
}
