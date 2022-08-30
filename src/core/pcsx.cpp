#include "pcsx.h"
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include "../utils/common.h"
#include "../bios/bios.h"
#include "../plugins/dfsound/spu.h"
#include "../plugins/dfxvideo/gpu.h"
#include "sio.h"
#include "gte.h"
#include "mdec.h"
#include "cdriso.h"
#include "cdrom.h"

#define H_SPUirqAddr 0x0da4
#define H_SPUaddr    0x0da6
#define H_SPUdata    0x0da8
#define H_SPUctrl    0x0daa
#define H_SPUstat    0x0dae
#define H_SPUon1     0x0d88
#define H_SPUon2     0x0d8a
#define H_SPUoff1    0x0d8c
#define H_SPUoff2    0x0d8e
#define PARSEPATH(dst, src)                                                                                            \
    ptr = src + strlen(src);                                                                                           \
    while (*ptr != '\\' && ptr != src)                                                                                 \
        ptr--;                                                                                                         \
    if (ptr != src) {                                                                                                  \
        strcpy(dst, ptr + 1);                                                                                          \
    }
#define MAX_SLOTS 5

int isMute = 0;


PCSX::PCSX()
{
    psxCIso  = new CDRISO(this);
    psxCdrom = new CDROM(this);
    psxMem   = new MEM(this);
    psxCpu   = new R3000Acpu(this);
    psxCntr  = new Counter(this);
    psxDma   = new DMA(this);
    psxGte   = new GTE(this);
    psxHw    = new HW(this);
    psxMdec  = new MDEC(this);
    psxPlugs = new PLUGINS(this);

    psxSio = new SIO(this);
}
void SPUirq(void)
{
    psxHu32ref(0x1070) |= SWAPu32(0x200);
}
void PCSX::CreateMemcard(char *filename, char *conf_mcd)
{
    struct stat buf;
    strcpy(conf_mcd, getenv("HOME"));
    strcat(conf_mcd, MEMCARD_DIR);
    strcat(conf_mcd, filename);
    if (stat(conf_mcd, &buf) == -1) {
        printf(_("Creating memory card: %s\n"), conf_mcd);
        psxSio->CreateMcd(conf_mcd);
    }
}
int PCSX::SysInit()
{
    if (EmuInit() == -1) {
        printf(_("PSX emulator couldn't be initialized.\n"));
        return -1;
    }
    psxSio->LoadMcds(Config.Mcd1, Config.Mcd2);
    return 0;
}
void PCSX::SysReset()
{
    EmuReset();
}
void PCSX::SysClose()
{
    EmuShutdown();
}
void PCSX::SysUpdate()
{
    PADhandleKey(PAD1_keypressed());
    PADhandleKey(PAD2_keypressed());
}
void PCSX::PADhandleKey(int key)
{
    char  Text[MAXPATHLEN];
    short rel = 0;
    if (key == 0)
        return;
    if ((key >> 30) & 1)
        rel = 1;
    if (key == SDLK_ESCAPE) {
        ClosePlugins();
        exit(1);
    }
}
void PCSX::SignalExit(int sig)
{
    ClosePlugins();
}
int PCSX::_OpenPlugins()
{
    int ret;

    ret = psxPlugs->CDRopen();
    if (ret < 0) {
        SysMessage(_("Error opening CD-ROM plugin!"));
        return -1;
    }
    ret = GPUopen(&gpuDisp, (char *)"PCSX", NULL);
    if (ret < 0) {
        SysMessage(_("Error opening GPU plugin!"));
        return -1;
    }
    ret = SPUopen();
    if (ret < 0) {
        SysMessage(_("Error opening SPU plugin!"));
        return -1;
    }
    SPUregisterCallback(SPUirq);
    ret = PAD1_open(&gpuDisp);
    if (ret < 0) {
        SysMessage(_("Error opening Controller 1 plugin!"));
        return -1;
    }
    ret = PAD2_open(&gpuDisp);
    if (ret < 0) {
        SysMessage(_("Error opening Controller 2 plugin!"));
        return -1;
    }
    return 0;
}
int PCSX::OpenPlugins()
{
    int ret;
    while ((ret = _OpenPlugins()) == -2) {
        psxSio->LoadMcds(Config.Mcd1, Config.Mcd2);
        if (psxPlugs->LoadPlugins() == -1)
            return -1;
    }
    return ret;
}
void PCSX::ClosePlugins()
{
    int ret;
    ret = psxPlugs->CDRclose();
    if (ret < 0) {
        SysMessage(_("Error closing CD-ROM plugin!"));
        return;
    }
    ret = SPUclose();
    if (ret < 0) {
        SysMessage(_("Error closing SPU plugin!"));
        return;
    }
    ret = PAD1_close();
    if (ret < 0) {
        SysMessage(_("Error closing Controller 1 Plugin!"));
        return;
    }
    ret = PAD2_close();
    if (ret < 0) {
        SysMessage(_("Error closing Controller 2 plugin!"));
        return;
    }
    ret = GPUclose();
    if (ret < 0) {
        SysMessage(_("Error closing GPU plugin!"));
        return;
    }
}
int PCSX::mainloop(const char *isofilename)
{

    char file[MAXPATHLEN] = "";
    char path[MAXPATHLEN];
    int  loadst = 0;
    int  i;

    psxPlugs->SetIsoFile(isofilename);

    memset(&Config, 0, sizeof(PcsxConfig));
    strcpy(Config.Net, "Disabled");

    Config.PsxAuto = 1;
    strcpy(Config.Gpu, "./libDFXVideo.so");
    strcpy(Config.Spu, "./libspuPeteNull.so.1.0.1");
    strcpy(Config.Cdr, "Disabled");
    strcpy(Config.Pad1, "./libDFInput.so");
    strcpy(Config.Pad2, "./libDFInput.so");
    strcpy(Config.Bios, "HLE");
    Config.HLE = TRUE;

    CreateMemcard((char *)"card1.mcd", Config.Mcd1);
    CreateMemcard((char *)"card2.mcd", Config.Mcd2);
    psxSio->LoadMcds(Config.Mcd1, Config.Mcd2);

    if (SysInit() == -1)
        return 1;

    if (psxPlugs->LoadPlugins() == -1) {
        printf("Error Failed loading plugins!");
        return 1;
    }

    if (OpenPlugins() == -1) {
        return 1;
    }

    SysReset();
    CheckCdrom();

    if (LoadCdrom() == -1) {
        ClosePlugins();
        printf("Could not load CD-ROM!\n");
        return -1;
    }

    while (1) {
        psxCpu->execI();
    }
    return 0;
}
