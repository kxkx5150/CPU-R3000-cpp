#include "../core/r3000a.h"
#include "bios.h"
#include "../core/hw.h"
#include "../core/cdrom.h"
#include "../core/mdec.h"
#include "../core/mem.h"
#include "../core/sio.h"
#include "../plugins/dfxvideo/gpu.h"
#include "../core/cdriso.h"

#define PSX_EXE          1
#define CPE_EXE          2
#define COFF_EXE         3
#define INVALID_EXE      4
#define ISODCL(from, to) (to - from + 1)

char CdromId[10]    = "";
char CdromLabel[33] = "";

extern PCSX *g_pcsx;


struct iso_directory_record
{
    char          length[ISODCL(1, 1)];
    char          ext_attr_length[ISODCL(2, 2)];
    char          extent[ISODCL(3, 10)];
    char          size[ISODCL(11, 18)];
    char          date[ISODCL(19, 25)];
    char          flags[ISODCL(26, 26)];
    char          file_unit_size[ISODCL(27, 27)];
    char          interleave[ISODCL(28, 28)];
    char          volume_sequence_number[ISODCL(29, 32)];
    unsigned char name_len[ISODCL(33, 33)];
    char          name[1];
};
void showdir(struct iso_directory_record *dir)
{
    unsigned char *b = (unsigned char *)dir;
    for (int ii = 0; ii < 33; ii++)
        printf("(%d:%x) ", ii, b[ii]);
    printf("\n");
}
void mmssdd(char *b, char *p)
{
    int m, s, d;
#if defined(__BIGENDIAN__)
    int block = (b[0] & 0xff) | ((b[1] & 0xff) << 8) | ((b[2] & 0xff) << 16) | (b[3] << 24);
#else
    int block = (b[0] & 0xff) | ((b[1] & 0xff) << 8) | ((b[2] & 0xff) << 16) | (b[3] << 24);
#endif
    block += 150;
    m     = block / 4500;
    block = block - m * 4500;
    s     = block / 75;
    d     = block - s * 75;
    m     = ((m / 10) << 4) | m % 10;
    s     = ((s / 10) << 4) | s % 10;
    d     = ((d / 10) << 4) | d % 10;
    p[0]  = m;
    p[1]  = s;
    p[2]  = d;
}
#define incTime()                                                                                                      \
    time[0] = btoi(time[0]);                                                                                           \
    time[1] = btoi(time[1]);                                                                                           \
    time[2] = btoi(time[2]);                                                                                           \
    time[2]++;                                                                                                         \
    if (time[2] == 75) {                                                                                               \
        time[2] = 0;                                                                                                   \
        time[1]++;                                                                                                     \
        if (time[1] == 60) {                                                                                           \
            time[1] = 0;                                                                                               \
            time[0]++;                                                                                                 \
        }                                                                                                              \
    }                                                                                                                  \
    time[0] = itob(time[0]);                                                                                           \
    time[1] = itob(time[1]);                                                                                           \
    time[2] = itob(time[2]);
#define READTRACK()                                                                                                    \
    if (g_pcsx->psxPlugs->CDRreadTrack(time) == -1)                                                                    \
        return -1;                                                                                                     \
    buf = g_pcsx->psxPlugs->CDRgetBuffer();                                                                            \
    if (buf == NULL)                                                                                                   \
        return -1;
#define READDIR(_dir)                                                                                                  \
    READTRACK();                                                                                                       \
    memcpy(_dir, buf + 12, 2048);                                                                                      \
    incTime();                                                                                                         \
    READTRACK();                                                                                                       \
    memcpy(_dir + 2048, buf + 12, 2048);
int GetCdromFile(u8 *mdir, u8 *time, s8 *filename)
{
    struct iso_directory_record *dir;
    u8                           ddir[4096];
    u8                          *buf;
    int                          i;
    if (!strlen(filename))
        return -1;
    i = 0;
    while (i < 4096) {
        dir = (struct iso_directory_record *)&mdir[i];
        if (dir->length[0] == 0) {
            return -1;
        }
        i += dir->length[0];
        if (dir->flags[0] & 0x2) {
            if (!strnicmp((char *)&dir->name[0], filename, dir->name_len[0])) {
                if (filename[dir->name_len[0]] != '\\')
                    continue;
                filename += dir->name_len[0] + 1;
                mmssdd(dir->extent, (char *)time);
                READDIR(ddir);
                i    = 0;
                mdir = ddir;
            }
        } else {
            if (!strnicmp((char *)&dir->name[0], filename, strlen(filename))) {
                mmssdd(dir->extent, (char *)time);
                break;
            }
        }
    }
    return 0;
}
int LoadCdrom()
{
    EXE_HEADER                   tmpHead;
    struct iso_directory_record *dir;
    u8                           time[4];
    u8                          *buf;
    u8                           mdir[4096];
    s8                           exename[257];
    if (!Config.HLE) {
        g_pcsx->psxCpu->psxRegs.pc = g_pcsx->psxCpu->psxRegs.GPR.n.ra;
        return 0;
    }
    time[0] = itob(0);
    time[1] = itob(2);
    time[2] = itob(0x10);
    READTRACK();
    printf("c\n");
    dir = (struct iso_directory_record *)&buf[12 + 156];
    mmssdd(dir->extent, (char *)time);
    READDIR(mdir);
    if (GetCdromFile(mdir, time, (char *)"SYSTEM.CNF;1") == -1) {
        if (GetCdromFile(mdir, time, (char *)"PSX.EXE;1") == -1)
            return -1;
        READTRACK();
    } else {
        READTRACK();
        sscanf((char *)buf + 12, "BOOT = cdrom:\\%256s", exename);
        if (GetCdromFile(mdir, time, exename) == -1) {
            sscanf((char *)buf + 12, "BOOT = cdrom:%256s", exename);
            printf("%s\n", buf);
            if (GetCdromFile(mdir, time, exename) == -1) {
                char *ptr = strstr((char *)buf + 12, "cdrom:");
                if (ptr != NULL) {
                    ptr += 6;
                    while (*ptr == '\\' || *ptr == '/')
                        ptr++;
                    strncpy(exename, ptr, 255);
                    exename[255] = '\0';
                    ptr          = exename;
                    while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n')
                        ptr++;
                    *ptr = '\0';
                    if (GetCdromFile(mdir, time, exename) == -1) {
                        printf("d\n");
                        return -1;
                    }
                } else {
                    printf("e\n");
                    return -1;
                }
            }
        }
        READTRACK();
    }
    memcpy(&tmpHead, buf + 12, sizeof(EXE_HEADER));
    g_pcsx->psxCpu->psxRegs.pc       = SWAP32(tmpHead.pc0);
    g_pcsx->psxCpu->psxRegs.GPR.n.gp = SWAP32(tmpHead.gp0);
    g_pcsx->psxCpu->psxRegs.GPR.n.sp = SWAP32(tmpHead.s_addr);
    if (g_pcsx->psxCpu->psxRegs.GPR.n.sp == 0)
        g_pcsx->psxCpu->psxRegs.GPR.n.sp = 0x801fff00;
    tmpHead.t_size = SWAP32(tmpHead.t_size);
    tmpHead.t_addr = SWAP32(tmpHead.t_addr);
    while (tmpHead.t_size) {
        void *ptr = (void *)PSXM(tmpHead.t_addr);
        incTime();
        READTRACK();
        if (ptr != NULL)
            memcpy(ptr, buf + 12, 2048);
        tmpHead.t_size -= 2048;
        tmpHead.t_addr += 2048;
    }
    return 0;
}
int LoadCdromFile(const char *filename, EXE_HEADER *head)
{
    struct iso_directory_record *dir;
    u8                           time[4], *buf;
    u8                           mdir[4096];
    s8                           exename[257];
    u32                          size, addr;
    sscanf(filename, "cdrom:\\%256s", exename);
    time[0] = itob(0);
    time[1] = itob(2);
    time[2] = itob(0x10);
    READTRACK();
    dir = (struct iso_directory_record *)&buf[12 + 156];
    mmssdd(dir->extent, (char *)time);
    READDIR(mdir);
    if (GetCdromFile(mdir, time, exename) == -1)
        return -1;
    READTRACK();
    memcpy(head, buf + 12, sizeof(EXE_HEADER));
    size = head->t_size;
    addr = head->t_addr;
    while (size) {
        incTime();
        READTRACK();
        memcpy((void *)PSXM(addr), buf + 12, 2048);
        size -= 2048;
        addr += 2048;
    }
    return 0;
}
int CheckCdrom()
{
    struct iso_directory_record *dir;
    unsigned char                time[4], *buf;
    unsigned char                mdir[4096];
    char                         exename[257];
    int                          i, c;
    time[0] = itob(0);
    time[1] = itob(2);
    time[2] = itob(0x10);
    printf("check cdrom\n");
    READTRACK();
    printf("check cdrom 2\n");
    for (int ii = 0; ii < 84; ii++)
        printf("%02x ", buf[ii]);
    printf("\n");
    CdromLabel[0] = '\0';
    CdromId[0]    = '\0';
    strncpy(CdromLabel, (s8 *)buf + 52, 32);
    printf("check cdrom 3, %s\n", CdromLabel);
    dir = (struct iso_directory_record *)&buf[12 + 156];
    mmssdd(dir->extent, (char *)time);
    for (int ii = 0; ii < 4; ii++)
        printf("%02x ", time[ii]);
    printf("\n");
    READDIR(mdir);
    printf("check cdrom 4\n");
    for (int ii = 0; ii < 40; ii++)
        printf("%02x ", mdir[ii]);
    printf("\n");
    if (GetCdromFile(mdir, time, (char *)"SYSTEM.CNF;1") != -1) {
        printf("check cdrom 5\n");
        READTRACK();
        for (int ii = 0; ii < 84; ii++)
            printf("%02x ", buf[ii]);
        printf("\n");
        sscanf((char *)buf + 12, "BOOT = cdrom:\\%256s", exename);
        printf("check cdrom 6 %s\n", exename);
        if (GetCdromFile(mdir, time, exename) == -1) {
            sscanf((char *)buf + 12, "BOOT = cdrom:%256s", exename);
            if (GetCdromFile(mdir, time, exename) == -1) {
                char *ptr = strstr((s8 *)buf + 12, "cdrom:");
                if (ptr != NULL) {
                    ptr += 6;
                    while (*ptr == '\\' || *ptr == '/')
                        ptr++;
                    strncpy(exename, ptr, 255);
                    exename[255] = '\0';
                    ptr          = exename;
                    while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n')
                        ptr++;
                    *ptr = '\0';
                    if (GetCdromFile(mdir, time, exename) == -1)
                        return -1;
                } else
                    return -1;
            }
        }
    } else if (GetCdromFile(mdir, time, (char *)"PSX.EXE;1") != -1) {
        strcpy(exename, "PSX.EXE;1");
        strcpy(CdromId, "SLUS99999");
    } else
        return -1;
    if (CdromId[0] == '\0') {
        i = strlen(exename);
        if (i >= 2) {
            if (exename[i - 2] == ';')
                i -= 2;
            c = 8;
            i--;
            while (i >= 0 && c >= 0) {
                if (isalnum(exename[i]))
                    CdromId[c--] = exename[i];
                i--;
            }
        }
    }
    if (Config.PsxAuto) {
        if (strstr(exename, "ES") != NULL)
            Config.PsxType = PSX_TYPE_PAL;
        else
            Config.PsxType = PSX_TYPE_NTSC;
    }
    if (CdromLabel[0] == ' ') {
        strncpy(CdromLabel, CdromId, 9);
    }
    SysPrintf(_("CD-ROM Label: %.32s\n"), CdromLabel);
    SysPrintf(_("CD-ROM ID: %.9s\n"), CdromId);
    return 0;
}
struct external_filehdr
{
    unsigned short f_magic;
    unsigned short f_nscns;
    unsigned long  f_timdat;
    unsigned long  f_symptr;
    unsigned long  f_nsyms;
    unsigned short f_opthdr;
    unsigned short f_flags;
};
static int PSXGetFileType(FILE *f)
{
    unsigned long            current;
    u8                       mybuf[2048];
    EXE_HEADER              *exe_hdr;
    struct external_filehdr *coff_hdr;
    current = ftell(f);
    fseek(f, 0L, SEEK_SET);
    fread(mybuf, 2048, 1, f);
    fseek(f, current, SEEK_SET);
    exe_hdr = (EXE_HEADER *)mybuf;
    if (memcmp(exe_hdr->id, "PS-X EXE", 8) == 0)
        return PSX_EXE;
    if (mybuf[0] == 'C' && mybuf[1] == 'P' && mybuf[2] == 'E')
        return CPE_EXE;
    coff_hdr = (struct external_filehdr *)mybuf;
    if (SWAPu16(coff_hdr->f_magic) == 0x0162)
        return COFF_EXE;
    return INVALID_EXE;
}
int Load(const char *ExePath)
{
    FILE      *tmpFile;
    EXE_HEADER tmpHead;
    int        type;
    int        retval = 0;
    u8         opcode;
    u32        section_address, section_size;
    strncpy(CdromId, "SLUS99999", 9);
    strncpy(CdromLabel, "SLUS_999.99", 11);
    tmpFile = fopen(ExePath, "rb");
    if (tmpFile == NULL) {
        SysPrintf(_("Error opening file: %s.\n"), ExePath);
        retval = -1;
    } else {
        type = PSXGetFileType(tmpFile);
        switch (type) {
            case PSX_EXE:
                fread(&tmpHead, sizeof(EXE_HEADER), 1, tmpFile);
                fseek(tmpFile, 0x800, SEEK_SET);
                fread((void *)PSXM(SWAP32(tmpHead.t_addr)), SWAP32(tmpHead.t_size), 1, tmpFile);
                fclose(tmpFile);
                g_pcsx->psxCpu->psxRegs.pc       = SWAP32(tmpHead.pc0);
                g_pcsx->psxCpu->psxRegs.GPR.n.gp = SWAP32(tmpHead.gp0);
                g_pcsx->psxCpu->psxRegs.GPR.n.sp = SWAP32(tmpHead.s_addr);
                if (g_pcsx->psxCpu->psxRegs.GPR.n.sp == 0)
                    g_pcsx->psxCpu->psxRegs.GPR.n.sp = 0x801fff00;
                retval = 0;
                break;
            case CPE_EXE:
                fseek(tmpFile, 6, SEEK_SET);
                do {
                    fread(&opcode, 1, 1, tmpFile);
                    switch (opcode) {
                        case 1:
                            fread(&section_address, 4, 1, tmpFile);
                            fread(&section_size, 4, 1, tmpFile);
                            section_address = SWAPu32(section_address);
                            section_size    = SWAPu32(section_size);
                            fread(PSXM(section_address), section_size, 1, tmpFile);
                            break;
                        case 3:
                            fseek(tmpFile, 2, SEEK_CUR);
                            fread(&g_pcsx->psxCpu->psxRegs.pc, 4, 1, tmpFile);
                            g_pcsx->psxCpu->psxRegs.pc = SWAPu32(g_pcsx->psxCpu->psxRegs.pc);
                            break;
                        case 0:
                            break;
                        default:
                            SysPrintf(_("Unknown CPE opcode %02x at position %08ld.\n"), opcode, ftell(tmpFile) - 1);
                            retval = -1;
                            break;
                    }
                } while (opcode != 0 && retval == 0);
                break;
            case COFF_EXE:
                SysPrintf(_("COFF files not supported.\n"));
                retval = -1;
                break;
            case INVALID_EXE:
                SysPrintf(_("This file does not appear to be a valid PSX file.\n"));
                retval = -1;
                break;
        }
    }
    if (retval != 0) {
        CdromId[0]    = '\0';
        CdromLabel[0] = '\0';
    }
    return retval;
}
static const char PcsxHeader[32] = "STv4 PCSX v" PACKAGE_VERSION;
static const u32  SaveVersion    = 0x8b410004;
void              trim(char *str)
{
    int   pos  = 0;
    char *dest = str;
    while (str[pos] <= ' ' && str[pos] > 0)
        pos++;
    while (str[pos]) {
        *(dest++) = str[pos];
        pos++;
    }
    *(dest--) = '\0';
    while (dest >= str && *dest <= ' ' && *dest > 0)
        *(dest--) = '\0';
}
static unsigned short crctab[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD,
    0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A,
    0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B,
    0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861,
    0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96,
    0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87,
    0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A,
    0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3,
    0x5004, 0x4025, 0x7046, 0x6067, 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290,
    0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E,
    0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F,
    0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 0xCB7D, 0xDB5C,
    0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83,
    0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74,
    0x2E93, 0x3EB2, 0x0ED1, 0x1EF0};
u16 calcCrc(u8 *d, int len)
{
    u16 crc = 0;
    int i;
    for (i = 0; i < len; i++) {
        crc = crctab[(crc >> 8) ^ d[i]] ^ (crc << 8);
    }
    return ~crc;
}
char *biosA0n[256] = {
    (char *)"open",
    (char *)"lseek",
    (char *)"read",
    (char *)"write",
    (char *)"close",
    (char *)"ioctl",
    (char *)"exit",
    (char *)"sys_a0_07",
    (char *)"getc",
    (char *)"putc",
    (char *)"todigit",
    (char *)"atof",
    (char *)"strtoul",
    (char *)"strtol",
    (char *)"abs",
    (char *)"labs",
    (char *)"atoi",
    (char *)"atol",
    (char *)"atob",
    (char *)"setjmp",
    (char *)"longjmp",
    (char *)"strcat",
    (char *)"strncat",
    (char *)"strcmp",
    (char *)"strncmp",
    (char *)"strcpy",
    (char *)"strncpy",
    (char *)"strlen",
    (char *)"index",
    (char *)"rindex",
    (char *)"strchr",
    (char *)"strrchr",
    (char *)"strpbrk",
    (char *)"strspn",
    (char *)"strcspn",
    (char *)"strtok",
    (char *)"strstr",
    (char *)"toupper",
    (char *)"tolower",
    (char *)"bcopy",
    (char *)"bzero",
    (char *)"bcmp",
    (char *)"memcpy",
    (char *)"memset",
    (char *)"memmove",
    (char *)"memcmp",
    (char *)"memchr",
    (char *)"rand",
    (char *)"srand",
    (char *)"qsort",
    (char *)"strtod",
    (char *)"malloc",
    (char *)"free",
    (char *)"lsearch",
    (char *)"bsearch",
    (char *)"calloc",
    (char *)"realloc",
    (char *)"InitHeap",
    (char *)"_exit",
    (char *)"getchar",
    (char *)"putchar",
    (char *)"gets",
    (char *)"puts",
    (char *)"printf",
    (char *)"sys_a0_40",
    (char *)"LoadTest",
    (char *)"Load",
    (char *)"Exec",
    (char *)"FlushCache",
    (char *)"InstallInterruptHandler",
    (char *)"GPU_dw",
    (char *)"mem2vram",
    (char *)"SendGPUStatus",
    (char *)"GPU_cw",
    (char *)"GPU_cwb",
    (char *)"SendPackets",
    (char *)"sys_a0_4c",
    (char *)"GetGPUStatus",
    (char *)"GPU_sync",
    (char *)"sys_a0_4f",
    (char *)"sys_a0_50",
    (char *)"LoadExec",
    (char *)"GetSysSp",
    (char *)"sys_a0_53",
    (char *)"_96_init()",
    (char *)"_bu_init()",
    (char *)"_96_remove()",
    (char *)"sys_a0_57",
    (char *)"sys_a0_58",
    (char *)"sys_a0_59",
    (char *)"sys_a0_5a",
    (char *)"dev_tty_init",
    (char *)"dev_tty_open",
    (char *)"sys_a0_5d",
    (char *)"dev_tty_ioctl",
    (char *)"dev_cd_open",
    (char *)"dev_cd_read",
    (char *)"dev_cd_close",
    (char *)"dev_cd_firstfile",
    (char *)"dev_cd_nextfile",
    (char *)"dev_cd_chdir",
    (char *)"dev_card_open",
    (char *)"dev_card_read",
    (char *)"dev_card_write",
    (char *)"dev_card_close",
    (char *)"dev_card_firstfile",
    (char *)"dev_card_nextfile",
    (char *)"dev_card_erase",
    (char *)"dev_card_undelete",
    (char *)"dev_card_format",
    (char *)"dev_card_rename",
    (char *)"dev_card_6f",
    (char *)"_bu_init",
    (char *)"_96_init",
    (char *)"_96_remove",
    (char *)"sys_a0_73",
    (char *)"sys_a0_74",
    (char *)"sys_a0_75",
    (char *)"sys_a0_76",
    (char *)"sys_a0_77",
    (char *)"_96_CdSeekL",
    (char *)"sys_a0_79",
    (char *)"sys_a0_7a",
    (char *)"sys_a0_7b",
    (char *)"_96_CdGetStatus",
    (char *)"sys_a0_7d",
    (char *)"_96_CdRead",
    (char *)"sys_a0_7f",
    (char *)"sys_a0_80",
    (char *)"sys_a0_81",
    (char *)"sys_a0_82",
    (char *)"sys_a0_83",
    (char *)"sys_a0_84",
    (char *)"_96_CdStop",
    (char *)"sys_a0_86",
    (char *)"sys_a0_87",
    (char *)"sys_a0_88",
    (char *)"sys_a0_89",
    (char *)"sys_a0_8a",
    (char *)"sys_a0_8b",
    (char *)"sys_a0_8c",
    (char *)"sys_a0_8d",
    (char *)"sys_a0_8e",
    (char *)"sys_a0_8f",
    (char *)"sys_a0_90",
    (char *)"sys_a0_91",
    (char *)"sys_a0_92",
    (char *)"sys_a0_93",
    (char *)"sys_a0_94",
    (char *)"sys_a0_95",
    (char *)"AddCDROMDevice",
    (char *)"AddMemCardDevide",
    (char *)"DisableKernelIORedirection",
    (char *)"EnableKernelIORedirection",
    (char *)"sys_a0_9a",
    (char *)"sys_a0_9b",
    (char *)"SetConf",
    (char *)"GetConf",
    (char *)"sys_a0_9e",
    (char *)"SetMem",
    (char *)"_boot",
    (char *)"SystemError",
    (char *)"EnqueueCdIntr",
    (char *)"DequeueCdIntr",
    (char *)"sys_a0_a4",
    (char *)"ReadSector",
    (char *)"get_cd_status",
    (char *)"bufs_cb_0",
    (char *)"bufs_cb_1",
    (char *)"bufs_cb_2",
    (char *)"bufs_cb_3",
    (char *)"_card_info",
    (char *)"_card_load",
    (char *)"_card_auto",
    (char *)"bufs_cd_4",
    (char *)"sys_a0_af",
    (char *)"sys_a0_b0",
    (char *)"sys_a0_b1",
    (char *)"do_a_long_jmp",
    (char *)"sys_a0_b3",
    (char *)"?? sub_function",
};
char *biosB0n[256] = {
    (char *)"SysMalloc",
    (char *)"sys_b0_01",
    (char *)"sys_b0_02",
    (char *)"sys_b0_03",
    (char *)"sys_b0_04",
    (char *)"sys_b0_05",
    (char *)"sys_b0_06",
    (char *)"DeliverEvent",
    (char *)"OpenEvent",
    (char *)"CloseEvent",
    (char *)"WaitEvent",
    (char *)"TestEvent",
    (char *)"EnableEvent",
    (char *)"DisableEvent",
    (char *)"OpenTh",
    (char *)"CloseTh",
    (char *)"ChangeTh",
    (char *)"sys_b0_11",
    (char *)"InitPAD",
    (char *)"StartPAD",
    (char *)"StopPAD",
    (char *)"PAD_init",
    (char *)"PAD_dr",
    (char *)"ReturnFromExecption",
    (char *)"ResetEntryInt",
    (char *)"HookEntryInt",
    (char *)"sys_b0_1a",
    (char *)"sys_b0_1b",
    (char *)"sys_b0_1c",
    (char *)"sys_b0_1d",
    (char *)"sys_b0_1e",
    (char *)"sys_b0_1f",
    (char *)"UnDeliverEvent",
    (char *)"sys_b0_21",
    (char *)"sys_b0_22",
    (char *)"sys_b0_23",
    (char *)"sys_b0_24",
    (char *)"sys_b0_25",
    (char *)"sys_b0_26",
    (char *)"sys_b0_27",
    (char *)"sys_b0_28",
    (char *)"sys_b0_29",
    (char *)"sys_b0_2a",
    (char *)"sys_b0_2b",
    (char *)"sys_b0_2c",
    (char *)"sys_b0_2d",
    (char *)"sys_b0_2e",
    (char *)"sys_b0_2f",
    (char *)"sys_b0_30",
    (char *)"sys_b0_31",
    (char *)"open",
    (char *)"lseek",
    (char *)"read",
    (char *)"write",
    (char *)"close",
    (char *)"ioctl",
    (char *)"exit",
    (char *)"sys_b0_39",
    (char *)"getc",
    (char *)"putc",
    (char *)"getchar",
    (char *)"putchar",
    (char *)"gets",
    (char *)"puts",
    (char *)"cd",
    (char *)"format",
    (char *)"firstfile",
    (char *)"nextfile",
    (char *)"rename",
    (char *)"delete",
    (char *)"undelete",
    (char *)"AddDevice",
    (char *)"RemoteDevice",
    (char *)"PrintInstalledDevices",
    (char *)"InitCARD",
    (char *)"StartCARD",
    (char *)"StopCARD",
    (char *)"sys_b0_4d",
    (char *)"_card_write",
    (char *)"_card_read",
    (char *)"_new_card",
    (char *)"Krom2RawAdd",
    (char *)"sys_b0_52",
    (char *)"sys_b0_53",
    (char *)"_get_errno",
    (char *)"_get_error",
    (char *)"GetC0Table",
    (char *)"GetB0Table",
    (char *)"_card_chan",
    (char *)"sys_b0_59",
    (char *)"sys_b0_5a",
    (char *)"ChangeClearPAD",
    (char *)"_card_status",
    (char *)"_card_wait",
};
char *biosC0n[256] = {
    (char *)"InitRCnt",           (char *)"InitException",
    (char *)"SysEnqIntRP",        (char *)"SysDeqIntRP",
    (char *)"get_free_EvCB_slot", (char *)"get_free_TCB_slot",
    (char *)"ExceptionHandler",   (char *)"InstallExeptionHandler",
    (char *)"SysInitMemory",      (char *)"SysInitKMem",
    (char *)"ChangeClearRCnt",    (char *)"SystemError",
    (char *)"InitDefInt",         (char *)"sys_c0_0d",
    (char *)"sys_c0_0e",          (char *)"sys_c0_0f",
    (char *)"sys_c0_10",          (char *)"sys_c0_11",
    (char *)"InstallDevices",     (char *)"FlushStfInOutPut",
    (char *)"sys_c0_14",          (char *)"_cdevinput",
    (char *)"_cdevscan",          (char *)"_circgetc",
    (char *)"_circputc",          (char *)"ioabort",
    (char *)"sys_c0_1a",          (char *)"KernelRedirect",
    (char *)"PatchAOTable",
};
#define at  (g_pcsx->psxCpu->psxRegs.GPR.n.at)
#define v0  (g_pcsx->psxCpu->psxRegs.GPR.n.v0)
#define v1  (g_pcsx->psxCpu->psxRegs.GPR.n.v1)
#define a0  (g_pcsx->psxCpu->psxRegs.GPR.n.a0)
#define a1  (g_pcsx->psxCpu->psxRegs.GPR.n.a1)
#define a2  (g_pcsx->psxCpu->psxRegs.GPR.n.a2)
#define a3  (g_pcsx->psxCpu->psxRegs.GPR.n.a3)
#define t0  (g_pcsx->psxCpu->psxRegs.GPR.n.t0)
#define t1  (g_pcsx->psxCpu->psxRegs.GPR.n.t1)
#define t2  (g_pcsx->psxCpu->psxRegs.GPR.n.t2)
#define t3  (g_pcsx->psxCpu->psxRegs.GPR.n.t3)
#define t4  (g_pcsx->psxCpu->psxRegs.GPR.n.t4)
#define t5  (g_pcsx->psxCpu->psxRegs.GPR.n.t5)
#define t6  (g_pcsx->psxCpu->psxRegs.GPR.n.t6)
#define t7  (g_pcsx->psxCpu->psxRegs.GPR.n.t7)
#define t8  (g_pcsx->psxCpu->psxRegs.GPR.n.t8)
#define t9  (g_pcsx->psxCpu->psxRegs.GPR.n.t9)
#define s0  (g_pcsx->psxCpu->psxRegs.GPR.n.s0)
#define s1  (g_pcsx->psxCpu->psxRegs.GPR.n.s1)
#define s2  (g_pcsx->psxCpu->psxRegs.GPR.n.s2)
#define s3  (g_pcsx->psxCpu->psxRegs.GPR.n.s3)
#define s4  (g_pcsx->psxCpu->psxRegs.GPR.n.s4)
#define s5  (g_pcsx->psxCpu->psxRegs.GPR.n.s5)
#define s6  (g_pcsx->psxCpu->psxRegs.GPR.n.s6)
#define s7  (g_pcsx->psxCpu->psxRegs.GPR.n.s7)
#define k0  (g_pcsx->psxCpu->psxRegs.GPR.n.k0)
#define k1  (g_pcsx->psxCpu->psxRegs.GPR.n.k1)
#define gp  (g_pcsx->psxCpu->psxRegs.GPR.n.gp)
#define sp  (g_pcsx->psxCpu->psxRegs.GPR.n.sp)
#define fp  (g_pcsx->psxCpu->psxRegs.GPR.n.s8)
#define ra  (g_pcsx->psxCpu->psxRegs.GPR.n.ra)
#define pc0 (g_pcsx->psxCpu->psxRegs.pc)
#define Ra0 ((char *)PSXM(a0))
#define Ra1 ((char *)PSXM(a1))
#define Ra2 ((char *)PSXM(a2))
#define Ra3 ((char *)PSXM(a3))
#define Rv0 ((char *)PSXM(v0))
#define Rsp ((char *)PSXM(sp))
typedef struct
{
    u32 desc;
    s32 status;
    s32 mode;
    u32 fhandler;
} EvCB[32];
#define EvStUNUSED  0x0000
#define EvStWAIT    0x1000
#define EvStACTIVE  0x2000
#define EvStALREADY 0x4000
#define EvMdINTR    0x1000
#define EvMdNOINTR  0x2000
typedef struct
{
    s32 status;
    s32 mode;
    u32 reg[32];
    u32 func;
} TCB;
typedef struct
{
    u32 _pc0;
    u32 gp0;
    u32 t_addr;
    u32 t_size;
    u32 d_addr;
    u32 d_size;
    u32 b_addr;
    u32 b_size;
    u32 S_addr;
    u32 s_size;
    u32 _sp, _fp, _gp, ret, base;
} EXEC;
struct DIRENTRY
{
    char             name[20];
    s32              attr;
    s32              size;
    struct DIRENTRY *next;
    s32              head;
    char             system[4];
};
typedef struct
{
    char name[32];
    u32  mode;
    u32  offset;
    u32  size;
    u32  mcfile;
} FileDesc;
static u32        *jmp_int  = NULL;
static int        *pad_buf  = NULL;
static char       *pad_buf1 = NULL, *pad_buf2 = NULL;
static int         pad_buf1len, pad_buf2len;
static u32         regs[35];
static EvCB       *Event;
static EvCB       *HwEV;
static EvCB       *EvEV;
static EvCB       *RcEV;
static EvCB       *UeEV;
static EvCB       *SwEV;
static EvCB       *ThEV;
static u32        *heap_addr = NULL;
static u32        *heap_end  = NULL;
static u32         SysIntRP[8];
static int         CardState = -1;
static TCB         Thread[8];
static int         CurThread = 0;
static FileDesc    FDesc[32];
boolean            hleSoftCall = FALSE;
static inline void softCall(u32 pc)
{
    pc0         = pc;
    ra          = 0x80001000;
    hleSoftCall = TRUE;
    while (pc0 != 0x80001000)
        g_pcsx->psxCpu->intExecuteBlock();
    hleSoftCall = FALSE;
}
static inline void softCall2(u32 pc)
{
    u32 sra     = ra;
    pc0         = pc;
    ra          = 0x80001000;
    hleSoftCall = TRUE;
    while (pc0 != 0x80001000)
        g_pcsx->psxCpu->intExecuteBlock();
    ra          = sra;
    hleSoftCall = FALSE;
}
static inline void DeliverEvent(u32 ev, u32 spec)
{
    if (Event[ev][spec].status != EvStACTIVE)
        return;
    if (Event[ev][spec].mode == EvMdINTR) {
        softCall2(Event[ev][spec].fhandler);
    } else
        Event[ev][spec].status = EvStALREADY;
}
static inline void SaveRegs()
{
    memcpy(regs, g_pcsx->psxCpu->psxRegs.GPR.r, 32 * 4);
    regs[32] = g_pcsx->psxCpu->psxRegs.GPR.n.lo;
    regs[33] = g_pcsx->psxCpu->psxRegs.GPR.n.hi;
    regs[34] = g_pcsx->psxCpu->psxRegs.pc;
}
static inline void LoadRegs()
{
    memcpy(g_pcsx->psxCpu->psxRegs.GPR.r, regs, 32 * 4);
    g_pcsx->psxCpu->psxRegs.GPR.n.lo = regs[32];
    g_pcsx->psxCpu->psxRegs.GPR.n.hi = regs[33];
}
void psxBios_abs()
{
    if ((s32)a0 < 0)
        v0 = -(s32)a0;
    else
        v0 = a0;
    pc0 = ra;
}
void psxBios_labs()
{
    psxBios_abs();
}
void psxBios_atoi()
{
    s32   n = 0, f = 0;
    char *p = (char *)Ra0;
    for (;; p++) {
        switch (*p) {
            case ' ':
            case '\t':
                continue;
            case '-':
                f++;
            case '+':
                p++;
        }
        break;
    }
    while (*p >= '0' && *p <= '9') {
        n = n * 10 + *p++ - '0';
    }
    v0  = (f ? -n : n);
    pc0 = ra;
}
void psxBios_atol()
{
    psxBios_atoi();
}
void psxBios_setjmp()
{
    u32 *jmp_buf = (u32 *)Ra0;
    int  i;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x13]);
#endif
    jmp_buf[0] = ra;
    jmp_buf[1] = sp;
    jmp_buf[2] = fp;
    for (i = 0; i < 8; i++)
        jmp_buf[3 + i] = g_pcsx->psxCpu->psxRegs.GPR.r[16 + i];
    jmp_buf[11] = gp;
    v0          = 0;
    pc0         = ra;
}
void psxBios_longjmp()
{
    u32 *jmp_buf = (u32 *)Ra0;
    int  i;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x14]);
#endif
    ra = jmp_buf[0];
    sp = jmp_buf[1];
    fp = jmp_buf[2];
    for (i = 0; i < 8; i++)
        g_pcsx->psxCpu->psxRegs.GPR.r[16 + i] = jmp_buf[3 + i];
    gp  = jmp_buf[11];
    v0  = a1;
    pc0 = ra;
}
void psxBios_strcat()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s, %s\n", biosA0n[0x15], Ra0, Ra1);
#endif
    while (*p1++)
        ;
    --p1;
    while ((*p1++ = *p2++) != '\0')
        ;
    v0  = a0;
    pc0 = ra;
}
void psxBios_strncat()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    s32   n = a2;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s (%x), %s (%x), %d\n", biosA0n[0x16], Ra0, a0, Ra1, a1, a2);
#endif
    while (*p1++)
        ;
    --p1;
    while ((*p1++ = *p2++) != '\0') {
        if (--n < 0) {
            *--p1 = '\0';
            break;
        }
    }
    v0  = a0;
    pc0 = ra;
}
void psxBios_strcmp()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s (%x), %s (%x)\n", biosA0n[0x17], Ra0, a0, Ra1, a1);
#endif
    while (*p1 == *p2++) {
        if (*p1++ == '\0') {
            v0  = 0;
            pc0 = ra;
            return;
        }
    }
    v0  = (*p1 - *--p2);
    pc0 = ra;
}
void psxBios_strncmp()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    s32   n = a2;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s (%x), %s (%x), %d\n", biosA0n[0x18], Ra0, a0, Ra1, a1, a2);
#endif
    while (--n >= 0 && *p1 == *p2++) {
        if (*p1++ == '\0') {
            v0  = 0;
            pc0 = ra;
            return;
        }
    }
    v0  = (n < 0 ? 0 : *p1 - *--p2);
    pc0 = ra;
}
void psxBios_strcpy()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    while ((*p1++ = *p2++) != '\0')
        ;
    v0  = a0;
    pc0 = ra;
}
void psxBios_strncpy()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    s32   n = a2, i;
    for (i = 0; i < n; i++) {
        if ((*p1++ = *p2++) == '\0') {
            while (++i < n) {
                *p1++ = '\0';
            }
            v0  = a0;
            pc0 = ra;
            return;
        }
    }
    v0  = a0;
    pc0 = ra;
}
void psxBios_strlen()
{
    char *p = (char *)Ra0;
    v0      = 0;
    while (*p++)
        v0++;
    pc0 = ra;
}
void psxBios_index()
{
    char *p = (char *)Ra0;
    do {
        if (*p == a1) {
            v0  = a0 + (p - (char *)Ra0);
            pc0 = ra;
            return;
        }
    } while (*p++ != '\0');
    v0  = 0;
    pc0 = ra;
}
void psxBios_rindex()
{
    char *p = (char *)Ra0;
    v0      = 0;
    do {
        if (*p == a1)
            v0 = a0 + (p - (char *)Ra0);
    } while (*p++ != '\0');
    pc0 = ra;
}
void psxBios_strchr()
{
    psxBios_index();
}
void psxBios_strrchr()
{
    psxBios_rindex();
}
void psxBios_strpbrk()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1, *scanp, c, sc;
    while ((c = *p1++) != '\0') {
        for (scanp = p2; (sc = *scanp++) != '\0';) {
            if (sc == c) {
                v0  = a0 + (p1 - 1 - (char *)Ra0);
                pc0 = ra;
                return;
            }
        }
    }
    v0  = a0;
    pc0 = ra;
}
void psxBios_strspn()
{
    char *p1, *p2;
    for (p1 = (char *)Ra0; *p1 != '\0'; p1++) {
        for (p2 = (char *)Ra1; *p2 != '\0' && *p2 != *p1; p2++)
            ;
        if (*p2 == '\0')
            break;
    }
    v0  = p1 - (char *)Ra0;
    pc0 = ra;
}
void psxBios_strcspn()
{
    char *p1, *p2;
    for (p1 = (char *)Ra0; *p1 != '\0'; p1++) {
        for (p2 = (char *)Ra1; *p2 != '\0' && *p2 != *p1; p2++)
            ;
        if (*p2 != '\0')
            break;
    }
    v0  = p1 - (char *)Ra0;
    pc0 = ra;
}
void psxBios_strtok()
{
    char *pcA0  = (char *)Ra0;
    char *pcRet = strtok(pcA0, (char *)Ra1);
    if (pcRet)
        v0 = a0 + pcRet - pcA0;
    else
        v0 = 0;
    pc0 = ra;
}
void psxBios_strstr()
{
    char *p = (char *)Ra0, *p1, *p2;
    while (*p != '\0') {
        p1 = p;
        p2 = (char *)Ra1;
        while (*p1 != '\0' && *p2 != '\0' && *p1 == *p2) {
            p1++;
            p2++;
        }
        if (*p2 == '\0') {
            v0  = a0 + (p - (char *)Ra0);
            pc0 = ra;
            return;
        }
        p++;
    }
    v0  = 0;
    pc0 = ra;
}
void psxBios_toupper()
{
    v0 = (s8)(a0 & 0xff);
    if (v0 >= 'a' && v0 <= 'z')
        v0 -= 'a' - 'A';
    pc0 = ra;
}
void psxBios_tolower()
{
    v0 = (s8)(a0 & 0xff);
    if (v0 >= 'A' && v0 <= 'Z')
        v0 += 'a' - 'A';
    pc0 = ra;
}
void psxBios_bcopy()
{
    char *p1 = (char *)Ra1, *p2 = (char *)Ra0;
    while (a2-- > 0)
        *p1++ = *p2++;
    pc0 = ra;
}
void psxBios_bzero()
{
    char *p = (char *)Ra0;
    while (a1-- > 0)
        *p++ = '\0';
    pc0 = ra;
}
void psxBios_bcmp()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    if (a0 == 0 || a1 == 0) {
        v0  = 0;
        pc0 = ra;
        return;
    }
    while (a2-- > 0) {
        if (*p1++ != *p2++) {
            v0  = *p1 - *p2;
            pc0 = ra;
            return;
        }
    }
    v0  = 0;
    pc0 = ra;
}
void psxBios_memcpy()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    while (a2-- > 0)
        *p1++ = *p2++;
    v0  = a0;
    pc0 = ra;
}
void psxBios_memset()
{
    char *p = (char *)Ra0;
    while (a2-- > 0)
        *p++ = (char)a1;
    v0  = a0;
    pc0 = ra;
}
void psxBios_memmove()
{
    char *p1 = (char *)Ra0, *p2 = (char *)Ra1;
    if (p2 <= p1 && p2 + a2 > p1) {
        a2++;
        p1 += a2;
        p2 += a2;
        while (a2-- > 0)
            *--p1 = *--p2;
    } else {
        while (a2-- > 0)
            *p1++ = *p2++;
    }
    v0  = a0;
    pc0 = ra;
}
void psxBios_memcmp()
{
    psxBios_bcmp();
}
void psxBios_memchr()
{
    char *p = (char *)Ra0;
    while (a2-- > 0) {
        if (*p++ != (s8)a1)
            continue;
        v0  = a0 + (p - (char *)Ra0 - 1);
        pc0 = ra;
        return;
    }
    v0  = 0;
    pc0 = ra;
}
void psxBios_rand()
{
    u32 s              = psxMu32(0x9010) * 1103515245 + 12345;
    v0                 = (s >> 16) & 0x7fff;
    psxMu32ref(0x9010) = SWAPu32(s);
    pc0                = ra;
}
void psxBios_srand()
{
    psxMu32ref(0x9010) = SWAPu32(a0);
    pc0                = ra;
}
static u32        qscmpfunc, qswidth;
static inline int qscmp(char *a, char *b)
{
    u32 sa0 = a0;
    a0      = sa0 + (a - (char *)PSXM(sa0));
    a1      = sa0 + (b - (char *)PSXM(sa0));
    softCall2(qscmpfunc);
    a0 = sa0;
    return (s32)v0;
}
static inline void qexchange(char *i, char *j)
{
    char t;
    int  n = qswidth;
    do {
        t    = *i;
        *i++ = *j;
        *j++ = t;
    } while (--n);
}
static inline void q3exchange(char *i, char *j, char *k)
{
    char t;
    int  n = qswidth;
    do {
        t    = *i;
        *i++ = *k;
        *k++ = *j;
        *j++ = t;
    } while (--n);
}
static void qsort_main(char *a, char *l)
{
    char        *i, *j, *lp, *hp;
    int          c;
    unsigned int n;
start:
    if ((n = l - a) <= qswidth)
        return;
    n  = qswidth * (n / (2 * qswidth));
    hp = lp = a + n;
    i       = a;
    j       = l - qswidth;
    while (TRUE) {
        if (i < lp) {
            if ((c = qscmp(i, lp)) == 0) {
                qexchange(i, lp -= qswidth);
                continue;
            }
            if (c < 0) {
                i += qswidth;
                continue;
            }
        }
    loop:
        if (j > hp) {
            if ((c = qscmp(hp, j)) == 0) {
                qexchange(hp += qswidth, j);
                goto loop;
            }
            if (c > 0) {
                if (i == lp) {
                    q3exchange(i, hp += qswidth, j);
                    i = lp += qswidth;
                    goto loop;
                }
                qexchange(i, j);
                j -= qswidth;
                i += qswidth;
                continue;
            }
            j -= qswidth;
            goto loop;
        }
        if (i == lp) {
            if (lp - a >= l - hp) {
                qsort_main(hp + qswidth, l);
                l = lp;
            } else {
                qsort_main(a, lp);
                a = hp + qswidth;
            }
            goto start;
        }
        q3exchange(j, lp -= qswidth, i);
        j = hp -= qswidth;
    }
}
void psxBios_qsort()
{
    qswidth   = a2;
    qscmpfunc = a3;
    qsort_main((char *)Ra0, (char *)Ra0 + a1 * a2);
    pc0 = ra;
}
void psxBios_malloc()
{
    unsigned int *chunk, *newchunk;
    unsigned int  dsize, csize, cstat;
    int           colflag;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x33]);
#endif
    chunk   = heap_addr;
    colflag = 0;
    while (chunk < heap_end) {
        csize = ((u32)*chunk) & 0xfffffffc;
        cstat = ((u32)*chunk) & 1;
        if (cstat == 1) {
            if (colflag == 0) {
                newchunk = chunk;
                dsize    = csize;
                colflag  = 1;
            } else
                dsize += (csize + 4);
        } else {
            if (colflag == 1) {
                colflag   = 0;
                *newchunk = SWAP32(dsize | 1);
            }
        }
        chunk = (u32 *)((uptr)chunk + csize + 4);
    }
    if (colflag == 1)
        *newchunk = SWAP32(dsize | 1);
    chunk = heap_addr;
    csize = ((u32)*chunk) & 0xfffffffc;
    cstat = ((u32)*chunk) & 1;
    dsize = (a0 + 3) & 0xfffffffc;
    if (chunk == NULL) {
        SysPrintf("malloc %x,%x: Uninitialized Heap!\n", v0, a0);
        v0  = 0;
        pc0 = ra;
        return;
    }
    while ((dsize > csize || cstat == 0) && chunk < heap_end) {
        chunk = (u32 *)((uptr)chunk + csize + 4);
        csize = ((u32)*chunk) & 0xfffffffc;
        cstat = ((u32)*chunk) & 1;
    }
    if (chunk >= heap_end) {
        SysPrintf("malloc %x,%x: Out of memory error!\n", v0, a0);
        v0  = 0;
        pc0 = ra;
        return;
    }
    if (dsize == csize) {
        *chunk &= 0xfffffffc;
    } else {
        *chunk    = SWAP32(dsize);
        newchunk  = (u32 *)((uptr)chunk + dsize + 4);
        *newchunk = SWAP32((csize - dsize - 4) & 0xfffffffc | 1);
    }
    v0 = ((unsigned long)chunk - (unsigned long)g_pcsx->psxMem->psxM) + 4;
    v0 |= 0x80000000;
    SysPrintf("malloc %x,%x\n", v0, a0);
    pc0 = ra;
}
void psxBios_free()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x34]);
#endif
    SysPrintf("free %x: %x bytes\n", a0, *(u32 *)(Ra0 - 4));
    *(u32 *)(Ra0 - 4) |= 1;
    pc0 = ra;
}
void psxBios_calloc()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x37]);
#endif
    a0 = a0 * a1;
    psxBios_malloc();
    memset(Rv0, 0, a0);
}
void psxBios_realloc()
{
    u32 block = a0;
    u32 size  = a1;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x38]);
#endif
    a0 = block;
    psxBios_free();
    a0 = size;
    psxBios_malloc();
}
void psxBios_InitHeap()
{
    unsigned int size;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x39]);
#endif
    if (((a0 & 0x1fffff) + a1) >= 0x200000)
        size = 0x1ffffc - (a0 & 0x1fffff);
    else
        size = a1;
    size &= 0xfffffffc;
    heap_addr  = (u32 *)Ra0;
    heap_end   = (u32 *)((u8 *)heap_addr + size);
    *heap_addr = SWAP32(size | 1);
    SysPrintf("InitHeap %x,%x : %x\n", a0, a1, size);
    pc0 = ra;
}
void psxBios_getchar()
{
    v0  = getchar();
    pc0 = ra;
}
void psxBios_printf()
{
    char  tmp[1024];
    char  tmp2[1024];
    u32   save[4];
    char *ptmp = tmp;
    int   n = 1, i = 0, j;
    memcpy(save, (char *)PSXM(sp), 4 * 4);
    psxMu32ref(sp)      = SWAP32((u32)a0);
    psxMu32ref(sp + 4)  = SWAP32((u32)a1);
    psxMu32ref(sp + 8)  = SWAP32((u32)a2);
    psxMu32ref(sp + 12) = SWAP32((u32)a3);
    while (Ra0[i]) {
        switch (Ra0[i]) {
            case '%':
                j         = 0;
                tmp2[j++] = '%';
            _start:
                switch (Ra0[++i]) {
                    case '.':
                    case 'l':
                        tmp2[j++] = Ra0[i];
                        goto _start;
                    default:
                        if (Ra0[i] >= '0' && Ra0[i] <= '9') {
                            tmp2[j++] = Ra0[i];
                            goto _start;
                        }
                        break;
                }
                tmp2[j++] = Ra0[i];
                tmp2[j]   = 0;
                switch (Ra0[i]) {
                    case 'f':
                    case 'F':
                        ptmp += sprintf(ptmp, tmp2, (float)psxMu32(sp + n * 4));
                        n++;
                        break;
                    case 'a':
                    case 'A':
                    case 'e':
                    case 'E':
                    case 'g':
                    case 'G':
                        ptmp += sprintf(ptmp, tmp2, (double)psxMu32(sp + n * 4));
                        n++;
                        break;
                    case 'p':
                    case 'i':
                    case 'd':
                    case 'D':
                    case 'o':
                    case 'O':
                    case 'x':
                    case 'X':
                        ptmp += sprintf(ptmp, tmp2, (unsigned int)psxMu32(sp + n * 4));
                        n++;
                        break;
                    case 'c':
                        ptmp += sprintf(ptmp, tmp2, (unsigned char)psxMu32(sp + n * 4));
                        n++;
                        break;
                    case 's':
                        ptmp += sprintf(ptmp, tmp2, (char *)PSXM(psxMu32(sp + n * 4)));
                        n++;
                        break;
                    case '%':
                        *ptmp++ = Ra0[i];
                        break;
                }
                i++;
                break;
            default:
                *ptmp++ = Ra0[i++];
        }
    }
    *ptmp = 0;
    memcpy((char *)PSXM(sp), save, 4 * 4);
    SysPrintf("%s", tmp);
    pc0 = ra;
}
void psxBios_Load()
{
    EXE_HEADER eheader;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s, %x\n", biosA0n[0x42], Ra0, a1);
#endif
    if (LoadCdromFile(Ra0, &eheader) == 0) {
        memcpy(Ra1, ((char *)&eheader) + 16, sizeof(EXEC));
        v0 = 1;
    } else
        v0 = 0;
    pc0 = ra;
}
void psxBios_Exec()
{
    EXEC *header = (EXEC *)Ra0;
    u32   tmp;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x, %x, %x\n", biosA0n[0x43], a0, a1, a2);
#endif
    header->_sp  = sp;
    header->_fp  = fp;
    header->_sp  = sp;
    header->_gp  = gp;
    header->ret  = ra;
    header->base = s0;
    if (header->S_addr != 0) {
        tmp = header->S_addr + header->s_size;
        sp  = tmp;
        fp  = sp;
    }
    gp  = header->gp0;
    s0  = a0;
    a0  = a1;
    a1  = a2;
    ra  = 0x8000;
    pc0 = header->_pc0;
}
void psxBios_FlushCache()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x44]);
#endif
    pc0 = ra;
}
void psxBios_GPU_dw()
{
    int  size;
    s32 *ptr;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x46]);
#endif
    GPUwriteData(0xa0000000);
    GPUwriteData((a1 << 16) | (a0 & 0xffff));
    GPUwriteData((a3 << 16) | (a2 & 0xffff));
    size = (a2 * a3 + 1) / 2;
    ptr  = (s32 *)PSXM(Rsp[4]);
    do {
        GPUwriteData(SWAP32(*ptr));
        ptr++;
    } while (--size);
    pc0 = ra;
}
void psxBios_mem2vram()
{
    int size;
    GPUwriteData(0xa0000000);
    GPUwriteData((a1 << 16) | (a0 & 0xffff));
    GPUwriteData((a3 << 16) | (a2 & 0xffff));
    size = (a2 * a3 + 1) / 2;
    GPUwriteStatus(0x04000002);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010f4, 0);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010f0, g_pcsx->psxHw->psxHwRead32(0x1f8010f0) | 0x800);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a0, Rsp[4]);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a4, ((size / 16) << 16) | 16);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a8, 0x01000201);
    pc0 = ra;
}
void psxBios_SendGPU()
{
    GPUwriteStatus(a0);
    pc0 = ra;
}
void psxBios_GPU_cw()
{
    GPUwriteData(a0);
    pc0 = ra;
}
void psxBios_GPU_cwb()
{
    s32 *ptr  = (s32 *)Ra0;
    int  size = a1;
    while (size--) {
        GPUwriteData(SWAP32(*ptr));
        ptr++;
    }
    pc0 = ra;
}
void psxBios_GPU_SendPackets()
{
    GPUwriteStatus(0x04000002);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010f4, 0);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010f0, g_pcsx->psxHw->psxHwRead32(0x1f8010f0) | 0x800);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a0, a0);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a4, 0);
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a8, 0x010000401);
    pc0 = ra;
}
void psxBios_sys_a0_4c()
{
    g_pcsx->psxHw->psxHwWrite32(0x1f8010a8, 0x00000401);
    GPUwriteData(0x0400000);
    GPUwriteData(0x0200000);
    GPUwriteData(0x0100000);
    pc0 = ra;
}
void psxBios_GPU_GetGPUStatus()
{
    v0  = GPUreadStatus();
    pc0 = ra;
}
#undef s_addr
void psxBios_LoadExec()
{
    EXEC *header = (EXEC *)PSXM(0xf000);
    u32   s_addr, s_size;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s: %x,%x\n", biosA0n[0x51], Ra0, a1, a2);
#endif
    s_addr = a1;
    s_size = a2;
    a1     = 0xf000;
    psxBios_Load();
    header->S_addr = s_addr;
    header->s_size = s_size;
    a0             = 0xf000;
    a1             = 0;
    a2             = 0;
    psxBios_Exec();
}
void psxBios__bu_init()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x70]);
#endif
    DeliverEvent(0x11, 0x2);
    DeliverEvent(0x81, 0x2);
    pc0 = ra;
}
void psxBios__96_init()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x71]);
#endif
    pc0 = ra;
}
void psxBios__96_remove()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosA0n[0x72]);
#endif
    pc0 = ra;
}
void psxBios_SetMem()
{
    u32 _new = psxHu32(0x1060);
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x, %x\n", biosA0n[0x9f], a0, a1);
#endif
    switch (a0) {
        case 2:
            psxHu32ref(0x1060) = SWAP32(_new);
            psxMu32ref(0x060)  = a0;
            SysPrintf("Change effective memory : %d MBytes\n", a0);
            break;
        case 8:
            psxHu32ref(0x1060) = SWAP32(_new | 0x300);
            psxMu32ref(0x060)  = a0;
            SysPrintf("Change effective memory : %d MBytes\n", a0);
        default:
            SysPrintf("Effective memory must be 2/8 MBytes\n");
            break;
    }
    pc0 = ra;
}
void psxBios__card_info()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosA0n[0xab], a0);
#endif
    DeliverEvent(0x81, 0x2);
    v0  = 1;
    pc0 = ra;
}
void psxBios__card_load()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosA0n[0xac], a0);
#endif
    DeliverEvent(0x81, 0x2);
    v0  = 1;
    pc0 = ra;
}
void psxBios_SetRCnt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x02]);
#endif
    a0 &= 0x3;
    if (a0 != 3) {
        u32 mode = 0;
        g_pcsx->psxCntr->psxRcntWtarget(a0, a1);
        if (a2 & 0x1000)
            mode |= 0x050;
        if (a2 & 0x0100)
            mode |= 0x008;
        if (a2 & 0x0010)
            mode |= 0x001;
        if (a0 == 2) {
            if (a2 & 0x0001)
                mode |= 0x200;
        } else {
            if (a2 & 0x0001)
                mode |= 0x100;
        }
        g_pcsx->psxCntr->psxRcntWmode(a0, mode);
    }
    pc0 = ra;
}
void psxBios_GetRCnt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x03]);
#endif
    a0 &= 0x3;
    if (a0 != 3)
        v0 = g_pcsx->psxCntr->psxRcntRcount(a0);
    else
        v0 = 0;
    pc0 = ra;
}
void psxBios_StartRCnt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x04]);
#endif
    a0 &= 0x3;
    if (a0 != 3)
        psxHu32ref(0x1074) |= SWAP32((u32)((1 << (a0 + 4))));
    else
        psxHu32ref(0x1074) |= SWAPu32(0x1);
    v0  = 1;
    pc0 = ra;
}
void psxBios_StopRCnt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x05]);
#endif
    a0 &= 0x3;
    if (a0 != 3)
        psxHu32ref(0x1074) &= SWAP32((u32)(~(1 << (a0 + 4))));
    else
        psxHu32ref(0x1074) &= SWAPu32(~0x1);
    pc0 = ra;
}
void psxBios_ResetRCnt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x06]);
#endif
    a0 &= 0x3;
    if (a0 != 3) {
        g_pcsx->psxCntr->psxRcntWmode(a0, 0);
        g_pcsx->psxCntr->psxRcntWtarget(a0, 0);
        g_pcsx->psxCntr->psxRcntWcount(a0, 0);
    }
    pc0 = ra;
}
#define GetEv()                                                                                                        \
    ev = (a0 >> 24) & 0xf;                                                                                             \
    if (ev == 0xf)                                                                                                     \
        ev = 0x5;                                                                                                      \
    ev *= 32;                                                                                                          \
    ev += a0 & 0x1f;
#define GetSpec()                                                                                                      \
    spec = 0;                                                                                                          \
    switch (a1) {                                                                                                      \
        case 0x0301:                                                                                                   \
            spec = 16;                                                                                                 \
            break;                                                                                                     \
        case 0x0302:                                                                                                   \
            spec = 17;                                                                                                 \
            break;                                                                                                     \
        default:                                                                                                       \
            for (i = 0; i < 16; i++)                                                                                   \
                if (a1 & (1 << i)) {                                                                                   \
                    spec = i;                                                                                          \
                    break;                                                                                             \
                }                                                                                                      \
            break;                                                                                                     \
    }
void psxBios_DeliverEvent()
{
    int ev, spec;
    int i;
    GetEv();
    GetSpec();
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x\n", biosB0n[0x07], ev, spec);
#endif
    DeliverEvent(ev, spec);
    pc0 = ra;
}
void psxBios_OpenEvent()
{
    int ev, spec;
    int i;
    GetEv();
    GetSpec();
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x (class:%x, spec:%x, mode:%x, func:%x)\n", biosB0n[0x08], ev, spec, a0, a1, a2, a3);
#endif
    Event[ev][spec].status   = EvStWAIT;
    Event[ev][spec].mode     = a2;
    Event[ev][spec].fhandler = a3;
    v0                       = ev | (spec << 8);
    pc0                      = ra;
}
void psxBios_CloseEvent()
{
    int ev, spec;
    ev   = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x\n", biosB0n[0x09], ev, spec);
#endif
    Event[ev][spec].status = EvStUNUSED;
    v0                     = 1;
    pc0                    = ra;
}
void psxBios_WaitEvent()
{
    int ev, spec;
    ev   = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x\n", biosB0n[0x0a], ev, spec);
#endif
    Event[ev][spec].status = EvStACTIVE;
    v0                     = 1;
    pc0                    = ra;
}
void psxBios_TestEvent()
{
    int ev, spec;
    ev   = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;
    if (Event[ev][spec].status == EvStALREADY) {
        Event[ev][spec].status = EvStACTIVE;
        v0                     = 1;
    } else
        v0 = 0;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x: %x\n", biosB0n[0x0b], ev, spec, v0);
#endif
    pc0 = ra;
}
void psxBios_EnableEvent()
{
    int ev, spec;
    ev   = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x\n", biosB0n[0x0c], ev, spec);
#endif
    Event[ev][spec].status = EvStACTIVE;
    v0                     = 1;
    pc0                    = ra;
}
void psxBios_DisableEvent()
{
    int ev, spec;
    ev   = a0 & 0xff;
    spec = (a0 >> 8) & 0xff;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x\n", biosB0n[0x0d], ev, spec);
#endif
    Event[ev][spec].status = EvStWAIT;
    v0                     = 1;
    pc0                    = ra;
}
void psxBios_OpenTh()
{
    int th;
    for (th = 1; th < 8; th++)
        if (Thread[th].status == 0)
            break;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x0e], th);
#endif
    Thread[th].status  = 1;
    Thread[th].func    = a0;
    Thread[th].reg[29] = a1;
    Thread[th].reg[28] = a2;
    v0                 = th;
    pc0                = ra;
}
void psxBios_CloseTh()
{
    int th = a0 & 0xff;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x0f], th);
#endif
    if (Thread[th].status == 0) {
        v0 = 0;
    } else {
        Thread[th].status = 0;
        v0                = 1;
    }
    pc0 = ra;
}
void psxBios_ChangeTh()
{
    int th = a0 & 0xff;
#ifdef PSXBIOS_LOG
#endif
    if (Thread[th].status == 0 || CurThread == th) {
        v0  = 0;
        pc0 = ra;
    } else {
        v0 = 1;
        if (Thread[CurThread].status == 2) {
            Thread[CurThread].status = 1;
            Thread[CurThread].func   = ra;
            memcpy(Thread[CurThread].reg, g_pcsx->psxCpu->psxRegs.GPR.r, 32 * 4);
        }
        memcpy(g_pcsx->psxCpu->psxRegs.GPR.r, Thread[th].reg, 32 * 4);
        pc0               = Thread[th].func;
        Thread[th].status = 2;
        CurThread         = th;
    }
}
void psxBios_InitPAD()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x12]);
#endif
    pad_buf1    = (char *)Ra0;
    pad_buf1len = a1;
    pad_buf2    = (char *)Ra2;
    pad_buf2len = a3;
    v0          = 1;
    pc0         = ra;
}
void psxBios_StartPAD()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x13]);
#endif
    g_pcsx->psxHw->psxHwWrite16(0x1f801074, (unsigned short)(g_pcsx->psxHw->psxHwRead16(0x1f801074) | 0x1));
    g_pcsx->psxCpu->psxRegs.CP0.n.Status |= 0x401;
    pc0 = ra;
}
void psxBios_StopPAD()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x14]);
#endif
    pad_buf1 = NULL;
    pad_buf2 = NULL;
    pc0      = ra;
}
void psxBios_PAD_init()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x15]);
#endif
    g_pcsx->psxHw->psxHwWrite16(0x1f801074, (u16)(g_pcsx->psxHw->psxHwRead16(0x1f801074) | 0x1));
    pad_buf  = (int *)Ra1;
    *pad_buf = -1;
    g_pcsx->psxCpu->psxRegs.CP0.n.Status |= 0x401;
    pc0 = ra;
}
void psxBios_PAD_dr()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x16]);
#endif
    v0  = -1;
    pc0 = ra;
}
void psxBios_ReturnFromException()
{
    LoadRegs();
    pc0 = g_pcsx->psxCpu->psxRegs.CP0.n.EPC;
    if (g_pcsx->psxCpu->psxRegs.CP0.n.Cause & 0x80000000)
        pc0 += 4;
    g_pcsx->psxCpu->psxRegs.CP0.n.Status =
        (g_pcsx->psxCpu->psxRegs.CP0.n.Status & 0xfffffff0) | ((g_pcsx->psxCpu->psxRegs.CP0.n.Status & 0x3c) >> 2);
}
void psxBios_ResetEntryInt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x18]);
#endif
    jmp_int = NULL;
    pc0     = ra;
}
void psxBios_HookEntryInt()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x19]);
#endif
    jmp_int = (u32 *)Ra0;
    pc0     = ra;
}
void psxBios_UnDeliverEvent()
{
    int ev, spec;
    int i;
    GetEv();
    GetSpec();
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s %x,%x\n", biosB0n[0x20], ev, spec);
#endif
    if (Event[ev][spec].status == EvStALREADY && Event[ev][spec].mode == EvMdNOINTR)
        Event[ev][spec].status = EvStACTIVE;
    pc0 = ra;
}
#define buopen(mcd)                                                                                                    \
    {                                                                                                                  \
        strcpy(FDesc[1 + mcd].name, Ra0 + 5);                                                                          \
        FDesc[1 + mcd].offset = 0;                                                                                     \
        FDesc[1 + mcd].mode   = a1;                                                                                    \
        for (i = 1; i < 16; i++) {                                                                                     \
            ptr = g_pcsx->psxSio->Mcd##mcd##Data + 128 * i;                                                            \
            if ((*ptr & 0xF0) != 0x50)                                                                                 \
                continue;                                                                                              \
            if (strcmp(FDesc[1 + mcd].name, ptr + 0xa))                                                                \
                continue;                                                                                              \
            FDesc[1 + mcd].mcfile = i;                                                                                 \
            SysPrintf("open %s\n", ptr + 0xa);                                                                         \
            v0 = 1 + mcd;                                                                                              \
            break;                                                                                                     \
        }                                                                                                              \
        if (a1 & 0x200 && v0 == -1) {                                                                                  \
            for (i = 1; i < 16; i++) {                                                                                 \
                int j, _xor = 0;                                                                                       \
                ptr = g_pcsx->psxSio->Mcd##mcd##Data + 128 * i;                                                        \
                if ((*ptr & 0xF0) == 0x50)                                                                             \
                    continue;                                                                                          \
                ptr[0] = 0x50 | (u8)(a1 >> 16);                                                                        \
                ptr[4] = 0x00;                                                                                         \
                ptr[5] = 0x20;                                                                                         \
                ptr[6] = 0x00;                                                                                         \
                ptr[7] = 0x00;                                                                                         \
                ptr[8] = 'B';                                                                                          \
                ptr[9] = 'I';                                                                                          \
                strcpy(ptr + 0xa, FDesc[1 + mcd].name);                                                                \
                for (j = 0; j < 127; j++)                                                                              \
                    _xor ^= ptr[j];                                                                                    \
                ptr[127]              = _xor;                                                                          \
                FDesc[1 + mcd].mcfile = i;                                                                             \
                SysPrintf("openC %s\n", ptr);                                                                          \
                v0 = 1 + mcd;                                                                                          \
                g_pcsx->psxSio->SaveMcd(Config.Mcd##mcd, g_pcsx->psxSio->Mcd##mcd##Data, 128 * i, 128);                \
                break;                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    }
void psxBios_open()
{
    int   i;
    char *ptr;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s,%x\n", biosB0n[0x32], Ra0, a1);
#endif
    v0 = -1;
    if (!strncmp(Ra0, "bu00", 4)) {
        buopen(1);
    }
    if (!strncmp(Ra0, "bu10", 4)) {
        buopen(2);
    }
    pc0 = ra;
}
void psxBios_lseek()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x, %x, %x\n", biosB0n[0x33], a0, a1, a2);
#endif
    switch (a2) {
        case 0:
            FDesc[a0].offset = a1;
            v0               = a1;
            break;
        case 1:
            FDesc[a0].offset += a1;
            v0 = FDesc[a0].offset;
            break;
    }
    pc0 = ra;
}
#define buread(mcd)                                                                                                    \
    {                                                                                                                  \
        SysPrintf("read %d: %x,%x (%s)\n", FDesc[1 + mcd].mcfile, FDesc[1 + mcd].offset, a2,                           \
                  g_pcsx->psxSio->Mcd##mcd##Data + 128 * FDesc[1 + mcd].mcfile + 0xa);                                 \
        ptr = g_pcsx->psxSio->Mcd##mcd##Data + 8192 * FDesc[1 + mcd].mcfile + FDesc[1 + mcd].offset;                   \
        memcpy(Ra1, ptr, a2);                                                                                          \
        if (FDesc[1 + mcd].mode & 0x8000)                                                                              \
            v0 = 0;                                                                                                    \
        else                                                                                                           \
            v0 = a2;                                                                                                   \
        FDesc[1 + mcd].offset += v0;                                                                                   \
        DeliverEvent(0x11, 0x2);                                                                                       \
        DeliverEvent(0x81, 0x2);                                                                                       \
    }
void psxBios_read()
{
    char *ptr;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x, %x, %x\n", biosB0n[0x34], a0, a1, a2);
#endif
    v0 = -1;
    switch (a0) {
        case 2:
            buread(1);
            break;
        case 3:
            buread(2);
            break;
    }
    pc0 = ra;
}
#define buwrite(mcd)                                                                                                   \
    {                                                                                                                  \
        u32 offset = +8192 * FDesc[1 + mcd].mcfile + FDesc[1 + mcd].offset;                                            \
        SysPrintf("write %d: %x,%x\n", FDesc[1 + mcd].mcfile, FDesc[1 + mcd].offset, a2);                              \
        ptr = g_pcsx->psxSio->Mcd##mcd##Data + offset;                                                                 \
        memcpy(ptr, Ra1, a2);                                                                                          \
        FDesc[1 + mcd].offset += a2;                                                                                   \
        g_pcsx->psxSio->SaveMcd(Config.Mcd##mcd, g_pcsx->psxSio->Mcd##mcd##Data, offset, a2);                          \
        if (FDesc[1 + mcd].mode & 0x8000)                                                                              \
            v0 = 0;                                                                                                    \
        else                                                                                                           \
            v0 = a2;                                                                                                   \
        DeliverEvent(0x11, 0x2);                                                                                       \
        DeliverEvent(0x81, 0x2);                                                                                       \
    }
void psxBios_write()
{
    char *ptr;
    if (a0 == 1) {
        char *ptr = Ra1;
        while (a2 > 0) {
            SysPrintf("%c", *ptr++);
            a2--;
        }
        pc0 = ra;
        return;
    }
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x,%x,%x\n", biosB0n[0x35], a0, a1, a2);
#endif
    v0 = -1;
    switch (a0) {
        case 2:
            buwrite(1);
            break;
        case 3:
            buwrite(2);
            break;
    }
    pc0 = ra;
}
void psxBios_close()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x36], a0);
#endif
    v0  = a0;
    pc0 = ra;
}
void psxBios_putchar()
{
    SysPrintf("%c", (char)a0);
    pc0 = ra;
}
void psxBios_puts()
{
    SysPrintf("%s", Ra0);
    pc0 = ra;
}
char ffile[64], *pfile;
int  nfile;
#define bufile(mcd)                                                                                                    \
    {                                                                                                                  \
        while (nfile < 16) {                                                                                           \
            int match = 1;                                                                                             \
            ptr       = g_pcsx->psxSio->Mcd##mcd##Data + 128 * nfile;                                                  \
            nfile++;                                                                                                   \
            if ((*ptr & 0xF0) != 0x50)                                                                                 \
                continue;                                                                                              \
            ptr += 0xa;                                                                                                \
            if (pfile[0] == 0) {                                                                                       \
                strcpy(dir->name, ptr);                                                                                \
            } else                                                                                                     \
                for (i = 0; i < 20; i++) {                                                                             \
                    if (pfile[i] == ptr[i]) {                                                                          \
                        dir->name[i] = ptr[i];                                                                         \
                        if (ptr[i] == 0)                                                                               \
                            break;                                                                                     \
                        else                                                                                           \
                            continue;                                                                                  \
                    }                                                                                                  \
                    if (pfile[i] == '?') {                                                                             \
                        dir->name[i] = ptr[i];                                                                         \
                        continue;                                                                                      \
                    }                                                                                                  \
                    if (pfile[i] == '*') {                                                                             \
                        strcpy(dir->name + i, ptr + i);                                                                \
                        break;                                                                                         \
                    }                                                                                                  \
                    match = 0;                                                                                         \
                    break;                                                                                             \
                }                                                                                                      \
            SysPrintf("%d : %s = %s + %s (match=%d)\n", nfile, dir->name, pfile, ptr, match);                          \
            if (match == 0)                                                                                            \
                continue;                                                                                              \
            dir->size = 8192;                                                                                          \
            v0        = _dir;                                                                                          \
            break;                                                                                                     \
        }                                                                                                              \
    }
void psxBios_firstfile()
{
    struct DIRENTRY *dir  = (struct DIRENTRY *)Ra1;
    u32              _dir = a1;
    char            *ptr;
    int              i;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s\n", biosB0n[0x42], Ra0);
#endif
    v0 = 0;
    strcpy(ffile, Ra0);
    pfile = ffile + 5;
    nfile = 1;
    if (!strncmp(Ra0, "bu00", 4)) {
        bufile(1);
    }
    if (!strncmp(Ra0, "bu10", 4)) {
        bufile(2);
    }
    pc0 = ra;
}
void psxBios_nextfile()
{
    struct DIRENTRY *dir  = (struct DIRENTRY *)Ra0;
    u32              _dir = a0;
    char            *ptr;
    int              i, matched = 0;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s\n", biosB0n[0x43], dir->name);
#endif
    v0 = 0;
    if (!strncmp(ffile, "bu00", 4)) {
        bufile(1);
    }
    if (!strncmp(ffile, "bu10", 4)) {
        bufile(2);
    }
    pc0 = ra;
}
#define burename(mcd)                                                                                                  \
    {                                                                                                                  \
        for (i = 1; i < 16; i++) {                                                                                     \
            int namelen, j, _xor = 0;                                                                                  \
            ptr = g_pcsx->psxSio->Mcd##mcd##Data + 128 * i;                                                            \
            if ((*ptr & 0xF0) != 0x50)                                                                                 \
                continue;                                                                                              \
            if (strcmp(Ra0 + 5, ptr + 0xa))                                                                            \
                continue;                                                                                              \
            namelen = strlen(Ra1 + 5);                                                                                 \
            memcpy(ptr + 0xa, Ra1 + 5, namelen);                                                                       \
            memset(ptr + 0xa + namelen, 0, 0x75 - namelen);                                                            \
            for (j = 0; j < 127; j++)                                                                                  \
                _xor ^= ptr[j];                                                                                        \
            ptr[127] = _xor;                                                                                           \
            g_pcsx->psxSio->SaveMcd(Config.Mcd##mcd, g_pcsx->psxSio->Mcd##mcd##Data, 128 * i + 0xa, 0x76);             \
            v0 = 1;                                                                                                    \
            break;                                                                                                     \
        }                                                                                                              \
    }
void psxBios_rename()
{
    char *ptr;
    int   i;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s,%s\n", biosB0n[0x44], Ra0, Ra1);
#endif
    v0 = 0;
    if (!strncmp(Ra0, "bu00", 4) && !strncmp(Ra1, "bu00", 4)) {
        burename(1);
    }
    if (!strncmp(Ra0, "bu10", 4) && !strncmp(Ra1, "bu10", 4)) {
        burename(2);
    }
    pc0 = ra;
}
#define budelete(mcd)                                                                                                  \
    {                                                                                                                  \
        for (i = 1; i < 16; i++) {                                                                                     \
            ptr = g_pcsx->psxSio->Mcd##mcd##Data + 128 * i;                                                            \
            if ((*ptr & 0xF0) != 0x50)                                                                                 \
                continue;                                                                                              \
            if (strcmp(Ra0 + 5, ptr + 0xa))                                                                            \
                continue;                                                                                              \
            *ptr = (*ptr & 0xf) | 0xA0;                                                                                \
            g_pcsx->psxSio->SaveMcd(Config.Mcd##mcd, g_pcsx->psxSio->Mcd##mcd##Data, 128 * i, 1);                      \
            SysPrintf("delete %s\n", ptr + 0xa);                                                                       \
            v0 = 1;                                                                                                    \
            break;                                                                                                     \
        }                                                                                                              \
    }
void psxBios_delete()
{
    char *ptr;
    int   i;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %s\n", biosB0n[0x45], Ra0);
#endif
    v0 = 0;
    if (!strncmp(Ra0, "bu00", 4)) {
        budelete(1);
    }
    if (!strncmp(Ra0, "bu10", 4)) {
        budelete(2);
    }
    pc0 = ra;
}
void psxBios_InitCARD()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x4a], a0);
#endif
    CardState = 0;
    pc0       = ra;
}
void psxBios_StartCARD()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x4b]);
#endif
    if (CardState == 0)
        CardState = 1;
    pc0 = ra;
}
void psxBios_StopCARD()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x4c]);
#endif
    if (CardState == 1)
        CardState = 0;
    pc0 = ra;
}
void psxBios__card_write()
{
    int port;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x,%x,%x\n", biosB0n[0x4e], a0, a1, a2);
#endif
    port = a0 >> 4;
    if (port == 0) {
        memcpy(g_pcsx->psxSio->Mcd1Data + a1 * 128, Ra2, 128);
        g_pcsx->psxSio->SaveMcd(Config.Mcd1, g_pcsx->psxSio->Mcd1Data, a1 * 128, 128);
    } else {
        memcpy(g_pcsx->psxSio->Mcd2Data + a1 * 128, Ra2, 128);
        g_pcsx->psxSio->SaveMcd(Config.Mcd2, g_pcsx->psxSio->Mcd2Data, a1 * 128, 128);
    }
    DeliverEvent(0x11, 0x2);
    v0  = 1;
    pc0 = ra;
}
void psxBios__card_read()
{
    int port;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x4f]);
#endif
    port = a0 >> 4;
    if (port == 0) {
        memcpy(Ra2, g_pcsx->psxSio->Mcd1Data + a1 * 128, 128);
    } else {
        memcpy(Ra2, g_pcsx->psxSio->Mcd2Data + a1 * 128, 128);
    }
    DeliverEvent(0x11, 0x2);
    v0  = 1;
    pc0 = ra;
}
void psxBios__new_card()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x50]);
#endif
    pc0 = ra;
}
void psxBios_Krom2RawAdd()
{
    int       i               = 0;
    const u32 table_8140[][2] = {
        {0x8140, 0x0000}, {0x8180, 0x0762}, {0x81ad, 0x0cc6}, {0x81b8, 0x0ca8}, {0x81c0, 0x0f00}, {0x81c8, 0x0d98},
        {0x81cf, 0x10c2}, {0x81da, 0x0e6a}, {0x81e9, 0x13ce}, {0x81f0, 0x102c}, {0x81f8, 0x1590}, {0x81fc, 0x111c},
        {0x81fd, 0x1626}, {0x824f, 0x113a}, {0x8259, 0x20ee}, {0x8260, 0x1266}, {0x827a, 0x24cc}, {0x8281, 0x1572},
        {0x829b, 0x28aa}, {0x829f, 0x187e}, {0x82f2, 0x32dc}, {0x8340, 0x2238}, {0x837f, 0x4362}, {0x8380, 0x299a},
        {0x8397, 0x4632}, {0x839f, 0x2c4c}, {0x83b7, 0x49f2}, {0x83bf, 0x2f1c}, {0x83d7, 0x4db2}, {0x8440, 0x31ec},
        {0x8461, 0x5dde}, {0x8470, 0x35ca}, {0x847f, 0x6162}, {0x8480, 0x378c}, {0x8492, 0x639c}, {0x849f, 0x39a8},
        {0xffff, 0}};
    const u32 table_889f[][2] = {
        {0x889f, 0x3d68},  {0x8900, 0x40ec},  {0x897f, 0x4fb0},  {0x8a00, 0x56f4},  {0x8a7f, 0x65b8},
        {0x8b00, 0x6cfc},  {0x8b7f, 0x7bc0},  {0x8c00, 0x8304},  {0x8c7f, 0x91c8},  {0x8d00, 0x990c},
        {0x8d7f, 0xa7d0},  {0x8e00, 0xaf14},  {0x8e7f, 0xbdd8},  {0x8f00, 0xc51c},  {0x8f7f, 0xd3e0},
        {0x9000, 0xdb24},  {0x907f, 0xe9e8},  {0x9100, 0xf12c},  {0x917f, 0xfff0},  {0x9200, 0x10734},
        {0x927f, 0x115f8}, {0x9300, 0x11d3c}, {0x937f, 0x12c00}, {0x9400, 0x13344}, {0x947f, 0x14208},
        {0x9500, 0x1494c}, {0x957f, 0x15810}, {0x9600, 0x15f54}, {0x967f, 0x16e18}, {0x9700, 0x1755c},
        {0x977f, 0x18420}, {0x9800, 0x18b64}, {0xffff, 0}};
    if (a0 >= 0x8140 && a0 <= 0x84be) {
        while (table_8140[i][0] <= a0)
            i++;
        a0 -= table_8140[i - 1][0];
        v0 = 0xbfc66000 + (a0 * 0x1e + table_8140[i - 1][1]);
    } else if (a0 >= 0x889f && a0 <= 0x9872) {
        while (table_889f[i][0] <= a0)
            i++;
        a0 -= table_889f[i - 1][0];
        v0 = 0xbfc66000 + (a0 * 0x1e + table_889f[i - 1][1]);
    } else {
        v0 = 0xffffffff;
    }
    pc0 = ra;
}
void psxBios_GetC0Table()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x56]);
#endif
    v0  = 0x674;
    pc0 = ra;
}
void psxBios_GetB0Table()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s\n", biosB0n[0x57]);
#endif
    v0  = 0x874;
    pc0 = ra;
}
void psxBios_ChangeClearPad()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosB0n[0x5b], a0);
#endif
    pc0 = ra;
}
void psxBios_SysEnqIntRP()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosC0n[0x02], a0);
#endif
    SysIntRP[a0] = a1;
    v0           = 0;
    pc0          = ra;
}
void psxBios_SysDeqIntRP()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x\n", biosC0n[0x03], a0);
#endif
    SysIntRP[a0] = 0;
    v0           = 0;
    pc0          = ra;
}
void psxBios_ChangeClearRCnt()
{
    u32 *ptr;
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("psxBios_%s: %x, %x\n", biosC0n[0x0a], a0, a1);
#endif
    ptr  = (u32 *)PSXM((a0 << 2) + 0x8600);
    v0   = *ptr;
    *ptr = a1;
    pc0  = ra;
}
void psxBios_dummy()
{
#ifdef PSXBIOS_LOG
    PSXBIOS_LOG("unk %x call: %x\n", pc0 & 0x1fffff, t1);
#endif
    pc0 = ra;
}
void (*biosA0[256])();
void (*biosB0[256])();
void (*biosC0[256])();
#include "../utils/sjisfont.h"
void psxBiosInit()
{
    u32    base, size;
    u32   *ptr;
    int    i;
    uLongf len;
    for (i = 0; i < 256; i++) {
        biosA0[i] = NULL;
        biosB0[i] = NULL;
        biosC0[i] = NULL;
    }
    biosA0[0x3e] = psxBios_puts;
    biosA0[0x3f] = psxBios_printf;
    biosB0[0x3d] = psxBios_putchar;
    biosB0[0x3f] = psxBios_puts;
    if (!Config.HLE)
        return;
    for (i = 0; i < 256; i++) {
        if (biosA0[i] == NULL)
            biosA0[i] = psxBios_dummy;
        if (biosB0[i] == NULL)
            biosB0[i] = psxBios_dummy;
        if (biosC0[i] == NULL)
            biosC0[i] = psxBios_dummy;
    }
    biosA0[0x00] = psxBios_open;
    biosA0[0x01] = psxBios_lseek;
    biosA0[0x02] = psxBios_read;
    biosA0[0x03] = psxBios_write;
    biosA0[0x04] = psxBios_close;
    biosA0[0x0e] = psxBios_abs;
    biosA0[0x0f] = psxBios_labs;
    biosA0[0x10] = psxBios_atoi;
    biosA0[0x11] = psxBios_atol;
    biosA0[0x13] = psxBios_setjmp;
    biosA0[0x14] = psxBios_longjmp;
    biosA0[0x15] = psxBios_strcat;
    biosA0[0x16] = psxBios_strncat;
    biosA0[0x17] = psxBios_strcmp;
    biosA0[0x18] = psxBios_strncmp;
    biosA0[0x19] = psxBios_strcpy;
    biosA0[0x1a] = psxBios_strncpy;
    biosA0[0x1b] = psxBios_strlen;
    biosA0[0x1c] = psxBios_index;
    biosA0[0x1d] = psxBios_rindex;
    biosA0[0x1e] = psxBios_strchr;
    biosA0[0x1f] = psxBios_strrchr;
    biosA0[0x20] = psxBios_strpbrk;
    biosA0[0x21] = psxBios_strspn;
    biosA0[0x22] = psxBios_strcspn;
    biosA0[0x23] = psxBios_strtok;
    biosA0[0x24] = psxBios_strstr;
    biosA0[0x25] = psxBios_toupper;
    biosA0[0x26] = psxBios_tolower;
    biosA0[0x27] = psxBios_bcopy;
    biosA0[0x28] = psxBios_bzero;
    biosA0[0x29] = psxBios_bcmp;
    biosA0[0x2a] = psxBios_memcpy;
    biosA0[0x2b] = psxBios_memset;
    biosA0[0x2c] = psxBios_memmove;
    biosA0[0x2d] = psxBios_memcmp;
    biosA0[0x2e] = psxBios_memchr;
    biosA0[0x2f] = psxBios_rand;
    biosA0[0x30] = psxBios_srand;
    biosA0[0x31] = psxBios_qsort;
    biosA0[0x33] = psxBios_malloc;
    biosA0[0x34] = psxBios_free;
    biosA0[0x37] = psxBios_calloc;
    biosA0[0x38] = psxBios_realloc;
    biosA0[0x39] = psxBios_InitHeap;
    biosA0[0x3b] = psxBios_getchar;
    biosA0[0x3c] = psxBios_putchar;
    biosA0[0x42] = psxBios_Load;
    biosA0[0x43] = psxBios_Exec;
    biosA0[0x44] = psxBios_FlushCache;
    biosA0[0x46] = psxBios_GPU_dw;
    biosA0[0x47] = psxBios_mem2vram;
    biosA0[0x48] = psxBios_SendGPU;
    biosA0[0x49] = psxBios_GPU_cw;
    biosA0[0x4a] = psxBios_GPU_cwb;
    biosA0[0x4b] = psxBios_GPU_SendPackets;
    biosA0[0x4c] = psxBios_sys_a0_4c;
    biosA0[0x4d] = psxBios_GPU_GetGPUStatus;
    biosA0[0x51] = psxBios_LoadExec;
    biosA0[0x70] = psxBios__bu_init;
    biosA0[0x71] = psxBios__96_init;
    biosA0[0x72] = psxBios__96_remove;
    biosA0[0x9f] = psxBios_SetMem;
    biosA0[0xab] = psxBios__card_info;
    biosA0[0xac] = psxBios__card_load;
    biosB0[0x02] = psxBios_SetRCnt;
    biosB0[0x03] = psxBios_GetRCnt;
    biosB0[0x04] = psxBios_StartRCnt;
    biosB0[0x05] = psxBios_StopRCnt;
    biosB0[0x06] = psxBios_ResetRCnt;
    biosB0[0x07] = psxBios_DeliverEvent;
    biosB0[0x08] = psxBios_OpenEvent;
    biosB0[0x09] = psxBios_CloseEvent;
    biosB0[0x0a] = psxBios_WaitEvent;
    biosB0[0x0b] = psxBios_TestEvent;
    biosB0[0x0c] = psxBios_EnableEvent;
    biosB0[0x0d] = psxBios_DisableEvent;
    biosB0[0x0e] = psxBios_OpenTh;
    biosB0[0x0f] = psxBios_CloseTh;
    biosB0[0x10] = psxBios_ChangeTh;
    biosB0[0x12] = psxBios_InitPAD;
    biosB0[0x13] = psxBios_StartPAD;
    biosB0[0x14] = psxBios_StopPAD;
    biosB0[0x15] = psxBios_PAD_init;
    biosB0[0x16] = psxBios_PAD_dr;
    biosB0[0x17] = psxBios_ReturnFromException;
    biosB0[0x18] = psxBios_ResetEntryInt;
    biosB0[0x19] = psxBios_HookEntryInt;
    biosB0[0x20] = psxBios_UnDeliverEvent;
    biosB0[0x32] = psxBios_open;
    biosB0[0x33] = psxBios_lseek;
    biosB0[0x34] = psxBios_read;
    biosB0[0x35] = psxBios_write;
    biosB0[0x36] = psxBios_close;
    biosB0[0x3c] = psxBios_getchar;
    biosB0[0x42] = psxBios_firstfile;
    biosB0[0x43] = psxBios_nextfile;
    biosB0[0x44] = psxBios_rename;
    biosB0[0x45] = psxBios_delete;
    biosB0[0x4a] = psxBios_InitCARD;
    biosB0[0x4b] = psxBios_StartCARD;
    biosB0[0x4c] = psxBios_StopCARD;
    biosB0[0x4e] = psxBios__card_write;
    biosB0[0x4f] = psxBios__card_read;
    biosB0[0x50] = psxBios__new_card;
    biosB0[0x51] = psxBios_Krom2RawAdd;
    biosB0[0x56] = psxBios_GetC0Table;
    biosB0[0x57] = psxBios_GetB0Table;
    biosB0[0x5b] = psxBios_ChangeClearPad;
    biosC0[0x02] = psxBios_SysEnqIntRP;
    biosC0[0x03] = psxBios_SysDeqIntRP;
    biosC0[0x0a] = psxBios_ChangeClearRCnt;
    base         = 0x1000;
    size         = sizeof(EvCB) * 32;
    Event        = (EvCB *)&g_pcsx->psxMem->psxR[base];
    base += size * 6;
    memset(Event, 0, size * 6);
    HwEV   = Event;
    EvEV   = Event + 32;
    RcEV   = Event + 32 * 2;
    UeEV   = Event + 32 * 3;
    SwEV   = Event + 32 * 4;
    ThEV   = Event + 32 * 5;
    ptr    = (u32 *)&g_pcsx->psxMem->psxM[0x0874];
    ptr[0] = SWAPu32(0x4c54 - 0x884);
    ptr    = (u32 *)&g_pcsx->psxMem->psxM[0x0674];
    ptr[6] = SWAPu32(0xc80);
    memset(SysIntRP, 0, sizeof(SysIntRP));
    memset(Thread, 0, sizeof(Thread));
    Thread[0].status   = 2;
    psxMu32ref(0x0150) = SWAPu32(0x160);
    psxMu32ref(0x0154) = SWAPu32(0x320);
    psxMu32ref(0x0160) = SWAPu32(0x248);
    strcpy((char *)&g_pcsx->psxMem->psxM[0x248], "bu");
    psxRu32ref(0x0000) = SWAPu32((0x3b << 26) | 4);
    psxMu32ref(0x0000) = SWAPu32((0x3b << 26) | 0);
    psxMu32ref(0x00a0) = SWAPu32((0x3b << 26) | 1);
    psxMu32ref(0x00b0) = SWAPu32((0x3b << 26) | 2);
    psxMu32ref(0x00c0) = SWAPu32((0x3b << 26) | 3);
    psxMu32ref(0x4c54) = SWAPu32((0x3b << 26) | 0);
    psxMu32ref(0x8000) = SWAPu32((0x3b << 26) | 5);
    psxMu32ref(0x07a0) = SWAPu32((0x3b << 26) | 0);
    psxMu32ref(0x0884) = SWAPu32((0x3b << 26) | 0);
    psxMu32ref(0x0894) = SWAPu32((0x3b << 26) | 0);
    psxMu32ref(0x6c80) = SWAPu32(0x000085c8);
    psxMu32ref(0x9010) = SWAPu32(0xac20cc00);
    memcpy((Bytef *)(g_pcsx->psxMem->psxR + 0x66000), font_8140, sizeof(font_8140));
    memcpy((Bytef *)(g_pcsx->psxMem->psxR + 0x69d68), font_889f, sizeof(font_8140));
    psxHu32ref(0x1060) = SWAPu32(0x00000b88);
    hleSoftCall        = FALSE;
}
void psxBiosShutdown()
{
}
#define psxBios_PADpoll(pad)                                                                                           \
    {                                                                                                                  \
        PAD##pad##_startPoll(pad);                                                                                     \
        pad_buf##pad[0] = 0;                                                                                           \
        pad_buf##pad[1] = PAD##pad##_poll(0x42);                                                                       \
        if (!(pad_buf##pad[1] & 0x0f)) {                                                                               \
            bufcount = 32;                                                                                             \
        } else {                                                                                                       \
            bufcount = (pad_buf##pad[1] & 0x0f) * 2;                                                                   \
        }                                                                                                              \
        PAD##pad##_poll(0);                                                                                            \
        i = 2;                                                                                                         \
        while (bufcount--) {                                                                                           \
            pad_buf##pad[i++] = PAD##pad##_poll(0);                                                                    \
        }                                                                                                              \
    }
void biosInterrupt()
{
    int i, bufcount;
    if (pad_buf != NULL) {
        u32 *buf = (u32 *)pad_buf;
        if (!Config.UseNet) {
            PAD1_startPoll(1);
            if (PAD1_poll(0x42) == 0x23) {
                PAD1_poll(0);
                *buf = PAD1_poll(0) << 8;
                *buf |= PAD1_poll(0);
                PAD1_poll(0);
                *buf &= ~((PAD1_poll(0) > 0x20) ? 1 << 6 : 0);
                *buf &= ~((PAD1_poll(0) > 0x20) ? 1 << 7 : 0);
            } else {
                PAD1_poll(0);
                *buf = PAD1_poll(0) << 8;
                *buf |= PAD1_poll(0);
            }
            PAD2_startPoll(2);
            if (PAD2_poll(0x42) == 0x23) {
                PAD2_poll(0);
                *buf |= PAD2_poll(0) << 24;
                *buf |= PAD2_poll(0) << 16;
                PAD2_poll(0);
                *buf &= ~((PAD2_poll(0) > 0x20) ? 1 << 22 : 0);
                *buf &= ~((PAD2_poll(0) > 0x20) ? 1 << 23 : 0);
            } else {
                PAD2_poll(0);
                *buf |= PAD2_poll(0) << 24;
                *buf |= PAD2_poll(0) << 16;
            }
        }
    }
    {
        if (pad_buf1) {
            psxBios_PADpoll(1);
        }
        if (pad_buf2) {
            psxBios_PADpoll(2);
        }
    }
    if (psxHu32(0x1070) & 0x1) {
        if (RcEV[3][1].status == EvStACTIVE) {
            softCall(RcEV[3][1].fhandler);
        }
    }
    if (psxHu32(0x1070) & 0x70) {
        int i;
        for (i = 0; i < 3; i++) {
            if (psxHu32(0x1070) & (1 << (i + 4))) {
                if (RcEV[i][1].status == EvStACTIVE) {
                    softCall(RcEV[i][1].fhandler);
                }
                g_pcsx->psxHw->psxHwWrite32(0x1f801070, ~(1 << (i + 4)));
            }
        }
    }
}
void psxBiosException()
{
    int i;
    switch (g_pcsx->psxCpu->psxRegs.CP0.n.Cause & 0x3c) {
        case 0x00:
#ifdef PSXCPU_LOG
#endif
            SaveRegs();
            sp = psxMu32(0x6c80);
            biosInterrupt();
            for (i = 0; i < 8; i++) {
                if (SysIntRP[i]) {
                    u32 *queue = (u32 *)PSXM(SysIntRP[i]);
                    s0         = queue[2];
                    softCall(queue[1]);
                }
            }
            if (jmp_int != NULL) {
                int i;
                g_pcsx->psxHw->psxHwWrite32(0x1f801070, 0xffffffff);
                ra = jmp_int[0];
                sp = jmp_int[1];
                fp = jmp_int[2];
                for (i = 0; i < 8; i++)
                    g_pcsx->psxCpu->psxRegs.GPR.r[16 + i] = jmp_int[3 + i];
                gp  = jmp_int[11];
                v0  = 1;
                pc0 = ra;
                return;
            }
            g_pcsx->psxHw->psxHwWrite16(0x1f801070, 0);
            break;
        case 0x20:
#ifdef PSXCPU_LOG
            PSXCPU_LOG("syscall exp %x\n", a0);
#endif
            switch (a0) {
                case 1:
                    g_pcsx->psxCpu->psxRegs.CP0.n.Status &= ~0x404;
                    v0 = 1;
                    break;
                case 2:
                    g_pcsx->psxCpu->psxRegs.CP0.n.Status |= 0x404;
                    break;
            }
            pc0                                  = g_pcsx->psxCpu->psxRegs.CP0.n.EPC + 4;
            g_pcsx->psxCpu->psxRegs.CP0.n.Status = (g_pcsx->psxCpu->psxRegs.CP0.n.Status & 0xfffffff0) |
                                                   ((g_pcsx->psxCpu->psxRegs.CP0.n.Status & 0x3c) >> 2);
            return;
        default:
#ifdef PSXCPU_LOG
            PSXCPU_LOG("unknown bios exception!\n");
#endif
            break;
    }
    pc0 = g_pcsx->psxCpu->psxRegs.CP0.n.EPC;
    if (g_pcsx->psxCpu->psxRegs.CP0.n.Cause & 0x80000000)
        pc0 += 4;
    g_pcsx->psxCpu->psxRegs.CP0.n.Status =
        (g_pcsx->psxCpu->psxRegs.CP0.n.Status & 0xfffffff0) | ((g_pcsx->psxCpu->psxRegs.CP0.n.Status & 0x3c) >> 2);
}
#define bfreeze(ptr, size)                                                                                             \
    {                                                                                                                  \
        if (Mode == 1)                                                                                                 \
            memcpy(&g_pcsx->psxMem->psxR[base], ptr, size);                                                            \
        if (Mode == 0)                                                                                                 \
            memcpy(ptr, &g_pcsx->psxMem->psxR[base], size);                                                            \
        base += size;                                                                                                  \
    }
#define bfreezes(ptr) bfreeze(ptr, sizeof(ptr))
#define bfreezel(ptr) bfreeze(ptr, sizeof(*ptr))
#define bfreezepsxMptr(ptr, type)                                                                                      \
    {                                                                                                                  \
        if (Mode == 1) {                                                                                               \
            if (ptr)                                                                                                   \
                psxRu32ref(base) = SWAPu32((s8 *)(ptr)-g_pcsx->psxMem->psxM);                                          \
            else                                                                                                       \
                psxRu32ref(base) = 0;                                                                                  \
        } else {                                                                                                       \
            if (psxRu32(base) != 0)                                                                                    \
                ptr = (type *)(g_pcsx->psxMem->psxM + psxRu32(base));                                                  \
            else                                                                                                       \
                (ptr) = NULL;                                                                                          \
        }                                                                                                              \
        base += sizeof(u32);                                                                                           \
    }
void psxBiosFreeze(int Mode)
{
    u32 base = 0x40000;
    bfreezepsxMptr(jmp_int, u32);
    bfreezepsxMptr(pad_buf, int);
    bfreezepsxMptr(pad_buf1, char);
    bfreezepsxMptr(pad_buf2, char);
    bfreezepsxMptr(heap_addr, u32);
    bfreezel(&pad_buf1len);
    bfreezel(&pad_buf2len);
    bfreezes(regs);
    bfreezes(SysIntRP);
    bfreezel(&CardState);
    bfreezes(Thread);
    bfreezel(&CurThread);
    bfreezes(FDesc);
}
