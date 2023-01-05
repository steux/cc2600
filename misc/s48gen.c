#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main()
{
    int height, i = 0, j;
    char buffer[256];
    unsigned char *image = NULL, *n;
    while (fgets(buffer, 256, stdin)) {
        if ((n = strstr(buffer, "height")) != NULL) {
            height = atoi(n + 9);
            image = malloc(6 * height);
            memset(image, 6 * height, 0);
        } else if ((n = strstr(buffer, "true")) != NULL) {
            if (image && i < 48 * height) {
                image[i / 8] |= (1 << (7 - (i % 8)));
                i++;
            }
        } else if ((strstr(buffer, "false") != NULL) || (strstr(buffer, "null") != NULL)) {
            i++;
        } else if (strstr(buffer, "],") != NULL) {
            i = (((i - 1) / 48) + 1) * 48;
        }
    }
    fprintf(stderr, "Read %d pixels\n", i);
    for (i = 0; i != 6; i++) {
        printf("const unsigned char s48_%d[%d] = { ", i, height);
        for (j = 0; j != height; j++) {
            printf("0x%02x", (int)image[(height - 1 - j) * 6 + i]);
            if (j != height - 1) printf(", ");
        }
        printf(" };\n");
    }  
    free(image); 
}
