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

MK_BANK void mini_kernel_6_sprites()
{
    char i, j;
    strobe(WSYNC);
    
    *COLUBK = 0;
    *GRP0 = 0;
    *GRP1 = 0;
    *NUSIZ0 = 0x33;
    *NUSIZ1 = 0x33;
    *VDELP0 = 1;
    *VDELP1 = 1;
    *HMP1 = 0xA0; 
    *HMP0 = 0x90;
    strobe(RESP0);
    strobe(RESP1);
    Y = 7;
    strobe(WSYNC);
    
    strobe(HMOVE); // 3
    *GRP0 = mk_s0[Y];
    *GRP1 = mk_s1[Y];
    *GRP0 = mk_s2[Y];
    X = mk_s4[Y];
    j = mk_s5[Y];
    load(mk_s3[Y]);
    Y = j;
    store(*GRP1);
    *GRP0 = X;
    *GRP1 = Y;
    strobe(GRP0);
    Y--;

    do {
        strobe(WSYNC);
        i = Y;          // 3
        *GRP0 = mk_s0[Y];
        *GRP1 = mk_s1[Y];
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
    
    //strobe(WSYNC);
    *VDELP0 = 0;
    *VDELP1 = 0;
    *GRP0 = 0;
    *GRP1 = 0;
    strobe(HMCLR);
}

#endif // __MINIKERNEL_H__
