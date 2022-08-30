#ifndef __PLUGINS_H__
#define __PLUGINS_H__
#ifdef __cplusplus

extern "C" {
#endif
#include "../utils/common.h"
#include "../utils/psemu_plugin_defs.h"
#include "../plugins/sdlinput/pad.h"

#define PAD1_configure    PADconfigure
#define PAD1_about        PADabout
#define PAD1_init         PADinit
#define PAD1_shutdown     PADshutdown
#define PAD1_test         PADtest
#define PAD1_open         PADopen
#define PAD1_close        PADclose
#define PAD1_query        PADquery
#define PAD1_readPort1    PADreadPort1
#define PAD1_keypressed   PADkeypressed
#define PAD1_startPoll    PADstartPoll
#define PAD1_poll         PADpoll
#define PAD1_setSensitive PADsetSensitive
#define PAD2_configure    PADconfigure
#define PAD2_about        PADabout
#define PAD2_init         PADinit
#define PAD2_shutdown     PADshutdown
#define PAD2_test         PADtest
#define PAD2_open         PADopen
#define PAD2_close        PADclose
#define PAD2_query        PADquery
#define PAD2_readPort1    PADreadPort1
#define PAD2_keypressed   PADkeypressed
#define PAD2_startPoll    PADstartPoll
#define PAD2_poll         PADpoll
#define PAD2_setSensitive PADsetSensitive

typedef struct
{
    uint32_t      ulFreezeVersion;
    uint32_t      ulStatus;
    uint32_t      ulControl[256];
    unsigned char psxVRam[1024 * 512 * 2];
} GPUFreeze_t;

struct CdrStat
{
    uint32_t      Type;
    uint32_t      Status;
    unsigned char Time[3];
};

struct SubQ
{
    char          res0[12];
    unsigned char ControlAndADR;
    unsigned char TrackNumber;
    unsigned char IndexNumber;
    unsigned char TrackRelativeAddress[3];
    unsigned char Filler;
    unsigned char AbsoluteAddress[3];
    unsigned char CRC[2];
    char          res1[72];
};

typedef void *HWND;
typedef unsigned long (*PSEgetLibType)(void);
typedef unsigned long (*PSEgetLibVersion)(void);
typedef char *(*PSEgetLibName)(void);

typedef long (*CDRinit)(void);
typedef long (*CDRshutdown)(void);
typedef long (*CDRopen)(void);
typedef long (*CDRclose)(void);
typedef long (*CDRgetTN)(unsigned char *);
typedef long (*CDRgetTD)(unsigned char, unsigned char *);
typedef long (*CDRreadTrack)(unsigned char *);

typedef unsigned char *(*CDRgetBuffer)(void);
typedef unsigned char *(*CDRgetBufferSub)(void);

typedef long (*CDRconfigure)(void);
typedef long (*CDRtest)(void);
typedef void (*CDRabout)(void);
typedef long (*CDRplay)(unsigned char *);
typedef long (*CDRstop)(void);
typedef long (*CDRsetfilename)(char *);

typedef long (*CDRgetStatus)(struct CdrStat *);
typedef char *(*CDRgetDriveLetter)(void);

typedef long (*CDRreadCDDA)(unsigned char, unsigned char, unsigned char, unsigned char *);
typedef long (*CDRgetTE)(unsigned char, unsigned char *, unsigned char *, unsigned char *);


class PLUGINS {
  public:
    PCSX *pcsx = nullptr;

    char        IsoFile[MAXPATHLEN] = "";
    s64         cdOpenCaseTime      = 0;
    const char *err;

    unsigned char buf[256];
    unsigned char stdpar[10]   = {0x00, 0x41, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    unsigned char mousepar[8]  = {0x00, 0x12, 0x5a, 0xff, 0xff, 0xff, 0xff};
    unsigned char analogpar[9] = {0x00, 0xff, 0x5a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    int           bufcount, bufc;
    PadDataS      padd1, padd2;

  public:
    PLUGINS(PCSX *_pcsx);

    long CDRinit(void);
    long CDRshutdown(void);
    long CDRopen(void);
    long CDRclose(void);

    long CDRgetTN(unsigned char *buffer);
    long CDRgetTD(unsigned char track, unsigned char *buffer);

    long           CDRreadTrack(unsigned char *time);
    unsigned char *CDRgetBuffer(void);

    long  CDRplay(unsigned char *time);
    long  CDRstop(void);
    long  CDRgetStatus(struct CdrStat *stat);
    char *CDRgetDriveLetter(void);
    long  CDRgetBufferSub(void);
    long  CDRconfigure(void);
    long  CDRabout(void);
    long  CDRsetfilename(void);
    long  CDRreadCDDA(void);
    long  CDRgetTE(void);

  public:
    void GPU__displayText(char *pText);
    long GPU__configure(void);
    long GPU__test(void);
    void GPU__about(void);
    void GPU__makeSnapshot(void);
    void GPU__keypressed(int key);
    long GPU__getScreenPic(unsigned char *pMem);
    long GPU__showScreenPic(unsigned char *pMem);
    void GPUclearDynarec(void (*callback)(void));
    void GPUvBlank(int val);

    long  CDR__play(unsigned char *sector);
    long  CDR__stop(void);
    long  CDR__getStatus(struct CdrStat *stat);
    char *CDR__getDriveLetter(void);
    long  CDR__configure(void);
    long  CDR__test(void);
    void  CDR__about(void);
    long  CDR__setfilename(char *filename);

    long SPU__configure(void);
    void SPU__about(void);
    long SPU__test(void);

    unsigned char _PADstartPoll(PadDataS *pad);
    unsigned char _PADpoll(unsigned char value);
    unsigned char PAD1__startPoll(int pad);
    unsigned char PAD1__poll(unsigned char value);
    long          PAD1__configure(void);
    void          PAD1__about(void);
    long          PAD1__test(void);
    long          PAD1__query(void);
    long          PAD1__keypressed();
    long          PAD2__configure(void);
    void          PAD2__about(void);
    long          PAD2__test(void);
    long          PAD2__query(void);
    long          PAD2__keypressed();

    void        clearDynarec(void);
    int         LoadPlugins();
    void        SetIsoFile(const char *filename);
    const char *GetIsoFile(void);
    boolean     UsingIso(void);
    void        SetCdOpenCaseTime(s64 time);

  public:
};

#ifdef __cplusplus
}
#endif
#endif
