#include "vcs.h"

#ifndef kernel_macro
#define kernel_macro
#endif
    
#ifndef kernel_long_macro
#define kernel_long_macro strobe(WSYNC)
#endif
   
#ifndef MS_MAX_NB_SPRITES
#define MS_MAX_NB_SPRITES 10
#endif

#ifndef MS_PLAYFIELD_HEIGHT
#define MS_PLAYFIELD_HEIGHT 192
#endif

char ms_v, ms_colup0, ms_colup1;
char *ms_colup0ptr, *ms_colup1ptr, *ms_grp0ptr, *ms_grp1ptr, *ms_scenery;
char ms_next_void_slice, ms_next_p0_slice, ms_next_p1_slice, ms_next_p0_p1_slice;
char ms_x;
char ms_sprite_iter;
char ms_sprite_x[MS_MAX_NB_SPRITES];
char ms_sprite_y[MS_MAX_NB_SPRITES];
char ms_sprite_id[MS_MAX_NB_SPRITES];
char ms_nusiz[MS_MAX_NB_SPRITES];
char ms_sorted_by_y[MS_MAX_NB_SPRITES];
char ms_tmp;
char ms_id_p[2];
char ms_nb_sprites;

const char ms_sprite_wait[153] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
const char ms_sprite_hm[153] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0};

void ms_select_sprites()
{
    Y = 0;
    ms_sorted_by_y[Y] &= 0x7f; // Display this one (a priori)
    char candidate1 = ms_sorted_by_y[Y];
    for (Y = 1; Y != ms_nb_sprites; Y++) {
        char candidate2 = ms_sorted_by_y[Y];
        if (candidate2 & 0x80) { // If it was not displayed at previous iteration
            // Let's see if this candidate overlaps with our previous candidate
            char y2 = ms_sprite_y[X = candidate2 & 0x7f];
            char y1 = ms_sprite_y[X = candidate1 & 0x7f];
            char height1 = ms_height[X = ms_sprite_id[X]];
            if (y1 + height1 + 8 >= y2) {
                // Yes. It overlaps. Skip candidate1 and set it as prioritary for next time
                ms_sorted_by_y[--Y] |= 0x80;
                Y++;
            } 
            ms_sorted_by_y[Y] &= 0x7f;
        }
        candidate1 = candidate2;
    }
}

inline char ms_allocate_sprite()
{
    X = ms_sprite_iter;
    if (X != ms_nb_sprites) {
        ms_tmp = ms_sorted_by_y[X];
        if (ms_tmp & 0x80) { // was removed
            X++;
            if (X == ms_nb_sprites) return -1;
            ms_tmp = ms_sorted_by_y[X];
        }
        X++;
        ms_sprite_iter = X;
        return ms_tmp;
    }
    return -1;
}

inline char ms_mark_as_removed()
{
    X = ms_sprite_iter;
    ms_sorted_by_y[--X] |= 0x80;
}

void kernel_repo0()
{
    // Entering at [46/76]
    ms_tmp = ms_scenery[Y];             // 9 [55/76] (variable)
    X = ms_id_p[0];                     // 3
    *NUSIZ0 = ms_nusiz[X];              // 7 [65/76]
    strobe(WSYNC);                      // 3

    X = ms_sprite_x[X];                 // 6
    *HMP0 = ms_sprite_hm[X];            // 7
    X = ms_sprite_wait[X];              // 6 [19/76]
    do { X--; } while (X);              // 5 / cycle. 4 for the last one. 
    strobe(RESP1);                      // 3. Minimum = 26 cycles
    strobe(WSYNC);                      // 3
   
    X = ms_tmp;                         // 3
    VSYNC[X] = ms_v;                    // 7 [10]
    X = ms_id_p[0];                     // 3 
    X = ms_sprite_id[X];                // 6  
    ms_grp0ptr = ms_grptr[X] - ms_tmp;  // 21 // TODO: Compute ms_tmp
    ms_colup0ptr = ms_coluptr[X] - ms_tmp; // 21
    strobe(HMOVE);                      // Early hmove
    //strobe(WSYNC);                    // 3

    ms_v = ms_scenery[Y++];             // 11
    // Must leave at < [53/76]
} // RTS: 6

void kernel()
{
    ms_select_sprites();
    
    // Phase 1: before the kernel actually starts, allocates and positions sprites p0 and p1.
    ms_sprite_iter = 0;
    *GRP0 = 0;
    *VDELP0 = 0;
    X = ms_allocate_sprite(); // 47
    // Position sprite 0
    if (X != -1) {
        ms_id_p[0] = X;
        Y = ms_sprite_id[X];
        ms_tmp = ms_sprite_y[X];
        ms_grp0ptr = ms_grptr[Y] - ms_tmp;   // 21
        ms_colup0ptr = ms_coluptr[Y] - ms_tmp; // 21
        strobe(WSYNC);                  // 3
        
        X = ms_sprite_x[X];             // 6
        X = ms_sprite_wait[X];          // 6
        *HMP0 = ms_sprite_hm[X];        // 7
        do { X--; } while (X);          // 5 / cycle. 4 for the last one. 
        strobe(RESP0);                  // 3. Minimum = 26 cycles
        strobe(WSYNC);                  // 3
        strobe(HMOVE);
    }
    *GRP1 = 0;
    X = ms_allocate_sprite(); // 47
    // Position sprite 1
    if (X != -1) {
        ms_id_p[1] = X;
        Y = ms_sprite_id[X];
        ms_tmp = ms_sprite_y[X];
        ms_grp1ptr = ms_grptr[Y] - ms_tmp;   // 21
        ms_colup1ptr = ms_coluptr[Y] - ms_tmp; // 21
        strobe(WSYNC);                  // 3
        
        X = ms_sprite_x[X];             // 6
        X = ms_sprite_wait[X];          // 6
        *HMP0 = ms_sprite_hm[X];        // 7
        do { X--; } while (X);          // 5 / cycle. 4 for the last one. 
        strobe(RESP1);                  // 3. Minimum = 26 cycles
        strobe(WSYNC);                  // 3
        strobe(HMOVE);
    }

    // We are ready for the actual kernel
    ms_tmp = MS_PLAYFIELD_HEIGHT + 2;
    ms_next_void_slice = MS_PLAYFIELD_HEIGHT + 2;
    ms_next_p0_slice = MS_PLAYFIELD_HEIGHT + 2;
    ms_next_p1_slice = MS_PLAYFIELD_HEIGHT + 2;
    ms_next_p0_p1_slice = MS_PLAYFIELD_HEIGHT + 2;
    X = ms_id_p[0];
    if (X != -1) {
        ms_tmp = ms_sprite_y[X];
        X = ms_sprite_id[X];
        ms_next_p0_slice = ms_tmp + ms_height[X];
        if (ms_next_p0_slice < ms_height[X]) ms_tmp = 0; // Manage out of upper screen sprites
        X = ms_id_p[1];
        if (X != 1) {
            if (ms_sprite_y[X] < ms_next_p0_slice) {
                ms_next_p0_p1_slice = ms_next_p0_slice;   
                ms_next_p0_slice = ms_sprite_y[X];
                X = ms_sprite_id[X];
                ms_next_p1_slice = ms_next_p0_slice + ms_height[X];
            } else {
                ms_next_void_slice = ms_sprite_y[X];
                X = ms_sprite_id[X];
                ms_next_p1_slice = ms_next_void_slice + ms_height[X];
            }
        }
    }

    // 2 lines to prepare for drawing
    Y = 0;
    ms_v = ms_scenery[Y];               // 9
    Y++;                                // 2
    X = ms_scenery[Y];                  // 8
    strobe(WSYNC);                      // 3
    // Second prep line has started
    VSYNC[X] = ms_v;                    // 7
    Y++;                                // 2

repo_kernel:

    X = ms_id_p[0];                     // 3
    if (X == -1) { // 4. There is no sprite 0. Allocate 1 ?
        X = ms_id_p[1];                 // 3
        if (X != -1) { // 4. There is a sprite 1
            if (Y >= ms_sprite_y[X] + 8) X = -1; // 17
        }
        if (X != -1) { // 4 [44/76] This is OK. Let's try to allocate one                                                   
            strobe(WSYNC);                  // 3 

            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_allocate_sprite();       // 47 [58/76]
            if (X != -1) {                  // 5/7
                // Check if y position is compatible
                ms_id_p[0] = X;             // 3
                strobe(WSYNC);              // 3
                
                X = ms_scenery[Y];          // 8
                VSYNC[X] = ms_v;            // 7
                Y++;                        // 2
                ms_v = ms_scenery[Y++];     // 9
                
                X = ms_id_p[0];             // 3 [29/76]
                if (Y < ms_sprite_y[X] + 8) { // 11/13 [40/76]
                    kernel_repo0();          // 6 [46/76] 
                } else {
                    // This one will be skipped. Let's set it as prioritary for next time
                    strobe(WSYNC);
                    ms_mark_as_removed();
                    ms_id_p[0] = -1;
                }
            }
        }
    }
    X = ms_scenery[Y];                  // 8
    strobe(WSYNC);                      // 3
    
    VSYNC[X] = ms_v;                    // 7
    Y++;                                // 2

void_kernel:
    if (Y < ms_tmp) {       // 5/6
        do { 
            kernel_long_macro;              // Macro max 76 * 2 - 39 = 113 cycles (min 76 - 19 = 57 cycles)
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y];              // 8
            strobe(WSYNC);                  // 3

            VSYNC[X] = ms_v;                // 7
            Y++;                            // 2
        } while (Y < ms_tmp);   // 5/6
    }

kernel_start:
    // [14 or 15/76]
    // Now we are going into display 0 and repo 1 kernel
    X = ms_id_p[1];                     // 3
    if (X == -1) {                      // 5/7 // No sprite 1 is allocated. Try to repo one.
        // [22 or 23/76]
        // Display sprite 0 while preparing for next sprite 1
        X = ms_allocate_sprite();       // 47 [69/76]
        strobe(WSYNC);                  // 3  [72/76]

        *GRP0 = ms_grp0ptr[Y];          // 9
        *COLUP0 = ms_colup0ptr[Y];      // 9 
        if (X != -1) {                  // 5/7 [23/76]
                                        // Check if y position is compatible
            ms_tmp = ms_sprite_y[X];    // 7 [30]
            if (Y < ms_tmp + 8) {       // 11/13 [41/76]
                ms_id_p[1] = X;                 // 3
                ms_v = ms_scenery[Y];           // 9
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                load(ms_grp0ptr[Y]);            // 6
                strobe(WSYNC);                  // 3 [72/76]

                store(*GRP0);                   // 3
                *COLUP0 = ms_colup0ptr[Y];      // 9 
                VSYNC[X] = ms_v;                // 7
                Y++;                            // 2
                ms_v = ms_scenery[Y];           // 9
                strobe(WSYNC);                  // 3 [33/76]

                *GRP0 = ms_grp0ptr[Y];          // 9
                *COLUP0 = ms_colup0ptr[Y];      // 9 
                Y++;                            // 2 [20]
                X = ms_id_p[1];                 // 3
                *NUSIZ1 = ms_nusiz[X];          // 7 [30]
                X = ms_sprite_x[X];             // 6
                *HMP1 = ms_sprite_hm[X];        // 7
                ms_x = X;                       // 3 [46]
                X = ms_scenery[Y];              // 8
                *COLUP0 = ms_colup0ptr[Y];      // 9 // This color change is anticipateed. C olor artifact when sprite 0 is on the right of the screen
                load(ms_grp0ptr[Y]);            // 6
                Y++;                            // 2
                strobe(WSYNC);                  // 3 [74/76]                                                                                                                                                                                         otal (2) = 67

                store(*GRP0);                   // 3
                VSYNC[X] = ms_v;                // 7
                // Player 1 repositionning
                X = ms_x;                       // 3
                X = ms_sprite_wait[X];          // 6
                do { X--; } while (X);          // 5 / cycle. 4 for the last one. 
                strobe(RESP1);                  // 3. Minimum = 26 cycles
                strobe(WSYNC);                  // 3. Total (3) = Unknown

                *GRP0 = ms_grp0ptr[Y];          // 9
                *COLUP0 = ms_colup0ptr[Y];      // 9 
                ms_v = ms_scenery[Y];           // 9
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                ms_colup0 = ms_colup0ptr[Y];    // 9 
                load(ms_grp0ptr[Y]);            // 6
                strobe(WSYNC);                  // 3 [55/76]

                store(*GRP0);                   // 3
                *COLUP0 = ms_colup0;            // 6 [9]
                VSYNC[X] = ms_v;                // 7 [16]
                // Wait for 73 - 16 cycles exactly
                X = ms_id_p[1];                 // 3 [19]
                X = ms_sprite_id[X];            // 6 [25] 
                ms_grp1ptr = ms_grptr[X] - ms_tmp;   // 21 [46] 
                ms_colup1ptr = ms_coluptr[X] - ms_tmp; // 21 [65]
                Y++;                            // 2 [67]
                csleep(3);                      // 3 [70]
                strobe(HMOVE);                  // 3. Early HMOVE at cycle 73
                //strobe(WSYNC);                // Total (5) = 74

                *GRP0 = ms_grp0ptr[Y];          // 9 (anticipated)
                *COLUP0 = ms_colup0ptr[Y];      // 9 
                ms_v = ms_scenery[Y];           // 9
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                strobe(WSYNC);                  // 3. Total (6) = 38 cycles

                *GRP0 = ms_grp0ptr[Y];          // 9
                VSYNC[X] = ms_v;                // 7
                *COLUP0 = ms_colup0ptr[Y];      // 9  // This color change is slightly delayed. Color artifact when sprite 0 is on the left of the screen
                Y++;                            // 2
            }
        }
    }
    //[24 or 25/76] (in case of p1)

    // Simply display sprite p0
    if (Y < ms_next_p0_slice) { // 5/6
        do {
            ms_v = ms_scenery[Y];       // 9
            strobe(WSYNC);              // 3

            *GRP0 = ms_grp0ptr[Y];      // 9
            *COLUP0 = ms_colup0ptr[Y];  // 9 
            kernel_macro; // Max 15 cycles
            Y++;                        // 2
            X = ms_scenery[Y];          // 8
            load(ms_grp0ptr[Y]);        // 6
            strobe(WSYNC);              // 3

            store(*GRP0);               // 3
            *COLUP0 = ms_colup0ptr[Y];  // 9 
            VSYNC[X] = ms_v;            // 7
            Y++;                        // 2
        } while (Y < ms_next_p0_slice);   // 5/6
    }

    // [34/76] (in case of no p0 alone)
    // [27/76] (in case of p0 only)
    // And then let's go into display 0 & 1 kernel
    if (Y < ms_next_p0_p1_slice) { // 6/8
        *VDELP0 = 1;                    // 5
        // [45/76] (in case of no p0 only)
        // [38/76] (in case of p0 only)
        ms_v = ms_scenery[Y];           // 9
        *COLUP0 = ms_colup0ptr[Y];      // 9
        *COLUP1 = ms_colup1ptr[Y];      // 9
        *GRP0 = ms_grp0ptr[Y];          // 9
        *GRP1 = ms_grp1ptr[Y];          // 9. Total: 45 + 9 * 5 = 90... In hblank of next line...
        // Skip this WSYNC as we are late on this scanline...
        //strobe(WSYNC);                // 3
        // Don't execute the kernel macro on the first line in order to resync
        //kernel_macro;                 // Max 15 cycles
        Y++;                            // 2
        X = ms_scenery[Y];              // 8
        ms_colup0 = ms_colup0ptr[Y];    // 9
        ms_colup1 = ms_colup1ptr[Y];    // 9
        *GRP0 = ms_grp0ptr[Y];          // 9
        load(ms_grp1ptr[Y]);            // 6
        strobe(WSYNC);                  // 3

        store(*GRP1);                   // 3
        *COLUP0 = ms_colup0;            // 6
        *COLUP1 = ms_colup1;            // 6
        VSYNC[X] = ms_v;                // 7
        Y++;                            // 2
        
        do { // [30/76] while looping
            ms_v = ms_scenery[Y];           // 9
            ms_colup0 = ms_colup0ptr[Y];    // 9
            ms_colup1 = ms_colup1ptr[Y];    // 9
            *GRP0 = ms_grp0ptr[Y];          // 9
            load(ms_grp1ptr[Y]);            // 6
            strobe(WSYNC);                  // 3 ; Total (1) = 30 + 36 + 9 = 76

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
        } while (Y < ms_next_p0_p1_slice);  // 5/6
        *VDELP0 = 0;                        // 5
        // [37/76] when getting out (including JMP)
    } else {
        // Void with repo 0
        if (Y < ms_next_void_slice) {       // 5/6
            do { 
                kernel_long_macro;              // Macro max 76 * 2 - 39 = 113 cycles (min 76 - 19 = 57 cycles)
                ms_v = ms_scenery[Y];           // 9
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                strobe(WSYNC);                  // 3

                VSYNC[X] = ms_v;                // 7
                Y++;                            // 2
            } while (Y < ms_next_void_slice);   // 5/6
        }
    }

    // Display 1 with repo 0
    
    // And then let's go into display 0 & 1 kernel
    if (Y < ms_next_p0_p1_slice) { // 6/8
        *VDELP0 = 1;                    // 5
        // [45/76] (in case of no p0 only)
        // [38/76] (in case of p0 only)
        ms_v = ms_scenery[Y];           // 9
        *COLUP0 = ms_colup0ptr[Y];      // 9
        *COLUP1 = ms_colup1ptr[Y];      // 9
        *GRP0 = ms_grp0ptr[Y];          // 9
        *GRP1 = ms_grp1ptr[Y];          // 9. Total: 45 + 9 * 5 = 90... In hblank of next line...
        // Skip this WSYNC as we are late on this scanline...
        //strobe(WSYNC);                // 3
        // Don't execute the kernel macro on the first line in order to resync
        //kernel_macro;                 // Max 15 cycles
        Y++;                            // 2
        X = ms_scenery[Y];              // 8
        ms_colup0 = ms_colup0ptr[Y];    // 9
        ms_colup1 = ms_colup1ptr[Y];    // 9
        *GRP0 = ms_grp0ptr[Y];          // 9
        load(ms_grp1ptr[Y]);            // 6
        strobe(WSYNC);                  // 3

        store(*GRP1);                   // 3
        *COLUP0 = ms_colup0;            // 6
        *COLUP1 = ms_colup1;            // 6
        VSYNC[X] = ms_v;                // 7
        Y++;                            // 2
        
        do { // [30/76] while looping
            ms_v = ms_scenery[Y];           // 9
            ms_colup0 = ms_colup0ptr[Y];    // 9
            ms_colup1 = ms_colup1ptr[Y];    // 9
            *GRP0 = ms_grp0ptr[Y];          // 9
            load(ms_grp1ptr[Y]);            // 6
            strobe(WSYNC);                  // 3 ; Total (1) = 30 + 36 + 9 = 76

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
        } while (Y < ms_next_p0_p1_slice);  // 5/6
        *VDELP0 = 0;                        // 5
        // [37/76] when getting out (including JMP)
    } else {
        // Void with repo 1
    }

    if (Y < MS_PLAYFIELD_HEIGHT + 2)
        goto kernel_start;
}
