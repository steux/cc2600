#include <stdio.h>
#include <stdlib.h>

const unsigned char hmv[16] = {8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 0xff};

void main(int argc, char *argv[])
{
    int i;
    char swait[160], hm[160];
    int resp = 0;
    int posresp = atoi(argv[1]) * 3 - 68 + (3 + 3) * 3 + 5;
#ifdef EARLY_HMOVE
    const int span = 0;
#else
    const int span = 8;
#endif
    for (i = 0; i != 160; i++) {
        int max_reachable = posresp + span;
        if (i > max_reachable) {
            if (resp == 0) {
                posresp += 15 - 6;
                max_reachable += 15 - 6;
            } else {
                posresp += 15;
                max_reachable += 15;
            }
            resp++;
        }
        swait[i] = resp;
        hm[i] = max_reachable - i;
    }
    printf("const char ms_sprite_wait[160] = {");
    for (i = 0; i != 160; i++) {
        printf("%d", swait[i]);
        if (i < 159) printf(", ");
    }
    printf("};\n");
    printf("const char ms_sprite_hm[160] = {");
    for (i = 0; i != 160; i++) {
        //printf("%d", hm[i]);
        printf("0x%02x", (unsigned int)hmv[hm[i]] << 4);
        if (i < 159) printf(", ");
    }
    printf("};\n");
}
