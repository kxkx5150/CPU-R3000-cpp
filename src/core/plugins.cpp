#include "plugins.h"
#include "cdriso.h"
#include "../plugins/sdlinput/pad.h"
#include "../plugins/dfxvideo/gpu.h"
#include "../plugins/dfsound/spu.h"
#include <cstddef>

#define CheckErr(func)                                                                                                 \
    {                                                                                                                  \
        SysMessage("Error loading");                                                                                   \
    }



PLUGINS::PLUGINS(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void PLUGINS::GPU__displayText(char *pText)
{
    SysPrintf("%s\n", pText);
}
long PLUGINS::GPU__configure(void)
{
    return 0;
}
long PLUGINS::GPU__test(void)
{
    return 0;
}
void PLUGINS::GPU__about(void)
{
}
void PLUGINS::GPU__makeSnapshot(void)
{
}
void PLUGINS::GPU__keypressed(int key)
{
}
long PLUGINS::GPU__getScreenPic(unsigned char *pMem)
{
    return -1;
}
long PLUGINS::GPU__showScreenPic(unsigned char *pMem)
{
    return -1;
}
void PLUGINS::GPUclearDynarec(void (*callback)(void))
{
}
void PLUGINS::GPUvBlank(int val)
{
}


long PLUGINS::CDR__play(unsigned char *sector)
{
    return 0;
}
long PLUGINS::CDR__stop(void)
{
    return 0;
}
long PLUGINS::CDR__getStatus(struct CdrStat *stat)
{
    if (cdOpenCaseTime < 0 || cdOpenCaseTime > (s64)time(NULL))
        stat->Status = 0x10;
    else
        stat->Status = 0;
    return 0;
}
char *PLUGINS::CDR__getDriveLetter(void)
{
    return NULL;
}
long PLUGINS::CDR__configure(void)
{
    return 0;
}
long PLUGINS::CDR__test(void)
{
    return 0;
}
void PLUGINS::CDR__about(void)
{
}
long PLUGINS::CDR__setfilename(char *filename)
{
    return 0;
}



long PLUGINS::CDRinit(void)
{
    return pcsx->psxCIso->ISOinit();
}
long PLUGINS::CDRshutdown(void)
{
    return pcsx->psxCIso->ISOshutdown();
}
long PLUGINS::CDRopen(void)
{
    return pcsx->psxCIso->ISOopen();
}
long PLUGINS::CDRclose(void)
{
    return pcsx->psxCIso->ISOclose();
}
long PLUGINS::CDRgetTN(unsigned char *buffer)
{
    return pcsx->psxCIso->ISOgetTN(buffer);
}
long PLUGINS::CDRgetTD(unsigned char track, unsigned char *buffer)
{
    return pcsx->psxCIso->ISOgetTD(track, buffer);
}
long PLUGINS::CDRreadTrack(unsigned char *time)
{
    return pcsx->psxCIso->ISOreadTrack(time);
}
unsigned char *PLUGINS::CDRgetBuffer(void)
{
    return pcsx->psxCIso->ISOgetBuffer();
}
long PLUGINS::CDRplay(unsigned char *time)
{
    return pcsx->psxCIso->ISOplay(time);
}
long PLUGINS::CDRstop(void)
{
    return pcsx->psxCIso->ISOstop();
}
long PLUGINS::CDRgetStatus(struct CdrStat *stat)
{
    return pcsx->psxCIso->ISOgetStatus(stat);
}
char *PLUGINS::CDRgetDriveLetter(void)
{
    return NULL;
}
long PLUGINS::CDRgetBufferSub(void)
{
    return 0;    //
}
long PLUGINS::CDRconfigure(void)
{
    return 0;    //
}
long PLUGINS::CDRabout(void)
{
    return 0;    //
}
long PLUGINS::CDRsetfilename(void)
{
    return 0;    //
}
long PLUGINS::CDRreadCDDA(void)
{
    return 0;    //
}
long PLUGINS::CDRgetTE(void)
{
    return 0;    //
}



long PLUGINS::SPU__configure(void)
{
    return 0;
}
void PLUGINS::SPU__about(void)
{
}
long PLUGINS::SPU__test(void)
{
    return 0;
}



unsigned char PLUGINS::_PADstartPoll(PadDataS *pad)
{
    bufc = 0;
    switch (pad->controllerType) {
        case PSE_PAD_TYPE_MOUSE:
            mousepar[3] = pad->buttonStatus & 0xff;
            mousepar[4] = pad->buttonStatus >> 8;
            mousepar[5] = pad->moveX;
            mousepar[6] = pad->moveY;
            memcpy(buf, mousepar, 7);
            bufcount = 6;
            break;
        case PSE_PAD_TYPE_NEGCON:
            analogpar[1] = 0x23;
            analogpar[3] = pad->buttonStatus & 0xff;
            analogpar[4] = pad->buttonStatus >> 8;
            analogpar[5] = pad->rightJoyX;
            analogpar[6] = pad->rightJoyY;
            analogpar[7] = pad->leftJoyX;
            analogpar[8] = pad->leftJoyY;
            memcpy(buf, analogpar, 9);
            bufcount = 8;
            break;
        case PSE_PAD_TYPE_ANALOGPAD:
            analogpar[1] = 0x73;
            analogpar[3] = pad->buttonStatus & 0xff;
            analogpar[4] = pad->buttonStatus >> 8;
            analogpar[5] = pad->rightJoyX;
            analogpar[6] = pad->rightJoyY;
            analogpar[7] = pad->leftJoyX;
            analogpar[8] = pad->leftJoyY;
            memcpy(buf, analogpar, 9);
            bufcount = 8;
            break;
        case PSE_PAD_TYPE_ANALOGJOY:
            analogpar[1] = 0x53;
            analogpar[3] = pad->buttonStatus & 0xff;
            analogpar[4] = pad->buttonStatus >> 8;
            analogpar[5] = pad->rightJoyX;
            analogpar[6] = pad->rightJoyY;
            analogpar[7] = pad->leftJoyX;
            analogpar[8] = pad->leftJoyY;
            memcpy(buf, analogpar, 9);
            bufcount = 8;
            break;
        case PSE_PAD_TYPE_STANDARD:
        default:
            stdpar[3] = pad->buttonStatus & 0xff;
            stdpar[4] = pad->buttonStatus >> 8;
            memcpy(buf, stdpar, 5);
            bufcount = 4;
    }
    return buf[bufc++];
}
unsigned char PLUGINS::_PADpoll(unsigned char value)
{
    if (bufc > bufcount)
        return 0;
    return buf[bufc++];
}
unsigned char PLUGINS::PAD1__startPoll(int pad)
{
    PadDataS padd;
    PADreadPort1(&padd);
    return _PADstartPoll(&padd);
}
unsigned char PLUGINS::PAD1__poll(unsigned char value)
{
    return _PADpoll(value);
}


long PLUGINS::PAD1__configure(void)
{
    return 0;
}
void PLUGINS::PAD1__about(void)
{
}
long PLUGINS::PAD1__test(void)
{
    return 0;
}
long PLUGINS::PAD1__query(void)
{
    return 3;
}
long PLUGINS::PAD1__keypressed()
{
    return 0;
}
long PLUGINS::PAD2__configure(void)
{
    return 0;
}
void PLUGINS::PAD2__about(void)
{
}
long PLUGINS::PAD2__test(void)
{
    return 0;
}
long PLUGINS::PAD2__query(void)
{
    return PSE_PAD_USE_PORT1 | PSE_PAD_USE_PORT2;
}
long PLUGINS::PAD2__keypressed()
{
    return 0;
}


void PLUGINS::clearDynarec(void)
{
}
int PLUGINS::LoadPlugins()
{
    int  ret;
    char Plugin[MAXPATHLEN];
    printf("LoadPlugins()\n");
    pcsx->psxCIso->cdrIsoInit();
    printf("cdrisoInit done\n");
    Config.UseNet = FALSE;
    ret           = pcsx->psxCIso->ISOinit();
    if (ret < 0) {
        SysMessage(_("Error initializing CD-ROM plugin: %d"), ret);
        return -1;
    } else
        printf("cdr_init done\n");
    ret = GPUinit();
    if (ret < 0) {
        SysMessage(_("Error initializing GPU plugin: %d"), ret);
        return -1;
    } else
        printf("gpu_init done\n");
    ret = SPUinit();
    if (ret < 0) {
        SysMessage(_("Error initializing SPU plugin: %d"), ret);
        return -1;
    } else
        printf("spu_init done\n");
    ret = PADinit(1);
    if (ret < 0) {
        SysMessage(_("Error initializing Controller 1 plugin: %d"), ret);
        return -1;
    }
    ret = PADinit(2);
    if (ret < 0) {
        SysMessage(_("Error initializing Controller 2 plugin: %d"), ret);
        return -1;
    }
    SysPrintf(_("Plugins loaded.\n"));
    return 0;
}
void PLUGINS::SetIsoFile(const char *filename)
{
    if (filename == NULL) {
        IsoFile[0] = '\0';
        return;
    }
    strncpy(IsoFile, filename, MAXPATHLEN);
}
const char *PLUGINS::GetIsoFile(void)
{
    return IsoFile;
}
boolean PLUGINS::UsingIso(void)
{
    return (IsoFile[0] != '\0');
}
void PLUGINS::SetCdOpenCaseTime(s64 time)
{
    cdOpenCaseTime = time;
}
