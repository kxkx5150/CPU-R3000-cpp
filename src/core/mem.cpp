#include "mem.h"
#include "r3000a.h"
#include "hw.h"
#include <sys/mman.h>
#include "hw.h"


MEM::MEM(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
int MEM::psxMemInit()
{
    int i;
    psxMemRLUT = (u8 **)malloc(0x10000 * sizeof(void *));
    psxMemWLUT = (u8 **)malloc(0x10000 * sizeof(void *));
    memset(psxMemRLUT, 0, 0x10000 * sizeof(void *));
    memset(psxMemWLUT, 0, 0x10000 * sizeof(void *));


    psxM = (s8 *)mmap(0, 0x00220000, PROT_WRITE | PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    psxP = &psxM[0x200000];
    psxH = &psxM[0x210000];
    psxR = (s8 *)malloc(0x00080000);

    if (psxMemRLUT == NULL || psxMemWLUT == NULL || psxM == NULL || psxP == NULL || psxH == NULL) {
        SysMessage(_("Error allocating memory!"));
        return -1;
    }

    for (i = 0; i < 0x80; i++)
        psxMemRLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];
    memcpy(psxMemRLUT + 0x8000, psxMemRLUT, 0x80 * sizeof(void *));
    memcpy(psxMemRLUT + 0xa000, psxMemRLUT, 0x80 * sizeof(void *));
    psxMemRLUT[0x1f00] = (u8 *)psxP;
    psxMemRLUT[0x1f80] = (u8 *)psxH;
    for (i = 0; i < 0x08; i++)
        psxMemRLUT[i + 0x1fc0] = (u8 *)&psxR[i << 16];
    memcpy(psxMemRLUT + 0x9fc0, psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));
    memcpy(psxMemRLUT + 0xbfc0, psxMemRLUT + 0x1fc0, 0x08 * sizeof(void *));
    for (i = 0; i < 0x80; i++)
        psxMemWLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];
    memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
    memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));
    psxMemWLUT[0x1f00] = (u8 *)psxP;
    psxMemWLUT[0x1f80] = (u8 *)psxH;
    return 0;
}
void MEM::psxMemReset()
{
    FILE *f = NULL;
    char  bios[1024];
    memset(psxM, 0, 0x00200000);
    memset(psxP, 0, 0x00010000);
    Config.HLE = TRUE;
}
void MEM::psxMemShutdown()
{
    munmap(psxM, 0x00220000);
    free(psxR);
    free(psxMemRLUT);
    free(psxMemWLUT);
}
u8 MEM::psxMemRead8(u32 mem)
{
    char *p;
    u32   t;
    t = mem >> 16;
    if (t == 0x1f80) {
        if (mem < 0x1f801000)
            return (*(u8 *)&psxH[(mem)&0xffff]);
        else
            return pcsx->psxHw->psxHwRead8(mem);
    } else {
        p = (char *)(psxMemRLUT[t]);
        if (p != NULL) {
            return *(u8 *)(p + (mem & 0xffff));
        } else {
            return 0;
        }
    }
}
u16 MEM::psxMemRead16(u32 mem)
{
    char *p;
    u32   t;
    t = mem >> 16;
    if (t == 0x1f80) {
        if (mem < 0x1f801000)
            return (SWAP16(*(u16 *)&psxH[(mem)&0xffff]));
        else
            return pcsx->psxHw->psxHwRead16(mem);
    } else {
        p = (char *)(psxMemRLUT[t]);
        if (p != NULL) {
            return SWAPu16(*(u16 *)(p + (mem & 0xffff)));
        } else {
            return 0;
        }
    }
}
u32 MEM::psxMemRead32(u32 mem)
{
    char *p;
    u32   t;
    t = mem >> 16;
    if (t == 0x1f80) {
        if (mem < 0x1f801000)
            return (SWAP32(*(u32 *)&psxH[(mem)&0xffff]));
        else
            return pcsx->psxHw->psxHwRead32(mem);
    } else {
        p = (char *)(psxMemRLUT[t]);
        if (p != NULL) {
            return SWAPu32(*(u32 *)(p + (mem & 0xffff)));
        } else {
            return 0;
        }
    }
}
void MEM::psxMemWrite8(u32 mem, u8 value)
{
    char *p;
    u32   t;
    t = mem >> 16;
    if (t == 0x1f80) {
        if (mem < 0x1f801000)
            (*(u8 *)&psxH[(mem)&0xffff]) = value;
        else
            pcsx->psxHw->psxHwWrite8(mem, value);
    } else {
        p = (char *)(psxMemWLUT[t]);
        if (p != NULL) {
            *(u8 *)(p + (mem & 0xffff)) = value;
            pcsx->psxCpu->intClear((mem & (~3)), 1);

        } else {
        }
    }
}
void MEM::psxMemWrite16(u32 mem, u16 value)
{
    char *p;
    u32   t;
    t = mem >> 16;
    if (t == 0x1f80) {
        if (mem < 0x1f801000)
            (*(u16 *)&psxH[(mem)&0xffff]) = SWAPu16(value);
        else
            pcsx->psxHw->psxHwWrite16(mem, value);
    } else {
        p = (char *)(psxMemWLUT[t]);
        if (p != NULL) {
            *(u16 *)(p + (mem & 0xffff)) = SWAPu16(value);
            pcsx->psxCpu->intClear((mem & (~1)), 1);

        } else {
        }
    }
}
void MEM::psxMemWrite32(u32 mem, u32 value)
{
    char *p;
    u32   t;
    t = mem >> 16;
    if (t == 0x1f80) {
        if (mem < 0x1f801000)
            psxHu32ref(mem) = SWAPu32(value);
        else
            pcsx->psxHw->psxHwWrite32(mem, value);
    } else {
        p = (char *)(psxMemWLUT[t]);
        if (p != NULL) {
            *(u32 *)(p + (mem & 0xffff)) = SWAPu32(value);
            pcsx->psxCpu->intClear(mem, 1);

        } else {
            if (mem != 0xfffe0130) {
                if (!writeok)
                    pcsx->psxCpu->intClear(mem, 1);
            } else {
                int i;
                switch (value) {
                    case 0x800:
                    case 0x804:
                        if (writeok == 0)
                            break;
                        writeok = 0;
                        memset(psxMemWLUT + 0x0000, 0, 0x80 * sizeof(void *));
                        memset(psxMemWLUT + 0x8000, 0, 0x80 * sizeof(void *));
                        memset(psxMemWLUT + 0xa000, 0, 0x80 * sizeof(void *));
                        break;
                    case 0x00:
                    case 0x1e988:
                        if (writeok == 1)
                            break;
                        writeok = 1;
                        for (i = 0; i < 0x80; i++)
                            psxMemWLUT[i + 0x0000] = (u8 *)&psxM[(i & 0x1f) << 16];
                        memcpy(psxMemWLUT + 0x8000, psxMemWLUT, 0x80 * sizeof(void *));
                        memcpy(psxMemWLUT + 0xa000, psxMemWLUT, 0x80 * sizeof(void *));
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
