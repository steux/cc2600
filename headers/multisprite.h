/*
    cc2600 Multisprite Library 
    Copyright (C) 2024 Bruno STEUX 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Contact info: bruno.steux@gmail.com
*/

// v0.1: Initial version
// TODO: Add support for SARA chip
// TODO: Add support for single color (3 bits of model_id)
//
#ifndef __MULTISPRITE_H__
#define __MULTISPRITE_H__

#include "vcs.h"

#ifndef kernel_short_macro
#define kernel_short_macro
#endif
    
#ifndef kernel_medium_macro
#define kernel_medium_macro
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

#ifndef MS_OFFSCREEN_BANK
#define MS_OFFSCREEN_BANK
#define ms_grptr_offscreen ms_grptr
#define ms_coluptr_offscreen ms_coluptr
#define ms_height_offscreen ms_height
#endif

#ifndef MS_KERNEL_BANK
#define MS_KERNEL_BANK
#endif

#ifndef MS_OFFSCREEN_DATA
#define MS_OFFSCREEN_DATA
#endif

#ifndef MS_KERNEL_DATA
#define MS_KERNEL_DATA
#endif

#define MS_UNALLOCATED 255
#define MS_OFFSET 32

char ms_v, y0, y1, h0, h1;
char *ms_colup0ptr, *ms_colup1ptr, *ms_grp0ptr, *ms_grp1ptr, *ms_scenery;
char ms_sprite_iter;
char ms_sprite_x[MS_MAX_NB_SPRITES];
char ms_sprite_y[MS_MAX_NB_SPRITES];
char ms_sprite_model[MS_MAX_NB_SPRITES];
char ms_nusiz[MS_MAX_NB_SPRITES];
char ms_sorted_by_y[MS_MAX_NB_SPRITES];
char ms_id_p[2];
char ms_nb_sprites;

// Generated by hmgen2 17
MS_OFFSCREEN_BANK aligned(256) const char ms_sprite_wait_offscreen[160] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11};
MS_OFFSCREEN_DATA
MS_OFFSCREEN_BANK aligned(256) const char ms_sprite_hm_offscreen[160] = {0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60};

// Generated by hmgen2 19 (compiled with -DEARLY_HMOVE)
MS_KERNEL_BANK aligned(256) const char ms_sprite_wait[160] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11};
MS_KERNEL_DATA
MS_KERNEL_BANK aligned(256) const char ms_sprite_hm[160] = {0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80, 0x60, 0x50, 0x40};

MS_OFFSCREEN_BANK void multisprite_init(char *scenery)
{
    ms_nb_sprites = 0;
    ms_scenery = scenery;
}

// Create a new sprite at nx, ny (model and nusiz provided)
MS_OFFSCREEN_BANK char multisprite_new(char model, char nx, char ny, char nusiz)
{
    ny += MS_OFFSET;
    // Look for right ny position
    for (X = ms_nb_sprites; X != 0; X--) {
        X--;
        Y = ms_sorted_by_y[X] & 0x7f;
        X++;
        if (ny >= ms_sprite_y[Y]) break;
        ms_sorted_by_y[X] = Y;
    }

    // Put new sprite data
    // Look for a free place
    for (Y = 0; Y != ms_nb_sprites; Y++) {
        if (ms_sprite_model[Y] == -1) break;
    }

    ms_sorted_by_y[X] = Y;
    ms_sprite_x[Y] = nx;
    ms_sprite_y[Y] = ny;
    ms_sprite_model[Y] = model;
    ms_nusiz[Y] = nusiz;

    // Update number of sprites
    ms_nb_sprites++;
    return Y;
}

MS_OFFSCREEN_BANK void multisprite_delete(char i)
{
    // Remove from ms_sorted_by_y array
    for (X = 0; X != ms_nb_sprites; X++) {
        if ((ms_sorted_by_y[X] & 0x7f) == i) break;
    }
    for (; X < ms_nb_sprites - 1; X++) {
        Y = ++X;
        X--;
        ms_sorted_by_y[X] = ms_sorted_by_y[Y];
    }
    ms_sprite_model[X = i] = -1; // Mark as free
    ms_nb_sprites--;
}

MS_OFFSCREEN_BANK void multisprite_move(char i, char nx, char ny)
{
    ny += MS_OFFSET;
    ms_sprite_x[X = i] = nx;
    if (ms_sprite_y[X] == ny) return; // No vertical move, so nothing to check
    Y = 0;
    while ((ms_sorted_by_y[Y] & 0x7f) != i) Y++;
    // Update ms_sorted_by_y if needed    
    if (ms_sprite_y[X] < ny) {
        // We have gone downwards
        ms_sprite_y[X] = ny;
        for (; Y != ms_nb_sprites; Y++) {
            Y++;
            X = ms_sorted_by_y[Y] & 0x7f;
            Y--;
            if (ny > ms_sprite_y[X]) {
                Y++;
                X = ms_sorted_by_y[Y];
                Y--;
                ms_sorted_by_y[Y] = X;
            } else break;
        }
        ms_sorted_by_y[Y] = i;
    } else {
        // We have gone upwards
        ms_sprite_y[X] = ny;
        for (; Y != 0; Y--) {
            Y--;
            X = ms_sorted_by_y[Y] & 0x7f;
            Y++;
            if (ny < ms_sprite_y[X]) {
                Y--;
                X = ms_sorted_by_y[Y];
                Y++;
                ms_sorted_by_y[Y] = X;
            } else break;
        }
        ms_sorted_by_y[Y] = i;
    }
}

inline void _ms_select_sprites()
{
    Y = 0;
    ms_sorted_by_y[Y] &= 0x7f; // Display this one (a priori)
    X = ms_sorted_by_y[Y];
    ms_nusiz[X] &= 0x3f; // Reset collision
    char candidate1 = X;
    for (Y = 1; Y < ms_nb_sprites; Y++) {
        char candidate2 = ms_sorted_by_y[Y];
        if (candidate2 & 0x80) { // If it was not displayed at previous iteration
            // Let's see if this candidate overlaps with our previous candidate
            char y2 = ms_sprite_y[X = candidate2 & 0x7f];
            char y1 = ms_sprite_y[X = candidate1 & 0x7f];
            char height1 = ms_height[X = ms_sprite_model[X]];
            if (y1 + height1 + 8 >= y2) {
                // Yes. It overlaps. Skip candidate1 and set it as prioritary for next time
                ms_sorted_by_y[--Y] |= 0x80;
                Y++;
            } 
            ms_sorted_by_y[Y] = candidate2 & 0x7f;
        } else {
            X = candidate2 & 0x7f;
            ms_nusiz[X] &= 0x3f; // Reset collision
        }
        candidate1 = candidate2;
    }
}

inline char _ms_allocate_sprite()
{
    char ms_tmp;
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

MS_KERNEL_BANK char _ms_mark_as_removed()
{
    X = ms_sprite_iter;
    ms_sorted_by_y[--X] |= 0x80;
    X = ms_scenery[Y];                  // 8
    strobe(WSYNC);                      // 3

    VSYNC[X] = ms_v;                    // 7
    Y++;                                // 2
}

MS_KERNEL_BANK char _ms_kernel_repo0()
{
    char ms_tmp;

    // Entering at [46/76]
    ms_tmp = ms_scenery[Y];             // 9 [55/76] (variable)
    X = ms_id_p[0];                     // 3
    *NUSIZ0 = ms_nusiz[X];              // 7 [65/76]
    *REFP0 = *NUSIZ0;                   // 3 [68/76]
    strobe(WSYNC);                      // 3

    X = ms_sprite_x[X];                 // 6
    *HMP0 = ms_sprite_hm[X];            // 7
    X = ms_sprite_wait[X];              // 6 [19/76]
    if (X) do { X--; } while (X);       // 3 for X = 0. 1 + X * 5 cycles. 
    strobe(RESP0);                      // 3. Minimum = 26 cycles
    strobe(WSYNC);                      // 3
   
    X = ms_tmp;                         // 3
    VSYNC[X] = ms_v;                    // 7 [10]
    X = ms_id_p[0];                     // 3 
    y0 = ms_sprite_y[X];                // 7 [20]
    X = ms_sprite_model[X];                // 6 [26]
    ms_grp0ptr = ms_grptr[X] - y0;      // 21 [47]
    ms_colup0ptr = ms_coluptr[X] - y0;  // 21 [68]
    Y++;                                // 2 [70]
    strobe(HMOVE);                      // Early hmove [73]
    //strobe(WSYNC);                    // 3

    ms_v = ms_scenery[Y++];             // 11
    return ms_height[X];
    // Must leave at < [53/76]
} // RTS: 6

MS_KERNEL_BANK char _ms_kernel_repo1()
{
    char ms_tmp;

    // Entering at [46/76]
    ms_tmp = ms_scenery[Y];             // 9 [55/76] (variable)
    X = ms_id_p[1];                     // 3
    *NUSIZ1 = ms_nusiz[X];              // 7 [65/76]
    *REFP1 = *NUSIZ1;                   // 3 [68/76]
    strobe(WSYNC);                      // 3

    X = ms_sprite_x[X];                 // 6
    *HMP1 = ms_sprite_hm[X];            // 7
    X = ms_sprite_wait[X];              // 6 [19/76]
    if (X) do { X--; } while (X);       // 3 for X = 0. 1 + X * 5 cycles. 
    strobe(RESP1);                      // 3. Minimum = 26 cycles
    strobe(WSYNC);                      // 3
   
    X = ms_tmp;                         // 3
    VSYNC[X] = ms_v;                    // 7 [10]
    X = ms_id_p[1];                     // 3 
    y1 = ms_sprite_y[X];                // 7 [20]
    X = ms_sprite_model[X];                // 6 [26]
    ms_grp1ptr = ms_grptr[X] - y1;      // 21 [47]
    ms_colup1ptr = ms_coluptr[X] - y1;  // 21 [68]
    Y++;                                // 2 [70]
    strobe(HMOVE);                      // Early hmove [73]
    //strobe(WSYNC);                    // 3

    ms_v = ms_scenery[Y++];             // 11
    return ms_height[X];
    // Must leave at < [53/76]
} // RTS: 6

MS_KERNEL_BANK void _ms_p0_kernel(char stop)
{
    do {
        ms_v = ms_scenery[Y];       // 9
        strobe(WSYNC);              // 3

        *GRP0 = ms_grp0ptr[Y];      // 9
        *COLUP0 = ms_colup0ptr[Y];  // 9 
        kernel_medium_macro; // Max 39 cycles
        Y++;                        // 2
        X = ms_scenery[Y];          // 8
        load(ms_grp0ptr[Y]);        // 6
        strobe(WSYNC);              // 3 // [37/76]

        store(*GRP0);               // 3
        *COLUP0 = ms_colup0ptr[Y];  // 9 
        VSYNC[X] = ms_v;            // 7
        Y++;                        // 2
    } while (Y < stop);   // 5/6
    // [32/76] including RTS
}

MS_KERNEL_BANK void _ms_p1_kernel(char stop)
{
    do {
        ms_v = ms_scenery[Y];       // 9
        strobe(WSYNC);              // 3

        *GRP1 = ms_grp1ptr[Y];      // 9
        *COLUP1 = ms_colup1ptr[Y];  // 9 
        kernel_medium_macro; // Max 39 cycles
        Y++;                        // 2
        X = ms_scenery[Y];          // 8
        load(ms_grp1ptr[Y]);        // 6
        strobe(WSYNC);              // 3 // [37/76]

        store(*GRP1);               // 3
        *COLUP1 = ms_colup1ptr[Y];  // 9 
        VSYNC[X] = ms_v;            // 7
        Y++;                        // 2
    } while (Y < stop);   // 5/6
    // [32/76] including RTS
}

MS_KERNEL_BANK void _ms_p0_p1_kernel(char stop)
{
    char ms_colup0, ms_colup1;

    *VDELP0 = 1;                    // 5
    // [46/76] when coming from p0/p1 multisprite_kernel
    ms_v = ms_scenery[Y];           // 9
    *GRP0 = ms_grp0ptr[Y];          // 9
    *COLUP1 = ms_colup1ptr[Y];      // 9
    *COLUP0 = ms_colup0ptr[Y];      // 9 [82/76]. No WSYNC necessary 
    *GRP1 = ms_grp1ptr[Y];          // 6 
    Y++;                            // 2
    X = ms_scenery[Y];              // 8
    ms_colup0 = ms_colup0ptr[Y];    // 9
    ms_colup1 = ms_colup1ptr[Y];    // 9
    *GRP0 = ms_grp0ptr[Y];          // 9
    load(ms_grp1ptr[Y]);            // 6
    strobe(WSYNC);                  // 3: Total (2) = 134 = 58 into the second line

    store(*GRP1);                   // 3
    *COLUP0 = ms_colup0;            // 6
    *COLUP1 = ms_colup1;            // 6
    VSYNC[X] = ms_v;                // 7
    Y++;                            // 2
    
    if (Y < stop) {

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
            kernel_short_macro;             // Max 15 cycles
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
        } while (Y < stop);                 // 5/6
    }
    *VDELP0 = 0;                        // 5
    // [40/76] when getting out (including RTS)
}

MS_KERNEL_BANK void _ms_void_kernel(char stop)
{
    // [45/76] when coming from no sprite 0 & 1 left to display
    strobe(WSYNC);                  // 3
    ms_v = ms_scenery[Y];           // 9
    Y++;                            // 2
    X = ms_scenery[Y++];            // 10 
    load(ms_v);                     // 3
    strobe(WSYNC);                  // 3

    store(VSYNC[X]);                // 4
    if (Y < stop) {
        do { 
            kernel_long_macro;              // Macro max 76 * 2 - 39 = 113 cycles (min 76 - 19 = 57 cycles)
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y++];            // 10 
            load(ms_v);                     // 3
            strobe(WSYNC);                  // 3

            store(VSYNC[X]);                // 4
        } while (Y < stop);                 // 5/6
    }
} //[15/76] when getting out (including RTS)

MS_OFFSCREEN_BANK void multisprite_kernel_prep()
{
    char ms_tmp;
    _ms_select_sprites();
    
    // Phase 1: before the multisprite_kernel actually starts, allocates and positions sprites p0 and p1.
    ms_sprite_iter = 0;
    ms_v = 0;
    *VDELP0 = 0;
    y0 = MS_UNALLOCATED;
    y1 = MS_UNALLOCATED;
    X = _ms_allocate_sprite(); // 47
    // Position sprite 0
    if (X != -1) {
        ms_id_p[0] = X;
        *NUSIZ0 = ms_nusiz[X];
        *REFP0 = *NUSIZ0; 
        Y = ms_sprite_model[X];
        y0 = ms_sprite_y[X];
        ms_grp0ptr = ms_grptr_offscreen[Y] - y0;   // 21
        ms_colup0ptr = ms_coluptr_offscreen[Y] - y0; // 21
        h0 = ms_height_offscreen[Y];
        X = ms_sprite_x[X];             // 6
        strobe(WSYNC);                  // 3
        
        *HMP0 = ms_sprite_hm_offscreen[X];  // 7
        X = ms_sprite_wait_offscreen[X]; // 6
        csleep(4);                      // 4 [17/76]
        if (X) do { X--; } while (X);   // 3 for X = 0. 1 + X * 5 cycles. 
        strobe(RESP0);                  // 3. Minimum = 23 cycles
        strobe(WSYNC);                  // 3
        strobe(HMOVE);
    }
    X = _ms_allocate_sprite(); // 47
    // Position sprite 1
    if (X != -1) {
        ms_id_p[1] = X;
        *NUSIZ1 = ms_nusiz[X];
        *REFP1 = *NUSIZ1; 
        Y = ms_sprite_model[X];
        y1 = ms_sprite_y[X];
        ms_grp1ptr = ms_grptr_offscreen[Y] - y1;   // 21
        ms_colup1ptr = ms_coluptr_offscreen[Y] - y1; // 21
        h1 = ms_height_offscreen[Y];
        X = ms_sprite_x[X];             // 6
        *HMP0 = 0;                      // 3
                                    
        if (y0 < MS_OFFSET && y1 < MS_OFFSET) {
            ms_v = 1;
            if (y1 + h1 >= y0 + h0) ms_v = 2;
        } 

        strobe(WSYNC);                  // 3        
        *HMP1 = ms_sprite_hm_offscreen[X];// 7 [13/76]
        X = ms_sprite_wait_offscreen[X];  // 6
        csleep(4);                      // 4 [17/76]
        if (X) do { X--; } while (X);   // 3 for X = 0. 1 + X * 5 cycles. 
        strobe(RESP1);                  // 3. Minimum = 23 cycles
        strobe(WSYNC);                  // 3
        strobe(HMOVE);
    }

    Y = MS_OFFSET;
    *GRP1 = 0;
    *GRP0 = 0;
}

MS_KERNEL_BANK _ms_check_collisions()
{
    if (*CXP0FB & 0x80) ms_nusiz[X = ms_id_p[0]] |= 0x40; // 8/20
    if (*CXP1FB & 0x80) ms_nusiz[X = ms_id_p[1]] |= 0x40;
    strobe(WSYNC);                  // 3
    if (*CXPPMM & 0x80) {
        ms_nusiz[X = ms_id_p[0]] |= 0x80;
        ms_nusiz[X = ms_id_p[1]] |= 0x80;
    }
    strobe(CXCLR);
}

MS_KERNEL_BANK void multisprite_kernel()
{
    char ms_tmp;

    // Prepare for drawing
    ms_tmp = ms_scenery[Y++];           // 11
    X = ms_scenery[Y++];                // 10 
    VSYNC[X] = ms_tmp;                  // 7
    strobe(HMCLR);                      // 3
    ms_tmp = y0 + h0;
    strobe(WSYNC);                      // 3
    *VBLANK = 0;

    if (ms_v == 0) {
        goto shortcut_sprite0;
    } else {
        y0 += h0;
        y1 += h1;
        if (ms_v == 1) {
            _ms_p0_p1_kernel(y1);
            _ms_p0_kernel(y0);
        } else {
            _ms_p0_p1_kernel(y0);
            _ms_p1_kernel(y1);
        }
        *GRP0 = 0;
        *GRP1 = 0;
        y0 = MS_UNALLOCATED;
        y1 = MS_UNALLOCATED;
        goto repo0_try_again;
    }
        
repo_kernel:
    if (y0 == MS_UNALLOCATED) { // 8. There is no sprite 0. Allocate 1 ?
repo0_kernel:
        if (y1 == MS_UNALLOCATED || Y < y1 - 8) {      // 22                                              
repo0_try_again: 
            strobe(WSYNC);                  // 3 

            ms_v = ms_scenery[Y++];         // 11 
            X = _ms_allocate_sprite();      // 47 [58/76]
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
                    h0 = _ms_kernel_repo0();// 6 [46/76] 
                } else {
                    // This one will be skipped. Let's set it as prioritary for next time
                    strobe(WSYNC);          // 3
                    ms_id_p[0] = -1;

                    _ms_mark_as_removed();
                    goto repo0_try_again;
                }
                strobe(HMCLR);              // 3
            }
        } else {
            strobe(WSYNC);                  // 3 
            ms_v = ms_scenery[Y++];         // 11 
        }
    } else {
        strobe(WSYNC);                  // 3 
        ms_v = ms_scenery[Y++];         // 11 
    }
    X = ms_scenery[Y++];            // 10
    strobe(WSYNC);                  // 3

    VSYNC[X] = ms_v;                // 7

    // Repo multisprite_kernel 1
    if (y1 == MS_UNALLOCATED) { // 7. There is no sprite 1. Allocate 1 ?
repo1_kernel:
        if (y0 == MS_UNALLOCATED || Y < y0 + 8) {      // 22                                             
repo1_try_again: 
            strobe(WSYNC);                  // 3 

            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = _ms_allocate_sprite();      // 47 [58/76]
            if (X != -1) {                  // 4/5
                // Check if y position is compatible
                ms_id_p[1] = X;             // 3
                strobe(WSYNC);              // 3
               
                X = ms_scenery[Y];          // 8
                VSYNC[X] = ms_v;            // 7
                Y++;                        // 2
                ms_v = ms_scenery[Y++];     // 9
                
                X = ms_id_p[1];             // 3 [29/76]
                if (Y < ms_sprite_y[X] + 8) { // 11/13 [40/76]
                    h1 = _ms_kernel_repo1();          // 6 [46/76] 
                } else {
                    // This one will be skipped. Let's set it as prioritary for next time
                    strobe(WSYNC);                      // 3
                    ms_id_p[1] = -1;

                    _ms_mark_as_removed();
                    goto repo1_try_again;
                }
                strobe(HMCLR);              // 3
            }
        } else {
            strobe(WSYNC);                  // 3 
            ms_v = ms_scenery[Y++];          
        }
    } else {
        strobe(WSYNC);                  // 3 
        ms_v = ms_scenery[Y++];          
    }
            
    X = ms_scenery[Y++];            // 10
    strobe(WSYNC);                  // 3

    VSYNC[X] = ms_v;                // 7

display_sprites:    
    if (y0 < y1) {                      // 8/9
display_sprite0:    
        ms_tmp = y0 + h0;
shortcut_sprite0:
        if (Y < y0) _ms_void_kernel(y0);
        if (ms_tmp < y1) {
            if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                _ms_p0_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                goto check_collisions_and_return;
            }
            _ms_p0_kernel(ms_tmp);
            if (Y >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 6) goto finish;
            strobe(WSYNC);                  // 3
            if (*CXP0FB & 0x80) ms_nusiz[X = ms_id_p[0]] |= 0x40;
            strobe(CXCLR); // Clear collisions
            ms_tmp = y1 - 4;
            y0 = MS_UNALLOCATED;
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y++];            // 10 
            load(ms_v);                     // 3
            strobe(WSYNC);                  // 3
            store(VSYNC[X]);                // 4

            if (Y < ms_tmp) goto repo0_kernel;
            goto display_sprite1; 
        } else {
            y0 = y1 + h1;
            if (y0 < ms_tmp) {
                _ms_p0_kernel(y1);
                if (y0 >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_p1_kernel(y0);
                if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_kernel(ms_tmp);
                goto repo_try_again;
            } else {
                _ms_p0_kernel(y1);
                if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_p1_kernel(ms_tmp);
                if (y0 >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p1_kernel(y0);
                goto repo_try_again;
            }
        }
    } else if (y1 < y0) { // 8 / 9
display_sprite1:
        ms_tmp = y1 + h1;
        if (Y < y1) _ms_void_kernel(y1);
        if (ms_tmp < y0) {
            if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                _ms_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                goto check_collisions_and_return;
            }
            _ms_p1_kernel(ms_tmp);
            if (Y >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 6) goto finish;
            strobe(WSYNC);                  // 3
            if (*CXP1FB & 0x80) ms_nusiz[X = ms_id_p[1]] |= 0x40;
            strobe(CXCLR); // Clear collisions
            ms_tmp = y0 - 4;
            y1 = MS_UNALLOCATED;
            ms_v = ms_scenery[Y];           // 9
            Y++;                            // 2
            X = ms_scenery[Y++];            // 10 
            load(ms_v);                     // 3
            strobe(WSYNC);                  // 3
            store(VSYNC[X]);                // 4

            if (Y < ms_tmp) goto repo_kernel;
            goto display_sprite0; 
        } else {
            y1 = y0 + h0;
            if (y1 < ms_tmp) {
                _ms_p1_kernel(y0);
                if (y1 >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_p1_kernel(y1);
                if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p1_kernel(ms_tmp);
                goto repo_try_again;
            } else {
                _ms_p1_kernel(y0);
                if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_p1_kernel(ms_tmp);
                if (y1 >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_kernel(y1);
                goto repo_try_again;
            }
        }
    } else {
        if (y0 != MS_UNALLOCATED) {
            ms_tmp = y0 + h0; // 11 [26/76]
            _ms_void_kernel(y0);
            y0 = y1 + h1;      // 10 [36/76]
            if (y0 < ms_tmp) { // 5/6 [42/76]
                if (y0 >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_p1_kernel(y0); // 12 [54/76]
                if (Y < ms_tmp) {
                    if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                        _ms_p0_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                        goto check_collisions_and_return;
                    }
                    _ms_p0_kernel(ms_tmp);
                }
                goto repo_try_again;
            } else {
                if (ms_tmp >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                    _ms_p0_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                    goto check_collisions_and_return;
                }
                _ms_p0_p1_kernel(ms_tmp);
                if (Y < y0) {
                    if (y0 >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 1) {
                        _ms_p1_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
                        goto check_collisions_and_return;
                    }
                    _ms_p1_kernel(y0);
                }
                goto repo_try_again;
            }
        } else {
            // This is the end of the multisprite kernel.Fill with void.
finish:
            if (Y < MS_OFFSET + MS_PLAYFIELD_HEIGHT)
                _ms_void_kernel(MS_OFFSET + MS_PLAYFIELD_HEIGHT);
            strobe(WSYNC);
            return;
        }
    }
repo_try_again:
    _ms_check_collisions();
    ms_v = ms_scenery[Y];           // 9
    Y++;                            // 2
    X = ms_scenery[Y++];            // 10 
    y0 = MS_UNALLOCATED;
    y1 = MS_UNALLOCATED;
    load(ms_v);                     // 3
    strobe(WSYNC);                  // 3
    store(VSYNC[X]);                // 4
    if (Y >= MS_OFFSET + MS_PLAYFIELD_HEIGHT - 6) goto finish;
    goto repo0_try_again;
check_collisions_and_return:
    *GRP0 = 0;
    *GRP1 = 0;
    _ms_check_collisions();
}

#endif // __MULTISPRITE_H__
