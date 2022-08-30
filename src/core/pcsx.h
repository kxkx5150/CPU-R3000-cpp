#ifndef __LINUX_H__
#define __LINUX_H__
#ifdef __cplusplus
extern "C" {
#endif

#define ENABLE_NLS       1
#define HAVE_DCGETTEXT   1
#define HAVE_DLFCN_H     1
#define HAVE_GETTEXT     1
#define HAVE_INTTYPES_H  1
#define HAVE_MEMORY_H    1
#define HAVE_STDINT_H    1
#define HAVE_STDLIB_H    1
#define HAVE_STRINGS_H   1
#define HAVE_STRING_H    1
#define HAVE_SYS_STAT_H  1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H    1

#define PACKAGE_VERSION "1.0"
#define STDC_HEADERS    1
#define VERSION         "1.0"
#define MEMCARD_DIR     "/.pcsx/memcards/"
#define __LINUX__       1

#ifdef __cplusplus

class R3000Acpu;
class GTE;
class MEM;
class DMA;
class SIO;
class MDEC;
class PLUGINS;
class HW;
class Counter;
class CDRISO;
class CDROM;

class PCSX {
  public:
    R3000Acpu *psxCpu   = nullptr;
    GTE       *psxGte   = nullptr;
    MEM       *psxMem   = nullptr;
    DMA       *psxDma   = nullptr;
    SIO       *psxSio   = nullptr;
    MDEC      *psxMdec  = nullptr;
    PLUGINS   *psxPlugs = nullptr;
    HW        *psxHw    = nullptr;
    Counter   *psxCntr  = nullptr;
    CDRISO    *psxCIso  = nullptr;
    CDROM     *psxCdrom = nullptr;

  public:
    unsigned long gpuDisp;
    short         modctrl = 0, modalt = 0;

  public:
    PCSX();

    void CreateMemcard(char *filename, char *conf_mcd);
    int  SysInit();
    void SysReset();
    void SysClose();
    void SysUpdate();
    void PADhandleKey(int key);
    void SignalExit(int sig);
    int  _OpenPlugins();
    int  OpenPlugins();
    void ClosePlugins();
    int  mainloop(const char *isofilename);
};
#endif


#ifdef __cplusplus
}
#endif
#endif
