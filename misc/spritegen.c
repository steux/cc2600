#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char graphics[256];
unsigned char colors[256];
int nb_sprites = 0;
int nb_lines = 0;

void print_sprite(int reversed)
{
    int i;
    if (nb_lines != 0) {
        printf("const unsigned char sprite%d[%d] = { ", nb_sprites, nb_lines);
        for (i = 0; i != nb_lines; i++) {
            printf("0x%02x", (reversed)?graphics[nb_lines - 1 - i]:graphics[i]);
            if (i != nb_lines - 1) printf(", ");
        }
        printf("};\n");
        printf("const unsigned char colors%d[%d] = { ", nb_sprites, nb_lines);
        for (i = 0; i != nb_lines; i++) {
            printf("0x%02x", (reversed)?colors[nb_lines - 1 - i]:colors[i]);
            if (i != nb_lines - 1) printf(", ");
        }
        printf("};\n");
    }
}

void main(int argc, char *argv[])
{
    char buffer[256];
    int i, j;
    unsigned char *ptr;
    int reversed = 0;

    if (argc > 1 && strstr(argv[1], "rev")) reversed = 1;

    while (fgets(buffer, 256, stdin)) {
        if (sscanf(buffer, "Frame%d", &i) == 1) {
            print_sprite(reversed);
            nb_lines = 0;
            nb_sprites = i;
        } else if (strstr(buffer, ";---End Graphics Data---")) {
            break; 
        } else  {
            if ((ptr = strstr(buffer, ".byte #%")) != NULL) {
                unsigned char *ptr2 = ptr + 8;
                unsigned char byte = 0;
                for (i = 0; i != 8; i++, ptr2++) {
                    byte <<= 1;
                    if (*ptr2 == '1') byte |= 1;
                }
                graphics[nb_lines] = byte;
                ptr2 += 2;
                colors[nb_lines] = strtol(ptr2, NULL, 16);
                nb_lines++;
            }
        }
    }
    print_sprite(reversed);
}
