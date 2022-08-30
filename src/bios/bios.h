
#ifndef __PSXBIOS_H__
#define __PSXBIOS_H__
#include "../utils/common.h"
#include "../core/mem.h"
#include "../core/sio.h"
#include "../core/plugins.h"

#ifdef __cplusplus

extern "C" {
#endif

extern char *biosA0n[256];
extern char *biosB0n[256];
extern char *biosC0n[256];

extern void (*biosA0[256])();
extern void (*biosB0[256])();
extern void (*biosC0[256])();
extern boolean hleSoftCall;

#undef s_addr
typedef struct
{
    unsigned char id[8];
    u32           text;
    u32           data;
    u32           pc0;
    u32           gp0;
    u32           t_addr;
    u32           t_size;
    u32           d_addr;
    u32           d_size;
    u32           b_addr;
    u32           b_size;
    u32           s_addr;
    u32           s_size;
    u32           SavedSP;
    u32           SavedFP;
    u32           SavedGP;
    u32           SavedRA;
    u32           SavedS0;
} EXE_HEADER;

extern char CdromId[10];
extern char CdromLabel[33];

int LoadCdrom();
int LoadCdromFile(const char *filename, EXE_HEADER *head);
int CheckCdrom();
int Load(const char *ExePath);

int SaveState(const char *file);
int LoadState(const char *file);
int CheckState(const char *file);

int SendPcsxInfo();
int RecvPcsxInfo();

void trim(char *str);
u16  calcCrc(u8 *d, int len);

void psxBiosInit();
void psxBiosShutdown();
void psxBiosException();
void psxBiosFreeze(int Mode);

#ifdef __cplusplus
}
#endif
#endif
