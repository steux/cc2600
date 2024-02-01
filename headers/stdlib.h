/*
    stdlib.h: A common header for the Atari 2600
    Copyleft 2024 Bruno STEUX 

    This file is distributed as a companion file to cc2600 - a subset of C compiler for the Atari 2600
*/

#ifndef __STDLIB_H__
#define __STDLIB_H__

#define NULL 0

short _libc_tmpshort;
char *_libc_tmpptr;

#define itoa(val,str,radix) \
    _libc_tmpshort = val; \
    _libc_tmpptr = str; \
    _libc_itoa(); \
    _libc_tmpptr

void _libc_itoa()
{
    char _save_x, _save_y, c;
    _save_x = X;
    _save_y = Y;
    Y = 0;
    X = 0;
    if (_libc_tmpshort < 0) {
        _libc_tmpptr[Y++] = '-';
        _libc_tmpshort = -_libc_tmpshort;
    }
    if (_libc_tmpshort >= 10000) {
        X = '0';
        do {
            X++;
            _libc_tmpshort -= 10000;
        } while (_libc_tmpshort >= 0);
        X--;
        _libc_tmpshort += 10000;
        _libc_tmpptr[Y++] = X;
    }
    if (X || _libc_tmpshort >= 1000) {
        X = '0';
        if (_libc_tmpshort >= 1000) {
            do {
                X++;
                _libc_tmpshort -= 1000;
            } while (_libc_tmpshort >= 0);
            X--;
            _libc_tmpshort += 1000;
        }
        _libc_tmpptr[Y++] = X;
    }
    if (X || _libc_tmpshort >= 100) {
        X = '0';
        if (_libc_tmpshort >= 100) {
            do {
                X++;
                _libc_tmpshort -= 100;
            } while (_libc_tmpshort >= 0);
            X--;
            _libc_tmpshort += 100;
        }
        _libc_tmpptr[Y++] = X;
    }
    c = _libc_tmpshort;
    if (X || c >= 10) {
        X = '0';
        if (c >= 10) {
            do {
                X++;
                c -= 10;
            } while (c >= 0);
            X--;
            c += 10;
        }
        _libc_tmpptr[Y++] = X;
    }
    _libc_tmpptr[Y++] = '0' + c;
    _libc_tmpptr[Y] = 0;
    X = _save_x;
    Y = _save_y;
}

#endif // __STDLIB_H__

