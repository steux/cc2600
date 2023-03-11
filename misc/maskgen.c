#include <stdio.h>
#include <stdlib.h>

void main(int argc, char *argv[])
{
    int i, sprite_height = atoi(argv[1]);
    printf("const char sprite_mask[%d] = {\n\t", 192 * 2 - sprite_height);
    for (i = 0; i != 192 - sprite_height; i++) {
        printf("0, ");
    }
    printf("\n\t");
    for (i = 0; i != sprite_height; i++) {
        printf("0xff, ");
    }
    printf("\n\t");
    for (i = 0; i != 191 - sprite_height; i++) {
        printf("0, ");
    }
    printf("0\n};\n");
}


