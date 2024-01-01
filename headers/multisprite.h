#include "vcs.h"

#ifndef kernel_macro
#define kernel_macro
#endif
    
char ms_next_mode;
char ms_v, ms_colup0, ms_colup1;
char *ms_colup0ptr, *ms_colup1ptr, *ms_grp0ptr, *ms_grp1ptr, *ms_scenery;
char ms_height0, ms_height1;
char ms_slice_size;

void kernel()
{
kernel_start:
    if (ms_next_mode & 1) {
        if (ms_next_mode & 2) {
            // 2 sprites display
            do {
                ms_v = ms_scenery[Y];         // 9
                ms_colup0 = ms_colup0ptr[Y];  // 9
                ms_colup1 = ms_colup1ptr[Y];  // 9
                *GRP0 = ms_grp0ptr[Y];     // 9
                load(ms_grp1ptr[Y]);   // 6
                strobe(WSYNC);      // 3 ; Total (1) = 29 + 36 + 9 = 75
                store(*GRP1);        // 3
                *COLUP0 = ms_colup0;   // 6 
                *COLUP1 = ms_colup1;   // 6
                kernel_macro; // Max 15 cycles
                Y++;                // 2
                X = ms_scenery[Y];     // 8
                ms_colup0 = ms_colup0ptr[Y];  // 9
                ms_colup1 = ms_colup1ptr[Y];  // 9
                *GRP0 = ms_grp0ptr[Y];     // 9
                load(ms_grp1ptr[Y]);   // 6
                strobe(WSYNC);      // 3: Total (2) = 61 
                store(*GRP1);        // 3
                *COLUP0 = ms_colup0;   // 6
                *COLUP1 = ms_colup1;   // 6
                VSYNC[X] = ms_v;       // 7
                Y++;                // 2
            } while (Y != ms_slice_size); // 6
            goto kernel_start;
            // We are 32 cycles inside this line
        } else {
            // No sprite display kernel
            // Prepare for next pair of sprites
            do {
                ms_v = ms_scenery[Y];         // 9
                strobe(WSYNC);
                kernel_macro; // Max 15 cycles
                Y++;                // 2
                X = ms_scenery[Y];     // 8
                strobe(WSYNC);
                VSYNC[X] = ms_v;       // 7
                Y++;                // 2
            } while (Y != ms_slice_size); // 6
            goto kernel_start;
        }
    } else {
        if (ms_next_mode & 2) {
            // Sprite 0 mode only
            // Display sprite 0 while preparing for next sprite 1
            // Turn off VDELP to save time
            // We are 32 + 8 + 7 = 47 cycles inside this line
            ms_v = ms_scenery[Y];         // 9
            strobe(WSYNC);
            *GRP0 = ms_grp0ptr[Y];        // 9
            *COLUP0 = ms_colup0ptr[Y];    // 9 
            Y++;                // 2
            X = ms_scenery[Y];     // 8
            strobe(WSYNC);      
            *GRP0 = ms_grp0ptr[Y];        // 9
            *COLUP0 = ms_colup0ptr[Y];    // 9 
            VSYNC[X] = ms_v;       // 7
            Y++;                // 2
            do {
                ms_v = ms_scenery[Y];         // 9
                strobe(WSYNC);
                *GRP0 = ms_grp0ptr[Y];        // 9
                *COLUP0 = ms_colup0ptr[Y];    // 9 
                kernel_macro; // Max 15 cycles
                Y++;                // 2
                X = ms_scenery[Y];     // 8
                strobe(WSYNC);
                *GRP0 = ms_grp0ptr[Y];        // 9
                *COLUP0 = ms_colup0ptr[Y];    // 9 
                VSYNC[X] = ms_v;       // 7
                Y++;                // 2
            } while (Y != ms_slice_size); // 3
            goto kernel_start;
        } else {
            // Sprite 1 mode only
            // Display sprite 1 while preparing for next sprite 1
            // We are 32 + 8 + 8 = 48 cycles inside this line
            do {
                ms_v = ms_scenery[Y];         // 9
                strobe(WSYNC);      // 3 
                *GRP1 = ms_grp1ptr[Y];        // 9
                *COLUP1 = ms_colup1ptr[Y];    // 9 
                kernel_macro; // Max 15 cycles
                Y++;                // 2
                X = ms_scenery[Y];     // 8
                strobe(WSYNC);      // 3 
                *GRP1 = ms_grp1ptr[Y];        // 9
                *COLUP1 = ms_colup1ptr[Y];    // 9 
                VSYNC[X] = ms_v;       // 7
                Y++;                // 2
            } while (Y != ms_slice_size); // 3
            goto kernel_start;
        }
    }
}

