/***************************************************************************
                          gpu.h  -  description
                             -------------------
    begin                : Sun Oct 28 2001
    copyright            : (C) 2001 by Pete Bernert
    email                : BlackDove@addcom.de
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#ifndef _GPU_INTERNALS_H
#define _GPU_INTERNALS_H
#ifdef __cplusplus

extern "C" {
#endif
#include <stdint.h>
#define OPAQUEON  10
#define OPAQUEOFF 11

#define KEY_RESETTEXSTORE 1
#define KEY_SHOWFPS       2
#define KEY_RESETOPAQUE   4
#define KEY_RESETDITHER   8
#define KEY_RESETFILTER   16
#define KEY_RESETADVBLEND 32
//#define KEY_BLACKWHITE    64
#define KEY_BADTEXTURES  128
#define KEY_CHECKTHISOUT 256

#if !defined(__BIG_ENDIAN__) || defined(__x86_64__) || defined(__i386__)
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#endif

#ifdef __LITTLE_ENDIAN__
#define RED(x)   (x & 0xff)
#define BLUE(x)  ((x >> 16) & 0xff)
#define GREEN(x) ((x >> 8) & 0xff)
#define COLOR(x) (x & 0xffffff)
#elif defined __BIG_ENDIAN__
#define RED(x)   ((x >> 24) & 0xff)
#define BLUE(x)  ((x >> 8) & 0xff)
#define GREEN(x) ((x >> 16) & 0xff)
#define COLOR(x) SWAP32(x & 0xffffff)
#endif

/////////////////////////////////////////////////////////////////////////////

void     updateDisplay(void);
void     SetAutoFrameCap(void);
void     SetFixes(void);
long     GPUinit();
uint32_t GPUreadData(void);
uint32_t GPUreadStatus(void);
void     GPUwriteData(uint32_t gdata);
void     GPUwriteStatus(uint32_t gdata);
void     GPUreadDataMem(uint32_t *pMem, int iSize);
void     GPUwriteDataMem(uint32_t *pMem, int iSize);
long     GPUdmaChain(uint32_t *baseAddrL, uint32_t addr);
void     GPUupdateLace(void);
long     GPUopen(unsigned long *disp, char *CapText, char *CfgFile);
long     GPUclose();
/////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif
#endif    // _GPU_INTERNALS_H
