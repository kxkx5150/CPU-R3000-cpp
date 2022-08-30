#include <sys/stat.h>
#include "../bios/bios.h"
#include "sio.h"


SIO::SIO(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void SIO::SIO_INT()
{
    if (!Config.Sio) {
        pcsx->psxCpu->psxRegs.interrupt |= 0x80;
        pcsx->psxCpu->psxRegs.intCycle[7 + 1] = 400;
        pcsx->psxCpu->psxRegs.intCycle[7]     = pcsx->psxCpu->psxRegs.cycle;
    }
}
void SIO::sioWrite8(unsigned char value)
{
    switch (padst) {
        case 1:
            SIO_INT();
            if ((value & 0x40) == 0x40) {
                padst = 2;
                parp  = 1;
                if (!Config.UseNet) {
                    switch (CtrlReg & 0x2002) {
                        case 0x0002:
                            buf[parp] = PAD1_poll(value);
                            break;
                        case 0x2002:
                            buf[parp] = PAD2_poll(value);
                            break;
                    }
                }
                if (!(buf[parp] & 0x0f)) {
                    bufcount = 2 + 32;
                } else {
                    bufcount = 2 + (buf[parp] & 0x0f) * 2;
                }
                if (buf[parp] == 0x41) {
                    switch (value) {
                        case 0x43:
                            buf[1] = 0x43;
                            break;
                        case 0x45:
                            buf[1] = 0xf3;
                            break;
                    }
                }
            } else
                padst = 0;
            return;
        case 2:
            parp++;
            if (!Config.UseNet) {
                switch (CtrlReg & 0x2002) {
                    case 0x0002:
                        buf[parp] = PAD1_poll(value);
                        break;
                    case 0x2002:
                        buf[parp] = PAD2_poll(value);
                        break;
                }
            }
            if (parp == bufcount) {
                padst = 0;
                return;
            }
            SIO_INT();
            return;
    }
    switch (mcdst) {
        case 1:
            SIO_INT();
            if (rdwr) {
                parp++;
                return;
            }
            parp = 1;
            switch (value) {
                case 0x52:
                    rdwr = 1;
                    break;
                case 0x57:
                    rdwr = 2;
                    break;
                default:
                    mcdst = 0;
            }
            return;
        case 2:
            SIO_INT();
            adrH     = value;
            *buf     = 0;
            parp     = 0;
            bufcount = 1;
            mcdst    = 3;
            return;
        case 3:
            SIO_INT();
            adrL     = value;
            *buf     = adrH;
            parp     = 0;
            bufcount = 1;
            mcdst    = 4;
            return;
        case 4:
            SIO_INT();
            parp = 0;
            switch (rdwr) {
                case 1:
                    buf[0] = 0x5c;
                    buf[1] = 0x5d;
                    buf[2] = adrH;
                    buf[3] = adrL;
                    switch (CtrlReg & 0x2002) {
                        case 0x0002:
                            memcpy(&buf[4], Mcd1Data + (adrL | (adrH << 8)) * 128, 128);
                            break;
                        case 0x2002:
                            memcpy(&buf[4], Mcd2Data + (adrL | (adrH << 8)) * 128, 128);
                            break;
                    }
                    {
                        char _xor = 0;
                        int  i;
                        for (i = 2; i < 128 + 4; i++)
                            _xor ^= buf[i];
                        buf[132] = _xor;
                    }
                    buf[133] = 0x47;
                    bufcount = 133;
                    break;
                case 2:
                    buf[0]   = adrL;
                    buf[1]   = value;
                    buf[129] = 0x5c;
                    buf[130] = 0x5d;
                    buf[131] = 0x47;
                    bufcount = 131;
                    break;
            }
            mcdst = 5;
            return;
        case 5:
            parp++;
            if (rdwr == 2) {
                if (parp < 128)
                    buf[parp + 1] = value;
            }
            SIO_INT();
            return;
    }
    switch (value) {
        case 0x01:
            StatReg |= RX_RDY;
            switch (CtrlReg & 0x2002) {
                case 0x0002:
                    buf[0] = PAD1_startPoll(1);
                    break;
                case 0x2002:
                    buf[0] = PAD2_startPoll(2);
                    break;
            }
            bufcount = 2;
            parp     = 0;
            padst    = 1;
            SIO_INT();
            return;
        case 0x81:
            StatReg |= RX_RDY;
            memcpy(buf, cardh, 4);
            parp     = 0;
            bufcount = 3;
            mcdst    = 1;
            rdwr     = 0;
            SIO_INT();
            return;
    }
}
void SIO::sioWriteStat16(unsigned short value)
{
}
void SIO::sioWriteMode16(unsigned short value)
{
    ModeReg = value;
}
void SIO::sioWriteCtrl16(unsigned short value)
{
    CtrlReg = value & ~RESET_ERR;
    if (value & RESET_ERR)
        StatReg &= ~IRQ;
    if ((CtrlReg & SIO_RESET) || (!CtrlReg)) {
        padst   = 0;
        mcdst   = 0;
        parp    = 0;
        StatReg = TX_RDY | TX_EMPTY;
        pcsx->psxCpu->psxRegs.interrupt &= ~0x80;
    }
}
void SIO::sioWriteBaud16(unsigned short value)
{
    BaudReg = value;
}
unsigned char SIO::sioRead8()
{
    unsigned char ret = 0;
    if ((StatReg & RX_RDY)) {
        ret = buf[parp];
        if (parp == bufcount) {
            StatReg &= ~RX_RDY;
            if (mcdst == 5) {
                mcdst = 0;
                if (rdwr == 2) {
                    switch (CtrlReg & 0x2002) {
                        case 0x0002:
                            memcpy(Mcd1Data + (adrL | (adrH << 8)) * 128, &buf[1], 128);
                            SaveMcd(Config.Mcd1, Mcd1Data, (adrL | (adrH << 8)) * 128, 128);
                            break;
                        case 0x2002:
                            memcpy(Mcd2Data + (adrL | (adrH << 8)) * 128, &buf[1], 128);
                            SaveMcd(Config.Mcd2, Mcd2Data, (adrL | (adrH << 8)) * 128, 128);
                            break;
                    }
                }
            }
            if (padst == 2)
                padst = 0;
            if (mcdst == 1) {
                mcdst = 2;
                StatReg |= RX_RDY;
            }
        }
    }

    return ret;
}
unsigned short SIO::sioReadStat16()
{
    return StatReg;
}
unsigned short SIO::sioReadMode16()
{
    return ModeReg;
}
unsigned short SIO::sioReadCtrl16()
{
    return CtrlReg;
}
unsigned short SIO::sioReadBaud16()
{
    return BaudReg;
}
void SIO::sioInterrupt()
{
    StatReg |= IRQ;
    psxHu32ref(0x1070) |= SWAPu32(0x80);
}
void SIO::LoadMcd(int mcd, char *str)
{
    FILE *f;
    char *data = NULL;
    if (mcd == 1)
        data = Mcd1Data;
    if (mcd == 2)
        data = Mcd2Data;
    if (*str == 0) {
        sprintf(str, "memcards/card%d.mcd", mcd);
        SysPrintf(_("No memory card value was specified - creating a default card %s\n"), str);
    }
    f = fopen(str, "rb");
    if (f == NULL) {
        SysPrintf(_("The memory card %s doesn't exist - creating it\n"), str);
        CreateMcd(str);
        f = fopen(str, "rb");
        if (f != NULL) {
            struct stat buf;
            if (stat(str, &buf) != -1) {
                if (buf.st_size == MCD_SIZE + 64)
                    fseek(f, 64, SEEK_SET);
                else if (buf.st_size == MCD_SIZE + 3904)
                    fseek(f, 3904, SEEK_SET);
            }
            fread(data, 1, MCD_SIZE, f);
            fclose(f);
        } else
            SysMessage(_("Memory card %s failed to load!\n"), str);
    } else {
        struct stat buf;
        SysPrintf(_("Loading memory card %s\n"), str);
        if (stat(str, &buf) != -1) {
            if (buf.st_size == MCD_SIZE + 64)
                fseek(f, 64, SEEK_SET);
            else if (buf.st_size == MCD_SIZE + 3904)
                fseek(f, 3904, SEEK_SET);
        }
        fread(data, 1, MCD_SIZE, f);
        fclose(f);
    }
}
void SIO::LoadMcds(char *mcd1, char *mcd2)
{
    LoadMcd(1, mcd1);
    LoadMcd(2, mcd2);
}
void SIO::SaveMcd(char *mcd, char *data, uint32_t adr, int size)
{
    FILE *f;
    f = fopen(mcd, "r+b");
    if (f != NULL) {
        struct stat buf;
        if (stat(mcd, &buf) != -1) {
            if (buf.st_size == MCD_SIZE + 64)
                fseek(f, adr + 64, SEEK_SET);
            else if (buf.st_size == MCD_SIZE + 3904)
                fseek(f, adr + 3904, SEEK_SET);
            else
                fseek(f, adr, SEEK_SET);
        } else
            fseek(f, adr, SEEK_SET);
        fwrite(data + adr, 1, size, f);
        fclose(f);
        return;
    }

    ConvertMcd(mcd, data);
}
void SIO::CreateMcd(char *mcd)
{
    FILE       *f;
    struct stat buf;
    int         s = MCD_SIZE;
    int         i = 0, j;
    f             = fopen(mcd, "wb");
    if (f == NULL) {
        printf("unable to create %s\n", mcd);
        return;
    }
    if (stat(mcd, &buf) != -1) {
        if ((buf.st_size == MCD_SIZE + 3904) || strstr(mcd, ".gme")) {
            s = s + 3904;
            fputc('1', f);
            s--;
            fputc('2', f);
            s--;
            fputc('3', f);
            s--;
            fputc('-', f);
            s--;
            fputc('4', f);
            s--;
            fputc('5', f);
            s--;
            fputc('6', f);
            s--;
            fputc('-', f);
            s--;
            fputc('S', f);
            s--;
            fputc('T', f);
            s--;
            fputc('D', f);
            s--;
            for (i = 0; i < 7; i++) {
                fputc(0, f);
                s--;
            }
            fputc(1, f);
            s--;
            fputc(0, f);
            s--;
            fputc(1, f);
            s--;
            fputc('M', f);
            s--;
            fputc('Q', f);
            s--;
            for (i = 0; i < 14; i++) {
                fputc(0xa0, f);
                s--;
            }
            fputc(0, f);
            s--;
            fputc(0xff, f);
            while (s-- > (MCD_SIZE + 1))
                fputc(0, f);
        } else if ((buf.st_size == MCD_SIZE + 64) || strstr(mcd, ".mem") || strstr(mcd, ".vgs")) {
            s = s + 64;
            fputc('V', f);
            s--;
            fputc('g', f);
            s--;
            fputc('s', f);
            s--;
            fputc('M', f);
            s--;
            for (i = 0; i < 3; i++) {
                fputc(1, f);
                s--;
                fputc(0, f);
                s--;
                fputc(0, f);
                s--;
                fputc(0, f);
                s--;
            }
            fputc(0, f);
            s--;
            fputc(2, f);
            while (s-- > (MCD_SIZE + 1))
                fputc(0, f);
        }
    }
    fputc('M', f);
    s--;
    fputc('C', f);
    s--;
    while (s-- > (MCD_SIZE - 127))
        fputc(0, f);
    fputc(0xe, f);
    s--;
    for (i = 0; i < 15; i++) {
        fputc(0xa0, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0xff, f);
        s--;
        fputc(0xff, f);
        s--;
        for (j = 0; j < 117; j++) {
            fputc(0x00, f);
            s--;
        }
        fputc(0xa0, f);
        s--;
    }
    for (i = 0; i < 20; i++) {
        fputc(0xff, f);
        s--;
        fputc(0xff, f);
        s--;
        fputc(0xff, f);
        s--;
        fputc(0xff, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0x00, f);
        s--;
        fputc(0xff, f);
        s--;
        fputc(0xff, f);
        s--;
        for (j = 0; j < 118; j++) {
            fputc(0x00, f);
            s--;
        }
    }
    while ((s--) >= 0)
        fputc(0, f);
    fclose(f);
}
void SIO::ConvertMcd(char *mcd, char *data)
{
    FILE *f;
    int   i = 0;
    int   s = MCD_SIZE;
    printf("convertmcd %s\n", mcd);
    if (strstr(mcd, ".gme")) {
        f = fopen(mcd, "wb");
        if (f != NULL) {
            fwrite(data - 3904, 1, MCD_SIZE + 3904, f);
            fclose(f);
        }
        f = fopen(mcd, "r+");
        s = s + 3904;
        fputc('1', f);
        s--;
        fputc('2', f);
        s--;
        fputc('3', f);
        s--;
        fputc('-', f);
        s--;
        fputc('4', f);
        s--;
        fputc('5', f);
        s--;
        fputc('6', f);
        s--;
        fputc('-', f);
        s--;
        fputc('S', f);
        s--;
        fputc('T', f);
        s--;
        fputc('D', f);
        s--;
        for (i = 0; i < 7; i++) {
            fputc(0, f);
            s--;
        }
        fputc(1, f);
        s--;
        fputc(0, f);
        s--;
        fputc(1, f);
        s--;
        fputc('M', f);
        s--;
        fputc('Q', f);
        s--;
        for (i = 0; i < 14; i++) {
            fputc(0xa0, f);
            s--;
        }
        fputc(0, f);
        s--;
        fputc(0xff, f);
        while (s-- > (MCD_SIZE + 1))
            fputc(0, f);
        fclose(f);
    } else if (strstr(mcd, ".mem") || strstr(mcd, ".vgs")) {
        f = fopen(mcd, "wb");
        if (f != NULL) {
            fwrite(data - 64, 1, MCD_SIZE + 64, f);
            fclose(f);
        }
        f = fopen(mcd, "r+");
        s = s + 64;
        fputc('V', f);
        s--;
        fputc('g', f);
        s--;
        fputc('s', f);
        s--;
        fputc('M', f);
        s--;
        for (i = 0; i < 3; i++) {
            fputc(1, f);
            s--;
            fputc(0, f);
            s--;
            fputc(0, f);
            s--;
            fputc(0, f);
            s--;
        }
        fputc(0, f);
        s--;
        fputc(2, f);
        while (s-- > (MCD_SIZE + 1))
            fputc(0, f);
        fclose(f);
    } else {
        f = fopen(mcd, "wb");
        if (f != NULL) {
            fwrite(data, 1, MCD_SIZE, f);
            fclose(f);
        }
    }
}
void SIO::GetMcdBlockInfo(int mcd, int block)
{
    char          *data = NULL, *ptr, *str, *sstr;
    unsigned short clut[16];
    unsigned short c;
    int            i, x;

    // memset(Info, 0, sizeof(McdBlock));

    if (mcd == 1)
        data = Mcd1Data;
    if (mcd == 2)
        data = Mcd2Data;

    ptr       = data + block * 8192 + 2;
    IconCount = *ptr & 0x3;
    ptr += 2;
    x    = 0;
    str  = Title;
    sstr = sTitle;

    for (i = 0; i < 48; i++) {
        c = *(ptr) << 8;
        c |= *(ptr + 1);
        if (!c)
            break;
        if (c >= 0x8281 && c <= 0x829A)
            c = (c - 0x8281) + 'a';
        else if (c >= 0x824F && c <= 0x827A)
            c = (c - 0x824F) + '0';
        else if (c == 0x8140)
            c = ' ';
        else if (c == 0x8143)
            c = ',';
        else if (c == 0x8144)
            c = '.';
        else if (c == 0x8146)
            c = ':';
        else if (c == 0x8147)
            c = ';';
        else if (c == 0x8148)
            c = '?';
        else if (c == 0x8149)
            c = '!';
        else if (c == 0x815E)
            c = '/';
        else if (c == 0x8168)
            c = '"';
        else if (c == 0x8169)
            c = '(';
        else if (c == 0x816A)
            c = ')';
        else if (c == 0x816D)
            c = '[';
        else if (c == 0x816E)
            c = ']';
        else if (c == 0x817C)
            c = '-';
        else {
            str[i]    = ' ';
            sstr[x++] = *ptr++;
            sstr[x++] = *ptr++;
            continue;
        }
        str[i] = sstr[x++] = c;
        ptr += 2;
    }

    trim(str);
    trim(sstr);
    ptr = data + block * 8192 + 0x60;

    for (i = 0; i < 16; i++) {
        clut[i] = *((unsigned short *)ptr);
        ptr += 2;
    }

    for (i = 0; i < IconCount; i++) {
        short *icon = &Icon[i * 16 * 16];
        ptr         = data + block * 8192 + 128 + 128 * i;
        for (x = 0; x < 16 * 16; x++) {
            icon[x++] = clut[*ptr & 0xf];
            icon[x]   = clut[*ptr >> 4];
            ptr++;
        }
    }

    ptr   = data + block * 128;
    Flags = *ptr;
    ptr += 0xa;
    strncpy(ID, ptr, 12);
    ptr += 12;
    strncpy(Name, ptr, 16);
}
int SIO::sioFreeze(gzFile f, int Mode)
{
    gzfreeze(buf, sizeof(buf));
    gzfreeze(&StatReg, sizeof(StatReg));
    gzfreeze(&ModeReg, sizeof(ModeReg));
    gzfreeze(&CtrlReg, sizeof(CtrlReg));
    gzfreeze(&BaudReg, sizeof(BaudReg));
    gzfreeze(&bufcount, sizeof(bufcount));
    gzfreeze(&parp, sizeof(parp));
    gzfreeze(&mcdst, sizeof(mcdst));
    gzfreeze(&rdwr, sizeof(rdwr));
    gzfreeze(&adrH, sizeof(adrH));
    gzfreeze(&adrL, sizeof(adrL));
    gzfreeze(&padst, sizeof(padst));
    return 0;
}
