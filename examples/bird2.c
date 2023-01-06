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
*GRP0 = 0xF0;

strobe(WSYNC);
strobe(HMOVE);
START
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
*GRP0 = 0x80;
*GRP1 = 0x07;
*CTRLPF = 0x30;
*HMBL = 0x40;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP1 = YELLOW;
*GRP0 = 0x84;
*GRP1 = 0x70;
*HMBL = 0xE0;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = ORANGE;
*HMP0 = 0xD0;
*GRP0 = 0x1F;
*GRP1 = 0xF8;
*HMBL = 0xF0;

asm("nop");
strobe(HMOVE);
START
*GRP0 = 0x10;
*HMP0 = 0;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x0E;
*GRP1 = 0xF0;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = RED;
*GRP0 = 0x70;
*GRP1 = 0xC0;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*HMP0 = 0x10;
*GRP0 = 0xFC;
*GRP1 = 0;
*HMBL = 0x20;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x78;
*HMP0 = 0;
*ENABL = 0;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0;
*HMP0 = 0x20;
*COLUP1 = DGREEN;
*COLUP0 = GREEN;

strobe(WSYNC);
strobe(HMOVE);
START
*HMP0 = 0;
//*CTRLPF = 0x20;
