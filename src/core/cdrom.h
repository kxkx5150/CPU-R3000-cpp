#ifndef __CDROM_H__
#define __CDROM_H__

#ifdef __cplusplus
#include <cstdint>
#include "../utils/common.h"
#include "r3000a.h"
#include "plugins.h"
#include "mem.h"
#include "hw.h"
#include "../plugins/dfsound/spu.h"
#define CD_FRAMESIZE_RAW 2352
#define SHC              10
extern "C" {
#endif


typedef struct
{
    unsigned char  OCUP;
    unsigned char  Reg1Mode;
    unsigned char  Reg2;
    unsigned char  CmdProcess;
    unsigned char  Ctrl;
    unsigned char  Stat;
    unsigned char  StatP;
    unsigned char  Transfer[CD_FRAMESIZE_RAW];
    unsigned char *pTransfer;
    unsigned char  Prev[4];
    unsigned char  Param[8];
    unsigned char  Result[8];
    unsigned char  ParamC;
    unsigned char  ParamP;
    unsigned char  ResultC;
    unsigned char  ResultP;
    unsigned char  ResultReady;
    unsigned char  Cmd;
    unsigned char  Readed;
    u32            Reading;
    unsigned char  ResultTN[6];
    unsigned char  ResultTD[4];
    unsigned char  SetSector[4];
    unsigned char  SetSectorSeek[4];
    unsigned char  Track;
    boolean        Play, Muted;
    int            CurTrack;
    int            Mode, File, Channel;
    int            Reset;
    int            RErr;
    int            FirstSector;
    xa_decode_t    Xa;
    int            Init;
    unsigned char  Irq;
    u32            eCycle;
    boolean        Seeked;
} cdrStruct;

typedef struct
{
    u8 filenum;
    u8 channum;
    u8 submode;
    u8 coding;
    u8 filenum2;
    u8 channum2;
    u8 submode2;
    u8 coding2;
} xa_subheader_t;



class CDROM {
  public:
    PCSX *pcsx = nullptr;

    cdrStruct      cdr;
    struct CdrStat stat;
    struct SubQ   *subq;

  public:
    int           headtable[4] = {0, 2, 8, 10};
    unsigned char Test04[1]    = {0};
    unsigned char Test05[1]    = {0};
    unsigned char Test20[4]    = {0x98, 0x06, 0x10, 0xC3};
    unsigned char Test22[8]    = {0x66, 0x6F, 0x72, 0x20, 0x45, 0x75, 0x72, 0x6F};
    unsigned char Test23[8]    = {0x43, 0x58, 0x44, 0x32, 0x39, 0x34, 0x30, 0x51};

    int K0[4] = {static_cast<int>(0.0 * (1 << SHC)), static_cast<int>(0.9375 * (1 << SHC)),
                 static_cast<int>(1.796875 * (1 << SHC)), static_cast<int>(1.53125 * (1 << SHC))};
    int K1[4] = {static_cast<int>(0.0 * (1 << SHC)), static_cast<int>(0.0 * (1 << SHC)),
                 static_cast<int>(-0.8125 * (1 << SHC)), static_cast<int>(-0.859375 * (1 << SHC))};

    char *CmdName[0x100] = {(char *)"CdlSync",     (char *)"CdlNop",
                            (char *)"CdlSetloc",   (char *)"CdlPlay",
                            (char *)"CdlForward",  (char *)"CdlBackward",
                            (char *)"CdlReadN",    (char *)"CdlStandby",
                            (char *)"CdlStop",     (char *)"CdlPause",
                            (char *)"CdlInit",     (char *)"CdlMute",
                            (char *)"CdlDemute",   (char *)"CdlSetfilter",
                            (char *)"CdlSetmode",  (char *)"CdlGetmode",
                            (char *)"CdlGetlocL",  (char *)"CdlGetlocP",
                            (char *)"CdlReadT",    (char *)"CdlGetTN",
                            (char *)"CdlGetTD",    (char *)"CdlSeekL",
                            (char *)"CdlSeekP",    (char *)"CdlSetclock",
                            (char *)"CdlGetclock", (char *)"CdlTest",
                            (char *)"CdlID",       (char *)"CdlReadS",
                            (char *)"CdlReset",    NULL,
                            (char *)"CDlReadToc",  NULL};

  public:
    CDROM(PCSX *_pcsx);

    void          ReadTrack();
    void          AddIrqQueue(unsigned char irq, unsigned long ecycle);
    void          cdrInterrupt();
    void          cdrReadInterrupt();
    unsigned char cdrRead0(void);
    void          cdrWrite0(unsigned char rt);
    unsigned char cdrRead1(void);
    void          cdrWrite1(unsigned char rt);
    unsigned char cdrRead2(void);
    void          cdrWrite2(unsigned char rt);
    unsigned char cdrRead3(void);
    void          cdrWrite3(unsigned char rt);
    void          psxDma3(u32 madr, u32 bcr, u32 chcr);
    void          cdrReset();
    int           cdrFreeze(gzFile f, int Mode);
    void          ADPCM_InitDecode(ADPCM_Decode_t *decp);
    void ADPCM_DecodeBlock16(ADPCM_Decode_t *decp, u8 filter_range, const void *vblockp, short *destp, int inc);
    void xa_decode_data(xa_decode_t *xdp, unsigned char *srcp);
    int  parse_xa_audio_sector(xa_decode_t *xdp, xa_subheader_t *subheadp, unsigned char *sectorp, int is_first_sector);
    s32  xa_decode_sector(xa_decode_t *xdp, unsigned char *sectorp, int is_first_sector);
};


#ifdef __cplusplus
}
#endif
#endif
