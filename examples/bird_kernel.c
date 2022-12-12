bank2 void draw_bird1()
{
#include "bird1.c"
}

bank2 void kernel()
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
    draw_bird1();
    do {
        strobe(WSYNC);
        strobe(HMOVE);
        START
    } while (Y != 0); 
}
