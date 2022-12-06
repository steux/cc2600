#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char color_letters[] = {'B', 'W', 'R', 'G', 'B', 'O', 'P', 'Y', 'L', 'H'};
const char * const colors[] = {"BLACK", "WHITE", "RED", "GREEN", "BLUE", "ORANGE", "PURPLE", "YELLOW", "LBLUE", "GREY"};

const char * const find_color(char c) {
    int i;
    for (i = 0; i != sizeof(color_letters); i++) {
        if (color_letters[i] == c) {
            return colors[i];
        }
    }
    return NULL;
}

int find_hmove(int dx)
{
    const int hmove[16] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0, 0xF0, 0xE0, 0xD0, 0xc0, 0xB0, 0xA0, 0x90, 0x80};
    if (dx >= -7 && dx <= 8) {
        return hmove[dx + 7];
    }
    return -1;
}

int main()
{
    char line[256];
    int lnb = 1; 
    
    unsigned char colup0 = 0, grp0 = 0;
    signed char xp0 = 0, dxp0 = 0;
    unsigned char colup1 = 0, grp1 = 0;
    signed char xp1 = 0 , dxp1 = 0;
    unsigned char bsize = 0;
    signed char xb = 0, dxb = 0;
    
    while (fgets(line, 256, stdin)) {
        int c0 = -1, c1 = -1, lb = -1, rb = -1, lc0 = -1, lc1 = -1, xc0 = 0, xc1 = 0;
        unsigned int c0x = 0, c1x = 0;
        int x, i;
        // Analyze the characters on the line
        for (x = 0, i = 0; i < strlen(line); i += 3, x++) {
            char c = line[i];
            if (xc0 != 0 && xc0 != 8) { c0x <<= 1; xc0++; }
            if (xc1 != 0 && xc1 != 8) { c1x <<= 1; xc1++; }
            if (c == 'b') {
                if (lb == -1) lb = x;
                rb = x;
            } else if (c == ' ' || c == '.' || c == '\n') continue; else {
                const char *const color = find_color(c);
                if (color) {
                    if (c == colup0 || c == c0) {
                        c0 = c;
                        if (lc0 == -1) { lc0 = x; xc0 = 1; }
                        if (xc0 == 8) {
                            fprintf(stderr, "Too long color0 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c0x |= 1;
                    } else if (c == colup1 || c == c1) {
                        c1 = c;
                        if (lc1 == -1) { lc1 = x; xc1 = 1; }
                        if (xc1 == 8) {
                            fprintf(stderr, "Too long color1 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c1x |= 1;
                    } else if (c0 == -1) {
                        c0 = c;
                        if (lc0 == -1) { lc0 = x; xc0 = 1; }
                        if (xc0 == 8) {
                            fprintf(stderr, "Too long color0 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c0x |= 1;
                    } else if (c1 == -1) {
                        c1 = c;
                        if (lc1 == -1) { lc1 = x; xc1 = 1; }
                        if (xc1 == 8) {
                            fprintf(stderr, "Too long color1 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c1x |= 1;
                    }
                } else {
                    fprintf(stderr, "Unknown color in input at line %d, column %d\n", lnb, i + 1);
                    return -1;
                }
            }
        }
        for (; xc0 != 8; xc0++) c0x <<= 1;
        for (; xc1 != 8; xc1++) c1x <<= 1;

#ifdef DEBUG
        fprintf(stderr, "#%d: lc0=%d, c0x=%02x, lc1=%d, c1x=%02x\n", lnb, (int)lc0, (unsigned int)c0x, (int)lc1, (unsigned int)c1x);
#endif

        // Ok, now we have all the information we need
        int ndxp0 = 0, ndxp1 = 0;
        fprintf(stdout, "strobe(WSYNC);\nstrobe(HMOVE);\nSTART\n");
        if (c0 != -1 && c0 != colup0) {
            colup0 = c0;
            fprintf(stdout, "*COLUP0 = %s;\n", find_color(c0));
        }
        if (c1 != -1 && c1 != colup1) {
            colup1 = c1;
            fprintf(stdout, "*COLUP1 = %s;\n", find_color(c1));
        }
        if (lc0 == -1) {
            if (grp0) {
                fprintf(stdout, "*GRP0 = 0;\n");
                grp0 = 0;
            }
        } else {
            if (lc0 < xp0) { // We certainly have to move left
                ndxp0 = lc0 - xp0;
                xp0 = lc0;
                fprintf(stdout, "*HMP0 = 0x%02x;\n", find_hmove(ndxp0));
            } else {
                int ngrp0 = c0x >> (lc0 - xp0);
                if (ngrp0 << (lc0 - xp0) != c0x) {
                    // Oops. Some bits were lost
                    ndxp0 = xp0 - lc0;
                    xp0 = lc0;
                    fprintf(stdout, "*HMP0 = 0x%02x;\n", find_hmove(ndxp0));
                    ngrp0 = c0x;
                } // Perfect. it fits in 8 bits
                if (ngrp0 != grp0) {
                    grp0 = ngrp0;
                    fprintf(stdout, "*GRP0 = 0x%02x;\n", grp0);
                }
            }
            if (ndxp0 == 0 && dxp0 != 0) {
                fprintf(stdout, "*HMP0 = 0;\n");
            }
        }
        dxp0 = ndxp0;
        if (lc1 == -1) {
            if (grp1) {
                fprintf(stdout, "*GRP1 = 0;\n");
                grp1 = 0;
            }
        } else {
            if (lc1 < xp1) { // We certainly have to move left
                ndxp1 = lc1 - xp1;
                xp1 = lc1;
                fprintf(stdout, "*HMP1 = 0x%02x;\n", find_hmove(ndxp1));
            } else {
                int ngrp1 = c1x >> (lc1 - xp1);
                if (ngrp1 << (lc1 - xp1) != c1x) {
                    // Oops. Some bits were lost
                    ndxp1 = xp1 - lc1;
                    xp1 = lc1;
                    fprintf(stdout, "*HMP1 = 0x%02x;\n", find_hmove(ndxp1));
                    ngrp1 = c1x;
                } // Perfect. it fits in 8 bits
                if (ngrp1 != grp1) {
                    grp1 = ngrp1;
                    fprintf(stdout, "*GRP1 = 0x%02x;\n", grp1);
                }
            }
        }
        if (ndxp1 == 1 && dxp1 != 1) {
            fprintf(stdout, "*HMP1 = 0;\n");
        }
        dxp1 = ndxp1;
        fprintf(stdout, "\n");
        lnb++;
    }
    
    fprintf(stdout, "strobe(WSYNC);\nstrobe(HMOVE);\nSTART\n");
    if (grp0) fprintf(stdout, "*GRP0 = 0\n");
    if (grp1) fprintf(stdout, "*GRP1 = 0\n");
    if (xp0 != 0) {
        dxp0 = -xp0;
        fprintf(stdout, "*HMP0 = 0x%02x;\n", find_hmove(dxp0));
    } else if (dxp0) {
        dxp0 = 0;
        fprintf(stdout, "*HMP0 = 0\n");
    }
    if (xp1 != 0) {
        dxp1 = -xp1;
        fprintf(stdout, "*HMP1=0x%02x;\n", find_hmove(dxp1));
    } else if (dxp1) {
        dxp1 = 1;
        fprintf(stdout, "*HMP1 = 0\n");
    }
    
    fprintf(stdout, "strobe(WSYNC);\nstrobe(HMOVE);\nSTART\n");
    if (dxp0) fprintf(stdout, "*HMP0 = 0\n");
    if (dxp1) fprintf(stdout, "*HMP1 = 0\n");
    if (dxb) fprintf(stdout, "*HMBL = 0\n");
    return 0;
}

