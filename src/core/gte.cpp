#include "gte.h"
#include "mem.h"
#include "r3000a.h"



#define VX(n)   (n < 3 ? pcsx->psxCpu->psxRegs.CP2D.p[n << 1].sw.l : pcsx->psxCpu->psxRegs.CP2D.p[9].sw.l)
#define VY(n)   (n < 3 ? pcsx->psxCpu->psxRegs.CP2D.p[n << 1].sw.h : pcsx->psxCpu->psxRegs.CP2D.p[10].sw.l)
#define VZ(n)   (n < 3 ? pcsx->psxCpu->psxRegs.CP2D.p[(n << 1) + 1].sw.l : pcsx->psxCpu->psxRegs.CP2D.p[11].sw.l)
#define MX11(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3)].sw.l : 0)
#define MX12(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3)].sw.h : 0)
#define MX13(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 1].sw.l : 0)
#define MX21(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 1].sw.h : 0)
#define MX22(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 2].sw.l : 0)
#define MX23(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 2].sw.h : 0)
#define MX31(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 3].sw.l : 0)
#define MX32(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 3].sw.h : 0)
#define MX33(n) (n < 3 ? pcsx->psxCpu->psxRegs.CP2C.p[(n << 3) + 4].sw.l : 0)
#define CV1(n)  (n < 3 ? (s32)pcsx->psxCpu->psxRegs.CP2C.r[(n << 3) + 5] : 0)
#define CV2(n)  (n < 3 ? (s32)pcsx->psxCpu->psxRegs.CP2C.r[(n << 3) + 6] : 0)
#define CV3(n)  (n < 3 ? (s32)pcsx->psxCpu->psxRegs.CP2C.r[(n << 3) + 7] : 0)

#define fSX(n) ((pcsx->psxCpu->psxRegs.CP2D.p)[((n) + 12)].sw.l)
#define fSY(n) ((pcsx->psxCpu->psxRegs.CP2D.p)[((n) + 12)].sw.h)
#define fSZ(n) ((pcsx->psxCpu->psxRegs.CP2D.p)[((n) + 17)].w.l)

#define gteVXY0  (pcsx->psxCpu->psxRegs.CP2D.r[0])
#define gteVX0   (pcsx->psxCpu->psxRegs.CP2D.p[0].sw.l)
#define gteVY0   (pcsx->psxCpu->psxRegs.CP2D.p[0].sw.h)
#define gteVZ0   (pcsx->psxCpu->psxRegs.CP2D.p[1].sw.l)
#define gteVXY1  (pcsx->psxCpu->psxRegs.CP2D.r[2])
#define gteVX1   (pcsx->psxCpu->psxRegs.CP2D.p[2].sw.l)
#define gteVY1   (pcsx->psxCpu->psxRegs.CP2D.p[2].sw.h)
#define gteVZ1   (pcsx->psxCpu->psxRegs.CP2D.p[3].sw.l)
#define gteVXY2  (pcsx->psxCpu->psxRegs.CP2D.r[4])
#define gteVX2   (pcsx->psxCpu->psxRegs.CP2D.p[4].sw.l)
#define gteVY2   (pcsx->psxCpu->psxRegs.CP2D.p[4].sw.h)
#define gteVZ2   (pcsx->psxCpu->psxRegs.CP2D.p[5].sw.l)
#define gteRGB   (pcsx->psxCpu->psxRegs.CP2D.r[6])
#define gteR     (pcsx->psxCpu->psxRegs.CP2D.p[6].b.l)
#define gteG     (pcsx->psxCpu->psxRegs.CP2D.p[6].b.h)
#define gteB     (pcsx->psxCpu->psxRegs.CP2D.p[6].b.h2)
#define gteCODE  (pcsx->psxCpu->psxRegs.CP2D.p[6].b.h3)
#define gteOTZ   (pcsx->psxCpu->psxRegs.CP2D.p[7].w.l)
#define gteIR0   (pcsx->psxCpu->psxRegs.CP2D.p[8].sw.l)
#define gteIR1   (pcsx->psxCpu->psxRegs.CP2D.p[9].sw.l)
#define gteIR2   (pcsx->psxCpu->psxRegs.CP2D.p[10].sw.l)
#define gteIR3   (pcsx->psxCpu->psxRegs.CP2D.p[11].sw.l)
#define gteSXY0  (pcsx->psxCpu->psxRegs.CP2D.r[12])
#define gteSX0   (pcsx->psxCpu->psxRegs.CP2D.p[12].sw.l)
#define gteSY0   (pcsx->psxCpu->psxRegs.CP2D.p[12].sw.h)
#define gteSXY1  (pcsx->psxCpu->psxRegs.CP2D.r[13])
#define gteSX1   (pcsx->psxCpu->psxRegs.CP2D.p[13].sw.l)
#define gteSY1   (pcsx->psxCpu->psxRegs.CP2D.p[13].sw.h)
#define gteSXY2  (pcsx->psxCpu->psxRegs.CP2D.r[14])
#define gteSX2   (pcsx->psxCpu->psxRegs.CP2D.p[14].sw.l)
#define gteSY2   (pcsx->psxCpu->psxRegs.CP2D.p[14].sw.h)
#define gteSXYP  (pcsx->psxCpu->psxRegs.CP2D.r[15])
#define gteSXP   (pcsx->psxCpu->psxRegs.CP2D.p[15].sw.l)
#define gteSYP   (pcsx->psxCpu->psxRegs.CP2D.p[15].sw.h)
#define gteSZ0   (pcsx->psxCpu->psxRegs.CP2D.p[16].w.l)
#define gteSZ1   (pcsx->psxCpu->psxRegs.CP2D.p[17].w.l)
#define gteSZ2   (pcsx->psxCpu->psxRegs.CP2D.p[18].w.l)
#define gteSZ3   (pcsx->psxCpu->psxRegs.CP2D.p[19].w.l)
#define gteRGB0  (pcsx->psxCpu->psxRegs.CP2D.r[20])
#define gteR0    (pcsx->psxCpu->psxRegs.CP2D.p[20].b.l)
#define gteG0    (pcsx->psxCpu->psxRegs.CP2D.p[20].b.h)
#define gteB0    (pcsx->psxCpu->psxRegs.CP2D.p[20].b.h2)
#define gteCODE0 (pcsx->psxCpu->psxRegs.CP2D.p[20].b.h3)
#define gteRGB1  (pcsx->psxCpu->psxRegs.CP2D.r[21])
#define gteR1    (pcsx->psxCpu->psxRegs.CP2D.p[21].b.l)
#define gteG1    (pcsx->psxCpu->psxRegs.CP2D.p[21].b.h)
#define gteB1    (pcsx->psxCpu->psxRegs.CP2D.p[21].b.h2)
#define gteCODE1 (pcsx->psxCpu->psxRegs.CP2D.p[21].b.h3)
#define gteRGB2  (pcsx->psxCpu->psxRegs.CP2D.r[22])
#define gteR2    (pcsx->psxCpu->psxRegs.CP2D.p[22].b.l)
#define gteG2    (pcsx->psxCpu->psxRegs.CP2D.p[22].b.h)
#define gteB2    (pcsx->psxCpu->psxRegs.CP2D.p[22].b.h2)
#define gteCODE2 (pcsx->psxCpu->psxRegs.CP2D.p[22].b.h3)
#define gteRES1  (pcsx->psxCpu->psxRegs.CP2D.r[23])
#define gteMAC0  (((s32 *)pcsx->psxCpu->psxRegs.CP2D.r)[24])
#define gteMAC1  (((s32 *)pcsx->psxCpu->psxRegs.CP2D.r)[25])
#define gteMAC2  (((s32 *)pcsx->psxCpu->psxRegs.CP2D.r)[26])
#define gteMAC3  (((s32 *)pcsx->psxCpu->psxRegs.CP2D.r)[27])
#define gteIRGB  (pcsx->psxCpu->psxRegs.CP2D.r[28])
#define gteORGB  (pcsx->psxCpu->psxRegs.CP2D.r[29])
#define gteLZCS  (pcsx->psxCpu->psxRegs.CP2D.r[30])
#define gteLZCR  (pcsx->psxCpu->psxRegs.CP2D.r[31])

#define gteR11R12 (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[0])
#define gteR22R23 (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[2])
#define gteR11    (pcsx->psxCpu->psxRegs.CP2C.p[0].sw.l)
#define gteR12    (pcsx->psxCpu->psxRegs.CP2C.p[0].sw.h)
#define gteR13    (pcsx->psxCpu->psxRegs.CP2C.p[1].sw.l)
#define gteR21    (pcsx->psxCpu->psxRegs.CP2C.p[1].sw.h)
#define gteR22    (pcsx->psxCpu->psxRegs.CP2C.p[2].sw.l)
#define gteR23    (pcsx->psxCpu->psxRegs.CP2C.p[2].sw.h)
#define gteR31    (pcsx->psxCpu->psxRegs.CP2C.p[3].sw.l)
#define gteR32    (pcsx->psxCpu->psxRegs.CP2C.p[3].sw.h)
#define gteR33    (pcsx->psxCpu->psxRegs.CP2C.p[4].sw.l)
#define gteTRX    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[5])
#define gteTRY    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[6])
#define gteTRZ    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[7])
#define gteL11    (pcsx->psxCpu->psxRegs.CP2C.p[8].sw.l)
#define gteL12    (pcsx->psxCpu->psxRegs.CP2C.p[8].sw.h)
#define gteL13    (pcsx->psxCpu->psxRegs.CP2C.p[9].sw.l)
#define gteL21    (pcsx->psxCpu->psxRegs.CP2C.p[9].sw.h)
#define gteL22    (pcsx->psxCpu->psxRegs.CP2C.p[10].sw.l)
#define gteL23    (pcsx->psxCpu->psxRegs.CP2C.p[10].sw.h)
#define gteL31    (pcsx->psxCpu->psxRegs.CP2C.p[11].sw.l)
#define gteL32    (pcsx->psxCpu->psxRegs.CP2C.p[11].sw.h)
#define gteL33    (pcsx->psxCpu->psxRegs.CP2C.p[12].sw.l)
#define gteRBK    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[13])
#define gteGBK    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[14])
#define gteBBK    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[15])
#define gteLR1    (pcsx->psxCpu->psxRegs.CP2C.p[16].sw.l)
#define gteLR2    (pcsx->psxCpu->psxRegs.CP2C.p[16].sw.h)
#define gteLR3    (pcsx->psxCpu->psxRegs.CP2C.p[17].sw.l)
#define gteLG1    (pcsx->psxCpu->psxRegs.CP2C.p[17].sw.h)
#define gteLG2    (pcsx->psxCpu->psxRegs.CP2C.p[18].sw.l)
#define gteLG3    (pcsx->psxCpu->psxRegs.CP2C.p[18].sw.h)
#define gteLB1    (pcsx->psxCpu->psxRegs.CP2C.p[19].sw.l)
#define gteLB2    (pcsx->psxCpu->psxRegs.CP2C.p[19].sw.h)
#define gteLB3    (pcsx->psxCpu->psxRegs.CP2C.p[20].sw.l)
#define gteRFC    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[21])
#define gteGFC    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[22])
#define gteBFC    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[23])
#define gteOFX    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[24])
#define gteOFY    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[25])
#define gteH      (pcsx->psxCpu->psxRegs.CP2C.p[26].sw.l)
#define gteDQA    (pcsx->psxCpu->psxRegs.CP2C.p[27].sw.l)
#define gteDQB    (((s32 *)pcsx->psxCpu->psxRegs.CP2C.r)[28])
#define gteZSF3   (pcsx->psxCpu->psxRegs.CP2C.p[29].sw.l)
#define gteZSF4   (pcsx->psxCpu->psxRegs.CP2C.p[30].sw.l)
#define gteFLAG   (pcsx->psxCpu->psxRegs.CP2C.r[31])

#define GTE_OP(op)    ((op >> 20) & 31)
#define GTE_SF(op)    ((op >> 19) & 1)
#define GTE_MX(op)    ((op >> 17) & 3)
#define GTE_V(op)     ((op >> 15) & 3)
#define GTE_CV(op)    ((op >> 13) & 3)
#define GTE_CD(op)    ((op >> 11) & 3)
#define GTE_LM(op)    ((op >> 10) & 1)
#define GTE_CT(op)    ((op >> 6) & 15)
#define GTE_FUNCT(op) (op & 63)
#define gteop         (pcsx->psxCpu->psxRegs.code & 0x1ffffff)

#define A1(a)       BOUNDS((a), 0x7fffffff, (1 << 30), -(s64)0x80000000, (1 << 31) | (1 << 27))
#define A2(a)       BOUNDS((a), 0x7fffffff, (1 << 29), -(s64)0x80000000, (1 << 31) | (1 << 26))
#define A3(a)       BOUNDS((a), 0x7fffffff, (1 << 28), -(s64)0x80000000, (1 << 31) | (1 << 25))
#define limB1(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1 << 31) | (1 << 24))
#define limB2(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1 << 31) | (1 << 23))
#define limB3(a, l) LIM((a), 0x7fff, -0x8000 * !l, (1 << 22))
#define limC1(a)    LIM((a), 0x00ff, 0x0000, (1 << 21))
#define limC2(a)    LIM((a), 0x00ff, 0x0000, (1 << 20))
#define limC3(a)    LIM((a), 0x00ff, 0x0000, (1 << 19))
#define limD(a)     LIM((a), 0xffff, 0x0000, (1 << 31) | (1 << 18))

#define F(a)     BOUNDS((a), 0x7fffffff, (1 << 31) | (1 << 16), -(s64)0x80000000, (1 << 31) | (1 << 15))
#define limG1(a) LIM((a), 0x3ff, -0x400, (1 << 31) | (1 << 14))
#define limG2(a) LIM((a), 0x3ff, -0x400, (1 << 31) | (1 << 13))
#define limH(a)  LIM((a), 0xfff, 0x000, (1 << 12))
#define _oB_g    (pcsx->psxCpu->psxRegs.GPR.r[_Rs_] + _Imm_)



GTE::GTE(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
s64 GTE::BOUNDS(s64 n_value, s64 n_max, int n_maxflag, s64 n_min, int n_minflag)
{
    if (n_value > n_max) {
        gteFLAG |= n_maxflag;
    } else if (n_value < n_min) {
        gteFLAG |= n_minflag;
    }
    return n_value;
}
s32 GTE::LIM(s32 value, s32 max, s32 min, u32 flag)
{
    s32 ret = value;
    if (value > max) {
        gteFLAG |= flag;
        ret = max;
    } else if (value < min) {
        gteFLAG |= flag;
        ret = min;
    }
    return ret;
}
u32 GTE::limE(u32 result)
{
    if (result > 0x1ffff) {
        gteFLAG |= (1 << 31) | (1 << 17);
        return 0x1ffff;
    }
    return result;
}
u32 GTE::MFC2(int reg)
{
    switch (reg) {
        case 1:
        case 3:
        case 5:
        case 8:
        case 9:
        case 10:
        case 11:
            pcsx->psxCpu->psxRegs.CP2D.r[reg] = (s32)pcsx->psxCpu->psxRegs.CP2D.p[reg].sw.l;
            break;
        case 7:
        case 16:
        case 17:
        case 18:
        case 19:
            pcsx->psxCpu->psxRegs.CP2D.r[reg] = (u32)pcsx->psxCpu->psxRegs.CP2D.p[reg].w.l;
            break;
        case 15:
            pcsx->psxCpu->psxRegs.CP2D.r[reg] = gteSXY2;
            break;
        case 28:
        case 30:
            return 0;
        case 29:
            pcsx->psxCpu->psxRegs.CP2D.r[reg] = LIM(gteIR1 >> 7, 0x1f, 0, 0) | (LIM(gteIR2 >> 7, 0x1f, 0, 0) << 5) |
                                                (LIM(gteIR3 >> 7, 0x1f, 0, 0) << 10);
            break;
    }
    return pcsx->psxCpu->psxRegs.CP2D.r[reg];
}
u32 GTE::DIVIDE(s16 n, u16 d)
{
    if (n >= 0 && n < d * 2) {
        u32 offset = d;
        int shift  = 0;
        u64 reciprocal;
        u32 r, s;
        while (offset <= 0x8000) {
            offset <<= 1;
            shift++;
        }
        r          = initial_guess[offset & 0x7fff] | 0x10000;
        s          = (u64)offset * r >> 16;
        r          = (u64)r * (0x20000 - s) >> 16;
        s          = (u64)offset * r >> 16;
        r          = (u64)r * (0x20000 - s) >> 16;
        reciprocal = (u64)(r) << shift;
        return (u32)(((reciprocal * n) + 0x8000) >> 16);
    }
    return 0xffffffff;
}
void GTE::MTC2(u32 value, int reg)
{
    switch (reg) {
        case 15:
            gteSXY0 = gteSXY1;
            gteSXY1 = gteSXY2;
            gteSXY2 = value;
            gteSXYP = value;
            break;
        case 28:
            gteIRGB = value;
            gteIR1  = (value & 0x1f) << 7;
            gteIR2  = (value & 0x3e0) << 2;
            gteIR3  = (value & 0x7c00) >> 3;
            break;
        case 30: {
            int a;
            gteLZCS = value;
            a       = gteLZCS;
            if (a > 0) {
                int i;
                for (i = 31; (a & (1 << i)) == 0 && i >= 0; i--)
                    ;
                gteLZCR = 31 - i;
            } else if (a < 0) {
                int i;
                a ^= 0xffffffff;
                for (i = 31; (a & (1 << i)) == 0 && i >= 0; i--)
                    ;
                gteLZCR = 31 - i;
            } else {
                gteLZCR = 32;
            }
        } break;
        case 7:
        case 29:
        case 31:
            return;
        default:
            pcsx->psxCpu->psxRegs.CP2D.r[reg] = value;
    }
}
void GTE::CTC2(u32 value, int reg)
{
    switch (reg) {
        case 4:
        case 12:
        case 20:
        case 26:
        case 27:
        case 29:
        case 30:
            value = (s32)(s16)value;
            break;
        case 31:
            value = value & 0x7ffff000;
            if (value & 0x7f87e000)
                value |= 0x80000000;
            break;
    }
    pcsx->psxCpu->psxRegs.CP2C.r[reg] = value;
}
void GTE::gteMFC2()
{
    if (!_Rt_)
        return;
    pcsx->psxCpu->psxRegs.GPR.r[_Rt_] = MFC2(_Rd_);
}
void GTE::gteCFC2()
{
    if (!_Rt_)
        return;
    pcsx->psxCpu->psxRegs.GPR.r[_Rt_] = pcsx->psxCpu->psxRegs.CP2C.r[_Rd_];
}
void GTE::gteMTC2()
{
    MTC2(pcsx->psxCpu->psxRegs.GPR.r[_Rt_], _Rd_);
}
void GTE::gteCTC2()
{
    CTC2(pcsx->psxCpu->psxRegs.GPR.r[_Rt_], _Rd_);
}
void GTE::gteLWC2()
{
    MTC2(pcsx->psxMem->psxMemRead32(_oB_g), _Rt_);
}
void GTE::gteSWC2()
{
    pcsx->psxMem->psxMemWrite32(_oB_g, MFC2(_Rt_));
}
void GTE::gteRTPS()
{
    int quotient;
    gteFLAG  = 0;
    gteMAC1  = A1((((s64)gteTRX << 12) + (gteR11 * gteVX0) + (gteR12 * gteVY0) + (gteR13 * gteVZ0)) >> 12);
    gteMAC2  = A2((((s64)gteTRY << 12) + (gteR21 * gteVX0) + (gteR22 * gteVY0) + (gteR23 * gteVZ0)) >> 12);
    gteMAC3  = A3((((s64)gteTRZ << 12) + (gteR31 * gteVX0) + (gteR32 * gteVY0) + (gteR33 * gteVZ0)) >> 12);
    gteIR1   = limB1(gteMAC1, 0);
    gteIR2   = limB2(gteMAC2, 0);
    gteIR3   = limB3(gteMAC3, 0);
    gteSZ0   = gteSZ1;
    gteSZ1   = gteSZ2;
    gteSZ2   = gteSZ3;
    gteSZ3   = limD(gteMAC3);
    quotient = limE(DIVIDE(gteH, gteSZ3));
    gteSXY0  = gteSXY1;
    gteSXY1  = gteSXY2;
    gteSX2   = limG1(F((s64)gteOFX + ((s64)gteIR1 * quotient)) >> 16);
    gteSY2   = limG2(F((s64)gteOFY + ((s64)gteIR2 * quotient)) >> 16);
    gteMAC0  = F((s64)(gteDQB + ((s64)gteDQA * quotient)) >> 12);
    gteIR0   = limH(gteMAC0);
}
void GTE::gteRTPT()
{
    int quotient;
    int v;
    s32 vx, vy, vz;
    gteFLAG = 0;
    gteSZ0  = gteSZ3;
    for (v = 0; v < 3; v++) {
        vx       = VX(v);
        vy       = VY(v);
        vz       = VZ(v);
        gteMAC1  = A1((((s64)gteTRX << 12) + (gteR11 * vx) + (gteR12 * vy) + (gteR13 * vz)) >> 12);
        gteMAC2  = A2((((s64)gteTRY << 12) + (gteR21 * vx) + (gteR22 * vy) + (gteR23 * vz)) >> 12);
        gteMAC3  = A3((((s64)gteTRZ << 12) + (gteR31 * vx) + (gteR32 * vy) + (gteR33 * vz)) >> 12);
        gteIR1   = limB1(gteMAC1, 0);
        gteIR2   = limB2(gteMAC2, 0);
        gteIR3   = limB3(gteMAC3, 0);
        fSZ(v)   = limD(gteMAC3);
        quotient = limE(DIVIDE(gteH, fSZ(v)));
        fSX(v)   = limG1(F((s64)gteOFX + ((s64)gteIR1 * quotient)) >> 16);
        fSY(v)   = limG2(F((s64)gteOFY + ((s64)gteIR2 * quotient)) >> 16);
    }
    gteMAC0 = F((s64)(gteDQB + ((s64)gteDQA * quotient)) >> 12);
    gteIR0  = limH(gteMAC0);
}
void GTE::gteMVMVA()
{
    int shift = 12 * GTE_SF(gteop);
    int mx    = GTE_MX(gteop);
    int v     = GTE_V(gteop);
    int cv    = GTE_CV(gteop);
    int lm    = GTE_LM(gteop);
    s32 vx    = VX(v);
    s32 vy    = VY(v);
    s32 vz    = VZ(v);
    gteFLAG   = 0;
    gteMAC1   = A1((((s64)CV1(cv) << 12) + (MX11(mx) * vx) + (MX12(mx) * vy) + (MX13(mx) * vz)) >> shift);
    gteMAC2   = A2((((s64)CV2(cv) << 12) + (MX21(mx) * vx) + (MX22(mx) * vy) + (MX23(mx) * vz)) >> shift);
    gteMAC3   = A3((((s64)CV3(cv) << 12) + (MX31(mx) * vx) + (MX32(mx) * vy) + (MX33(mx) * vz)) >> shift);
    gteIR1    = limB1(gteMAC1, lm);
    gteIR2    = limB2(gteMAC2, lm);
    gteIR3    = limB3(gteMAC3, lm);
}
void GTE::gteNCLIP()
{
    gteFLAG = 0;
    gteMAC0 = F((s64)gteSX0 * (gteSY1 - gteSY2) + gteSX1 * (gteSY2 - gteSY0) + gteSX2 * (gteSY0 - gteSY1));
}
void GTE::gteAVSZ3()
{
    gteFLAG = 0;
    gteMAC0 = F((s64)(gteZSF3 * gteSZ1) + (gteZSF3 * gteSZ2) + (gteZSF3 * gteSZ3));
    gteOTZ  = limD(gteMAC0 >> 12);
}
void GTE::gteAVSZ4()
{
    gteFLAG = 0;
    gteMAC0 = F((s64)(gteZSF4 * (gteSZ0 + gteSZ1 + gteSZ2 + gteSZ3)));
    gteOTZ  = limD(gteMAC0 >> 12);
}
void GTE::gteSQR()
{
    int shift = 12 * GTE_SF(gteop);
    int lm    = GTE_LM(gteop);
    gteFLAG   = 0;
    gteMAC1   = A1((gteIR1 * gteIR1) >> shift);
    gteMAC2   = A2((gteIR2 * gteIR2) >> shift);
    gteMAC3   = A3((gteIR3 * gteIR3) >> shift);
    gteIR1    = limB1(gteMAC1 >> shift, lm);
    gteIR2    = limB2(gteMAC2 >> shift, lm);
    gteIR3    = limB3(gteMAC3 >> shift, lm);
}
void GTE::gteNCCS()
{
    gteFLAG  = 0;
    gteMAC1  = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
    gteMAC2  = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
    gteMAC3  = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
    gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
    gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1(((s64)gteR * gteIR1) >> 8);
    gteMAC2  = A2(((s64)gteG * gteIR2) >> 8);
    gteMAC3  = A3(((s64)gteB * gteIR3) >> 8);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteRGB0  = gteRGB1;
    gteRGB1  = gteRGB2;
    gteCODE2 = gteCODE;
    gteR2    = limC1(gteMAC1 >> 4);
    gteG2    = limC2(gteMAC2 >> 4);
    gteB2    = limC3(gteMAC3 >> 4);
}
void GTE::gteNCCT()
{
    int v;
    s32 vx, vy, vz;
    gteFLAG = 0;
    for (v = 0; v < 3; v++) {
        vx       = VX(v);
        vy       = VY(v);
        vz       = VZ(v);
        gteMAC1  = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
        gteMAC2  = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
        gteMAC3  = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
        gteIR1   = limB1(gteMAC1, 1);
        gteIR2   = limB2(gteMAC2, 1);
        gteIR3   = limB3(gteMAC3, 1);
        gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
        gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
        gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
        gteIR1   = limB1(gteMAC1, 1);
        gteIR2   = limB2(gteMAC2, 1);
        gteIR3   = limB3(gteMAC3, 1);
        gteMAC1  = A1(((s64)gteR * gteIR1) >> 8);
        gteMAC2  = A2(((s64)gteG * gteIR2) >> 8);
        gteMAC3  = A3(((s64)gteB * gteIR3) >> 8);
        gteRGB0  = gteRGB1;
        gteRGB1  = gteRGB2;
        gteCODE2 = gteCODE;
        gteR2    = limC1(gteMAC1 >> 4);
        gteG2    = limC2(gteMAC2 >> 4);
        gteB2    = limC3(gteMAC3 >> 4);
    }
    gteIR1 = limB1(gteMAC1, 1);
    gteIR2 = limB2(gteMAC2, 1);
    gteIR3 = limB3(gteMAC3, 1);
}
void GTE::gteNCDS()
{
    gteFLAG  = 0;
    gteMAC1  = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
    gteMAC2  = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
    gteMAC3  = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
    gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
    gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1(((((s64)gteR << 4) * gteIR1) + (gteIR0 * limB1(gteRFC - ((gteR * gteIR1) >> 8), 0))) >> 12);
    gteMAC2  = A2(((((s64)gteG << 4) * gteIR2) + (gteIR0 * limB2(gteGFC - ((gteG * gteIR2) >> 8), 0))) >> 12);
    gteMAC3  = A3(((((s64)gteB << 4) * gteIR3) + (gteIR0 * limB3(gteBFC - ((gteB * gteIR3) >> 8), 0))) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteRGB0  = gteRGB1;
    gteRGB1  = gteRGB2;
    gteCODE2 = gteCODE;
    gteR2    = limC1(gteMAC1 >> 4);
    gteG2    = limC2(gteMAC2 >> 4);
    gteB2    = limC3(gteMAC3 >> 4);
}
void GTE::gteNCDT()
{
    int v;
    s32 vx, vy, vz;
    gteFLAG = 0;
    for (v = 0; v < 3; v++) {
        vx       = VX(v);
        vy       = VY(v);
        vz       = VZ(v);
        gteMAC1  = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
        gteMAC2  = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
        gteMAC3  = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
        gteIR1   = limB1(gteMAC1, 1);
        gteIR2   = limB2(gteMAC2, 1);
        gteIR3   = limB3(gteMAC3, 1);
        gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
        gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
        gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
        gteIR1   = limB1(gteMAC1, 1);
        gteIR2   = limB2(gteMAC2, 1);
        gteIR3   = limB3(gteMAC3, 1);
        gteMAC1  = A1(((((s64)gteR << 4) * gteIR1) + (gteIR0 * limB1(gteRFC - ((gteR * gteIR1) >> 8), 0))) >> 12);
        gteMAC2  = A2(((((s64)gteG << 4) * gteIR2) + (gteIR0 * limB2(gteGFC - ((gteG * gteIR2) >> 8), 0))) >> 12);
        gteMAC3  = A3(((((s64)gteB << 4) * gteIR3) + (gteIR0 * limB3(gteBFC - ((gteB * gteIR3) >> 8), 0))) >> 12);
        gteRGB0  = gteRGB1;
        gteRGB1  = gteRGB2;
        gteCODE2 = gteCODE;
        gteR2    = limC1(gteMAC1 >> 4);
        gteG2    = limC2(gteMAC2 >> 4);
        gteB2    = limC3(gteMAC3 >> 4);
    }
    gteIR1 = limB1(gteMAC1, 1);
    gteIR2 = limB2(gteMAC2, 1);
    gteIR3 = limB3(gteMAC3, 1);
}
void GTE::gteOP()
{
    int shift = 12 * GTE_SF(gteop);
    int lm    = GTE_LM(gteop);
    gteFLAG   = 0;
    gteMAC1   = A1(((s64)(gteR22 * gteIR3) - (gteR33 * gteIR2)) >> shift);
    gteMAC2   = A2(((s64)(gteR33 * gteIR1) - (gteR11 * gteIR3)) >> shift);
    gteMAC3   = A3(((s64)(gteR11 * gteIR2) - (gteR22 * gteIR1)) >> shift);
    gteIR1    = limB1(gteMAC1, lm);
    gteIR2    = limB2(gteMAC2, lm);
    gteIR3    = limB3(gteMAC3, lm);
}
void GTE::gteDCPL()
{
    int lm   = GTE_LM(gteop);
    s64 RIR1 = ((s64)gteR * gteIR1) >> 8;
    s64 GIR2 = ((s64)gteG * gteIR2) >> 8;
    s64 BIR3 = ((s64)gteB * gteIR3) >> 8;
    gteFLAG  = 0;
    gteMAC1  = A1(RIR1 + ((gteIR0 * limB1(gteRFC - RIR1, 0)) >> 12));
    gteMAC2  = A2(GIR2 + ((gteIR0 * limB1(gteGFC - GIR2, 0)) >> 12));
    gteMAC3  = A3(BIR3 + ((gteIR0 * limB1(gteBFC - BIR3, 0)) >> 12));
    gteIR1   = limB1(gteMAC1, lm);
    gteIR2   = limB2(gteMAC2, lm);
    gteIR3   = limB3(gteMAC3, lm);
    gteRGB0  = gteRGB1;
    gteRGB1  = gteRGB2;
    gteCODE2 = gteCODE;
    gteR2    = limC1(gteMAC1 >> 4);
    gteG2    = limC2(gteMAC2 >> 4);
    gteB2    = limC3(gteMAC3 >> 4);
}
void GTE::gteGPF()
{
    int shift = 12 * GTE_SF(gteop);
    gteFLAG   = 0;
    gteMAC1   = A1(((s64)gteIR0 * gteIR1) >> shift);
    gteMAC2   = A2(((s64)gteIR0 * gteIR2) >> shift);
    gteMAC3   = A3(((s64)gteIR0 * gteIR3) >> shift);
    gteIR1    = limB1(gteMAC1, 0);
    gteIR2    = limB2(gteMAC2, 0);
    gteIR3    = limB3(gteMAC3, 0);
    gteRGB0   = gteRGB1;
    gteRGB1   = gteRGB2;
    gteCODE2  = gteCODE;
    gteR2     = limC1(gteMAC1 >> 4);
    gteG2     = limC2(gteMAC2 >> 4);
    gteB2     = limC3(gteMAC3 >> 4);
}
void GTE::gteGPL()
{
    int shift = 12 * GTE_SF(gteop);
    gteFLAG   = 0;
    gteMAC1   = A1((((s64)gteMAC1 << shift) + (gteIR0 * gteIR1)) >> shift);
    gteMAC2   = A2((((s64)gteMAC2 << shift) + (gteIR0 * gteIR2)) >> shift);
    gteMAC3   = A3((((s64)gteMAC3 << shift) + (gteIR0 * gteIR3)) >> shift);
    gteIR1    = limB1(gteMAC1, 0);
    gteIR2    = limB2(gteMAC2, 0);
    gteIR3    = limB3(gteMAC3, 0);
    gteRGB0   = gteRGB1;
    gteRGB1   = gteRGB2;
    gteCODE2  = gteCODE;
    gteR2     = limC1(gteMAC1 >> 4);
    gteG2     = limC2(gteMAC2 >> 4);
    gteB2     = limC3(gteMAC3 >> 4);
}
void GTE::gteDPCS()
{
    int shift = 12 * GTE_SF(gteop);
    gteFLAG   = 0;
    gteMAC1   = A1(((gteR << 16) + (gteIR0 * limB1(A1((s64)gteRFC - (gteR << 4)) << (12 - shift), 0))) >> 12);
    gteMAC2   = A2(((gteG << 16) + (gteIR0 * limB2(A2((s64)gteGFC - (gteG << 4)) << (12 - shift), 0))) >> 12);
    gteMAC3   = A3(((gteB << 16) + (gteIR0 * limB3(A3((s64)gteBFC - (gteB << 4)) << (12 - shift), 0))) >> 12);
    gteIR1    = limB1(gteMAC1, 0);
    gteIR2    = limB2(gteMAC2, 0);
    gteIR3    = limB3(gteMAC3, 0);
    gteRGB0   = gteRGB1;
    gteRGB1   = gteRGB2;
    gteCODE2  = gteCODE;
    gteR2     = limC1(gteMAC1 >> 4);
    gteG2     = limC2(gteMAC2 >> 4);
    gteB2     = limC3(gteMAC3 >> 4);
}
void GTE::gteDPCT()
{
    int v;
    gteFLAG = 0;
    for (v = 0; v < 3; v++) {
        gteMAC1  = A1((((s64)gteR0 << 16) + ((s64)gteIR0 * (limB1(gteRFC - (gteR0 << 4), 0)))) >> 12);
        gteMAC2  = A2((((s64)gteG0 << 16) + ((s64)gteIR0 * (limB1(gteGFC - (gteG0 << 4), 0)))) >> 12);
        gteMAC3  = A3((((s64)gteB0 << 16) + ((s64)gteIR0 * (limB1(gteBFC - (gteB0 << 4), 0)))) >> 12);
        gteRGB0  = gteRGB1;
        gteRGB1  = gteRGB2;
        gteCODE2 = gteCODE;
        gteR2    = limC1(gteMAC1 >> 4);
        gteG2    = limC2(gteMAC2 >> 4);
        gteB2    = limC3(gteMAC3 >> 4);
    }
    gteIR1 = limB1(gteMAC1, 0);
    gteIR2 = limB2(gteMAC2, 0);
    gteIR3 = limB3(gteMAC3, 0);
}
void GTE::gteNCS()
{
    gteFLAG  = 0;
    gteMAC1  = A1((((s64)gteL11 * gteVX0) + (gteL12 * gteVY0) + (gteL13 * gteVZ0)) >> 12);
    gteMAC2  = A2((((s64)gteL21 * gteVX0) + (gteL22 * gteVY0) + (gteL23 * gteVZ0)) >> 12);
    gteMAC3  = A3((((s64)gteL31 * gteVX0) + (gteL32 * gteVY0) + (gteL33 * gteVZ0)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
    gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
    gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteRGB0  = gteRGB1;
    gteRGB1  = gteRGB2;
    gteCODE2 = gteCODE;
    gteR2    = limC1(gteMAC1 >> 4);
    gteG2    = limC2(gteMAC2 >> 4);
    gteB2    = limC3(gteMAC3 >> 4);
}
void GTE::gteNCT()
{
    int v;
    s32 vx, vy, vz;
    gteFLAG = 0;
    for (v = 0; v < 3; v++) {
        vx       = VX(v);
        vy       = VY(v);
        vz       = VZ(v);
        gteMAC1  = A1((((s64)gteL11 * vx) + (gteL12 * vy) + (gteL13 * vz)) >> 12);
        gteMAC2  = A2((((s64)gteL21 * vx) + (gteL22 * vy) + (gteL23 * vz)) >> 12);
        gteMAC3  = A3((((s64)gteL31 * vx) + (gteL32 * vy) + (gteL33 * vz)) >> 12);
        gteIR1   = limB1(gteMAC1, 1);
        gteIR2   = limB2(gteMAC2, 1);
        gteIR3   = limB3(gteMAC3, 1);
        gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
        gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
        gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
        gteRGB0  = gteRGB1;
        gteRGB1  = gteRGB2;
        gteCODE2 = gteCODE;
        gteR2    = limC1(gteMAC1 >> 4);
        gteG2    = limC2(gteMAC2 >> 4);
        gteB2    = limC3(gteMAC3 >> 4);
    }
    gteIR1 = limB1(gteMAC1, 1);
    gteIR2 = limB2(gteMAC2, 1);
    gteIR3 = limB3(gteMAC3, 1);
}
void GTE::gteCC()
{
    gteFLAG  = 0;
    gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
    gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
    gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1(((s64)gteR * gteIR1) >> 8);
    gteMAC2  = A2(((s64)gteG * gteIR2) >> 8);
    gteMAC3  = A3(((s64)gteB * gteIR3) >> 8);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteRGB0  = gteRGB1;
    gteRGB1  = gteRGB2;
    gteCODE2 = gteCODE;
    gteR2    = limC1(gteMAC1 >> 4);
    gteG2    = limC2(gteMAC2 >> 4);
    gteB2    = limC3(gteMAC3 >> 4);
}
void GTE::gteINTPL()
{
    int shift = 12 * GTE_SF(gteop);
    int lm    = GTE_LM(gteop);
    gteFLAG   = 0;
    gteMAC1   = A1(((gteIR1 << 12) + (gteIR0 * limB1(((s64)gteRFC - gteIR1), 0))) >> shift);
    gteMAC2   = A2(((gteIR2 << 12) + (gteIR0 * limB2(((s64)gteGFC - gteIR2), 0))) >> shift);
    gteMAC3   = A3(((gteIR3 << 12) + (gteIR0 * limB3(((s64)gteBFC - gteIR3), 0))) >> shift);
    gteIR1    = limB1(gteMAC1, lm);
    gteIR2    = limB2(gteMAC2, lm);
    gteIR3    = limB3(gteMAC3, lm);
    gteRGB0   = gteRGB1;
    gteRGB1   = gteRGB2;
    gteCODE2  = gteCODE;
    gteR2     = limC1(gteMAC1 >> 4);
    gteG2     = limC2(gteMAC2 >> 4);
    gteB2     = limC3(gteMAC3 >> 4);
}
void GTE::gteCDP()
{
    gteFLAG  = 0;
    gteMAC1  = A1((((s64)gteRBK << 12) + (gteLR1 * gteIR1) + (gteLR2 * gteIR2) + (gteLR3 * gteIR3)) >> 12);
    gteMAC2  = A2((((s64)gteGBK << 12) + (gteLG1 * gteIR1) + (gteLG2 * gteIR2) + (gteLG3 * gteIR3)) >> 12);
    gteMAC3  = A3((((s64)gteBBK << 12) + (gteLB1 * gteIR1) + (gteLB2 * gteIR2) + (gteLB3 * gteIR3)) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteMAC1  = A1(((((s64)gteR << 4) * gteIR1) + (gteIR0 * limB1(gteRFC - ((gteR * gteIR1) >> 8), 0))) >> 12);
    gteMAC2  = A2(((((s64)gteG << 4) * gteIR2) + (gteIR0 * limB2(gteGFC - ((gteG * gteIR2) >> 8), 0))) >> 12);
    gteMAC3  = A3(((((s64)gteB << 4) * gteIR3) + (gteIR0 * limB3(gteBFC - ((gteB * gteIR3) >> 8), 0))) >> 12);
    gteIR1   = limB1(gteMAC1, 1);
    gteIR2   = limB2(gteMAC2, 1);
    gteIR3   = limB3(gteMAC3, 1);
    gteRGB0  = gteRGB1;
    gteRGB1  = gteRGB2;
    gteCODE2 = gteCODE;
    gteR2    = limC1(gteMAC1 >> 4);
    gteG2    = limC2(gteMAC2 >> 4);
    gteB2    = limC3(gteMAC3 >> 4);
}
