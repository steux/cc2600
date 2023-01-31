#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main()
{
    char buffer[256];
    int frequencies[256] = {0};
    int nb_frequencies = 0;
    int i, j, k, started = 0;

    while (fgets(buffer, 256, stdin)) {
        if (strstr(buffer, ".freq_table_start") != NULL) {
            started = 1;
        } else if (started) {
            if (strstr(buffer, "= (* & $3ff)/4") != NULL) {
                char *tune = strtok(buffer, "=");
                if (tune) {
                    nb_frequencies++;
                    printf("#define %s %d\n", tune, nb_frequencies);
                    fgets(buffer, 256, stdin);
                    unsigned int f = 0;
                    sscanf(buffer, " DC.L %u", &f);
                    frequencies[nb_frequencies] = f;
                }
            }
        }
    }
    
    printf("\nconst frequency unsigned char frequencies[%d] = {\n\t", (nb_frequencies + 1) * 4);
    for (i = 0; i < nb_frequencies; i++) {
        unsigned int k = frequencies[i];
        for (j = 0; j != 4; j++) {
            printf("0x%02x, ", k & 0xff);
            k >>= 8;
        }
        if (!((i + 1) % 4)) printf("\n\t"); 
    }
    printf("0, 0, 0, 0 };\n");
}
