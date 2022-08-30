#ifndef __R3000A_H__
#define __R3000A_H__

#include "mem.h"
#include <vector>

#define _i32(x) *(s32 *)&x
#define _u32(x) x
#define _i16(x) *(short *)&x
#define _u16(x) *(unsigned short *)&x
#define _i8(x)  *(char *)&x
#define _u8(x)  *(unsigned char *)&x

#define _Rd_            _fRd_(pcsx->psxCpu->psxRegs.code)
#define _Rt_            _fRt_(pcsx->psxCpu->psxRegs.code)
#define _Rs_            _fRs_(pcsx->psxCpu->psxRegs.code)
#define _PC_            pcsx->psxCpu->psxRegs.pc
#define _fOp_(code)     ((code >> 26))
#define _fFunct_(code)  ((code)&0x3F)
#define _fRd_(code)     ((code >> 11) & 0x1F)
#define _fRt_(code)     ((code >> 16) & 0x1F)
#define _fRs_(code)     ((code >> 21) & 0x1F)
#define _fSa_(code)     ((code >> 6) & 0x1F)
#define _fIm_(code)     ((u16)code)
#define _fTarget_(code) (code & 0x03ffffff)
#define _fImm_(code)    ((s16)code)
#define _fImmU_(code)   (code & 0xffff)
#define _oB_            (_u32(_rRs_) + _Imm_)
#define _Op_            _fOp_(pcsx->psxCpu->psxRegs.code)
#define _Funct_         _fFunct_(pcsx->psxCpu->psxRegs.code)


#define _Sa_           _fSa_(pcsx->psxCpu->psxRegs.code)
#define _Im_           _fIm_(pcsx->psxCpu->psxRegs.code)
#define _Target_       _fTarget_(pcsx->psxCpu->psxRegs.code)
#define _Imm_          _fImm_(pcsx->psxCpu->psxRegs.code)
#define _ImmU_         _fImmU_(pcsx->psxCpu->psxRegs.code)
#define _rRs_          pcsx->psxCpu->psxRegs.GPR.r[_Rs_]
#define _rRt_          pcsx->psxCpu->psxRegs.GPR.r[_Rt_]
#define _rRd_          pcsx->psxCpu->psxRegs.GPR.r[_Rd_]
#define _rSa_          pcsx->psxCpu->psxRegs.GPR.r[_Sa_]
#define _rFs_          pcsx->psxCpu->psxRegs.CP0.r[_Rd_]
#define _c2dRs_        pcsx->psxCpu->psxRegs.CP2D.r[_Rs_]
#define _c2dRt_        pcsx->psxCpu->psxRegs.CP2D.r[_Rt_]
#define _c2dRd_        pcsx->psxCpu->psxRegs.CP2D.r[_Rd_]
#define _c2dSa_        pcsx->psxCpu->psxRegs.CP2D.r[_Sa_]
#define _rHi_          pcsx->psxCpu->psxRegs.GPR.n.hi
#define _rLo_          pcsx->psxCpu->psxRegs.GPR.n.lo
#define _JumpTarget_   ((_Target_ * 4) + (_PC_ & 0xf0000000))
#define _BranchTarget_ ((s16)_Im_ * 4 + _PC_)
#define _SetLink(x)    pcsx->psxCpu->psxRegs.GPR.r[x] = _PC_ + 4;

typedef union
{
    struct
    {
        u8 l, h, h2, h3;
    } b;
    struct
    {
        u16 l, h;
    } w;
    struct
    {
        s8 l, h, h2, h3;
    } sb;
    struct
    {
        s16 l, h;
    } sw;
} PAIR;
typedef union
{
    struct
    {
        u32 r0, at, v0, v1, a0, a1, a2, a3, t0, t1, t2, t3, t4, t5, t6, t7, s0, s1, s2, s3, s4, s5, s6, s7, t8, t9, k0,
            k1, gp, sp, s8, ra, lo, hi;
    } n;
    u32  r[34];
    PAIR p[34];
} psxGPRRegs;
typedef union
{
    struct
    {
        u32 Index, Random, EntryLo0, EntryLo1, Context, PageMask, Wired, Reserved0, BadVAddr, Count, EntryHi, Compare,
            Status, Cause, EPC, PRid, Config, LLAddr, WatchLO, WatchHI, XContext, Reserved1, Reserved2, Reserved3,
            Reserved4, Reserved5, ECC, CacheErr, TagLo, TagHi, ErrorEPC, Reserved6;
    } n;
    u32  r[32];
    PAIR p[32];
} psxCP0Regs;
typedef struct
{
    short x, y;
} SVector2D;
typedef struct
{
    short z, pad;
} SVector2Dz;
typedef struct
{
    short x, y, z, pad;
} SVector3D;
typedef struct
{
    short x, y, z, pad;
} LVector3D;
typedef struct
{
    unsigned char r, g, b, c;
} CBGR;
typedef struct
{
    short m11, m12, m13, m21, m22, m23, m31, m32, m33, pad;
} SMatrix3D;
typedef union
{
    struct
    {
        SVector3D  v0, v1, v2;
        CBGR       rgb;
        s32        otz;
        s32        ir0, ir1, ir2, ir3;
        SVector2D  sxy0, sxy1, sxy2, sxyp;
        SVector2Dz sz0, sz1, sz2, sz3;
        CBGR       rgb0, rgb1, rgb2;
        s32        reserved;
        s32        mac0, mac1, mac2, mac3;
        u32        irgb, orgb;
        s32        lzcs, lzcr;
    } n;
    u32  r[32];
    PAIR p[32];
} psxCP2Data;
typedef union
{
    struct
    {
        SMatrix3D rMatrix;
        s32       trX, trY, trZ;
        SMatrix3D lMatrix;
        s32       rbk, gbk, bbk;
        SMatrix3D cMatrix;
        s32       rfc, gfc, bfc;
        s32       ofx, ofy;
        s32       h;
        s32       dqa, dqb;
        s32       zsf3, zsf4;
        s32       flag;
    } n;
    u32  r[32];
    PAIR p[32];
} psxCP2Ctrl;
typedef struct
{
    psxGPRRegs GPR;
    psxCP0Regs CP0;
    psxCP2Data CP2D;
    psxCP2Ctrl CP2C;
    u32        pc;
    u32        code;
    u32        cycle;
    u32        interrupt;
    u32        intCycle[32];
} psxRegisters;

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
} HLEEXEC;



class R3000Acpu {
  private:
    typedef void (R3000Acpu::*op_handler)(void);

  public:
    PCSX *pcsx;

    psxRegisters psxRegs;

    int branch  = 0;
    int branch2 = 0;
    u32 branchPC;

    u32 LWL_MASK[4]  = {0xffffff, 0xffff, 0xff, 0};
    u32 LWL_SHIFT[4] = {24, 16, 8, 0};
    u32 LWR_MASK[4]  = {0, 0xff000000, 0xffff0000, 0xffffff00};
    u32 LWR_SHIFT[4] = {0, 8, 16, 24};
    u32 SWL_MASK[4]  = {0xffffff00, 0xffff0000, 0xff000000, 0};
    u32 SWL_SHIFT[4] = {24, 16, 8, 0};
    u32 SWR_MASK[4]  = {0, 0xff, 0xffff, 0xffffff};
    u32 SWR_SHIFT[4] = {0, 8, 16, 24};

  public:
    R3000Acpu(PCSX *_pcsx);

    int  intInit();
    int  psxInit();
    int  psxTestLoadDelay(int reg, u32 tmp);
    void psxReset();
    void psxShutdown();
    void psxException(u32 code, u32 bd);
    void psxBranchTest();
    void psxJumpTest();
    void psxExecuteBios();
    void delayRead(int reg, u32 bpc);
    void delayWrite(int reg, u32 bpc);
    void delayReadWrite(int reg, u32 bpc);
    void psxDelayTest(int reg, u32 bpc);
    void doBranch(u32 tar);
    void psxADDI();
    void psxADDIU();
    void psxANDI();
    void psxORI();
    void psxXORI();
    void psxSLTI();
    void psxSLTIU();
    void psxADD();
    void psxADDU();
    void psxSUB();
    void psxSUBU();
    void psxAND();
    void psxOR();
    void psxXOR();
    void psxNOR();
    void psxSLT();
    void psxSLTU();
    void psxDIV();
    void psxDIVU();
    void psxMULT();
    void psxMULTU();
    void psxBGEZ();
    void psxBGEZAL();
    void psxBGTZ();
    void psxBLEZ();
    void psxBLTZ();
    void psxBLTZAL();
    void psxSLL();
    void psxSRA();
    void psxSRL();
    void psxSLLV();
    void psxSRAV();
    void psxSRLV();
    void psxLUI();
    void psxMFHI();
    void psxMFLO();
    void psxMTHI();
    void psxMTLO();
    void psxBREAK();
    void psxSYSCALL();
    void psxRFE();
    void psxBEQ();
    void psxBNE();
    void psxJ();
    void psxJAL();
    void psxJR();
    void psxJALR();
    void psxLB();
    void psxLBU();
    void psxLH();
    void psxLHU();
    void psxLW();
    void psxLWL();
    void psxLWR();
    void psxSB();
    void psxSH();
    void psxSW();
    void psxSWL();
    void psxSWR();
    void psxMFC0();
    void psxCFC0();
    void psxTestSWInts();
    void MTC0(int reg, u32 val);
    void psxMTC0();
    void psxCTC0();
    void psxNULL();
    void psxSPECIAL();
    void psxREGIMM();
    void psxCOP0();
    void psxCOP2();
    void psxBASIC();
    void psxHLE();
    void intReset();
    void execI();
    void intExecute();
    void intExecuteBlock();
    void intClear(u32 Addr, u32 Size);
    void intShutdown();

  public:
    void _gteLWC2();
    void _gteSWC2();
    void _gteRTPS();
    void _gteNCLIP();
    void _gteDPCS();
    void _gteINTPL();
    void _gteOP();
    void _gteMVMVA();
    void _gteCDP();
    void _gteNCDT();
    void _gteCC();
    void _gteDCPL();
    void _gteNCDS();
    void _gteNCS();
    void _gteNCCS();
    void _gteNCT();
    void _gteGPL();
    void _gteNCCT();
    void _gteSQR();
    void _gteDPCT();
    void _gteAVSZ3();
    void _gteAVSZ4();
    void _gteRTPT();
    void _gteGPF();
    void _gteMFC2();
    void _gteCFC2();
    void _gteMTC2();
    void _gteCTC2();

  public:    // HLE
    void hleDummy();
    void hleA0();
    void hleB0();
    void hleC0();
    void hleBootstrap();
    void hleExecRet();

  public:
    std::vector<op_handler> psxBSC = {
        &R3000Acpu::psxSPECIAL, &R3000Acpu::psxREGIMM, &R3000Acpu::psxJ,    &R3000Acpu::psxJAL,   &R3000Acpu::psxBEQ,
        &R3000Acpu::psxBNE,     &R3000Acpu::psxBLEZ,   &R3000Acpu::psxBGTZ, &R3000Acpu::psxADDI,  &R3000Acpu::psxADDIU,
        &R3000Acpu::psxSLTI,    &R3000Acpu::psxSLTIU,  &R3000Acpu::psxANDI, &R3000Acpu::psxORI,   &R3000Acpu::psxXORI,
        &R3000Acpu::psxLUI,     &R3000Acpu::psxCOP0,   &R3000Acpu::psxNULL, &R3000Acpu::psxCOP2,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,   &R3000Acpu::psxLB,   &R3000Acpu::psxLH,    &R3000Acpu::psxLWL,
        &R3000Acpu::psxLW,      &R3000Acpu::psxLBU,    &R3000Acpu::psxLHU,  &R3000Acpu::psxLWR,   &R3000Acpu::psxNULL,
        &R3000Acpu::psxSB,      &R3000Acpu::psxSH,     &R3000Acpu::psxSWL,  &R3000Acpu::psxSW,    &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,    &R3000Acpu::psxSWR,    &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::_gteLWC2,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::_gteSWC2, &R3000Acpu::psxHLE,
        &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL};

    std::vector<op_handler> psxSPC = {
        &R3000Acpu::psxSLL,   &R3000Acpu::psxNULL, &R3000Acpu::psxSRL,     &R3000Acpu::psxSRA,   &R3000Acpu::psxSLLV,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxSRLV, &R3000Acpu::psxSRAV,    &R3000Acpu::psxJR,    &R3000Acpu::psxJALR,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxSYSCALL, &R3000Acpu::psxBREAK, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxMFHI, &R3000Acpu::psxMTHI,    &R3000Acpu::psxMFLO,  &R3000Acpu::psxMTLO,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,  &R3000Acpu::psxMULT,
        &R3000Acpu::psxMULTU, &R3000Acpu::psxDIV,  &R3000Acpu::psxDIVU,    &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxADD,     &R3000Acpu::psxADDU,  &R3000Acpu::psxSUB,
        &R3000Acpu::psxSUBU,  &R3000Acpu::psxAND,  &R3000Acpu::psxOR,      &R3000Acpu::psxXOR,   &R3000Acpu::psxNOR,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxSLT,     &R3000Acpu::psxSLTU,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,    &R3000Acpu::psxNULL};


    std::vector<op_handler> psxREG = {
        &R3000Acpu::psxBLTZ, &R3000Acpu::psxBGEZ,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxBLTZAL, &R3000Acpu::psxBGEZAL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL};

    std::vector<op_handler> psxCP0 = {
        &R3000Acpu::psxMFC0, &R3000Acpu::psxNULL, &R3000Acpu::psxCFC0, &R3000Acpu::psxNULL, &R3000Acpu::psxMTC0,
        &R3000Acpu::psxNULL, &R3000Acpu::psxCTC0, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxRFE,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL, &R3000Acpu::psxNULL};


    std::vector<op_handler> psxCP2 = {
        &R3000Acpu::psxBASIC, &R3000Acpu::_gteRTPS,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,   &R3000Acpu::_gteNCLIP, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::_gteOP,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::_gteDPCS, &R3000Acpu::_gteINTPL, &R3000Acpu::_gteMVMVA, &R3000Acpu::_gteNCDS,
        &R3000Acpu::_gteCDP,  &R3000Acpu::psxNULL,   &R3000Acpu::_gteNCDT,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::_gteNCCS,
        &R3000Acpu::_gteCC,   &R3000Acpu::psxNULL,   &R3000Acpu::_gteNCS,   &R3000Acpu::psxNULL,
        &R3000Acpu::_gteNCT,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::_gteSQR,  &R3000Acpu::_gteDCPL,  &R3000Acpu::_gteDPCT,  &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::_gteAVSZ3, &R3000Acpu::_gteAVSZ4, &R3000Acpu::psxNULL,
        &R3000Acpu::_gteRTPT, &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,   &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::_gteGPF,   &R3000Acpu::_gteGPL,   &R3000Acpu::_gteNCCT};


    std::vector<op_handler> psxCP2BSC = {
        &R3000Acpu::_gteMFC2, &R3000Acpu::psxNULL,  &R3000Acpu::_gteCFC2, &R3000Acpu::psxNULL, &R3000Acpu::_gteMTC2,
        &R3000Acpu::psxNULL,  &R3000Acpu::_gteCTC2, &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL, &R3000Acpu::psxNULL,
        &R3000Acpu::psxNULL,  &R3000Acpu::psxNULL};
};
#endif