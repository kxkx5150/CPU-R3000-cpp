#include <stdio.h>
#include "r3000a.h"
#include "cdrom.h"
#include "mdec.h"
#include "gte.h"
#include "../bios/bios.h"
#include "sio.h"


#define _tFunct_ ((tmp)&0x3F)
#define _tRd_    ((tmp >> 11) & 0x1F)
#define _tRt_    ((tmp >> 16) & 0x1F)
#define _tRs_    ((tmp >> 21) & 0x1F)
#define _tSa_    ((tmp >> 6) & 0x1F)
#define RepZBranchi32(op)                                                                                              \
    if (_i32(_rRs_) op 0)                                                                                              \
        doBranch(_BranchTarget_);
#define RepZBranchLinki32(op)                                                                                          \
    if (_i32(_rRs_) op 0) {                                                                                            \
        _SetLink(31);                                                                                                  \
        doBranch(_BranchTarget_);                                                                                      \
    }
#define RepBranchi32(op)                                                                                               \
    if (_i32(_rRs_) op _i32(_rRt_))                                                                                    \
        doBranch(_BranchTarget_);


#define CALL_MEMBER_FN(object, ptrToMember) ((object).*(ptrToMember))


R3000Acpu::R3000Acpu(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
int R3000Acpu::psxInit()
{
    SysPrintf(_("Running PCSX Version %s (%s).\n"), PACKAGE_VERSION, __DATE__);
    Log = 0;
    if (pcsx->psxMem->psxMemInit() == -1)
        return -1;
    return intInit();
}
void R3000Acpu::psxReset()
{
    intReset();
    pcsx->psxMem->psxMemReset();
    memset(&psxRegs, 0, sizeof(psxRegs));
    psxRegs.pc        = 0xbfc00000;
    psxRegs.CP0.r[12] = 0x10900000;
    psxRegs.CP0.r[15] = 0x00000002;
    pcsx->psxHw->psxHwReset();
    psxBiosInit();
    if (!Config.HLE)
        psxExecuteBios();

    Log = 0;
}
void R3000Acpu::psxShutdown()
{
    pcsx->psxMem->psxMemShutdown();
    psxBiosShutdown();
    intShutdown();
}
void R3000Acpu::psxException(u32 code, u32 bd)
{
    psxRegs.CP0.n.Cause = code;
    if (bd) {
        SysPrintf("bd set!!!\n");
        psxRegs.CP0.n.Cause |= 0x80000000;
        psxRegs.CP0.n.EPC = (psxRegs.pc - 4);
    } else
        psxRegs.CP0.n.EPC = (psxRegs.pc);
    if (psxRegs.CP0.n.Status & 0x400000)
        psxRegs.pc = 0xbfc00180;
    else
        psxRegs.pc = 0x80000080;
    psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status & ~0x3f) | ((psxRegs.CP0.n.Status & 0xf) << 2);
    if (!Config.HLE && (((PSXMu32(psxRegs.CP0.n.EPC) >> 24) & 0xfe) == 0x4a)) {
        PSXMu32ref(psxRegs.CP0.n.EPC) &= SWAPu32(~0x02000000);
    }
    if (Config.HLE)
        psxBiosException();
}
void R3000Acpu::psxBranchTest()
{
    if ((psxRegs.cycle - pcsx->psxCntr->psxNextsCounter) >= pcsx->psxCntr->psxNextCounter)
        pcsx->psxCntr->psxRcntUpdate();
    if (psxRegs.interrupt) {
        if ((psxRegs.interrupt & 0x80) && !Config.Sio) {
            if ((psxRegs.cycle - psxRegs.intCycle[7]) >= psxRegs.intCycle[7 + 1]) {
                psxRegs.interrupt &= ~0x80;
                pcsx->psxSio->sioInterrupt();
            }
        }
        if (psxRegs.interrupt & 0x04) {
            if ((psxRegs.cycle - psxRegs.intCycle[2]) >= psxRegs.intCycle[2 + 1]) {
                psxRegs.interrupt &= ~0x04;
                pcsx->psxCdrom->cdrInterrupt();
            }
        }
        if (psxRegs.interrupt & 0x040000) {
            if ((psxRegs.cycle - psxRegs.intCycle[2 + 16]) >= psxRegs.intCycle[2 + 16 + 1]) {
                psxRegs.interrupt &= ~0x040000;
                pcsx->psxCdrom->cdrReadInterrupt();
            }
        }
        if (psxRegs.interrupt & 0x01000000) {
            if ((psxRegs.cycle - psxRegs.intCycle[3 + 24]) >= psxRegs.intCycle[3 + 24 + 1]) {
                psxRegs.interrupt &= ~0x01000000;
                pcsx->psxDma->gpuInterrupt();
            }
        }
        if (psxRegs.interrupt & 0x02000000) {
            if ((psxRegs.cycle - psxRegs.intCycle[5 + 24]) >= psxRegs.intCycle[5 + 24 + 1]) {
                psxRegs.interrupt &= ~0x02000000;
                pcsx->psxMdec->mdec1Interrupt();
            }
        }
        if (psxRegs.interrupt & 0x04000000) {
            if ((psxRegs.cycle - psxRegs.intCycle[1 + 24]) >= psxRegs.intCycle[1 + 24 + 1]) {
                psxRegs.interrupt &= ~0x04000000;
                pcsx->psxDma->spuInterrupt();
            }
        }
    }
    if (psxHu32(0x1070) & psxHu32(0x1074)) {
        if ((psxRegs.CP0.n.Status & 0x401) == 0x401) {
            psxException(0x400, 0);
        }
    }
}
void R3000Acpu::psxJumpTest()
{
    if (!Config.HLE && Config.PsxOut) {
        u32 call = psxRegs.GPR.n.t1 & 0xff;
        switch (psxRegs.pc & 0x1fffff) {
            case 0xa0:
                if (biosA0[call])
                    biosA0[call]();
                break;
            case 0xb0:
                if (biosB0[call])
                    biosB0[call]();
                break;
            case 0xc0:
                if (biosC0[call])
                    biosC0[call]();
                break;
        }
    }
}
void R3000Acpu::psxExecuteBios()
{
    while (psxRegs.pc != 0x80030000)
        intExecuteBlock();
}
void R3000Acpu::delayRead(int reg, u32 bpc)
{
    u32 rold, rnew;
    rold = psxRegs.GPR.r[reg];
    CALL_MEMBER_FN(*this, psxBSC[psxRegs.code >> 26])();
    ;

    rnew       = psxRegs.GPR.r[reg];
    psxRegs.pc = bpc;
    psxBranchTest();
    psxRegs.GPR.r[reg] = rold;
    execI();
    psxRegs.GPR.r[reg] = rnew;
    branch             = 0;
}
void R3000Acpu::delayWrite(int reg, u32 bpc)
{
    CALL_MEMBER_FN(*this, psxBSC[psxRegs.code >> 26])();
    ;
    branch     = 0;
    psxRegs.pc = bpc;
    psxBranchTest();
}
void R3000Acpu::delayReadWrite(int reg, u32 bpc)
{
    branch     = 0;
    psxRegs.pc = bpc;
    psxBranchTest();
}
int R3000Acpu::psxTestLoadDelay(int reg, u32 tmp)
{
    if (tmp == 0)
        return 0;
    switch (tmp >> 26) {
        case 0x00:
            switch (_tFunct_) {
                case 0x00:
                case 0x02:
                case 0x03:
                    if (_tRd_ == reg && _tRt_ == reg)
                        return 1;
                    else if (_tRt_ == reg)
                        return 2;
                    else if (_tRd_ == reg)
                        return 3;
                    break;
                case 0x08:
                    if (_tRs_ == reg)
                        return 2;
                    break;
                case 0x09:
                    if (_tRd_ == reg && _tRs_ == reg)
                        return 1;
                    else if (_tRs_ == reg)
                        return 2;
                    else if (_tRd_ == reg)
                        return 3;
                    break;
                case 0x20:
                case 0x21:
                case 0x22:
                case 0x23:
                case 0x24:
                case 0x25:
                case 0x26:
                case 0x27:
                case 0x2a:
                case 0x2b:
                case 0x04:
                case 0x06:
                case 0x07:
                    if (_tRd_ == reg && (_tRt_ == reg || _tRs_ == reg))
                        return 1;
                    else if (_tRt_ == reg || _tRs_ == reg)
                        return 2;
                    else if (_tRd_ == reg)
                        return 3;
                    break;
                case 0x10:
                case 0x12:
                    if (_tRd_ == reg)
                        return 3;
                    break;
                case 0x11:
                case 0x13:
                    if (_tRs_ == reg)
                        return 2;
                    break;
                case 0x18:
                case 0x19:
                case 0x1a:
                case 0x1b:
                    if (_tRt_ == reg || _tRs_ == reg)
                        return 2;
                    break;
            }
            break;
        case 0x01:
            switch (_tRt_) {
                case 0x00:
                case 0x02:
                case 0x10:
                case 0x12:
                    if (_tRs_ == reg)
                        return 2;
                    break;
            }
            break;
        case 0x03:
            if (31 == reg)
                return 3;
            break;
        case 0x04:
        case 0x05:
            if (_tRs_ == reg || _tRt_ == reg)
                return 2;
            break;
        case 0x06:
        case 0x07:
            if (_tRs_ == reg)
                return 2;
            break;
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0c:
        case 0x0d:
        case 0x0e:
            if (_tRt_ == reg && _tRs_ == reg)
                return 1;
            else if (_tRs_ == reg)
                return 2;
            else if (_tRt_ == reg)
                return 3;
            break;
        case 0x0f:
            if (_tRt_ == reg)
                return 3;
            break;
        case 0x10:
            switch (_tFunct_) {
                case 0x00:
                    if (_tRt_ == reg)
                        return 3;
                    break;
                case 0x02:
                    if (_tRt_ == reg)
                        return 3;
                    break;
                case 0x04:
                    if (_tRt_ == reg)
                        return 2;
                    break;
                case 0x06:
                    if (_tRt_ == reg)
                        return 2;
                    break;
            }
            break;
        case 0x12:
            switch (_tFunct_) {
                case 0x00:
                    switch (_tRs_) {
                        case 0x00:
                            if (_tRt_ == reg)
                                return 3;
                            break;
                        case 0x02:
                            if (_tRt_ == reg)
                                return 3;
                            break;
                        case 0x04:
                            if (_tRt_ == reg)
                                return 2;
                            break;
                        case 0x06:
                            if (_tRt_ == reg)
                                return 2;
                            break;
                    }
                    break;
            }
            break;
        case 0x22:
        case 0x26:
            if (_tRt_ == reg)
                return 3;
            else if (_tRs_ == reg)
                return 2;
            break;
        case 0x20:
        case 0x21:
        case 0x23:
        case 0x24:
        case 0x25:
            if (_tRt_ == reg && _tRs_ == reg)
                return 1;
            else if (_tRs_ == reg)
                return 2;
            else if (_tRt_ == reg)
                return 3;
            break;
        case 0x28:
        case 0x29:
        case 0x2a:
        case 0x2b:
        case 0x2e:
            if (_tRt_ == reg || _tRs_ == reg)
                return 2;
            break;
        case 0x32:
        case 0x3a:
            if (_tRs_ == reg)
                return 2;
            break;
    }
    return 0;
}
void R3000Acpu::psxDelayTest(int reg, u32 bpc)
{
    u32 *code;
    u32  tmp;
    code   = (u32 *)PSXM(bpc);
    tmp    = ((code == NULL) ? 0 : SWAP32(*code));
    branch = 1;
    switch (psxTestLoadDelay(reg, tmp)) {
        case 1:
            delayReadWrite(reg, bpc);
            return;
        case 2:
            delayRead(reg, bpc);
            return;
        case 3:
            delayWrite(reg, bpc);
            return;
    }
    CALL_MEMBER_FN(*this, psxBSC[psxRegs.code >> 26])();
    ;
    branch     = 0;
    psxRegs.pc = bpc;
    psxBranchTest();
}
void R3000Acpu::doBranch(u32 tar)
{
    u32 *code;
    u32  tmp;
    branch2 = branch = 1;
    branchPC         = tar;
    code             = (u32 *)PSXM(psxRegs.pc);
    psxRegs.code     = ((code == NULL) ? 0 : SWAP32(*code));
    psxRegs.pc += 4;
    psxRegs.cycle += BIAS;
    tmp = psxRegs.code >> 26;
    switch (tmp) {
        case 0x10:
            switch (_Rs_) {
                case 0x00:
                case 0x02:
                    psxDelayTest(_Rt_, branchPC);
                    return;
            }
            break;
        case 0x12:
            switch (_Funct_) {
                case 0x00:
                    switch (_Rs_) {
                        case 0x00:
                        case 0x02:
                            psxDelayTest(_Rt_, branchPC);
                            return;
                    }
                    break;
            }
            break;
        case 0x32:
            psxDelayTest(_Rt_, branchPC);
            return;
        default:
            if (tmp >= 0x20 && tmp <= 0x26) {
                psxDelayTest(_Rt_, branchPC);
                return;
            }
            break;
    }
    CALL_MEMBER_FN(*this, psxBSC[psxRegs.code >> 26])();
    ;
    branch     = 0;
    psxRegs.pc = branchPC;
    psxBranchTest();
}
void R3000Acpu::psxADDI()
{
    if (!_Rt_)
        return;
    _rRt_ = _u32(_rRs_) + _Imm_;
}
void R3000Acpu::psxADDIU()
{
    if (!_Rt_)
        return;
    _rRt_ = _u32(_rRs_) + _Imm_;
}
void R3000Acpu::psxANDI()
{
    if (!_Rt_)
        return;
    _rRt_ = _u32(_rRs_) & _ImmU_;
}
void R3000Acpu::psxORI()
{
    if (!_Rt_)
        return;
    _rRt_ = _u32(_rRs_) | _ImmU_;
}
void R3000Acpu::psxXORI()
{
    if (!_Rt_)
        return;
    _rRt_ = _u32(_rRs_) ^ _ImmU_;
}
void R3000Acpu::psxSLTI()
{
    if (!_Rt_)
        return;
    _rRt_ = _i32(_rRs_) < _Imm_;
}
void R3000Acpu::psxSLTIU()
{
    if (!_Rt_)
        return;
    _rRt_ = _u32(_rRs_) < ((u32)_Imm_);
}
void R3000Acpu::psxADD()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) + _u32(_rRt_);
}
void R3000Acpu::psxADDU()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) + _u32(_rRt_);
}
void R3000Acpu::psxSUB()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) - _u32(_rRt_);
}
void R3000Acpu::psxSUBU()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) - _u32(_rRt_);
}
void R3000Acpu::psxAND()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) & _u32(_rRt_);
}
void R3000Acpu::psxOR()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) | _u32(_rRt_);
}
void R3000Acpu::psxXOR()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) ^ _u32(_rRt_);
}
void R3000Acpu::psxNOR()
{
    if (!_Rd_)
        return;
    _rRd_ = ~(_u32(_rRs_) | _u32(_rRt_));
}
void R3000Acpu::psxSLT()
{
    if (!_Rd_)
        return;
    _rRd_ = _i32(_rRs_) < _i32(_rRt_);
}
void R3000Acpu::psxSLTU()
{
    if (!_Rd_)
        return;
    _rRd_ = _u32(_rRs_) < _u32(_rRt_);
}
void R3000Acpu::psxDIV()
{
    if (_i32(_rRt_) != 0) {
        _i32(_rLo_) = _i32(_rRs_) / _i32(_rRt_);
        _i32(_rHi_) = _i32(_rRs_) % _i32(_rRt_);
    }
}
void R3000Acpu::psxDIVU()
{
    if (_rRt_ != 0) {
        _rLo_ = _rRs_ / _rRt_;
        _rHi_ = _rRs_ % _rRt_;
    }
}
void R3000Acpu::psxMULT()
{
    u64 res          = (s64)((s64)_i32(_rRs_) * (s64)_i32(_rRt_));
    psxRegs.GPR.n.lo = (u32)(res & 0xffffffff);
    psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}
void R3000Acpu::psxMULTU()
{
    u64 res          = (u64)((u64)_u32(_rRs_) * (u64)_u32(_rRt_));
    psxRegs.GPR.n.lo = (u32)(res & 0xffffffff);
    psxRegs.GPR.n.hi = (u32)((res >> 32) & 0xffffffff);
}
void R3000Acpu::psxBGEZ()
{
    RepZBranchi32(>=)
}
void R3000Acpu::psxBGEZAL()
{
    RepZBranchLinki32(>=)
}
void R3000Acpu::psxBGTZ()
{
    RepZBranchi32(>)
}
void R3000Acpu::psxBLEZ()
{
    RepZBranchi32(<=)
}
void R3000Acpu::psxBLTZ()
{
    RepZBranchi32(<)
}
void R3000Acpu::psxBLTZAL()
{
    RepZBranchLinki32(<)
}
void R3000Acpu::psxSLL()
{
    if (!_Rd_)
        return;
    _u32(_rRd_) = _u32(_rRt_) << _Sa_;
}
void R3000Acpu::psxSRA()
{
    if (!_Rd_)
        return;
    _i32(_rRd_) = _i32(_rRt_) >> _Sa_;
}
void R3000Acpu::psxSRL()
{
    if (!_Rd_)
        return;
    _u32(_rRd_) = _u32(_rRt_) >> _Sa_;
}
void R3000Acpu::psxSLLV()
{
    if (!_Rd_)
        return;
    _u32(_rRd_) = _u32(_rRt_) << _u32(_rRs_);
}
void R3000Acpu::psxSRAV()
{
    if (!_Rd_)
        return;
    _i32(_rRd_) = _i32(_rRt_) >> _u32(_rRs_);
}
void R3000Acpu::psxSRLV()
{
    if (!_Rd_)
        return;
    _u32(_rRd_) = _u32(_rRt_) >> _u32(_rRs_);
}
void R3000Acpu::psxLUI()
{
    if (!_Rt_)
        return;
    _u32(_rRt_) = psxRegs.code << 16;
}
void R3000Acpu::psxMFHI()
{
    if (!_Rd_)
        return;
    _rRd_ = _rHi_;
}
void R3000Acpu::psxMFLO()
{
    if (!_Rd_)
        return;
    _rRd_ = _rLo_;
}
void R3000Acpu::psxMTHI()
{
    _rHi_ = _rRs_;
}
void R3000Acpu::psxMTLO()
{
    _rLo_ = _rRs_;
}
void R3000Acpu::psxBREAK()
{
}
void R3000Acpu::psxSYSCALL()
{
    psxRegs.pc -= 4;
    psxException(0x20, branch);
}
void R3000Acpu::psxRFE()
{
    psxRegs.CP0.n.Status = (psxRegs.CP0.n.Status & 0xfffffff0) | ((psxRegs.CP0.n.Status & 0x3c) >> 2);
}
void R3000Acpu::psxBEQ()
{
    RepBranchi32(==)
}
void R3000Acpu::psxBNE()
{
    RepBranchi32(!=)
}
void R3000Acpu::psxJ()
{
    doBranch(_JumpTarget_);
}
void R3000Acpu::psxJAL()
{
    _SetLink(31);
    doBranch(_JumpTarget_);
}
void R3000Acpu::psxJR()
{
    doBranch(_u32(_rRs_));
    psxJumpTest();
}
void R3000Acpu::psxJALR()
{
    u32 temp = _u32(_rRs_);
    if (_Rd_) {
        _SetLink(_Rd_);
    }
    doBranch(temp);
}
void R3000Acpu::psxLB()
{
    if (_Rt_) {
        _i32(_rRt_) = (signed char)pcsx->psxMem->psxMemRead8(_oB_);
    } else {
        pcsx->psxMem->psxMemRead8(_oB_);
    }
}
void R3000Acpu::psxLBU()
{
    if (_Rt_) {
        _u32(_rRt_) = pcsx->psxMem->psxMemRead8(_oB_);
    } else {
        pcsx->psxMem->psxMemRead8(_oB_);
    }
}
void R3000Acpu::psxLH()
{
    if (_Rt_) {
        _i32(_rRt_) = (short)pcsx->psxMem->psxMemRead16(_oB_);
    } else {
        pcsx->psxMem->psxMemRead16(_oB_);
    }
}
void R3000Acpu::psxLHU()
{
    if (_Rt_) {
        _u32(_rRt_) = pcsx->psxMem->psxMemRead16(_oB_);
    } else {
        pcsx->psxMem->psxMemRead16(_oB_);
    }
}
void R3000Acpu::psxLW()
{
    if (_Rt_) {
        _u32(_rRt_) = pcsx->psxMem->psxMemRead32(_oB_);
    } else {
        pcsx->psxMem->psxMemRead32(_oB_);
    }
}
void R3000Acpu::psxLWL()
{
    u32 addr  = _oB_;
    u32 shift = addr & 3;
    u32 mem   = pcsx->psxMem->psxMemRead32(addr & ~3);
    if (!_Rt_)
        return;
    _u32(_rRt_) = (_u32(_rRt_) & LWL_MASK[shift]) | (mem << LWL_SHIFT[shift]);
}
void R3000Acpu::psxLWR()
{
    u32 addr  = _oB_;
    u32 shift = addr & 3;
    u32 mem   = pcsx->psxMem->psxMemRead32(addr & ~3);
    if (!_Rt_)
        return;
    _u32(_rRt_) = (_u32(_rRt_) & LWR_MASK[shift]) | (mem >> LWR_SHIFT[shift]);
}
void R3000Acpu::psxSB()
{
    pcsx->psxMem->psxMemWrite8(_oB_, _u8(_rRt_));
}
void R3000Acpu::psxSH()
{
    pcsx->psxMem->psxMemWrite16(_oB_, _u16(_rRt_));
}
void R3000Acpu::psxSW()
{
    pcsx->psxMem->psxMemWrite32(_oB_, _u32(_rRt_));
}
void R3000Acpu::psxSWL()
{
    u32 addr  = _oB_;
    u32 shift = addr & 3;
    u32 mem   = pcsx->psxMem->psxMemRead32(addr & ~3);
    pcsx->psxMem->psxMemWrite32(addr & ~3, (_u32(_rRt_) >> SWL_SHIFT[shift]) | (mem & SWL_MASK[shift]));
}
void R3000Acpu::psxSWR()
{
    u32 addr  = _oB_;
    u32 shift = addr & 3;
    u32 mem   = pcsx->psxMem->psxMemRead32(addr & ~3);
    pcsx->psxMem->psxMemWrite32(addr & ~3, (_u32(_rRt_) << SWR_SHIFT[shift]) | (mem & SWR_MASK[shift]));
}
void R3000Acpu::psxMFC0()
{
    if (!_Rt_)
        return;
    _i32(_rRt_) = (int)_rFs_;
}
void R3000Acpu::psxCFC0()
{
    if (!_Rt_)
        return;
    _i32(_rRt_) = (int)_rFs_;
}
void R3000Acpu::psxTestSWInts()
{
    if (psxRegs.CP0.n.Cause & psxRegs.CP0.n.Status & 0x0300 && psxRegs.CP0.n.Status & 0x1) {
        psxException(psxRegs.CP0.n.Cause, branch);
    }
}
void R3000Acpu::MTC0(int reg, u32 val)
{
    switch (reg) {
        case 12:
            psxRegs.CP0.r[12] = val;
            psxTestSWInts();
            break;
        case 13:
            psxRegs.CP0.n.Cause = val & ~(0xfc00);
            psxTestSWInts();
            break;
        default:
            psxRegs.CP0.r[reg] = val;
            break;
    }
}
void R3000Acpu::psxMTC0()
{
    MTC0(_Rd_, _u32(_rRt_));
}
void R3000Acpu::psxCTC0()
{
    MTC0(_Rd_, _u32(_rRt_));
}
void R3000Acpu::psxNULL()
{
}
void R3000Acpu::psxSPECIAL()
{
    CALL_MEMBER_FN(*this, psxSPC[_Funct_])();
}
void R3000Acpu::psxREGIMM()
{
    CALL_MEMBER_FN(*this, psxREG[_Rt_])();
}
void R3000Acpu::psxCOP0()
{
    CALL_MEMBER_FN(*this, psxCP0[_Rs_])();
}
void R3000Acpu::psxCOP2()
{
    CALL_MEMBER_FN(*this, psxCP2[_Funct_])();
}
void R3000Acpu::psxBASIC()
{
    CALL_MEMBER_FN(*this, psxCP2BSC[_Rs_])();
}
int R3000Acpu::intInit()
{
    return 0;
}
void R3000Acpu::intReset()
{
}
void R3000Acpu::execI()
{
    u32 *code    = (u32 *)PSXM(psxRegs.pc);
    psxRegs.code = ((code == NULL) ? 0 : SWAP32(*code));
    psxRegs.pc += 4;
    psxRegs.cycle += BIAS;
    CALL_MEMBER_FN(*this, psxBSC[psxRegs.code >> 26])();
    ;
}
void R3000Acpu::intExecute()
{
    while (1) {
        execI();
    }
}
void R3000Acpu::intExecuteBlock()
{
    branch2 = 0;
    while (!branch2)
        execI();
}
void R3000Acpu::intClear(u32 Addr, u32 Size)
{
}
void R3000Acpu::intShutdown()
{
}
void R3000Acpu::_gteLWC2()
{
    pcsx->psxGte->gteLWC2();
}
void R3000Acpu::_gteSWC2()
{
    pcsx->psxGte->gteSWC2();
}
void R3000Acpu::_gteRTPS()
{
    pcsx->psxGte->gteRTPS();
}
void R3000Acpu::_gteNCLIP()
{
    pcsx->psxGte->gteNCLIP();
}
void R3000Acpu::_gteDPCS()
{
    pcsx->psxGte->gteDPCS();
}
void R3000Acpu::_gteINTPL()
{
    pcsx->psxGte->gteINTPL();
}
void R3000Acpu::_gteOP()
{
    pcsx->psxGte->gteOP();
}
void R3000Acpu::_gteMVMVA()
{
    pcsx->psxGte->gteMVMVA();
}
void R3000Acpu::_gteCDP()
{
    pcsx->psxGte->gteCDP();
}
void R3000Acpu::_gteNCDT()
{
    pcsx->psxGte->gteNCDT();
}
void R3000Acpu::_gteCC()
{
    pcsx->psxGte->gteCC();
}
void R3000Acpu::_gteDCPL()
{
    pcsx->psxGte->gteDCPL();
}
void R3000Acpu::_gteNCDS()
{
    pcsx->psxGte->gteNCDS();
}
void R3000Acpu::_gteNCS()
{
    pcsx->psxGte->gteNCS();
}
void R3000Acpu::_gteNCCS()
{
    pcsx->psxGte->gteNCCS();
}
void R3000Acpu::_gteNCT()
{
    pcsx->psxGte->gteNCT();
}
void R3000Acpu::_gteGPL()
{
    pcsx->psxGte->gteGPL();
}
void R3000Acpu::_gteNCCT()
{
    pcsx->psxGte->gteNCCT();
}
void R3000Acpu::_gteSQR()
{
    pcsx->psxGte->gteSQR();
}
void R3000Acpu::_gteDPCT()
{
    pcsx->psxGte->gteDPCT();
}
void R3000Acpu::_gteAVSZ3()
{
    pcsx->psxGte->gteAVSZ3();
}
void R3000Acpu::_gteAVSZ4()
{
    pcsx->psxGte->gteAVSZ4();
}
void R3000Acpu::_gteRTPT()
{
    pcsx->psxGte->gteRTPT();
}
void R3000Acpu::_gteGPF()
{
    pcsx->psxGte->gteGPF();
}
void R3000Acpu::_gteMFC2()
{
    pcsx->psxGte->gteMFC2();
}
void R3000Acpu::_gteCFC2()
{
    pcsx->psxGte->gteCFC2();
}
void R3000Acpu::_gteMTC2()
{
    pcsx->psxGte->gteMTC2();
}
void R3000Acpu::_gteCTC2()
{
    pcsx->psxGte->gteCTC2();
}
// HLE
void R3000Acpu::psxHLE()
{
    int idx = psxRegs.code & 0x07;
    switch (idx) {
        case 0:
            hleDummy();
            break;
        case 1:
            hleA0();
            break;
        case 2:
            hleB0();
            break;
        case 3:
            hleC0();
            break;
        case 4:
            hleBootstrap();
            break;
        case 5:
            hleExecRet();
            break;
        case 6:
            hleDummy();
            break;
        case 7:
            hleDummy();
            break;
    }
}
void R3000Acpu::hleDummy()
{
    pcsx->psxCpu->psxRegs.pc = pcsx->psxCpu->psxRegs.GPR.n.ra;
    pcsx->psxCpu->psxBranchTest();
}
void R3000Acpu::hleA0()
{
    u32 call = pcsx->psxCpu->psxRegs.GPR.n.t1 & 0xff;
    if (biosA0[call])
        biosA0[call]();
    pcsx->psxCpu->psxBranchTest();
}
void R3000Acpu::hleB0()
{
    u32 call = pcsx->psxCpu->psxRegs.GPR.n.t1 & 0xff;
    if (biosB0[call])
        biosB0[call]();
    pcsx->psxCpu->psxBranchTest();
}
void R3000Acpu::hleC0()
{
    u32 call = pcsx->psxCpu->psxRegs.GPR.n.t1 & 0xff;
    if (biosC0[call])
        biosC0[call]();
    pcsx->psxCpu->psxBranchTest();
}
void R3000Acpu::hleBootstrap()
{
    SysPrintf("hleBootstrap\n");
    CheckCdrom();
    LoadCdrom();
    SysPrintf("CdromLabel: \"%s\": PC = %8.8x (SP = %8.8x)\n", CdromLabel, pcsx->psxCpu->psxRegs.pc,
              pcsx->psxCpu->psxRegs.GPR.n.sp);
}
void R3000Acpu::hleExecRet()
{
    HLEEXEC *header = (HLEEXEC *)PSXM(pcsx->psxCpu->psxRegs.GPR.n.s0);
    SysPrintf("ExecRet %x: %x\n", pcsx->psxCpu->psxRegs.GPR.n.s0, header->ret);
    pcsx->psxCpu->psxRegs.GPR.n.ra = header->ret;
    pcsx->psxCpu->psxRegs.GPR.n.sp = header->_sp;
    pcsx->psxCpu->psxRegs.GPR.n.s8 = header->_fp;
    pcsx->psxCpu->psxRegs.GPR.n.gp = header->_gp;
    pcsx->psxCpu->psxRegs.GPR.n.s0 = header->base;
    pcsx->psxCpu->psxRegs.GPR.n.v0 = 1;
    pcsx->psxCpu->psxRegs.pc       = pcsx->psxCpu->psxRegs.GPR.n.ra;
}
