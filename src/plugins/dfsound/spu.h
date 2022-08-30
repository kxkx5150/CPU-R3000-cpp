/***************************************************************************
                            spu.h  -  description
                             -------------------
    begin                : Wed May 15 2002
    copyright            : (C) 2002 by Pete Bernert
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
#ifndef __SPU_H__
#define __SPU_H__

#include <cstdint>
#ifdef __cplusplus
// #include "../../core/cdrom.h"

extern "C" {
typedef struct
{
    int32_t y0, y1;
} ADPCM_Decode_t;

typedef struct
{
    int            freq;
    int            nbits;
    int            stereo;
    int            nsamples;
    ADPCM_Decode_t left, right;
    short          pcm[16384];
} xa_decode_t;
#endif
void SetupTimer(void);
void RemoveTimer(void);
void SPUplayADPCMchannel(xa_decode_t *xap);
void SPUplayCDDAchannel(short *pcm, int bytes);
void SPUasync(unsigned long cycle);
void SPUregisterCallback(void (*callback)(void));
long SPUopen(void);
long SPUclose(void);
long SPUinit(void);
#ifdef __cplusplus
}
#endif
#endif
