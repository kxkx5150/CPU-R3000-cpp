
#include "dma.h"
#include "mdec.h"
#include "cdrom.h"
#include "../plugins/dfsound/dma.h"
#include "../plugins/dfxvideo/gpu.h"


DMA::DMA(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void DMA::spuInterrupt()
{
    HW_DMA4_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(4);
}
void DMA::_psxDma6(u32 madr, u32 bcr, u32 chcr)
{
    u32 *mem = (u32 *)PSXM(madr);

    if (chcr == 0x11000002) {
        if (mem == NULL) {

            HW_DMA6_CHCR &= SWAP32(~0x01000000);
            DMA_INTERRUPT(6);
            return;
        }
        while (bcr--) {
            *mem-- = SWAP32((madr - 4) & 0xffffff);
            madr -= 4;
        }
        mem++;
        *mem = 0xffffff;
    }

    HW_DMA6_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(6);
}
void DMA::_psxDma5(u32 madr, u32 bcr, u32 chcr)
{
}
void DMA::_psxDma4(u32 madr, u32 bcr, u32 chcr)
{
    u16 *ptr;
    u32  size;
    switch (chcr) {
        case 0x01000201:

            ptr = (u16 *)PSXM(madr);
            if (ptr == NULL) {

                break;
            }
            SPUwriteDMAMem(ptr, (bcr >> 16) * (bcr & 0xffff) * 2);
            SPUDMA_INT((bcr >> 16) * (bcr & 0xffff) / 2);
            return;
        case 0x01000200:

            ptr = (u16 *)PSXM(madr);
            if (ptr == NULL) {

                break;
            }
            size = (bcr >> 16) * (bcr & 0xffff) * 2;
            SPUreadDMAMem(ptr, size);
            pcsx->psxCpu->intClear(madr, size);
            break;
    }
    HW_DMA4_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(4);
}
void DMA::_psxDma3(u32 madr, u32 bcr, u32 chcr)
{
    pcsx->psxCdrom->psxDma3(madr, bcr, chcr);
}
void DMA::_psxDma2(u32 madr, u32 bcr, u32 chcr)
{
    u32 *ptr;
    u32  size;
    switch (chcr) {
        case 0x01000200:

            ptr = (u32 *)PSXM(madr);
            if (ptr == NULL) {

                break;
            }
            size = (bcr >> 16) * (bcr & 0xffff);
            GPUreadDataMem(ptr, size);
            pcsx->psxCpu->intClear(madr, size);
            break;
        case 0x01000201:

            ptr = (u32 *)PSXM(madr);
            if (ptr == NULL) {

                break;
            }
            size = (bcr >> 16) * (bcr & 0xffff);
            GPUwriteDataMem(ptr, size);
            GPUDMA_INT(size / 4);
            return;
        case 0x01000401:

            GPUdmaChain((u32 *)pcsx->psxMem->psxM, madr & 0x1fffff);
            break;
    }
    HW_DMA2_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(2);
}
void DMA::_psxDma1(u32 madr, u32 bcr, u32 chcr)
{
    pcsx->psxMdec->psxDma1(madr, bcr, chcr);
}
void DMA::_psxDma0(u32 madr, u32 bcr, u32 chcr)
{
    pcsx->psxMdec->psxDma0(madr, bcr, chcr);
}
void DMA::gpuInterrupt()
{
    HW_DMA2_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(2);
}
