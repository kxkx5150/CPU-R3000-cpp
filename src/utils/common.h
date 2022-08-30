
#ifndef __PSXCOMMON_H__
#define __PSXCOMMON_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "../core/pcsx.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <assert.h>
#include <zlib.h>

typedef char          s8;
typedef int16_t       s16;
typedef int32_t       s32;
typedef int64_t       s64;
typedef intptr_t      sptr;
typedef unsigned char u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef uint64_t      u64;
typedef uintptr_t     uptr;
typedef uint8_t       boolean;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define MAXPATHLEN 4096
#define _(msgid)   msgid
#define N_(msgid)  msgid

#define SysPrintf printf
void SysMessage(const char *fmt, ...);
#define SysMessage printf


#if defined(__LINUX__) || defined(__MACOSX__)
#define strnicmp strncasecmp
#endif
#define __inline inline

extern int Log;
void       __Log(char *fmt, ...);
typedef struct
{
    char    Gpu[MAXPATHLEN];
    char    Spu[MAXPATHLEN];
    char    Cdr[MAXPATHLEN];
    char    Pad1[MAXPATHLEN];
    char    Pad2[MAXPATHLEN];
    char    Net[MAXPATHLEN];
    char    Sio1[MAXPATHLEN];
    char    Mcd1[MAXPATHLEN];
    char    Mcd2[MAXPATHLEN];
    char    Bios[MAXPATHLEN];
    char    BiosDir[MAXPATHLEN];
    char    PluginsDir[MAXPATHLEN];
    char    PatchesDir[MAXPATHLEN];
    boolean Xa;
    boolean Sio;
    boolean Mdec;
    boolean PsxAuto;
    boolean Cdda;
    boolean HLE;
    boolean Debug;
    boolean PsxOut;
    boolean SpuIrq;
    boolean RCntFix;
    boolean UseNet;
    boolean VSyncWA;
    u8      Cpu;
    u8      PsxType;
#ifdef _WIN32
    char Lang[256];
#endif
} PcsxConfig;
extern PcsxConfig Config;

#if 0
#define gzfreeze(ptr, size)                                                                                            \
    {                                                                                                                  \
        if (Mode == 1)                                                                                                 \
            gzwrite(f, ptr, size);                                                                                     \
        if (Mode == 0)                                                                                                 \
            gzread(f, ptr, size);                                                                                      \
    }
#else
#define gzfreeze(ptr, size)                                                                                            \
    {                                                                                                                  \
    }
#endif
#define BIAS   2
#define PSXCLK 33868800
enum
{
    PSX_TYPE_NTSC = 0,
    PSX_TYPE_PAL
};
enum
{
    CPU_DYNAREC = 0,
    CPU_INTERPRETER
};
int  EmuInit();
void EmuReset();
void EmuShutdown();
void EmuUpdate();
#ifdef __cplusplus
}
#endif
#endif
