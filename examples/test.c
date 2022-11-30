#include "vcs.h"

unsigned char X, Y;

const unsigned char RED = 0x34;
const unsigned char BLUE = 0x84;
const unsigned char BLACK = 0x00;

void main()
{
  *COLUBK = BLUE;
  *COLUPF = BLACK;
  strobe(VSYNC);
  while(1);
}
