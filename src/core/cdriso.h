#ifndef CDRISO_H
#define CDRISO_H
#include "../utils/common.h"
#include "pcsx.h"
#ifdef __cplusplus
extern "C" {
#endif

#define MAXTRACKS         100
#define btoi(b)           ((b) / 16 * 10 + (b) % 16)
#define itob(i)           ((i) / 10 * 16 + (i) % 10)
#define MSF2SECT(m, s, f) (((m)*60 + (s)-2) * 75 + (f))
#define CD_FRAMESIZE_RAW  2352
#define DATA_SIZE         (CD_FRAMESIZE_RAW - 12)
#define SUB_FRAMESIZE     96


class CDRISO {
  public:
    PCSX *pcsx = nullptr;

    FILE         *cdHandle     = NULL;
    FILE         *cddaHandle   = NULL;
    FILE         *subHandle    = NULL;
    boolean       subChanMixed = FALSE;
    boolean       subChanRaw   = FALSE;
    unsigned char cdbuffer[DATA_SIZE];
    unsigned char subbuffer[SUB_FRAMESIZE];
    unsigned char sndbuffer[CD_FRAMESIZE_RAW * 10];

    unsigned int          initial_offset = 0;
    volatile boolean      playing        = FALSE;
    boolean               cddaBigEndian  = FALSE;
    volatile unsigned int cddaCurOffset  = 0;

    struct trackinfo
    {
        enum
        {
            DATA,
            CDDA
        } type;
        char start[3];
        char length[3];
    };
    int              numtracks = 0;
    struct trackinfo ti[MAXTRACKS];

  public:
    CDRISO(PCSX *_pcsx);

    unsigned int msf2sec(char *msf);
    void         sec2msf(unsigned int s, char *msf);
    void         tok2msf(char *time, char *msf);
    long         GetTickCount(void);
    void        *playthread(void *param);
    void         stopCDDA();
    void         startCDDA(unsigned int offset);

    int parsetoc(const char *isofile);
    int parsecue(const char *isofile);
    int parseccd(const char *isofile);
    int parsemds(const char *isofile);
    int opensubfile(const char *isoname);

    long           ISOinit(void);
    long           ISOshutdown(void);
    void           PrintTracks(void);
    long           ISOopen(void);
    long           ISOclose(void);
    long           ISOgetTN(unsigned char *buffer);
    long           ISOgetTD(unsigned char track, unsigned char *buffer);
    void           DecodeRawSubData(void);
    long           ISOreadTrack(unsigned char *time);
    unsigned char *ISOgetBuffer(void);
    long           ISOplay(unsigned char *time);
    long           ISOstop(void);
    unsigned char *ISOgetBufferSub(void);
    long           ISOgetStatus(struct CdrStat *stat);
    void           cdrIsoInit(void);
    int            cdrIsoActive(void);
};

#ifdef __cplusplus
}
#endif
#endif
