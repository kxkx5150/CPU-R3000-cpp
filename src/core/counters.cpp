#include "counters.h"
#include "../plugins/dfsound/spu.h"
#include "../plugins/dfxvideo/gpu.h"

enum
{
    Rc0Gate           = 0x0001,
    Rc1Gate           = 0x0001,
    Rc2Disable        = 0x0001,
    RcUnknown1        = 0x0002,
    RcUnknown2        = 0x0004,
    RcCountToTarget   = 0x0008,
    RcIrqOnTarget     = 0x0010,
    RcIrqOnOverflow   = 0x0020,
    RcIrqRegenerate   = 0x0040,
    RcUnknown7        = 0x0080,
    Rc0PixelClock     = 0x0100,
    Rc1HSyncClock     = 0x0100,
    Rc2Unknown8       = 0x0100,
    Rc0Unknown9       = 0x0200,
    Rc1Unknown9       = 0x0200,
    Rc2OneEighthClock = 0x0200,
    RcUnknown10       = 0x0400,
    RcCountEqTarget   = 0x0800,
    RcOverflow        = 0x1000,
    RcUnknown13       = 0x2000,
    RcUnknown14       = 0x4000,
    RcUnknown15       = 0x8000,
};


Counter::Counter(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void Counter::setIrq(u32 irq)
{
    psxHu32ref(0x1070) |= SWAPu32(irq);
}
void Counter::verboseLog(s32 level, const char *str, ...)
{
    if (level <= VerboseLevel) {
        va_list va;
        va_start(va, str);
        vprintf(str, va);
        va_end(va);
        fflush(stdout);
    }
}
void Counter::_psxRcntWcount(u32 index, u32 value)
{
    if (value > 0xffff) {
        verboseLog(1, "[RCNT %i] wcount > 0xffff: %x\n", index, value);
        value &= 0xffff;
    }
    rcnts[index].cycleStart = pcsx->psxCpu->psxRegs.cycle;
    rcnts[index].cycleStart -= value * rcnts[index].rate;
    if (value < rcnts[index].target) {
        rcnts[index].cycle        = rcnts[index].target * rcnts[index].rate;
        rcnts[index].counterState = CountToTarget;
    } else {
        rcnts[index].cycle        = 0xffff * rcnts[index].rate;
        rcnts[index].counterState = CountToOverflow;
    }
}
u32 Counter::_psxRcntRcount(u32 index)
{
    u32 count;
    count = pcsx->psxCpu->psxRegs.cycle;
    count -= rcnts[index].cycleStart;
    count /= rcnts[index].rate;
    if (count > 0xffff) {
        verboseLog(1, "[RCNT %i] rcount > 0xffff: %x\n", index, count);
        count &= 0xffff;
    }
    return count;
}
void Counter::psxRcntSet()
{
    s32 countToUpdate;
    u32 i;
    psxNextsCounter = pcsx->psxCpu->psxRegs.cycle;
    psxNextCounter  = 0x7fffffff;
    for (i = 0; i < CounterQuantity; ++i) {
        countToUpdate = rcnts[i].cycle - (psxNextsCounter - rcnts[i].cycleStart);
        if (countToUpdate < 0) {
            psxNextCounter = 0;
            break;
        }
        if (countToUpdate < (s32)psxNextCounter) {
            psxNextCounter = countToUpdate;
        }
    }
}
void Counter::psxRcntReset(u32 index)
{
    u32 count;
    if (rcnts[index].counterState == CountToTarget) {
        if (rcnts[index].mode & RcCountToTarget) {
            count = pcsx->psxCpu->psxRegs.cycle;
            count -= rcnts[index].cycleStart;
            count /= rcnts[index].rate;
            count -= rcnts[index].target;
        } else {
            count = _psxRcntRcount(index);
        }
        _psxRcntWcount(index, count);
        if (rcnts[index].mode & RcIrqOnTarget) {
            if ((rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState)) {
                verboseLog(3, "[RCNT %i] irq: %x\n", index, count);
                setIrq(rcnts[index].irq);
                rcnts[index].irqState = 1;
            }
        }
        rcnts[index].mode |= RcCountEqTarget;
    } else if (rcnts[index].counterState == CountToOverflow) {
        count = pcsx->psxCpu->psxRegs.cycle;
        count -= rcnts[index].cycleStart;
        count /= rcnts[index].rate;
        count -= 0xffff;
        _psxRcntWcount(index, count);
        if (rcnts[index].mode & RcIrqOnOverflow) {
            if ((rcnts[index].mode & RcIrqRegenerate) || (!rcnts[index].irqState)) {
                verboseLog(3, "[RCNT %i] irq: %x\n", index, count);
                setIrq(rcnts[index].irq);
                rcnts[index].irqState = 1;
            }
        }
        rcnts[index].mode |= RcOverflow;
    }
    rcnts[index].mode |= RcUnknown10;
    psxRcntSet();
}
void Counter::psxRcntUpdate()
{
    u32 cycle;
    cycle = pcsx->psxCpu->psxRegs.cycle;
    if (cycle - rcnts[0].cycleStart >= rcnts[0].cycle) {
        psxRcntReset(0);
    }
    if (cycle - rcnts[1].cycleStart >= rcnts[1].cycle) {
        psxRcntReset(1);
    }
    if (cycle - rcnts[2].cycleStart >= rcnts[2].cycle) {
        psxRcntReset(2);
    }
    if (cycle - rcnts[3].cycleStart >= rcnts[3].cycle) {
        psxRcntReset(3);
        spuSyncCount++;
        hSyncCount++;
        if (spuSyncCount >= SpuUpdInterval[Config.PsxType]) {
            spuSyncCount = 0;
            SPUasync(SpuUpdInterval[Config.PsxType] * rcnts[3].target);
        }
        if (hSyncCount == VBlankStart[Config.PsxType]) {
            pcsx->psxPlugs->GPUvBlank(1);
        }
        if (hSyncCount >= (Config.VSyncWA ? HSyncTotal[Config.PsxType] / BIAS : HSyncTotal[Config.PsxType])) {
            hSyncCount = 0;
            pcsx->psxPlugs->GPUvBlank(0);
            setIrq(0x01);
            GPUupdateLace();
            EmuUpdate();
        }
    }
}
void Counter::psxRcntWcount(u32 index, u32 value)
{
    verboseLog(2, "[RCNT %i] wcount: %x\n", index, value);
    psxRcntUpdate();
    _psxRcntWcount(index, value);
    psxRcntSet();
}
void Counter::psxRcntWmode(u32 index, u32 value)
{
    verboseLog(1, "[RCNT %i] wmode: %x\n", index, value);
    psxRcntUpdate();
    rcnts[index].mode     = value;
    rcnts[index].irqState = 0;
    switch (index) {
        case 0:
            if (value & Rc0PixelClock) {
                rcnts[index].rate = 5;
            } else {
                rcnts[index].rate = 1;
            }
            break;
        case 1:
            if (value & Rc1HSyncClock) {
                rcnts[index].rate = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));
            } else {
                rcnts[index].rate = 1;
            }
            break;
        case 2:
            if (value & Rc2OneEighthClock) {
                rcnts[index].rate = 8;
            } else {
                rcnts[index].rate = 1;
            }
            if (value & Rc2Disable) {
                rcnts[index].rate = 0xffffffff;
            }
            break;
    }
    _psxRcntWcount(index, 0);
    psxRcntSet();
}
void Counter::psxRcntWtarget(u32 index, u32 value)
{
    verboseLog(1, "[RCNT %i] wtarget: %x\n", index, value);
    psxRcntUpdate();
    rcnts[index].target = value;
    _psxRcntWcount(index, _psxRcntRcount(index));
    psxRcntSet();
}
u32 Counter::psxRcntRcount(u32 index)
{
    u32 count;
    psxRcntUpdate();
    count = _psxRcntRcount(index);
    if (Config.RCntFix) {
        if (index == 2) {
            if (rcnts[index].counterState == CountToTarget) {
                count /= BIAS;
            }
        }
    }
    verboseLog(2, "[RCNT %i] rcount: %x\n", index, count);
    return count;
}
u32 Counter::psxRcntRmode(u32 index)
{
    u16 mode;
    psxRcntUpdate();
    mode = rcnts[index].mode;
    rcnts[index].mode &= 0xe7ff;
    verboseLog(2, "[RCNT %i] rmode: %x\n", index, mode);
    return mode;
}
u32 Counter::psxRcntRtarget(u32 index)
{
    verboseLog(2, "[RCNT %i] rtarget: %x\n", index, rcnts[index].target);
    return rcnts[index].target;
}
void Counter::psxRcntInit()
{
    s32 i;
    rcnts[0].rate   = 1;
    rcnts[0].irq    = 0x10;
    rcnts[1].rate   = 1;
    rcnts[1].irq    = 0x20;
    rcnts[2].rate   = 1;
    rcnts[2].irq    = 0x40;
    rcnts[3].rate   = 1;
    rcnts[3].mode   = RcCountToTarget;
    rcnts[3].target = (PSXCLK / (FrameRate[Config.PsxType] * HSyncTotal[Config.PsxType]));
    for (i = 0; i < CounterQuantity; ++i) {
        _psxRcntWcount(i, 0);
    }
    psxRcntSet();
}
s32 Counter::psxRcntFreeze(gzFile f, s32 Mode)
{
    gzfreeze(&rcnts, sizeof(rcnts));
    gzfreeze(&hSyncCount, sizeof(hSyncCount));
    gzfreeze(&spuSyncCount, sizeof(spuSyncCount));
    gzfreeze(&psxNextCounter, sizeof(psxNextCounter));
    gzfreeze(&psxNextsCounter, sizeof(psxNextsCounter));
    return 0;
}
