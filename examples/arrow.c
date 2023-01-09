strobe(WSYNC);
strobe(HMOVE);
START
*COLUP0 = ORANGE;
*COLUP1 = RED;
*HMP0 = 0xE0;
*GRP0 = 0x71;
*HMP1 = 0xF0;
*GRP1 = 0x07;
*ENABL = 2;
*HMBL = 0x90;

strobe(WSYNC);
strobe(HMOVE);
START
*HMP0 = 0;
*HMP1 = 0;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START

strobe(WSYNC);
strobe(HMOVE);
START
*HMP0 = 0x20;
*GRP0 = 0xF0;
*HMP1 = 0xD0;
*GRP1 = 0xFF;
*HMBL = 0xE0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x78;
*HMP0 = 0;
*GRP1 = 0x7E;
*HMP1 = 0;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x3C;
*GRP1 = 0x3C;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x1E;
*GRP1 = 0x18;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x0F;
*GRP1 = 0;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0x06;
*CTRLPF = 0x00;
*HMBL = 0xE0;

strobe(WSYNC);
strobe(HMOVE);
START
*GRP0 = 0;
*HMBL = 0x10;

strobe(WSYNC);
strobe(HMOVE);
START
*ENABL = 0;
*HMBL = 0;

strobe(WSYNC);
strobe(HMOVE);
START

strobe(WSYNC);
strobe(HMOVE);
START
*HMP1 = 0x40;
*HMBL = 0x60;

strobe(WSYNC);
strobe(HMOVE);
START
*HMP1 = 0;
*HMBL = 0;
*CTRLPF = 0x20;
