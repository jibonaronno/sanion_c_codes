/*
 * VT100.h
 *
 *  Created on: Apr 16, 2015
 *      Author: root
 */

#ifndef VT100_H_
#define VT100_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "global.h"

void VT100_GotoHome(void);
void VT100_scrclr(void);
void VT100_goto(U8 u8X, U8 u8Y);

void VT100_SaveCursor(void);
void VT100_RestoreCursor(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* VT100_H_ */
