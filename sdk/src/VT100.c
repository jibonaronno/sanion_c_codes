/*
 * VT100.c
 *
 *  Created on: Apr 16, 2015
 *      Author: root
 */

#include <stdio.h>
#include "VT100.h"

void VT100_GotoHome(void)
{
    putchar(0x1B);
    putchar(0x5B);
    putchar('H');
}

void VT100_SaveCursor(void)
{
    putchar(0x1B);
    putchar(0x5B);
    putchar('s');
}

void VT100_RestoreCursor(void)
{
    putchar(0x1B);
    putchar(0x5B);
    putchar('u');
}

void VT100_scrclr(void)
{
    putchar(0x1B);
    putchar(0x5B);
    putchar('2');
    putchar('J');

    putchar(0x1B);
    putchar(0x5B);
    putchar('H');
}

void VT100_goto(U8 u8X, U8 u8Y)
{
    /*
     * itoa function?
     */
    U8 u8Tx, u8Ty;
    U8 u8Xx, u8Yyy;

    u8Tx = u8X;
    u8Ty = u8Y;
    u8Xx = u8Yyy = 0;
    while(u8Ty >= 10){ u8Ty -= 10; u8Yyy++; }
    while(u8Tx >= 10){ u8Tx -= 10; u8Xx++; }

    putchar(0x1B);
    putchar(0x5B);
    putchar('0' + u8Yyy);
    putchar('0' + u8Ty);
    putchar(';');
    putchar('0' + u8Xx);
    putchar('0' + u8Tx);
    putchar('H');
}



