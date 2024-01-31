#include <stdio.h>

void main()
{
    unsigned char array[2048];
    int i, size = 0;
    getchar();
    while (!feof(stdin) && size < 2048) {
        array[size++] = getchar();
    }
    printf("const char font[%d]={\n\t", size);
    for (i = 0; i != size; i++) {
        int j = (i / 8) * 8;
        int k = j + (7 - (i - j));
        printf("0x%02x", (int)array[k]);
        if (i != size - 1) {
            printf(", ");
            if (!((i + 1) % 16)) printf("\n\t");
        }
    }
    printf("\n};\n");
}

