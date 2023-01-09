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

#define PIXEL_WIDTH 3
#define DEFAULT_BSIZE 4

int main()
{
    char line[256];
    int lnb = 1; 
    
    unsigned char colup0 = 0, grp0 = 0;
    signed char xp0 = 0, dxp0 = 0;
    unsigned char colup1 = 0, grp1 = 0;
    signed char xp1 = 0 , dxp1 = 0;
    unsigned char bsize = DEFAULT_BSIZE, benabled = 0;
    signed char xb = 0, dxb = 0;
    
    while (fgets(line, 256, stdin)) {
        int lbm = -1, rbm = -1;
        int c0 = -1, c1 = -1, lb = -1, rb = -1, lc0 = -1, lc1 = -1, xc0 = 0, xc1 = 0;
        unsigned int c0x = 0, c1x = 0;
        int x, i;
        // Analyze the characters on the line
        for (x = 0, i = 0; i < strlen(line); i += PIXEL_WIDTH, x++) {
            char c = line[i];
            if (xc0 != 0 && xc0 != 8) { c0x <<= 1; xc0++; }
            if (xc1 != 0 && xc1 != 8) { c1x <<= 1; xc1++; }
            if (c == 'b') {
                if (lb == -1) lb = x;
                rb = x;
                rbm = -1;
            } else if (c == ' ' || c == '.') {
                if (lb == -1) lbm = x;
                if (rbm == -1) rbm = x; // The ball must not cross lbm or rbm;
            } else if (c == '\n') break; else {
                const char *const color = find_color(c);
                if (color) {
                    if (c == colup0 || c == c0) {
                        c0 = c;
                        if (lc0 == -1) { lc0 = x; xc0 = 1; }
                        if (xc0 == 9) {
                            fprintf(stderr, "Too long color0 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c0x |= 1;
                    } else if (c == colup1 || c == c1) {
                        c1 = c;
                        if (lc1 == -1) { lc1 = x; xc1 = 1; }
                        if (xc1 == 9) {
                            fprintf(stderr, "Too long color1 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c1x |= 1;
                    } else if (c0 == -1) {
                        c0 = c;
                        if (lc0 == -1) { lc0 = x; xc0 = 1; }
                        if (xc0 == 9) {
                            fprintf(stderr, "... Too long color0 pattern on line %d\n", lnb);
                            return -1;
                        }
                        c0x |= 1;
                    } else if (c1 == -1) {
                        c1 = c;
                        if (lc1 == -1) { lc1 = x; xc1 = 1; }
                        if (xc1 == 9) {
                            fprintf(stderr, "... Too long color1 pattern on line %d\n", lnb);
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
        if (rbm == -1) rbm = x;

#ifdef DEBUG
        fprintf(stderr, "#%d: lc0=%d, c0x=%02X, lc1=%d, c1x=%02X\n", lnb, (int)lc0, (unsigned int)c0x, (int)lc1, (unsigned int)c1x);
        fprintf(stderr, "#%d: lbm=%d, lb=%d, rb=%d, rbm=%d\n", lnb, (int)lbm, (int)lb, (int)rb, (int)rbm);
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
            int ngrp0;
            if (lc0 < xp0) { // We certainly have to move left
                ndxp0 = lc0 - xp0;
                xp0 = lc0;
                fprintf(stdout, "*HMP0 = 0x%02X;\n", find_hmove(ndxp0));
                ngrp0 = c0x;
            } else {
                int c;
                for (c = 0; c <= lc0 - xp0; c++) {
                    ngrp0 = c0x >> (lc0 - (xp0 + c));
                    if (ngrp0 << (lc0 - (xp0 + c)) == c0x) {
                        // Perfect. it fits in 8 bits
                        ndxp0 = c;
                        xp0 = xp0 + c;
                        if (ndxp0 != 0) fprintf(stdout, "*HMP0 = 0x%02X;\n", find_hmove(ndxp0));
                        break;
                    }
                }
            }
            if (ngrp0 != grp0) {
                grp0 = ngrp0;
                fprintf(stdout, "*GRP0 = 0x%02X;\n", grp0);
            }
        }
        if (ndxp0 == 0 && dxp0 != 0) {
            fprintf(stdout, "*HMP0 = 0;\n");
        }
        dxp0 = ndxp0;
        if (lc1 == -1) {
            if (grp1) {
                fprintf(stdout, "*GRP1 = 0;\n");
                grp1 = 0;
            }
        } else {
            int ngrp1;
            if (lc1 < xp1) { // We certainly have to move left
                ndxp1 = lc1 - xp1;
                xp1 = lc1;
                fprintf(stdout, "*HMP1 = 0x%02X;\n", find_hmove(ndxp1));
                ngrp1 = c1x;
            } else {
                int c;
                for (c = 0; c <= lc1 - xp1; c++) {
                    ngrp1 = c1x >> (lc1 - (xp1 + c));
                    if (ngrp1 << (lc1 - (xp1 + c)) == c1x) {
                        // Perfect. it fits in 8 bits
                        ndxp1 = c;
                        xp1 = xp1 + c;
                        if (ndxp1 != 0) fprintf(stdout, "*HMP1 = 0x%02X;\n", find_hmove(ndxp1));
                        break;
                    }
                }
            } 
            if (ngrp1 != grp1) {
                grp1 = ngrp1;
                fprintf(stdout, "*GRP1 = 0x%02X;\n", grp1);
            }
        }
        if (ndxp1 == 0 && dxp1 != 0) {
            fprintf(stdout, "*HMP1 = 0;\n");
        }
        dxp1 = ndxp1;

        // Check the ball status
        int ndxb = 0;
        if (lb != -1) {
            if (benabled == 0) {
                fprintf(stdout, "*ENABL = 2;\n");
                benabled = 1;
            } 
            int maxwidth = rbm - lbm - 1;
            int minwidth = rb - lb + 1;

            // The idea: Try to check if current bsize fits it
            if (bsize < minwidth || bsize > maxwidth) {
                // No, we have to switch bsize
                int bits = 0;
                if (minwidth > 4) bsize = 8;
                else if (minwidth > 2) bsize = 4;
                else if (minwidth > 1) bsize = 2;
                else bsize = 1;
                switch (bsize) {
                    case 1: bits = 0x00; break;
                    case 2: bits = 0x10; break;
                    case 4: bits = 0x20; break;
                    case 8: bits = 0x30; break;
                }
                fprintf(stdout, "*CTRLPF = 0x%02X;\n", bits);
            }
            // Check if there is a need to move the ball
            if (xb > lb || xb + bsize > rbm) {
                // We need to move the ball to the left
                char nxb = lb;
                while (nxb + bsize > rbm) nxb--;
                ndxb = nxb - xb;
                xb = nxb;
                fprintf(stdout, "*HMBL = 0x%02X;\n", find_hmove(ndxb));
            } else if (xb + bsize <= rb || xb <= lbm) {
                // We need to move the ball to the right
                char nxb = rb - bsize + 1;
                while (nxb <= lbm) nxb++;
                ndxb = nxb - xb;
                xb = nxb;
                fprintf(stdout, "*HMBL = 0x%02X;\n", find_hmove(ndxb));
            }
        } else {
            // No ball. Was it there ?
            if (benabled) {
                fprintf(stdout, "*ENABL = 0;\n");
                benabled = 0;
            }
        }
        if (ndxb == 0 && dxb != 0) {
            fprintf(stdout, "*HMBL = 0;\n");
        }
        dxb = ndxb;
     
#ifdef DEBUG
        fprintf(stderr, "#%d: xb=%d, bsize=%d, dxb=%d\n", lnb, (int)xb, (int)bsize, (int)dxb);
#endif
        fprintf(stdout, "\n");

        lnb++;
    }
    
    fprintf(stdout, "strobe(WSYNC);\nstrobe(HMOVE);\nSTART\n");
    if (grp0) fprintf(stdout, "*GRP0 = 0;\n");
    if (grp1) fprintf(stdout, "*GRP1 = 0;\n");
    if (xp0 != 0) {
        dxp0 = -xp0;
        fprintf(stdout, "*HMP0 = 0x%02X;\n", find_hmove(dxp0));
    } else if (dxp0) {
        dxp0 = 0;
        fprintf(stdout, "*HMP0 = 0;\n");
    }
    if (xp1 != 0) {
        dxp1 = -xp1;
        fprintf(stdout, "*HMP1 = 0x%02X;\n", find_hmove(dxp1));
    } else if (dxp1) {
        dxp1 = 0;
        fprintf(stdout, "*HMP1 = 0;\n");
    }
    if (xb != 0) {
        dxb = -xb;
        fprintf(stdout, "*HMBL = 0x%02X;\n", find_hmove(dxb));
    } else if (dxp1) {
        dxb = 0;;
        fprintf(stdout, "*HMBL = 0;\n");
    }
    if (benabled) {
        fprintf(stdout, "*ENABL = 0;\n");
        benabled = 0;
    }
    
    fprintf(stdout, "\nstrobe(WSYNC);\nstrobe(HMOVE);\nSTART\n");
    if (dxp0) fprintf(stdout, "*HMP0 = 0;\n");
    if (dxp1) fprintf(stdout, "*HMP1 = 0;\n");
    if (dxb) fprintf(stdout, "*HMBL = 0;\n");
    if (bsize != DEFAULT_BSIZE) {
        int bits = 0;
        switch (DEFAULT_BSIZE) {
            case 1: bits = 0x00; break;
            case 2: bits = 0x10; break;
            case 4: bits = 0x20; break;
            case 8: bits = 0x30; break;
        }
        fprintf(stdout, "*CTRLPF = 0x%02X;\n", bits);
    }
    return 0;
}

