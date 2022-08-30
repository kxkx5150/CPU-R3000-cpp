#ifndef _SIO_H_
#define _SIO_H_
#ifdef __cplusplus
#include "../utils/common.h"
#include "r3000a.h"
#include "mem.h"
#include "plugins.h"
#include "../utils/psemu_plugin_defs.h"
extern "C" {
#endif

#define TX_RDY      0x0001
#define RX_RDY      0x0002
#define TX_EMPTY    0x0004
#define PARITY_ERR  0x0008
#define RX_OVERRUN  0x0010
#define FRAMING_ERR 0x0020
#define SYNC_DETECT 0x0040
#define DSR         0x0080
#define CTS         0x0100
#define IRQ         0x0200
#define TX_PERM     0x0001
#define DTR         0x0002
#define RX_PERM     0x0004
#define BREAK       0x0008
#define RESET_ERR   0x0010
#define RTS         0x0020
#define SIO_RESET   0x0040

#define MCD_SIZE (1024 * 8 * 16)


class SIO {
  public:
    PCSX *pcsx = nullptr;

    char          Title[48 + 1];
    char          sTitle[48 * 2 + 1];
    char          ID[12 + 1];
    char          Name[16 + 1];
    int           IconCount;
    short         Icon[16 * 16 * 3];
    unsigned char Flags;

    unsigned short StatReg = TX_RDY | TX_EMPTY;
    unsigned short ModeReg;
    unsigned short CtrlReg;
    unsigned short BaudReg;
    unsigned int   bufcount;
    unsigned int   parp;
    unsigned int   mcdst, rdwr;
    unsigned char  adrH, adrL;
    unsigned int   padst;

    unsigned char buf[256];

    unsigned char cardh[4] = {0x00, 0x00, 0x5a, 0x5d};
    char          Mcd1Data[MCD_SIZE], Mcd2Data[MCD_SIZE];

  public:
    SIO(PCSX *_pcsx);

    void SIO_INT();

    void sioWrite8(unsigned char value);
    void sioWriteStat16(unsigned short value);
    void sioWriteMode16(unsigned short value);
    void sioWriteCtrl16(unsigned short value);
    void sioWriteBaud16(unsigned short value);

    unsigned char  sioRead8();
    unsigned short sioReadStat16();
    unsigned short sioReadMode16();
    unsigned short sioReadCtrl16();
    unsigned short sioReadBaud16();

    void netError();

    void sioInterrupt();
    int  sioFreeze(gzFile f, int Mode);
    void LoadMcd(int mcd, char *str);
    void LoadMcds(char *mcd1, char *mcd2);
    void SaveMcd(char *mcd, char *data, uint32_t adr, int size);
    void CreateMcd(char *mcd);
    void ConvertMcd(char *mcd, char *data);
    void GetMcdBlockInfo(int mcd, int block);
};


#ifdef __cplusplus
}
#endif
#endif
