#include "hw.h"
#include "cdrom.h"
#include "mdec.h"
#include "../plugins/dfsound/spu.h"
#include "../plugins/dfsound/registers.h"
#include "../plugins/dfxvideo/gpu.h"
#include "../plugins/dfsound/regs.h"



HW::HW(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void HW::psxHwReset()
{
    if (Config.Sio)
        psxHu32ref(0x1070) |= SWAP32(0x80);
    if (Config.SpuIrq)
        psxHu32ref(0x1070) |= SWAP32(0x200);

    memset(pcsx->psxMem->psxH, 0, 0x10000);
    pcsx->psxMdec->mdecInit();
    pcsx->psxCdrom->cdrReset();
    pcsx->psxCntr->psxRcntInit();
}
u8 HW::psxHwRead8(u32 add)
{
    unsigned char hard;
    switch (add) {
        case 0x1f801040:
            hard = pcsx->psxSio->sioRead8();
            break;
#ifdef ENABLE_SIO1API
        case 0x1f801050:
            hard = SIO1_readData8();
            break;
#endif
        case 0x1f801800:
            hard = pcsx->psxCdrom->cdrRead0();
            break;
        case 0x1f801801:
            hard = pcsx->psxCdrom->cdrRead1();
            break;
        case 0x1f801802:
            hard = pcsx->psxCdrom->cdrRead2();
            break;
        case 0x1f801803:
            hard = pcsx->psxCdrom->cdrRead3();
            break;
        default:
            hard = psxHu8(add);
            return hard;
    }
    return hard;
}
u16 HW::psxHwRead16(u32 add)
{
    unsigned short hard;
    switch (add) {
        case 0x1f801040:
            hard = pcsx->psxSio->sioRead8();
            hard |= pcsx->psxSio->sioRead8() << 8;
            return hard;
        case 0x1f801044:
            hard = pcsx->psxSio->sioReadStat16();
            return hard;
        case 0x1f801048:
            hard = pcsx->psxSio->sioReadMode16();
            return hard;
        case 0x1f80104a:
            hard = pcsx->psxSio->sioReadCtrl16();
            return hard;
        case 0x1f80104e:
            hard = pcsx->psxSio->sioReadBaud16();
            return hard;
#ifdef ENABLE_SIO1API
        case 0x1f801050:
            hard = SIO1_readData16();
            return hard;
        case 0x1f801054:
            hard = SIO1_readStat16();
            return hard;
        case 0x1f80105a:
            hard = SIO1_readCtrl16();
            return hard;
        case 0x1f80105e:
            hard = SIO1_readBaud16();
            return hard;
#endif
        case 0x1f801100:
            hard = pcsx->psxCntr->psxRcntRcount(0);
            return hard;
        case 0x1f801104:
            hard = pcsx->psxCntr->psxRcntRmode(0);
            return hard;
        case 0x1f801108:
            hard = pcsx->psxCntr->psxRcntRtarget(0);
            return hard;
        case 0x1f801110:
            hard = pcsx->psxCntr->psxRcntRcount(1);
            return hard;
        case 0x1f801114:
            hard = pcsx->psxCntr->psxRcntRmode(1);
            return hard;
        case 0x1f801118:
            hard = pcsx->psxCntr->psxRcntRtarget(1);
            return hard;
        case 0x1f801120:
            hard = pcsx->psxCntr->psxRcntRcount(2);
            return hard;
        case 0x1f801124:
            hard = pcsx->psxCntr->psxRcntRmode(2);
            return hard;
        case 0x1f801128:
            hard = pcsx->psxCntr->psxRcntRtarget(2);
            return hard;
        default:
            if (add >= 0x1f801c00 && add < 0x1f801e00) {
                hard = SPUreadRegister(add);
            } else {
                hard = psxHu16(add);
            }
            return hard;
    }
    return hard;
}
u32 HW::psxHwRead32(u32 add)
{
    u32 hard;
    switch (add) {
        case 0x1f801040:
            hard = pcsx->psxSio->sioRead8();
            hard |= pcsx->psxSio->sioRead8() << 8;
            hard |= pcsx->psxSio->sioRead8() << 16;
            hard |= pcsx->psxSio->sioRead8() << 24;
            return hard;
#ifdef ENABLE_SIO1API
        case 0x1f801050:
            hard = SIO1_readData32();
            return hard;
#endif
        case 0x1f801810:
            hard = GPUreadData();
            return hard;
        case 0x1f801814:
            hard = GPUreadStatus();
            return hard;
        case 0x1f801820:
            hard = pcsx->psxMdec->mdecRead0();
            break;
        case 0x1f801824:
            hard = pcsx->psxMdec->mdecRead1();
            break;
        case 0x1f801100:
            hard = pcsx->psxCntr->psxRcntRcount(0);
            return hard;
        case 0x1f801104:
            hard = pcsx->psxCntr->psxRcntRmode(0);
            return hard;
        case 0x1f801108:
            hard = pcsx->psxCntr->psxRcntRtarget(0);
            return hard;
        case 0x1f801110:
            hard = pcsx->psxCntr->psxRcntRcount(1);
            return hard;
        case 0x1f801114:
            hard = pcsx->psxCntr->psxRcntRmode(1);
            return hard;
        case 0x1f801118:
            hard = pcsx->psxCntr->psxRcntRtarget(1);
            return hard;
        case 0x1f801120:
            hard = pcsx->psxCntr->psxRcntRcount(2);
            return hard;
        case 0x1f801124:
            hard = pcsx->psxCntr->psxRcntRmode(2);
            return hard;
        case 0x1f801128:
            hard = pcsx->psxCntr->psxRcntRtarget(2);
            return hard;
        default:
            hard = psxHu32(add);
            return hard;
    }
    return hard;
}
void HW::psxHwWrite8(u32 add, u8 value)
{
    switch (add) {
        case 0x1f801040:
            pcsx->psxSio->sioWrite8(value);
            break;
#ifdef ENABLE_SIO1API
        case 0x1f801050:
            SIO1_writeData8(value);
            break;
#endif
        case 0x1f801800:
            pcsx->psxCdrom->cdrWrite0(value);
            break;
        case 0x1f801801:
            pcsx->psxCdrom->cdrWrite1(value);
            break;
        case 0x1f801802:
            pcsx->psxCdrom->cdrWrite2(value);
            break;
        case 0x1f801803:
            pcsx->psxCdrom->cdrWrite3(value);
            break;
        default:
            psxHu8(add) = value;
            return;
    }
    psxHu8(add) = value;
}
void HW::psxHwWrite16(u32 add, u16 value)
{
    switch (add) {
        case 0x1f801040:
            pcsx->psxSio->sioWrite8((unsigned char)value);
            pcsx->psxSio->sioWrite8((unsigned char)(value >> 8));
            return;
        case 0x1f801044:
            pcsx->psxSio->sioWriteStat16(value);
            return;
        case 0x1f801048:
            pcsx->psxSio->sioWriteMode16(value);
            return;
        case 0x1f80104a:
            pcsx->psxSio->sioWriteCtrl16(value);
            return;
        case 0x1f80104e:
            pcsx->psxSio->sioWriteBaud16(value);
            return;
#ifdef ENABLE_SIO1API
        case 0x1f801050:
            SIO1_writeData16(value);
            return;
        case 0x1f801054:
            SIO1_writeStat16(value);
            return;
        case 0x1f80105a:
            SIO1_writeCtrl16(value);
            return;
        case 0x1f80105e:
            SIO1_writeBaud16(value);
            return;
#endif
        case 0x1f801070:
            if (Config.Sio)
                psxHu16ref(0x1070) |= SWAPu16(0x80);
            if (Config.SpuIrq)
                psxHu16ref(0x1070) |= SWAPu16(0x200);
            psxHu16ref(0x1070) &= SWAPu16((psxHu16(0x1074) & value));
            return;
        case 0x1f801074:
            psxHu16ref(0x1074) = SWAPu16(value);
            return;
        case 0x1f801100:
            pcsx->psxCntr->psxRcntWcount(0, value);
            return;
        case 0x1f801104:
            pcsx->psxCntr->psxRcntWmode(0, value);
            return;
        case 0x1f801108:
            pcsx->psxCntr->psxRcntWtarget(0, value);
            return;
        case 0x1f801110:
            pcsx->psxCntr->psxRcntWcount(1, value);
            return;
        case 0x1f801114:
            pcsx->psxCntr->psxRcntWmode(1, value);
            return;
        case 0x1f801118:
            pcsx->psxCntr->psxRcntWtarget(1, value);
            return;
        case 0x1f801120:
            pcsx->psxCntr->psxRcntWcount(2, value);
            return;
        case 0x1f801124:
            pcsx->psxCntr->psxRcntWmode(2, value);
            return;
        case 0x1f801128:
            pcsx->psxCntr->psxRcntWtarget(2, value);
            return;
        default:
            if (add >= 0x1f801c00 && add < 0x1f801e00) {
                SPUwriteRegister(add, value);
                return;
            }
            psxHu16ref(add) = SWAPu16(value);
            return;
    }
    psxHu16ref(add) = SWAPu16(value);
}
#define DmaExec(n)                                                                                                     \
    {                                                                                                                  \
        HW_DMA##n##_CHCR = SWAPu32(value);                                                                             \
        if (SWAPu32(HW_DMA##n##_CHCR) & 0x01000000 && SWAPu32(HW_DMA_PCR) & (8 << (n * 4))) {                          \
            pcsx->psxDma->_psxDma##n(SWAPu32(HW_DMA##n##_MADR), SWAPu32(HW_DMA##n##_BCR), SWAPu32(HW_DMA##n##_CHCR));  \
        }                                                                                                              \
    }
void HW::psxHwWrite32(u32 add, u32 value)
{
    switch (add) {
        case 0x1f801040:
            pcsx->psxSio->sioWrite8((unsigned char)value);
            pcsx->psxSio->sioWrite8((unsigned char)((value & 0xff) >> 8));
            pcsx->psxSio->sioWrite8((unsigned char)((value & 0xff) >> 16));
            pcsx->psxSio->sioWrite8((unsigned char)((value & 0xff) >> 24));
            return;
#ifdef ENABLE_SIO1API
        case 0x1f801050:
            SIO1_writeData32(value);
            return;
#endif
        case 0x1f801070:
            if (Config.Sio)
                psxHu32ref(0x1070) |= SWAPu32(0x80);
            if (Config.SpuIrq)
                psxHu32ref(0x1070) |= SWAPu32(0x200);
            psxHu32ref(0x1070) &= SWAPu32((psxHu32(0x1074) & value));
            return;
        case 0x1f801074:
            psxHu32ref(0x1074) = SWAPu32(value);
            return;
        case 0x1f801088:
            DmaExec(0);
            return;
        case 0x1f801098:
            DmaExec(1);
            return;
        case 0x1f8010a8:
            DmaExec(2);
            return;
        case 0x1f8010b8:
            DmaExec(3);
            return;
        case 0x1f8010c8:
            DmaExec(4);
            return;
        case 0x1f8010e8:
            DmaExec(6);
            return;
        case 0x1f8010f4: {
            u32 tmp    = (~value) & SWAPu32(HW_DMA_ICR);
            HW_DMA_ICR = SWAPu32(((tmp ^ value) & 0xffffff) ^ tmp);
            return;
        }
        case 0x1f801810:
            GPUwriteData(value);
            return;
        case 0x1f801814:
            GPUwriteStatus(value);
            return;
        case 0x1f801820:
            pcsx->psxMdec->mdecWrite0(value);
            break;
        case 0x1f801824:
            pcsx->psxMdec->mdecWrite1(value);
            break;
        case 0x1f801100:
            pcsx->psxCntr->psxRcntWcount(0, value & 0xffff);
            return;
        case 0x1f801104:
            pcsx->psxCntr->psxRcntWmode(0, value);
            return;
        case 0x1f801108:
            pcsx->psxCntr->psxRcntWtarget(0, value & 0xffff);
            return;
        case 0x1f801110:
            pcsx->psxCntr->psxRcntWcount(1, value & 0xffff);
            return;
        case 0x1f801114:
            pcsx->psxCntr->psxRcntWmode(1, value);
            return;
        case 0x1f801118:
            pcsx->psxCntr->psxRcntWtarget(1, value & 0xffff);
            return;
        case 0x1f801120:
            pcsx->psxCntr->psxRcntWcount(2, value & 0xffff);
            return;
        case 0x1f801124:
            pcsx->psxCntr->psxRcntWmode(2, value);
            return;
        case 0x1f801128:
            pcsx->psxCntr->psxRcntWtarget(2, value & 0xffff);
            return;
        default:
            psxHu32ref(add) = SWAPu32(value);
            return;
    }
    psxHu32ref(add) = SWAPu32(value);
}
int HW::psxHwFreeze(gzFile f, int Mode)
{
    return 0;
}
