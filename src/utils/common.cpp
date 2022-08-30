#include "common.h"
#include "../core/r3000a.h"
#include "../bios/bios.h"

PcsxConfig   Config;
extern PCSX *g_pcsx;

int Log = 0;
int EmuInit()
{
    return g_pcsx->psxCpu->psxInit();
}
void EmuReset()
{
    g_pcsx->psxCpu->psxReset();
}
void EmuShutdown()
{
    g_pcsx->psxCpu->psxShutdown();
}
void EmuUpdate()
{
    if (!Config.HLE || !hleSoftCall)
        g_pcsx->SysUpdate();
}
void __Log(char *fmt, ...)
{
}
