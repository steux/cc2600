#include "vcs.h"

#ifndef kernel_macro
#define kernel_macro
#endif
    
char ms_next_mode;
char ms_v, ms_colup0, ms_colup1;
char *ms_colup0ptr, *ms_colup1ptr, *ms_grp0ptr, *ms_grp1ptr, *ms_scenery;
char ms_height0, ms_height1;
char ms_next_slice;
char ms_x;

const char sprite_wait[153] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
const char sprite_hm[153] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0};

void kernel()
{
kernel_start:
    if (ms_next_mode & 1) {
        if (ms_next_mode & 2) {
            *VDELP0 = 1;                        // 5
            // 2 sprites display
            do {
                ms_v = ms_scenery[Y];           // 9
                ms_colup0 = ms_colup0ptr[Y];    // 9
                ms_colup1 = ms_colup1ptr[Y];    // 9
                *GRP0 = ms_grp0ptr[Y];          // 9
                load(ms_grp1ptr[Y]);            // 6
                strobe(WSYNC);                  // 3 ; Total (1) = 29 + 36 + 9 = 75
                
                store(*GRP1);                   // 3
                *COLUP0 = ms_colup0;            // 6 
                *COLUP1 = ms_colup1;            // 6
                kernel_macro;                   // Max 15 cycles
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                ms_colup0 = ms_colup0ptr[Y];    // 9
                ms_colup1 = ms_colup1ptr[Y];    // 9
                *GRP0 = ms_grp0ptr[Y];          // 9
                load(ms_grp1ptr[Y]);            // 6
                strobe(WSYNC);                  // 3: Total (2) = 61 
                
                store(*GRP1);                   // 3
                *COLUP0 = ms_colup0;            // 6
                *COLUP1 = ms_colup1;            // 6
                VSYNC[X] = ms_v;                // 7
                Y++;                            // 2
            } while (Y != ms_next_slice);       // 6
            *VDELP0 = 0;                        // 5
            goto kernel_start;                  // 5
            // We are 37 cycles inside this line
        } else {
            // No sprite display kernel
            // Prepare for next pair of sprites
            do {
                ms_v = ms_scenery[Y];           // 9
                strobe(WSYNC);
                kernel_macro;                   // Max 15 cycles
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                strobe(WSYNC);
                VSYNC[X] = ms_v;                // 7
                Y++;                            // 2
            } while (Y != ms_next_slice);       // 6
            goto kernel_start;                  // 3
        }
    } else {
        if (ms_next_mode & 2) {
            // Sprite 0 mode only
            // Display sprite 0 while preparing for next sprite 1
            // We are 37 + 8 + 7 = 52 cycles inside this line
            *GRP1 = 0;                      // 5
            ms_v = ms_scenery[Y];           // 9
            strobe(WSYNC);                  // 3. Total (1) = 52 + 9 + 8 = 69           

            *GRP0 = ms_grp0ptr[Y];          // 9
            *COLUP0 = ms_colup0ptr[Y];      // 9 
            Y++;                            // 2
            // TODO: retrieve ms_x for next sprite
            X = ms_scenery[Y];              // 8
            *COLUP0 = ms_colup0ptr[Y];      // 9 // This color change is anticipateed. Color artifact when sprite 0 is on the right of the screen
            load(ms_grp0ptr[Y]);            // 6
            strobe(WSYNC);                  // 3. Total (2) = 46
            
            store(*GRP0);                   // 3
            VSYNC[X] = ms_v;                // 7
            // Player 1 repositionning
            X = ms_x;                       // 3
            *HMP1 = sprite_hm[X];           // 7
            X = sprite_wait[X];             // 6
            do { X--; } while (X);          // 5 / cycle. 4 for the last one. 
            strobe(RESP0);                  // 3. Minimum = 33 cycles
            Y++;                            // 2
            strobe(WSYNC);                  // 3. Total (3) = Unknown
            
            *GRP0 = ms_grp0ptr[Y];          // 9
            *COLUP0 = ms_colup0ptr[Y];      // 9 
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y];              // 8
            ms_colup0 = ms_colup0ptr[Y];    // 9 
            load(ms_grp0ptr[Y]);            // 6
            strobe(WSYNC);                  // 3. Total (4) = 57 cycles

            store(*GRP0);                   // 3
            *COLUP0 = ms_colup0;            // 6 
            VSYNC[X] = ms_v;                // 7
            Y++;                            // 2
            // Wait for 73 - 18 cycles exactly
            strobe(HMOVE);                  // 3. Early HMOVE at cycle 73
            strobe(WSYNC);                  // 3. Total (5) = 76
            
            *GRP0 = ms_grp0ptr[Y];          // 9
            *COLUP0 = ms_colup0ptr[Y];      // 9 
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y];              // 8
            strobe(WSYNC);                  // 3. Total (6)
            
            *GRP0 = ms_grp0ptr[Y];          // 9
            *COLUP0 = ms_colup0ptr[Y];      // 9 
            VSYNC[X] = ms_v;                // 7
            Y++;                            // 2
            
            do {
                ms_v = ms_scenery[Y];       // 9
                strobe(WSYNC);              // 3
                
                *GRP0 = ms_grp0ptr[Y];      // 9
                *COLUP0 = ms_colup0ptr[Y];  // 9 
                kernel_macro; // Max 15 cycles
                Y++;                        // 2
                X = ms_scenery[Y];          // 8
                strobe(WSYNC);              // 3
                
                *GRP0 = ms_grp0ptr[Y];      // 9
                *COLUP0 = ms_colup0ptr[Y];  // 9 
                VSYNC[X] = ms_v;            // 7
                Y++;                        // 2
            } while (Y != ms_next_slice);   // 6
            goto kernel_start;              // 3
        } else {
            // Sprite 1 mode only
            // Display sprite 1 while preparing for next sprite 1
            // We are 32 + 8 + 8 = 48 cycles inside this line
            do {
                ms_v = ms_scenery[Y];           // 9
                strobe(WSYNC);                  // 3 
                
                *GRP1 = ms_grp1ptr[Y];          // 9
                *COLUP1 = ms_colup1ptr[Y];      // 9 
                kernel_macro;                   // Max 15 cycles
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                strobe(WSYNC);                  // 3 
                
                *GRP1 = ms_grp1ptr[Y];          // 9
                *COLUP1 = ms_colup1ptr[Y];      // 9 
                VSYNC[X] = ms_v;                // 7
                Y++;                            // 2
            } while (Y != ms_next_slice);       // 3
            goto kernel_start;                  // 3
        }
    }
}

