#include <stdio.h>
#include <stdlib.h>

const unsigned char hmv[16] = {0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0, 0xF0, 0xE0, 0xD0, 0xC0, 0xB0, 0xA0, 0x90, 0x80};

void main(int argc, char *argv[])
{
    int i;
    char swait[153], hm[153];
    int offset = atoi(argv[1]);
    int cycle = 68 - 11 - offset;
    for (i = 0; i != 153; i++, cycle++) {
        swait[i] = (cycle + 7) / 15;
        hm[i] = cycle - swait[i] * 15 + 7;
        if (swait[i] * 15 + offset < 68) { // If we are in HBLANK
            swait[i] = 1;
            hm[i] = i;
        }
    }
    printf("const char sprite_wait[153] = {");
    for (i = 0; i != 153; i++) {
        printf("%d", swait[i]);
        if (i < 152) printf(", ");
    }
    printf("};\n");
    printf("const unsigned char sprite_hm[153] = {");
    for (i = 0; i != 153; i++) {
        printf("0x%02x", (unsigned int)hmv[hm[i]]);
        if (i < 152) printf(", ");
    }
    printf("};\n");
}
