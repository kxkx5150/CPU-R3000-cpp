#ifndef __PSXCOUNTERS_H__
#define __PSXCOUNTERS_H__

#ifdef __cplusplus
#include "../utils/common.h"
#include "r3000a.h"
#include "mem.h"
#include "plugins.h"
extern "C" {
#endif
#define CounterQuantity (4)


typedef struct Rcnt
{
    u16 mode, target;
    u32 rate, irq, counterState, irqState;
    u32 cycle, cycleStart;
} Rcnt;


class Counter {
  public:
    const u32 CountToOverflow   = 0;
    const u32 CountToTarget     = 1;
    const u32 FrameRate[2]      = {60, 50};
    const u32 VBlankStart[2]    = {240, 256};
    const u32 HSyncTotal[2]     = {262, 312};
    const u32 SpuUpdInterval[2] = {23, 22};
    const s32 VerboseLevel      = 0;

    PCSX *pcsx = nullptr;

    Rcnt rcnts[CounterQuantity];

    u32 hSyncCount     = 0;
    u32 spuSyncCount   = 0;
    u32 psxNextCounter = 0, psxNextsCounter = 0;

  public:
    Counter(PCSX *_pcsx);

    void setIrq(u32 irq);
    void verboseLog(s32 level, const char *str, ...);
    void _psxRcntWcount(u32 index, u32 value);
    u32  _psxRcntRcount(u32 index);
    void psxRcntSet();
    void psxRcntReset(u32 index);
    void psxRcntUpdate();
    void psxRcntWcount(u32 index, u32 value);
    void psxRcntWmode(u32 index, u32 value);
    void psxRcntWtarget(u32 index, u32 value);
    u32  psxRcntRcount(u32 index);
    u32  psxRcntRmode(u32 index);
    u32  psxRcntRtarget(u32 index);
    void psxRcntInit();
    s32  psxRcntFreeze(gzFile f, s32 Mode);
};


#ifdef __cplusplus
}
#endif
#endif
