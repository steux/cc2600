strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = RED;
*GRP0 = 0x1C;
*COLUP1 = WHITE;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x38;
*GRP1 = 0x04;
*ENABL = 2;
*HMBL = 0xD0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x70;
*GRP1 = 0x0C;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP1 = 0x0E;
*HMBL = 0xF0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP1 = 0x0A;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x10;
*CTRLPF = 0x30;
*HMBL = 0x40;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = YELLOW;
*GRP0 = 0xE0;
*GRP1 = 0x0B;
*HMBL = 0xF0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP1 = 0x0F;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0xF0;
*GRP1 = 0x38;
*HMP1 = 0xD0;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP1 = RED;
*GRP1 = 0x20;
*HMBL = 0xE0;
*HMP1 = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP1 = ORANGE;
*GRP0 = 0x70;
*GRP1 = 0x1F;
*HMBL = 0xF0;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = RED;
*GRP0 = 0x78;
*GRP1 = 0x10;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x7C;
*GRP1 = 0x0E;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x3E;
*GRP1 = 0;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x3F;
*ENABL = 0;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x1E;
*COLUP1 = DGREEN;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0;
*HMP1 = 0x30;
*HMBL = 0x20;
*COLUP0 = GREEN;

strobe(WSYNC);
strobe(HMOVE);
START
*HMP1 = 0;
*HMBL = 0;
//*CTRLPF = 0x20;
