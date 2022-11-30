#include "vcs.h"

unsigned char X;

void main()
{
  asm("SEI");
  X = 1;
  while (X == 1);
}
