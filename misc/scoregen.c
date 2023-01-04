#include <stdio.h>

unsigned char reverse(unsigned char input)
{
    unsigned char output = 0;
    if (input & 1) output |= 128;
    if (input & 2) output |= 64;
    if (input & 4) output |= 32;
    if (input & 8) output |= 16;
    if (input & 16) output |= 8;
    if (input & 32) output |= 4;
    if (input & 64) output |= 2;
    if (input & 128) output |= 1;
    return output;
}


/* Traditionnal ATARI 2600 scoring numbers 
unsigned char numbers[10][5] = {
    {0x0E, 0x0A, 0x0A, 0x0A, 0x0E},
    {0x02, 0x02, 0x02, 0x02, 0x02},
    {0x0E, 0x02, 0x0E, 0x08, 0x0E},
    {0x0E, 0x02, 0x0E, 0x02, 0x0E},
    {0x0A, 0x0A, 0x0E, 0x02, 0x02},
    {0x0E, 0x08, 0x08, 0x02, 0x0E},
    {0x0E, 0x08, 0x0E, 0x0A, 0x0E},
    {0x0E, 0x02, 0x02, 0x02, 0x02},
    {0x0E, 0x0A, 0x0E, 0x0A, 0x0E},
    {0x0E, 0x0A, 0x0E, 0x02, 0x0E}
};
*/

unsigned char numbers[10][15] = {
    {2, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 7, 2},
    {2, 2, 2, 6, 6, 6, 2, 2, 2, 2, 2, 2, 7, 7, 7},
    {2, 7, 7, 5, 5, 1, 3, 2, 6, 4, 4, 4, 7, 7, 7},
    {2, 7, 7, 5, 1, 1, 3, 2, 3, 1, 1, 5, 7, 7, 2},
    {4, 4, 4, 4, 5, 5, 7, 7, 7, 1, 1, 1, 1, 1, 1},
    {7, 7, 7, 4, 4, 4, 6, 7, 7, 1, 1, 1, 7, 7, 6}, 
    {2, 7, 7, 5, 4, 4, 6, 7, 7, 5, 5, 5, 7, 7, 2},
    {7, 7, 7, 1, 1, 1, 3, 2, 2, 6, 4, 4, 4, 4, 4},
    {2, 7, 7, 5, 5, 5, 7, 2, 7, 5, 5, 5, 7, 7, 2},
    {2, 7, 7, 5, 5, 5, 7, 7, 3, 1, 1, 5, 7, 7, 2}
};

void main()
{
    int i, line;
    for (line = 0; line < 15; line++) {
        printf("const bank3 unsigned char score_line%d_PF2[100]={\n\t", line + 1);
        for (i = 0; i < 100; i++) {
            unsigned char PF = (numbers[i % 10][line] << 1) | (numbers[i / 10][line] << 5);
            printf("0x%02x", reverse(PF));
            if (i != 99) printf(", ");
            if (!((i + 1) % 8)) printf("\n\t");
        }
        printf("};\n\n");
    }
    for (line = 0; line < 15; line++) {
        printf("const bank3 unsigned char score_line%d_PF1[100]={\n\t", line + 1);
        for (i = 0; i < 100; i++) {
            unsigned char PF = numbers[i % 10][line] << 1;
            if (i > 9) PF |= (numbers[i / 10][line] << 5);
            printf("0x%02x", PF);
            if (i != 99) printf(", ");
            if (!((i + 1) % 8)) printf("\n\t");
        }
        printf("};\n\n");
    }
}

