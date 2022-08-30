#include "cdrom.h"
#include "../bios/bios.h"
#include "cdriso.h"

#define CdlSync      0
#define CdlNop       1
#define CdlSetloc    2
#define CdlPlay      3
#define CdlForward   4
#define CdlBackward  5
#define CdlReadN     6
#define CdlStandby   7
#define CdlStop      8
#define CdlPause     9
#define CdlInit      10
#define CdlMute      11
#define CdlDemute    12
#define CdlSetfilter 13
#define CdlSetmode   14
#define CdlGetmode   15
#define CdlGetlocL   16
#define CdlGetlocP   17
#define CdlReadT     18
#define CdlGetTN     19
#define CdlGetTD     20
#define CdlSeekL     21
#define CdlSeekP     22
#define CdlSetclock  23
#define CdlGetclock  24
#define CdlTest      25
#define CdlID        26
#define CdlReadS     27
#define CdlReset     28
#define CdlReadToc   30
#define AUTOPAUSE    249
#define READ_ACK     250
#define READ         251
#define REPPLAY_ACK  252
#define REPPLAY      253
#define ASYNC        254
#define FIXED


#define NOT(_X_) (!(_X_))
#define XACLAMP(_X_, _MI_, _MA_)                                                                                       \
    {                                                                                                                  \
        if (_X_ < _MI_)                                                                                                \
            _X_ = _MI_;                                                                                                \
        if (_X_ > _MA_)                                                                                                \
            _X_ = _MA_;                                                                                                \
    }

#define SH     4
#define BLKSIZ 28

#define IK0(fid) (-K0[fid])
#define IK1(fid) (-K1[fid])

#define SUB_SUB_EOF     (1 << 7)
#define SUB_SUB_RT      (1 << 6)
#define SUB_SUB_FORM    (1 << 5)
#define SUB_SUB_TRIGGER (1 << 4)
#define SUB_SUB_DATA    (1 << 3)
#define SUB_SUB_AUDIO   (1 << 2)
#define SUB_SUB_VIDEO   (1 << 1)
#define SUB_SUB_EOR     (1 << 0)

#define AUDIO_CODING_GET_STEREO(_X_)   ((_X_)&3)
#define AUDIO_CODING_GET_FREQ(_X_)     (((_X_) >> 2) & 3)
#define AUDIO_CODING_GET_BPS(_X_)      (((_X_) >> 4) & 3)
#define AUDIO_CODING_GET_EMPHASIS(_X_) (((_X_) >> 6) & 1)
#define SUB_UNKNOWN                    0
#define SUB_VIDEO                      1
#define SUB_AUDIO                      2
#define cdReadTime                     (PSXCLK / 75)
#define CDR_INT(eCycle)                                                                                                \
    {                                                                                                                  \
        pcsx->psxCpu->psxRegs.interrupt |= 0x4;                                                                        \
        pcsx->psxCpu->psxRegs.intCycle[2 + 1] = eCycle;                                                                \
        pcsx->psxCpu->psxRegs.intCycle[2]     = pcsx->psxCpu->psxRegs.cycle;                                           \
    }
#define CDREAD_INT(eCycle)                                                                                             \
    {                                                                                                                  \
        pcsx->psxCpu->psxRegs.interrupt |= 0x40000;                                                                    \
        pcsx->psxCpu->psxRegs.intCycle[2 + 16 + 1] = eCycle;                                                           \
        pcsx->psxCpu->psxRegs.intCycle[2 + 16]     = pcsx->psxCpu->psxRegs.cycle;                                      \
    }
#define StartReading(type, eCycle)                                                                                     \
    {                                                                                                                  \
        cdr.Reading     = type;                                                                                        \
        cdr.FirstSector = 1;                                                                                           \
        cdr.Readed      = 0xff;                                                                                        \
        AddIrqQueue(READ_ACK, eCycle);                                                                                 \
    }
#define StopReading()                                                                                                  \
    {                                                                                                                  \
        if (cdr.Reading) {                                                                                             \
            cdr.Reading = 0;                                                                                           \
            pcsx->psxCpu->psxRegs.interrupt &= ~0x40000;                                                               \
        }                                                                                                              \
        cdr.StatP &= ~0x20;                                                                                            \
    }
#define StopCdda()                                                                                                     \
    {                                                                                                                  \
        if (cdr.Play) {                                                                                                \
            if (!Config.Cdda)                                                                                          \
                pcsx->psxPlugs->CDRstop();                                                                             \
            cdr.StatP &= ~0x80;                                                                                        \
            cdr.Play = FALSE;                                                                                          \
        }                                                                                                              \
    }
#define SetResultSize(size)                                                                                            \
    {                                                                                                                  \
        cdr.ResultP     = 0;                                                                                           \
        cdr.ResultC     = size;                                                                                        \
        cdr.ResultReady = 1;                                                                                           \
    }
#define NoIntr      0
#define DataReady   1
#define Complete    2
#define Acknowledge 3
#define DataEnd     4
#define DiskError   5


CDROM::CDROM(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void CDROM::ReadTrack()
{
    cdr.Prev[0] = itob(cdr.SetSector[0]);
    cdr.Prev[1] = itob(cdr.SetSector[1]);
    cdr.Prev[2] = itob(cdr.SetSector[2]);

    cdr.RErr = pcsx->psxPlugs->CDRreadTrack(cdr.Prev);
}
void CDROM::AddIrqQueue(unsigned char irq, unsigned long ecycle)
{
    cdr.Irq = irq;
    if (cdr.Stat) {
        cdr.eCycle = ecycle;
    } else {
        CDR_INT(ecycle);
    }
}
void CDROM::cdrInterrupt()
{
    int           i;
    unsigned char Irq = cdr.Irq;
    if (cdr.Stat) {
        CDR_INT(0x1000);
        return;
    }
    cdr.Irq = 0xff;
    cdr.Ctrl &= ~0x80;
    switch (Irq) {
        case CdlSync:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlNop:
            SetResultSize(1);
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            i             = stat.Status;
            if (pcsx->psxPlugs->CDRgetStatus(&stat) != -1) {
                if (stat.Type == 0xff)
                    cdr.Stat = DiskError;
                if (stat.Status & 0x10) {
                    cdr.Stat = DiskError;
                    cdr.Result[0] |= 0x11;
                    cdr.Result[0] &= ~0x02;
                } else if (i & 0x10) {
                    cdr.StatP |= 0x2;
                    cdr.Result[0] |= 0x2;
                    CheckCdrom();
                }
            }
            break;
        case CdlSetloc:
            cdr.CmdProcess = 0;
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlPlay:
            cdr.CmdProcess = 0;
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            cdr.StatP |= 0x80;
            break;
        case CdlForward:
            cdr.CmdProcess = 0;
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case CdlBackward:
            cdr.CmdProcess = 0;
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case CdlStandby:
            cdr.CmdProcess = 0;
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case CdlStop:
            cdr.CmdProcess = 0;
            SetResultSize(1);
            cdr.StatP &= ~0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            i             = stat.Status;
            if (pcsx->psxPlugs->CDRgetStatus(&stat) != -1) {
                if (stat.Type == 0xff)
                    cdr.Stat = DiskError;
                if (stat.Status & 0x10) {
                    cdr.Stat = DiskError;
                    cdr.Result[0] |= 0x11;
                    cdr.Result[0] &= ~0x02;
                } else if (i & 0x10) {
                    cdr.StatP |= 0x2;
                    cdr.Result[0] |= 0x2;
                    CheckCdrom();
                }
            }
            break;
        case CdlPause:
            SetResultSize(1);
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            AddIrqQueue(CdlPause + 0x20, 0x1000);
            cdr.Ctrl |= 0x80;
            break;
        case CdlPause + 0x20:
            SetResultSize(1);
            cdr.StatP &= ~0x20;
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case CdlInit:
            SetResultSize(1);
            cdr.StatP     = 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            AddIrqQueue(CdlInit + 0x20, 0x1000);
            break;
        case CdlInit + 0x20:
            SetResultSize(1);
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            cdr.Init      = 1;
            break;
        case CdlMute:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlDemute:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlSetfilter:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlSetmode:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlGetmode:
            SetResultSize(6);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Result[1] = cdr.Mode;
            cdr.Result[2] = cdr.File;
            cdr.Result[3] = cdr.Channel;
            cdr.Result[4] = 0;
            cdr.Result[5] = 0;
            cdr.Stat      = Acknowledge;
            break;
        case CdlGetlocL:
            SetResultSize(8);
            for (i = 0; i < 8; i++)
                cdr.Result[i] = cdr.Transfer[i];
            cdr.Stat = Acknowledge;
            break;
        case CdlGetlocP:
            SetResultSize(8);
            subq = (struct SubQ *)pcsx->psxPlugs->CDRgetBufferSub();
            if (subq != NULL) {
                cdr.Result[0] = subq->TrackNumber;
                cdr.Result[1] = subq->IndexNumber;
                memcpy(cdr.Result + 2, subq->TrackRelativeAddress, 3);
                memcpy(cdr.Result + 5, subq->AbsoluteAddress, 3);
                if (calcCrc((u8 *)subq + 12, 10) != (((u16)subq->CRC[0] << 8) | subq->CRC[1])) {
                    memset(cdr.Result + 2, 0, 3 + 3);
                }
            } else {
                cdr.Result[0] = 1;
                cdr.Result[1] = 1;
                cdr.Result[2] = btoi(cdr.Prev[0]);
                cdr.Result[3] = btoi(cdr.Prev[1]) - 2;
                cdr.Result[4] = cdr.Prev[2];
                if ((s8)cdr.Result[3] < 0) {
                    cdr.Result[3] += 60;
                    cdr.Result[2] -= 1;
                }
                cdr.Result[2] = itob(cdr.Result[2]);
                cdr.Result[3] = itob(cdr.Result[3]);
                memcpy(cdr.Result + 5, cdr.Prev, 3);
            }
            cdr.Stat = Acknowledge;
            break;
        case CdlGetTN:
            cdr.CmdProcess = 0;
            SetResultSize(3);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            if (pcsx->psxPlugs->CDRgetTN(cdr.ResultTN) == -1) {
                cdr.Stat = DiskError;
                cdr.Result[0] |= 0x01;
            } else {
                cdr.Stat      = Acknowledge;
                cdr.Result[1] = itob(cdr.ResultTN[0]);
                cdr.Result[2] = itob(cdr.ResultTN[1]);
            }
            break;
        case CdlGetTD:
            cdr.CmdProcess = 0;
            cdr.Track      = btoi(cdr.Param[0]);
            SetResultSize(4);
            cdr.StatP |= 0x2;
            if (pcsx->psxPlugs->CDRgetTD(cdr.Track, cdr.ResultTD) == -1) {
                cdr.Stat = DiskError;
                cdr.Result[0] |= 0x01;
            } else {
                cdr.Stat      = Acknowledge;
                cdr.Result[0] = cdr.StatP;
                cdr.Result[1] = itob(cdr.ResultTD[2]);
                cdr.Result[2] = itob(cdr.ResultTD[1]);
                cdr.Result[3] = itob(cdr.ResultTD[0]);
            }
            break;
        case CdlSeekL:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.StatP |= 0x40;
            cdr.Stat   = Acknowledge;
            cdr.Seeked = TRUE;
            AddIrqQueue(CdlSeekL + 0x20, 0x1000);
            break;
        case CdlSeekL + 0x20:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.StatP &= ~0x40;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case CdlSeekP:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.StatP |= 0x40;
            cdr.Stat = Acknowledge;
            AddIrqQueue(CdlSeekP + 0x20, 0x1000);
            break;
        case CdlSeekP + 0x20:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.StatP &= ~0x40;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case CdlTest:
            cdr.Stat = Acknowledge;
            switch (cdr.Param[0]) {
                case 0x20:
                    SetResultSize(4);
                    memcpy(cdr.Result, Test20, 4);
                    break;
                case 0x22:
                    SetResultSize(8);
                    memcpy(cdr.Result, Test22, 4);
                    break;
                case 0x23:
                case 0x24:
                    SetResultSize(8);
                    memcpy(cdr.Result, Test23, 4);
                    break;
            }
            break;
        case CdlID:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            AddIrqQueue(CdlID + 0x20, 0x1000);
            break;
        case CdlID + 0x20:
            SetResultSize(8);
            if (pcsx->psxPlugs->CDRgetStatus(&stat) == -1) {
                cdr.Result[0] = 0x00;
                cdr.Result[1] = 0x00;
            } else {
                if (stat.Type == 2) {
                    cdr.Result[0] = 0x08;
                    cdr.Result[1] = 0x10;
                } else {
                    cdr.Result[0] = 0x00;
                    cdr.Result[1] = 0x00;
                }
            }
            cdr.Result[1] |= 0x80;
            cdr.Result[2] = 0x00;
            cdr.Result[3] = 0x00;
            strncpy((char *)&cdr.Result[4], "PCSX", 4);
            cdr.Stat = Complete;
            break;
        case CdlReset:
            SetResultSize(1);
            cdr.StatP     = 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            break;
        case CdlReadToc:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Acknowledge;
            AddIrqQueue(CdlReadToc + 0x20, 0x1000);
            break;
        case CdlReadToc + 0x20:
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            cdr.Stat      = Complete;
            break;
        case AUTOPAUSE:
            cdr.OCUP = 0;
            AddIrqQueue(CdlPause, 0x800);
            break;
        case READ_ACK:
            if (!cdr.Reading)
                return;
            SetResultSize(1);
            cdr.StatP |= 0x2;
            cdr.Result[0] = cdr.StatP;
            if (!cdr.Seeked) {
                cdr.Seeked = TRUE;
                cdr.StatP |= 0x40;
            }
            cdr.StatP |= 0x20;
            cdr.Stat = Acknowledge;
            CDREAD_INT(0x80000);
            break;
        case REPPLAY_ACK:
            cdr.Stat      = Acknowledge;
            cdr.Result[0] = cdr.StatP;
            SetResultSize(1);
            AddIrqQueue(REPPLAY, cdReadTime);
            break;
        case REPPLAY:
            if ((cdr.Mode & 5) != 5)
                break;
            break;
        case 0xff:
            return;
        default:
            cdr.Stat = Complete;
            break;
    }
    if (cdr.Stat != NoIntr && cdr.Reg2 != 0x18) {
        psxHu32ref(0x1070) |= SWAP32((u32)0x4);
    }
}
void CDROM::cdrReadInterrupt()
{
    u8 *buf;
    if (!cdr.Reading)
        return;
    if (cdr.Stat) {
        CDREAD_INT(0x1000);
        return;
    }

    cdr.OCUP = 1;
    SetResultSize(1);
    cdr.StatP |= 0x22;
    cdr.StatP &= ~0x40;
    cdr.Result[0] = cdr.StatP;
    ReadTrack();
    buf = pcsx->psxPlugs->CDRgetBuffer();
    if (buf == NULL)
        cdr.RErr = -1;
    if (cdr.RErr == -1) {

        memset(cdr.Transfer, 0, DATA_SIZE);
        cdr.Stat = DiskError;
        cdr.Result[0] |= 0x01;
        CDREAD_INT((cdr.Mode & 0x80) ? (cdReadTime / 2) : cdReadTime);
        return;
    }
    memcpy(cdr.Transfer, buf, DATA_SIZE);
    cdr.Stat = DataReady;

    if ((!cdr.Muted) && (cdr.Mode & 0x40) && (!Config.Xa) && (cdr.FirstSector != -1)) {
        if ((cdr.Transfer[4 + 2] & 0x4) && ((cdr.Mode & 0x8) ? (cdr.Transfer[4 + 1] == cdr.Channel) : 1) &&
            (cdr.Transfer[4 + 0] == cdr.File)) {
            int ret = xa_decode_sector(&cdr.Xa, cdr.Transfer + 4, cdr.FirstSector);
            if (!ret) {
                SPUplayADPCMchannel(&cdr.Xa);
                cdr.FirstSector = 0;
            } else
                cdr.FirstSector = -1;
        }
    }
    cdr.SetSector[2]++;
    if (cdr.SetSector[2] == 75) {
        cdr.SetSector[2] = 0;
        cdr.SetSector[1]++;
        if (cdr.SetSector[1] == 60) {
            cdr.SetSector[1] = 0;
            cdr.SetSector[0]++;
        }
    }
    cdr.Readed = 0;
    if ((cdr.Transfer[4 + 2] & 0x80) && (cdr.Mode & 0x2)) {

        AddIrqQueue(CdlPause, 0x1000);
    } else {
        CDREAD_INT((cdr.Mode & 0x80) ? (cdReadTime / 2) : cdReadTime);
    }
    psxHu32ref(0x1070) |= SWAP32((u32)0x4);
}
unsigned char CDROM::cdrRead0(void)
{
    if (cdr.ResultReady)
        cdr.Ctrl |= 0x20;
    else
        cdr.Ctrl &= ~0x20;
    if (cdr.OCUP)
        cdr.Ctrl |= 0x40;
    cdr.Ctrl |= 0x18;

    return psxHu8(0x1800) = cdr.Ctrl;
}
void CDROM::cdrWrite0(unsigned char rt)
{

    cdr.Ctrl = rt | (cdr.Ctrl & ~0x3);
    if (rt == 0) {
        cdr.ParamP      = 0;
        cdr.ParamC      = 0;
        cdr.ResultReady = 0;
    }
}
unsigned char CDROM::cdrRead1(void)
{
    if (cdr.ResultReady) {
        psxHu8(0x1801) = cdr.Result[cdr.ResultP++];
        if (cdr.ResultP == cdr.ResultC)
            cdr.ResultReady = 0;
    } else {
        psxHu8(0x1801) = 0;
    }

    return psxHu8(0x1801);
}
void CDROM::cdrWrite1(unsigned char rt)
{
    int i;
    cdr.Cmd  = rt;
    cdr.OCUP = 0;

    if (cdr.Ctrl & 0x1)
        return;
    switch (cdr.Cmd) {
        case CdlSync:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlNop:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlSetloc:
            StopReading();
            cdr.Seeked = FALSE;
            for (i = 0; i < 3; i++)
                cdr.SetSector[i] = btoi(cdr.Param[i]);
            cdr.SetSector[3] = 0;
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlPlay:
            if (!cdr.SetSector[0] & !cdr.SetSector[1] & !cdr.SetSector[2]) {
                if (pcsx->psxPlugs->CDRgetTN(cdr.ResultTN) != -1) {
                    if (cdr.CurTrack > cdr.ResultTN[1])
                        cdr.CurTrack = cdr.ResultTN[1];
                    if (pcsx->psxPlugs->CDRgetTD((unsigned char)(cdr.CurTrack), cdr.ResultTD) != -1) {
                        int tmp         = cdr.ResultTD[2];
                        cdr.ResultTD[2] = cdr.ResultTD[0];
                        cdr.ResultTD[0] = tmp;
                        if (!Config.Cdda)
                            pcsx->psxPlugs->CDRplay(cdr.ResultTD);
                    }
                }
            } else if (!Config.Cdda) {
                pcsx->psxPlugs->CDRplay(cdr.SetSector);
            }
            cdr.Play = TRUE;
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlForward:
            if (cdr.CurTrack < 0xaa)
                cdr.CurTrack++;
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlBackward:
            if (cdr.CurTrack > 1)
                cdr.CurTrack--;
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlReadN:
            cdr.Irq = 0;
            StopReading();
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            StartReading(1, 0x1000);
            break;
        case CdlStandby:
            StopCdda();
            StopReading();
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlStop:
            StopCdda();
            StopReading();
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlPause:
            StopCdda();
            StopReading();
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x80000);
            break;
        case CdlReset:
        case CdlInit:
            StopCdda();
            StopReading();
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlMute:
            cdr.Muted = TRUE;
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlDemute:
            cdr.Muted = FALSE;
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlSetfilter:
            cdr.File    = cdr.Param[0];
            cdr.Channel = cdr.Param[1];
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlSetmode:

            cdr.Mode = cdr.Param[0];
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlGetmode:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlGetlocL:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlGetlocP:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlGetTN:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlGetTD:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlSeekL:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlSeekP:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlTest:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlID:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        case CdlReadS:
            cdr.Irq = 0;
            StopReading();
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            StartReading(2, 0x1000);
            break;
        case CdlReadToc:
            cdr.Ctrl |= 0x80;
            cdr.Stat = NoIntr;
            AddIrqQueue(cdr.Cmd, 0x1000);
            break;
        default:

            return;
    }
    if (cdr.Stat != NoIntr) {
        psxHu32ref(0x1070) |= SWAP32((u32)0x4);
    }
}
unsigned char CDROM::cdrRead2(void)
{
    unsigned char ret;
    if (cdr.Readed == 0) {
        ret = 0;
    } else {
        ret = *cdr.pTransfer++;
    }

    return ret;
}
void CDROM::cdrWrite2(unsigned char rt)
{

    if (cdr.Ctrl & 0x1) {
        switch (rt) {
            case 0x07:
                cdr.ParamP      = 0;
                cdr.ParamC      = 0;
                cdr.ResultReady = 1;
                cdr.Ctrl &= ~3;
                break;
            default:
                cdr.Reg2 = rt;
                break;
        }
    } else if (!(cdr.Ctrl & 0x1) && cdr.ParamP < 8) {
        cdr.Param[cdr.ParamP++] = rt;
        cdr.ParamC++;
    }
}
unsigned char CDROM::cdrRead3(void)
{
    if (cdr.Stat) {
        if (cdr.Ctrl & 0x1)
            psxHu8(0x1803) = cdr.Stat | 0xE0;
        else
            psxHu8(0x1803) = 0xff;
    } else {
        psxHu8(0x1803) = 0;
    }

    return psxHu8(0x1803);
}
void CDROM::cdrWrite3(unsigned char rt)
{

    if (rt == 0x07 && cdr.Ctrl & 0x1) {
        cdr.Stat = 0;
        if (cdr.Irq == 0xff) {
            cdr.Irq = 0;
            return;
        }
        if (cdr.Irq)
            CDR_INT(cdr.eCycle);
        if (cdr.Reading && !cdr.ResultReady)
            CDREAD_INT((cdr.Mode & 0x80) ? (cdReadTime / 2) : cdReadTime);
        return;
    }
    if (rt == 0x80 && !(cdr.Ctrl & 0x1) && cdr.Readed == 0) {
        cdr.Readed    = 1;
        cdr.pTransfer = cdr.Transfer;
        switch (cdr.Mode & 0x30) {
            case 0x10:
            case 0x00:
                cdr.pTransfer += 12;
                break;
            default:
                break;
        }
    }
}
void CDROM::psxDma3(u32 madr, u32 bcr, u32 chcr)
{
    u32 cdsize;
    u8 *ptr;

    switch (chcr) {
        case 0x11000000:
        case 0x11400100:
            if (cdr.Readed == 0) {

                break;
            }
            cdsize = (bcr & 0xffff) * 4;
            ptr    = (u8 *)PSXM(madr);
            if (ptr == NULL) {
                break;
            }
            memcpy(ptr, cdr.pTransfer, cdsize);
            pcsx->psxCpu->intClear(madr, cdsize / 4);
            cdr.pTransfer += cdsize;
            break;
        default:

            break;
    }
    HW_DMA3_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(3);
}
void CDROM::cdrReset()
{
    memset(&cdr, 0, sizeof(cdr));
    cdr.CurTrack = 1;
    cdr.File     = 1;
    cdr.Channel  = 1;
}
int CDROM::cdrFreeze(gzFile f, int Mode)
{
    uintptr_t tmp;
    gzfreeze(&cdr, sizeof(cdr));
    if (Mode == 1)
        tmp = cdr.pTransfer - cdr.Transfer;
    gzfreeze(&tmp, sizeof(tmp));
    if (Mode == 0)
        cdr.pTransfer = cdr.Transfer + tmp;
    return 0;
}
void CDROM::ADPCM_InitDecode(ADPCM_Decode_t *decp)
{
    decp->y0 = 0;
    decp->y1 = 0;
}
void CDROM::ADPCM_DecodeBlock16(ADPCM_Decode_t *decp, u8 filter_range, const void *vblockp, short *destp, int inc)
{
    int        i;
    int        range, filterid;
    s32        fy0, fy1;
    const u16 *blockp;
    blockp   = (const unsigned short *)vblockp;
    filterid = (filter_range >> 4) & 0x0f;
    range    = (filter_range >> 0) & 0x0f;
    fy0      = decp->y0;
    fy1      = decp->y1;
    for (i = BLKSIZ / 4; i; --i) {
        s32 y;
        s32 x0, x1, x2, x3;
        y  = *blockp++;
        x3 = (short)(y & 0xf000) >> range;
        x3 <<= SH;
        x2 = (short)((y << 4) & 0xf000) >> range;
        x2 <<= SH;
        x1 = (short)((y << 8) & 0xf000) >> range;
        x1 <<= SH;
        x0 = (short)((y << 12) & 0xf000) >> range;
        x0 <<= SH;
        x0 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC;
        fy1 = fy0;
        fy0 = x0;
        x1 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC;
        fy1 = fy0;
        fy0 = x1;
        x2 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC;
        fy1 = fy0;
        fy0 = x2;
        x3 -= (IK0(filterid) * fy0 + (IK1(filterid) * fy1)) >> SHC;
        fy1 = fy0;
        fy0 = x3;
        XACLAMP(x0, -(32768 << SH), 32767 << SH);
        *destp = x0 >> SH;
        destp += inc;
        XACLAMP(x1, -(32768 << SH), 32767 << SH);
        *destp = x1 >> SH;
        destp += inc;
        XACLAMP(x2, -(32768 << SH), 32767 << SH);
        *destp = x2 >> SH;
        destp += inc;
        XACLAMP(x3, -(32768 << SH), 32767 << SH);
        *destp = x3 >> SH;
        destp += inc;
    }
    decp->y0 = fy0;
    decp->y1 = fy1;
}
void CDROM::xa_decode_data(xa_decode_t *xdp, unsigned char *srcp)
{
    const u8 *sound_groupsp;
    const u8 *sound_datap, *sound_datap2;
    int       i, j, k, nbits;
    u16       data[4096], *datap;
    short    *destp;
    destp = xdp->pcm;
    nbits = xdp->nbits == 4 ? 4 : 2;
    if (xdp->stereo) {
        if ((xdp->nbits == 8) && (xdp->freq == 37800)) {
            for (j = 0; j < 18; j++) {
                sound_groupsp = srcp + j * 128;
                sound_datap   = sound_groupsp + 16;
                for (i = 0; i < nbits; i++) {
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 14; k++, sound_datap2 += 8) {
                        *(datap++) = (u16)sound_datap2[0] | (u16)(sound_datap2[4] << 8);
                    }
                    ADPCM_DecodeBlock16(&xdp->left, sound_groupsp[headtable[i] + 0], data, destp + 0, 2);
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 14; k++, sound_datap2 += 8) {
                        *(datap++) = (u16)sound_datap2[0] | (u16)(sound_datap2[4] << 8);
                    }
                    ADPCM_DecodeBlock16(&xdp->right, sound_groupsp[headtable[i] + 1], data, destp + 1, 2);
                    destp += 28 * 2;
                }
            }
        } else {
            for (j = 0; j < 18; j++) {
                sound_groupsp = srcp + j * 128;
                sound_datap   = sound_groupsp + 16;
                for (i = 0; i < nbits; i++) {
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 7; k++, sound_datap2 += 16) {
                        *(datap++) = (u16)(sound_datap2[0] & 0x0f) | ((u16)(sound_datap2[4] & 0x0f) << 4) |
                                     ((u16)(sound_datap2[8] & 0x0f) << 8) | ((u16)(sound_datap2[12] & 0x0f) << 12);
                    }
                    ADPCM_DecodeBlock16(&xdp->left, sound_groupsp[headtable[i] + 0], data, destp + 0, 2);
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 7; k++, sound_datap2 += 16) {
                        *(datap++) = (u16)(sound_datap2[0] >> 4) | ((u16)(sound_datap2[4] >> 4) << 4) |
                                     ((u16)(sound_datap2[8] >> 4) << 8) | ((u16)(sound_datap2[12] >> 4) << 12);
                    }
                    ADPCM_DecodeBlock16(&xdp->right, sound_groupsp[headtable[i] + 1], data, destp + 1, 2);
                    destp += 28 * 2;
                }
            }
        }
    } else {
        if ((xdp->nbits == 8) && (xdp->freq == 37800)) {
            for (j = 0; j < 18; j++) {
                sound_groupsp = srcp + j * 128;
                sound_datap   = sound_groupsp + 16;
                for (i = 0; i < nbits; i++) {
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 14; k++, sound_datap2 += 8) {
                        *(datap++) = (u16)sound_datap2[0] | (u16)(sound_datap2[4] << 8);
                    }
                    ADPCM_DecodeBlock16(&xdp->left, sound_groupsp[headtable[i] + 0], data, destp, 1);
                    destp += 28;
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 14; k++, sound_datap2 += 8) {
                        *(datap++) = (u16)sound_datap2[0] | (u16)(sound_datap2[4] << 8);
                    }
                    ADPCM_DecodeBlock16(&xdp->left, sound_groupsp[headtable[i] + 1], data, destp, 1);
                    destp += 28;
                }
            }
        } else {
            for (j = 0; j < 18; j++) {
                sound_groupsp = srcp + j * 128;
                sound_datap   = sound_groupsp + 16;
                for (i = 0; i < nbits; i++) {
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 7; k++, sound_datap2 += 16) {
                        *(datap++) = (u16)(sound_datap2[0] & 0x0f) | ((u16)(sound_datap2[4] & 0x0f) << 4) |
                                     ((u16)(sound_datap2[8] & 0x0f) << 8) | ((u16)(sound_datap2[12] & 0x0f) << 12);
                    }
                    ADPCM_DecodeBlock16(&xdp->left, sound_groupsp[headtable[i] + 0], data, destp, 1);
                    destp += 28;
                    datap        = data;
                    sound_datap2 = sound_datap + i;
                    for (k = 0; k < 7; k++, sound_datap2 += 16) {
                        *(datap++) = (u16)(sound_datap2[0] >> 4) | ((u16)(sound_datap2[4] >> 4) << 4) |
                                     ((u16)(sound_datap2[8] >> 4) << 8) | ((u16)(sound_datap2[12] >> 4) << 12);
                    }
                    ADPCM_DecodeBlock16(&xdp->left, sound_groupsp[headtable[i] + 1], data, destp, 1);
                    destp += 28;
                }
            }
        }
    }
}
int CDROM::parse_xa_audio_sector(xa_decode_t *xdp, xa_subheader_t *subheadp, unsigned char *sectorp,
                                 int is_first_sector)
{
    if (is_first_sector) {
        switch (AUDIO_CODING_GET_FREQ(subheadp->coding)) {
            case 0:
                xdp->freq = 37800;
                break;
            case 1:
                xdp->freq = 18900;
                break;
            default:
                xdp->freq = 0;
                break;
        }
        switch (AUDIO_CODING_GET_BPS(subheadp->coding)) {
            case 0:
                xdp->nbits = 4;
                break;
            case 1:
                xdp->nbits = 8;
                break;
            default:
                xdp->nbits = 0;
                break;
        }
        switch (AUDIO_CODING_GET_STEREO(subheadp->coding)) {
            case 0:
                xdp->stereo = 0;
                break;
            case 1:
                xdp->stereo = 1;
                break;
            default:
                xdp->stereo = 0;
                break;
        }
        if (xdp->freq == 0)
            return -1;
        ADPCM_InitDecode(&xdp->left);
        ADPCM_InitDecode(&xdp->right);
        xdp->nsamples = 18 * 28 * 8;
        if (xdp->stereo == 1)
            xdp->nsamples /= 2;
    }
    xa_decode_data(xdp, sectorp);
    return 0;
}
s32 CDROM::xa_decode_sector(xa_decode_t *xdp, unsigned char *sectorp, int is_first_sector)
{
    if (parse_xa_audio_sector(xdp, (xa_subheader_t *)sectorp, sectorp + sizeof(xa_subheader_t), is_first_sector))
        return -1;
    return 0;
}
