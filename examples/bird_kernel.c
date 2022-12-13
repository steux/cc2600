bank1 void kernel()
{
    X = 0;
    Y = KERNAL - 1;
    i = Y >> 4; 
    
    // Renable output (disable VBLANK)
    strobe(WSYNC);
    strobe(HMOVE);
    *VBLANK = X;
    START; 

    do {
        strobe(WSYNC);
        strobe(HMOVE);
        START
    } while (Y != ybird >> 8);
    strobe(WSYNC);
    strobe(HMOVE);
    START
#ifdef BIRD1
#include "bird1.c"
#else
#include "bird2.c"
#endif
    do {
        strobe(WSYNC);
        strobe(HMOVE);
        START
    } while (Y != 0); 
    strobe(WSYNC);
    *COLUBK = GREEN;
    *PF0 = 0;
    *PF1 = 0;
    *PF2 = 0;
    strobe(WSYNC);
}
