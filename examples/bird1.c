strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = RED; 
*COLUP1 = WHITE; 
*GRP0 = 0x1C;
*HMBL = 0xD0; // +3 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x38;
*GRP1 = 0x04;
*ENABL = 2;
*HMBL = 0xF0; // + 1 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x70;
*GRP1 = 0x0c;
*HMBL = 0x00;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP1 = 0x0e;
*HMBL = 0xF0; // +1 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*GRP1 = 0x0a;
*HMBL = 0x00;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x10;
*CTRLPF = 0x30;
*HMBL = 0x40; // -4 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0xe0;
*COLUP0 = YELLOW;
*GRP1 = 0x0b; 
*HMBL = 0xF0; // +1 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*HMBL = 0x00;
*GRP1 = 0x0f;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0xF0;
*GRP1 = 0x38;
*HMP1 = 0xD0; // P1 : + 3

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP1 = RED;
*HMP1 = 0x00;
*GRP1 = 0x20;
*HMBL = 0xe0; // +2 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x70;
*GRP1 = 0x1f;
*COLUP1 = ORANGE;
*HMBL = 0xf0; // + 1 BALL

strobe(WSYNC);
strobe(HMOVE);
START
*HMBL = 0x10; // -1 BALL
*GRP0 = 0x78;
*GRP1 = 0x10;
*COLUP0 = RED;

strobe(WSYNC);
strobe(HMOVE);
START
*HMBL = 0x00;
*GRP0 = 0x7c;
*GRP1 = 0x0e;

strobe(WSYNC);
strobe(HMOVE);
START
*HMBL = 0x10; // -1 BALL
*GRP1 = 0;
*GRP0 = 0x3e;

strobe(WSYNC);
strobe(HMOVE);
START
*HMBL = 0x30; // -3 BALL
*ENABL = 0;
*GRP0 = 0x3f;
*HMP1 = 0x30; // P1: - 3

strobe(WSYNC);
strobe(HMOVE);
START
*HMBL = 0;
*GRP0 = 0x1e;
*HMP1 = 0;
*CTRLPF = 0x20; // Ball back to 4 pixels

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0;
