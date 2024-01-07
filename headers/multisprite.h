#include "vcs.h"

#ifndef kernel_macro
#define kernel_macro
#endif
    
#ifndef kernel_long_macro
#define kernel_long_macro strobe(WSYNC)
#endif
   
#ifndef MAX_NB_SPRITES
#define MAX_NB_SPRITES 10
#endif

char ms_next_mode;
char ms_v, ms_colup0, ms_colup1;
char *ms_colup0ptr, *ms_colup1ptr, *ms_grp0ptr, *ms_grp1ptr, *ms_scenery;
char ms_next_slice;
char ms_x;
char ms_sprite_iter;
char ms_sprite_x[MAX_NB_SPRITES];
char ms_sprite_y[MAX_NB_SPRITES];
char ms_sprite_id[MAX_NB_SPRITES];
char ms_nusiz[MAX_NB_SPRITES];
char ms_sorted_by_y[MAX_NB_SPRITES];
char ms_tmp, ms_tmp2;
char ms_id_p[2];
char ms_nb_sprites;

const char ms_sprite_wait[153] = {1, 1, 1, 1, 1, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};
const char ms_sprite_hm[153] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0};

const char sprite1[1] = {0};

#define NB_SPRITES_DEF 1
const char *ms_grptr[NB_SPRITES_DEF] = {sprite1};
const char *ms_coluptr[NB_SPRITES_DEF] = {sprite1};
const char ms_height[NB_SPRITES_DEF] = {8};

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

void kernel()
{
    ms_select_sprites();
    // Phase 1: before the kernel actually starts, allocates and positions sprites p0 and p1.
    ms_sprite_iter = 0;
    ms_id_p[0] = ms_allocate_sprite(); // 47
    ms_id_p[1] = ms_allocate_sprite(); // 47
}

void try_kernel()
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
            ms_v = ms_scenery[Y];               // 9
            Y++;                                // 2
            X = ms_scenery[Y];                  // 8
            strobe(WSYNC);                      // 3
            VSYNC[X] = ms_v;                    // 7
            Y++;                                // 2
            do {
                kernel_long_macro;              // Macro max 76 * 2 - 40 = 112 cycles (min 76 - 9 = 67 cycles)
                 ms_v = ms_scenery[Y];          // 9
                Y++;                            // 2
                X = ms_scenery[Y];              // 8
                strobe(WSYNC);                  // 3

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
            // Retrieve ms_x for next sprite
            X = ms_sprite_iter;             // 3
            X = ms_sorted_by_y[X];          // 6
            ms_tmp = X;                     // 3. To be used later
            ms_x = ms_sprite_x[X];          // 7
            X = ms_scenery[Y];              // 8
            *COLUP0 = ms_colup0ptr[Y];      // 9 // This color change is anticipateed. Color artifact when sprite 0 is on the right of the screen
            load(ms_grp0ptr[Y]);            // 6
            Y++;                            // 2
            strobe(WSYNC);                  // 3. Total (2) = 67
            
            store(*GRP0);                   // 3
            VSYNC[X] = ms_v;                // 7
            // Player 1 repositionning
            X = ms_x;                       // 3
            X = ms_sprite_wait[X];          // 6
            if (X) {                        // 3
                do { X--; } while (X);      // 5 / cycle. 4 for the last one. 
            }
            strobe(RESP0);                  // 3. Minimum = 24 cycles (ideally 28, so left part of the screen is not reachable)
            strobe(WSYNC);                  // 3. Total (3) = Unknown
            
            *GRP0 = ms_grp0ptr[Y];          // 9
            *COLUP0 = ms_colup0ptr[Y];      // 9 
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            // Setup new NUSIZ for new sprite 
            X = ms_tmp;                     // 3
            *NUSIZ1 = ms_nusiz[X];          // 7
            X = ms_x;                       // 3
            *HMP1 = ms_sprite_hm[X];        // 7
            X = ms_scenery[Y];              // 8
            ms_colup0 = ms_colup0ptr[Y];    // 9 
            load(ms_grp0ptr[Y]);            // 6
            strobe(WSYNC);                  // 3. Total (4) = 75 cycles

            store(*GRP0);                   // 3
            *COLUP0 = ms_colup0;            // 6 / 9 
            VSYNC[X] = ms_v;                // 7 / 16
            // Wait for 73 - 16 cycles exactly
            X = ms_tmp;                     // 3 / 19 
            X = ms_sprite_id[X];            // 6 / 25 
            ms_tmp = Y;                     // 3 / 28
            ms_grp1ptr = ms_grptr[X] - ms_tmp;   // 21 / 39
            ms_colup1ptr = ms_coluptr[X] - ms_tmp; // 21 / 71
            strobe(HMOVE);                  // 3. Early HMOVE at cycle 74
            //strobe(WSYNC);                // Total (5) = 74
            
            Y++;                            // 2 (for free) 
            *GRP0 = ms_grp0ptr[Y];          // 9
            *COLUP0 = ms_colup0ptr[Y];      // 9 
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y];              // 8
            strobe(WSYNC);                  // 3. Total (6) = 40 cycles
            
            *GRP0 = ms_grp0ptr[Y];          // 9
            VSYNC[X] = ms_v;                // 7
            *COLUP0 = ms_colup0ptr[Y];      // 9  // This color change is slightly delayed. Color artifact when sprite 0 is on the left of the screen
            Y++;                            // 2
            
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

