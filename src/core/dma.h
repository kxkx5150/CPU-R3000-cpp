#ifndef __PSXDMA_H__
#define __PSXDMA_H__
#ifdef __cplusplus
#include "../utils/common.h"
#include "r3000a.h"
#include "hw.h"
#include "mem.h"

extern "C" {
#endif


#define GPUDMA_INT(eCycle)                                                                                             \
    {                                                                                                                  \
        pcsx->psxCpu->psxRegs.interrupt |= 0x01000000;                                                                 \
        pcsx->psxCpu->psxRegs.intCycle[3 + 24 + 1] = eCycle;                                                           \
        pcsx->psxCpu->psxRegs.intCycle[3 + 24]     = pcsx->psxCpu->psxRegs.cycle;                                      \
    }
#define SPUDMA_INT(eCycle)                                                                                             \
    {                                                                                                                  \
        pcsx->psxCpu->psxRegs.interrupt |= 0x04000000;                                                                 \
        pcsx->psxCpu->psxRegs.intCycle[1 + 24 + 1] = eCycle;                                                           \
        pcsx->psxCpu->psxRegs.intCycle[1 + 24]     = pcsx->psxCpu->psxRegs.cycle;                                      \
    }
#define MDECOUTDMA_INT(eCycle)                                                                                         \
    {                                                                                                                  \
        pcsx->psxCpu->psxRegs.interrupt |= 0x02000000;                                                                 \
        pcsx->psxCpu->psxRegs.intCycle[5 + 24 + 1] = eCycle;                                                           \
        pcsx->psxCpu->psxRegs.intCycle[5 + 24]     = pcsx->psxCpu->psxRegs.cycle;                                      \
    }

class DMA {
  public:
    PCSX *pcsx = nullptr;

  public:
    DMA(PCSX *_pcsx);

    void _psxDma0(u32 madr, u32 bcr, u32 chcr);
    void _psxDma1(u32 madr, u32 bcr, u32 chcr);
    void _psxDma2(u32 madr, u32 bcr, u32 chcr);
    void _psxDma3(u32 madr, u32 bcr, u32 chcr);
    void _psxDma4(u32 madr, u32 bcr, u32 chcr);
    void _psxDma5(u32 madr, u32 bcr, u32 chcr);
    void _psxDma6(u32 madr, u32 bcr, u32 chcr);

    void gpuInterrupt();
    void spuInterrupt();
};

#ifdef __cplusplus
}
#endif
#endif
