
#ifndef __PSXHW_H__
#define __PSXHW_H__

#ifdef __cplusplus
#include "sio.h"
#include "counters.h"
extern "C" {
#endif

#define HW_DMA0_MADR (psxHu32ref(0x1080))
#define HW_DMA0_BCR  (psxHu32ref(0x1084))
#define HW_DMA0_CHCR (psxHu32ref(0x1088))

#define HW_DMA1_MADR (psxHu32ref(0x1090))
#define HW_DMA1_BCR  (psxHu32ref(0x1094))
#define HW_DMA1_CHCR (psxHu32ref(0x1098))

#define HW_DMA2_MADR (psxHu32ref(0x10a0))
#define HW_DMA2_BCR  (psxHu32ref(0x10a4))
#define HW_DMA2_CHCR (psxHu32ref(0x10a8))

#define HW_DMA3_MADR (psxHu32ref(0x10b0))
#define HW_DMA3_BCR  (psxHu32ref(0x10b4))
#define HW_DMA3_CHCR (psxHu32ref(0x10b8))

#define HW_DMA4_MADR (psxHu32ref(0x10c0))
#define HW_DMA4_BCR  (psxHu32ref(0x10c4))
#define HW_DMA4_CHCR (psxHu32ref(0x10c8))

#define HW_DMA6_MADR (psxHu32ref(0x10e0))
#define HW_DMA6_BCR  (psxHu32ref(0x10e4))
#define HW_DMA6_CHCR (psxHu32ref(0x10e8))

#define HW_DMA_PCR (psxHu32ref(0x10f0))
#define HW_DMA_ICR (psxHu32ref(0x10f4))

#define DMA_INTERRUPT(n)                                                                                               \
    if (SWAPu32(HW_DMA_ICR) & (1 << (16 + n))) {                                                                       \
        HW_DMA_ICR |= SWAP32(1 << (24 + n));                                                                           \
        psxHu32ref(0x1070) |= SWAP32(8);                                                                               \
    }

class HW {
  public:
    PCSX *pcsx = nullptr;

  public:
    HW(PCSX *_pcsx);

    void psxHwReset();
    u8   psxHwRead8(u32 add);
    u16  psxHwRead16(u32 add);
    u32  psxHwRead32(u32 add);
    void psxHwWrite8(u32 add, u8 value);
    void psxHwWrite16(u32 add, u16 value);
    void psxHwWrite32(u32 add, u32 value);
    int  psxHwFreeze(gzFile f, int Mode);
};


#ifdef __cplusplus
}
#endif
#endif