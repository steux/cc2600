/*
    cc2600 Minikernel Library 
    Copyright (C) 2024 Bruno STEUX 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    Contact info: bruno.steux@gmail.com
*/

// v0.1: Initial version

#ifndef __MINIKERNEL_H__
#define __MINIKERNEL_H__

#include "vcs.h"

#ifndef MK_BANK
#define MK_BANK
#endif

char *mk_s0, *mk_s1, *mk_s2, *mk_s3, *mk_s4, *mk_s5;

#ifdef MK_ARMY_FONT
MK_BANK aligned(256) const char _mk_digits_font[10 * 7] = {
    0x28, 0x6c, 0xc6, 0xc6, 0xc6, 0x6c, 0x28, 0x7a, 0x18, 0x18, 0x18, 0x18, 0x38, 0x18, 
    0xee, 0x60, 0x28, 0x0c, 0x06, 0xc6, 0x6c, 0x6c, 0xc6, 0x06, 0x2c, 0x08, 0x0c, 0x6e, 
    0x0c, 0x0c, 0xee, 0xcc, 0x6c, 0x2c, 0x0c, 0x6c, 0xc6, 0x06, 0x06, 0xec, 0xc0, 0xee, 
    0x6c, 0xc6, 0xc6, 0xec, 0xc0, 0x60, 0x2e, 0x30, 0x30, 0x30, 0x10, 0x84, 0x86, 0xf6, 
    0x6c, 0x86, 0x8e, 0x68, 0xe4, 0xc4, 0x68, 0x68, 0x0c, 0x06, 0x6e, 0xc6, 0xc6, 0x6c 
};

MK_BANK const char *_mk_digits[10] = { 
    _mk_digits_font + 0,
    _mk_digits_font + 7,
    _mk_digits_font + 14,
    _mk_digits_font + 21,
    _mk_digits_font + 28,
    _mk_digits_font + 35,
    _mk_digits_font + 42,
    _mk_digits_font + 49,
    _mk_digits_font + 56,
    _mk_digits_font + 63
};
#else
MK_BANK const char *_mk_digits[1] = { 0 };
#endif

MK_BANK void mini_kernel_update_score_4_digits(int score)
{
    char c;
    if (score >= 10000) {
        mk_s2 = _mk_digits[9];
        mk_s3 = _mk_digits[9];
        mk_s4 = _mk_digits[9];
        mk_s5 = _mk_digits[9];
        return;
    }
    X = 0;
    if (score >= 1000) {
        do {
            X++;
            score -= 1000;
        } while (score >= 0);
        X--;
        score += 1000;
    }
    mk_s2 = _mk_digits[X];
    X = 0;
    if (score >= 100) {
        do {
            X++;
            score -= 100;
        } while (score >= 0);
        X--;
        score += 100;
    }
    mk_s3 = _mk_digits[X];
    c = score;
    X = 0;
    if (c >= 10) {
        do {
            X++;
            c -= 10;
        } while (c >= 0);
        X--;
        c += 10;
    }
    mk_s4 = _mk_digits[X];
    mk_s5 = _mk_digits[X = c];
}

MK_BANK void mini_kernel_6_sprites()
{
    char i, j;
    strobe(WSYNC);
    
    *COLUBK = 0;
    *GRP0 = 0;
    *GRP1 = 0;
    *REFP0 = 0;
    *REFP1 = 0;
    *HMP1 = 0x90; 
    *HMP0 = 0x80;
    csleep(7);
    strobe(RESP0);
    strobe(RESP1);
    strobe(WSYNC);
    
    strobe(HMOVE); // 3
    *VDELP0 = 1;
    *VDELP1 = 1;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    Y = 6;
    csleep(7);
    do {
        *GRP0 = mk_s0[Y];
        strobe(WSYNC);
        *GRP1 = mk_s1[Y];
        i = Y;          // 3
        *GRP0 = mk_s2[Y];
        X = mk_s4[Y];
        j = mk_s5[Y];
        load(mk_s3[Y]);
        Y = j;
        store(*GRP1);
        *GRP0 = X;
        *GRP1 = Y;
        strobe(GRP0);
        Y = i;
        Y--;
    } while (Y >= 0);
    
    *GRP0 = 0;
    *GRP1 = 0;
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
}

MK_BANK void mini_kernel_position_sprites_center()
{
    strobe(WSYNC);
    
    *COLUBK = 0;
    *GRP0 = 0;
    *GRP1 = 0;
    *REFP0 = 0;
    *REFP1 = 0;
    *HMP1 = 0x90; 
    *HMP0 = 0x80;
    csleep(7);
    strobe(RESP0);
    strobe(RESP1);
    strobe(WSYNC);
    
    strobe(HMOVE); // 3
    *VDELP0 = 1;
    *VDELP1 = 1;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
}

MK_BANK void mini_kernel_display_text(char *line, char sizey)
{
    char i, j;
    strobe(WSYNC);
    i = line;
    j = line >> 8;
    mk_s0 = i | (j << 8);
    i += sizey;
    mk_s1 = i | (j << 8);
    i += sizey;
    mk_s2 = i | (j << 8);
    i += sizey;
    mk_s3 = i | (j << 8);
    i += sizey;
    mk_s4 = i | (j << 8);
    i += sizey;
    mk_s5 = i | (j << 8);

    strobe(WSYNC);
    Y = sizey;
    Y--;
    csleep(7);
    do {
        *GRP0 = mk_s0[Y];
        strobe(WSYNC);
        *GRP1 = mk_s1[Y];
        i = Y;          // 3
        *GRP0 = mk_s2[Y];
        X = mk_s4[Y];
        j = mk_s5[Y];
        load(mk_s3[Y]);
        Y = j;
        store(*GRP1);
        *GRP0 = X;
        *GRP1 = Y;
        strobe(GRP0);
        Y = i;
        Y--;
    } while (Y >= 0);
    
    *GRP0 = 0;
    *GRP1 = 0;
    *GRP0 = 0;
}

#endif // __MINIKERNEL_H__
