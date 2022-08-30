#ifndef __PSXMEMORY_H__
#define __PSXMEMORY_H__
#ifdef __cplusplus
#include "../utils/common.h"
extern "C" {
#endif

extern PCSX *g_pcsx;
#define _psxMem g_pcsx->psxMem


class MEM {
    int writeok = 1;

  public:
    PCSX *pcsx;

    s8  *psxM       = NULL;
    s8  *psxP       = NULL;
    s8  *psxR       = NULL;
    s8  *psxH       = NULL;
    u8 **psxMemWLUT = NULL;
    u8 **psxMemRLUT = NULL;

  public:
    MEM(PCSX *_pcsx);

    int  psxMemInit();
    void psxMemReset();
    void psxMemShutdown();
    u8   psxMemRead8(u32 mem);
    u16  psxMemRead16(u32 mem);
    u32  psxMemRead32(u32 mem);
    void psxMemWrite8(u32 mem, u8 value);
    void psxMemWrite16(u32 mem, u16 value);
    void psxMemWrite32(u32 mem, u32 value);
};


#define SWAP16(b)  (b)
#define SWAP32(b)  (b)
#define SWAPu16(b) (b)
#define SWAPu32(b) (b)

#define psxMs8(mem)     _psxMem->psxM[(mem)&0x1fffff]
#define psxMs16(mem)    (SWAP16(*(s16 *)&_psxMem->psxM[(mem)&0x1fffff]))
#define psxMs32(mem)    (SWAP32(*(s32 *)&_psxMem->psxM[(mem)&0x1fffff]))
#define psxMu8(mem)     (*(u8 *)&_psxMem->psxM[(mem)&0x1fffff])
#define psxMu16(mem)    (SWAP16(*(u16 *)&_psxMem->psxM[(mem)&0x1fffff]))
#define psxMu32(mem)    (SWAP32(*(u32 *)&_psxMem->psxM[(mem)&0x1fffff]))
#define psxMs8ref(mem)  _psxMem->psxM[(mem)&0x1fffff]
#define psxMs16ref(mem) (*(s16 *)&_psxMem->psxM[(mem)&0x1fffff])
#define psxMs32ref(mem) (*(s32 *)&_psxMem->psxM[(mem)&0x1fffff])
#define psxMu8ref(mem)  (*(u8 *)&_psxMem->psxM[(mem)&0x1fffff])
#define psxMu16ref(mem) (*(u16 *)&_psxMem->psxM[(mem)&0x1fffff])
#define psxMu32ref(mem) (*(u32 *)&_psxMem->psxM[(mem)&0x1fffff])

#define psxPs8(mem)     _psxMem->psxP[(mem)&0xffff]
#define psxPs16(mem)    (SWAP16(*(s16 *)&_psxMem->psxP[(mem)&0xffff]))
#define psxPs32(mem)    (SWAP32(*(s32 *)&_psxMem->psxP[(mem)&0xffff]))
#define psxPu8(mem)     (*(u8 *)&_psxMem->psxP[(mem)&0xffff])
#define psxPu16(mem)    (SWAP16(*(u16 *)&_psxMem->psxP[(mem)&0xffff]))
#define psxPu32(mem)    (SWAP32(*(u32 *)&_psxMem->psxP[(mem)&0xffff]))
#define psxPs8ref(mem)  _psxMem->psxP[(mem)&0xffff]
#define psxPs16ref(mem) (*(s16 *)&_psxMem->psxP[(mem)&0xffff])
#define psxPs32ref(mem) (*(s32 *)&_psxMem->psxP[(mem)&0xffff])
#define psxPu8ref(mem)  (*(u8 *)&_psxMem->psxP[(mem)&0xffff])
#define psxPu16ref(mem) (*(u16 *)&_psxMem->psxP[(mem)&0xffff])
#define psxPu32ref(mem) (*(u32 *)&_psxMem->psxP[(mem)&0xffff])

#define psxRs8(mem)     _psxMem->psxR[(mem)&0x7ffff]
#define psxRs16(mem)    (SWAP16(*(s16 *)&_psxMem->psxR[(mem)&0x7ffff]))
#define psxRs32(mem)    (SWAP32(*(s32 *)&_psxMem->psxR[(mem)&0x7ffff]))
#define psxRu8(mem)     (*(u8 *)&_psxMem->psxR[(mem)&0x7ffff])
#define psxRu16(mem)    (SWAP16(*(u16 *)&_psxMem->psxR[(mem)&0x7ffff]))
#define psxRu32(mem)    (SWAP32(*(u32 *)&_psxMem->psxR[(mem)&0x7ffff]))
#define psxRs8ref(mem)  _psxMem->psxR[(mem)&0x7ffff]
#define psxRs16ref(mem) (*(s16 *)&_psxMem->psxR[(mem)&0x7ffff])
#define psxRs32ref(mem) (*(s32 *)&_psxMem->psxR[(mem)&0x7ffff])
#define psxRu8ref(mem)  (*(u8 *)&_psxMem->psxR[(mem)&0x7ffff])
#define psxRu16ref(mem) (*(u16 *)&_psxMem->psxR[(mem)&0x7ffff])
#define psxRu32ref(mem) (*(u32 *)&_psxMem->psxR[(mem)&0x7ffff])

#define psxHs8(mem)     _psxMem->psxH[(mem)&0xffff]
#define psxHs16(mem)    (SWAP16(*(s16 *)&_psxMem->psxH[(mem)&0xffff]))
#define psxHs32(mem)    (SWAP32(*(s32 *)&_psxMem->psxH[(mem)&0xffff]))
#define psxHu8(mem)     (*(u8 *)&_psxMem->psxH[(mem)&0xffff])
#define psxHu16(mem)    (SWAP16(*(u16 *)&_psxMem->psxH[(mem)&0xffff]))
#define psxHu32(mem)    (SWAP32(*(u32 *)&_psxMem->psxH[(mem)&0xffff]))
#define psxHs8ref(mem)  _psxMem->psxH[(mem)&0xffff]
#define psxHs16ref(mem) (*(s16 *)&_psxMem->psxH[(mem)&0xffff])
#define psxHs32ref(mem) (*(s32 *)&_psxMem->psxH[(mem)&0xffff])
#define psxHu8ref(mem)  (*(u8 *)&_psxMem->psxH[(mem)&0xffff])
#define psxHu16ref(mem) (*(u16 *)&_psxMem->psxH[(mem)&0xffff])
#define psxHu32ref(mem) (*(u32 *)&_psxMem->psxH[(mem)&0xffff])

#define PSXM(mem)                                                                                                      \
    (_psxMem->psxMemRLUT[(mem) >> 16] == 0 && printf("psxm error"),                                                    \
     (u8 *)(_psxMem->psxMemRLUT[(mem) >> 16] + ((mem)&0xffff)))


#define PSXMs8(mem)     (*(s8 *)PSXM(mem))
#define PSXMs16(mem)    (SWAP16(*(s16 *)PSXM(mem)))
#define PSXMs32(mem)    (SWAP32(*(s32 *)PSXM(mem)))
#define PSXMu8(mem)     (*(u8 *)PSXM(mem))
#define PSXMu16(mem)    (SWAP16(*(u16 *)PSXM(mem)))
#define PSXMu32(mem)    (SWAP32(*(u32 *)PSXM(mem)))
#define PSXMu32ref(mem) (*(u32 *)PSXM(mem))

#if !defined(PSXREC) && (defined(__x86_64__) || defined(__i386__) || defined(__ppc__)) && !defined(NOPSXREC)
#define PSXREC
#endif

#ifdef __cplusplus
}
#endif
#endif
