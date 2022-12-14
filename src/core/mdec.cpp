#include "mdec.h"

#define SCALE(x, n)        ((x) >> (n))
#define SCALER(x, n)       (((x) + ((1 << (n)) >> 1)) >> (n))
#define AAN_CONST_BITS     12
#define AAN_PRESCALE_BITS  16
#define AAN_CONST_SIZE     24
#define AAN_CONST_SCALE    (AAN_CONST_SIZE - AAN_CONST_BITS)
#define AAN_PRESCALE_SIZE  20
#define AAN_PRESCALE_SCALE (AAN_PRESCALE_SIZE - AAN_PRESCALE_BITS)
#define AAN_EXTRA          12
#define FIX_1_082392200    SCALER(18159528, AAN_CONST_SCALE)
#define FIX_1_414213562    SCALER(23726566, AAN_CONST_SCALE)
#define FIX_1_847759065    SCALER(31000253, AAN_CONST_SCALE)
#define FIX_2_613125930    SCALER(43840978, AAN_CONST_SCALE)
#define MULS(var, const)   (SCALE((var) * (const), AAN_CONST_BITS))
#define RLE_RUN(a)         ((a) >> 10)
#define RLE_VAL(a)         (((int)(a) << (sizeof(int) * 8 - 10)) >> (sizeof(int) * 8 - 10))

#define MDEC0_STP       0x02000000
#define MDEC0_RGB24     0x08000000
#define MDEC0_SIZE_MASK 0xFFFF
#define MDEC1_BUSY      0x20000000
#define MDEC1_DREQ      0x18000000
#define MDEC1_FIFO      0xc0000000
#define MDEC1_RGB24     0x02000000
#define MDEC1_STP       0x00800000
#define MDEC1_RESET     0x80000000

#define MULR(a)               ((1434 * (a)))
#define MULB(a)               ((1807 * (a)))
#define MULG2(a, b)           ((-351 * (a)-728 * (b)))
#define MULY(a)               ((a) << 10)
#define MAKERGB15(r, g, b, a) (SWAP16(a | ((b) << 10) | ((g) << 5) | (r)))
#define SCALE8(c)             SCALER(c, 20)
#define SCALE5(c)             SCALER(c, 23)
#define CLAMP5(c)             (((c) < -16) ? 0 : (((c) > (31 - 16)) ? 31 : ((c) + 16)))
#define CLAMP8(c)             (((c) < -128) ? 0 : (((c) > (255 - 128)) ? 255 : ((c) + 128)))
#define CLAMP_SCALE8(a)       (CLAMP8(SCALE8(a)))
#define CLAMP_SCALE5(a)       (CLAMP5(SCALE5(a)))
#define MDEC_END_OF_DATA      0xfe00


MDEC::MDEC(PCSX *_pcsx)
{
    pcsx = _pcsx;
}
void MDEC::fillcol(int *blk, int val)
{
    blk[0 * DSIZE] = blk[1 * DSIZE] = blk[2 * DSIZE] = blk[3 * DSIZE] = blk[4 * DSIZE] = blk[5 * DSIZE] =
        blk[6 * DSIZE] = blk[7 * DSIZE] = val;
}
void MDEC::fillrow(int *blk, int val)
{
    blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = val;
}
void MDEC::idct(int *block, int used_col)
{
    int  tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int  z5, z10, z11, z12, z13;
    int *ptr;
    int  i;
    if (used_col == -1) {
        int v = block[0];
        for (i = 0; i < DSIZE2; i++)
            block[i] = v;
        return;
    }

    ptr = block;

    for (i = 0; i < DSIZE; i++, ptr++) {
        if ((used_col & (1 << i)) == 0) {
            if (ptr[DSIZE * 0]) {
                fillcol(ptr, ptr[0]);
                used_col |= (1 << i);
            }
            continue;
        }

        z10  = ptr[DSIZE * 0] + ptr[DSIZE * 4];
        z11  = ptr[DSIZE * 0] - ptr[DSIZE * 4];
        z13  = ptr[DSIZE * 2] + ptr[DSIZE * 6];
        z12  = MULS(ptr[DSIZE * 2] - ptr[DSIZE * 6], FIX_1_414213562) - z13;
        tmp0 = z10 + z13;
        tmp3 = z10 - z13;
        tmp1 = z11 + z12;
        tmp2 = z11 - z12;
        z13  = ptr[DSIZE * 3] + ptr[DSIZE * 5];
        z10  = ptr[DSIZE * 3] - ptr[DSIZE * 5];
        z11  = ptr[DSIZE * 1] + ptr[DSIZE * 7];
        z12  = ptr[DSIZE * 1] - ptr[DSIZE * 7];
        tmp7 = z11 + z13;
        z5   = (z12 - z10) * (FIX_1_847759065);
        tmp6 = SCALE(z10 * (FIX_2_613125930) + z5, AAN_CONST_BITS) - tmp7;
        tmp5 = MULS(z11 - z13, FIX_1_414213562) - tmp6;
        tmp4 = SCALE(z12 * (FIX_1_082392200)-z5, AAN_CONST_BITS) + tmp5;

        ptr[DSIZE * 0] = (tmp0 + tmp7);
        ptr[DSIZE * 7] = (tmp0 - tmp7);
        ptr[DSIZE * 1] = (tmp1 + tmp6);
        ptr[DSIZE * 6] = (tmp1 - tmp6);
        ptr[DSIZE * 2] = (tmp2 + tmp5);
        ptr[DSIZE * 5] = (tmp2 - tmp5);
        ptr[DSIZE * 4] = (tmp3 + tmp4);
        ptr[DSIZE * 3] = (tmp3 - tmp4);
    }

    ptr = block;
    if (used_col == 1) {
        for (i = 0; i < DSIZE; i++)
            fillrow(block + DSIZE * i, block[DSIZE * i]);

    } else {
        for (i = 0; i < DSIZE; i++, ptr += DSIZE) {
            z10 = ptr[0] + ptr[4];
            z11 = ptr[0] - ptr[4];
            z13 = ptr[2] + ptr[6];

            z12 = MULS(ptr[2] - ptr[6], FIX_1_414213562) - z13;

            tmp0 = z10 + z13;
            tmp3 = z10 - z13;
            tmp1 = z11 + z12;
            tmp2 = z11 - z12;

            z13 = ptr[3] + ptr[5];
            z10 = ptr[3] - ptr[5];
            z11 = ptr[1] + ptr[7];
            z12 = ptr[1] - ptr[7];

            tmp7 = z11 + z13;
            z5   = (z12 - z10) * FIX_1_847759065;
            tmp6 = SCALE(z10 * FIX_2_613125930 + z5, AAN_CONST_BITS) - tmp7;
            tmp5 = MULS(z11 - z13, FIX_1_414213562) - tmp6;
            tmp4 = SCALE(z12 * FIX_1_082392200 - z5, AAN_CONST_BITS) + tmp5;

            ptr[0] = tmp0 + tmp7;
            ptr[7] = tmp0 - tmp7;
            ptr[1] = tmp1 + tmp6;
            ptr[6] = tmp1 - tmp6;
            ptr[2] = tmp2 + tmp5;
            ptr[5] = tmp2 - tmp5;
            ptr[4] = tmp3 + tmp4;
            ptr[3] = tmp3 - tmp4;
        }
    }
}
void MDEC::iqtab_init(int *iqtab, unsigned char *iq_y)
{
    int i;
    for (i = 0; i < DSIZE2; i++) {
        iqtab[i] = (iq_y[i] * SCALER(aanscales[zscan[i]], AAN_PRESCALE_SCALE));
    }
}
unsigned short *MDEC::rl2blk(int *blk, unsigned short *mdec_rl)
{
    int  i, k, q_scale, rl, used_col;
    int *iqtab;
    memset(blk, 0, 6 * DSIZE2 * sizeof(int));
    iqtab = iq_uv;
    for (i = 0; i < 6; i++) {
        if (i == 2)
            iqtab = iq_y;
        rl = SWAP16(*mdec_rl);
        mdec_rl++;
        q_scale = RLE_RUN(rl);
        blk[0]  = SCALER(iqtab[0] * RLE_VAL(rl), AAN_EXTRA - 3);
        for (k = 0, used_col = 0;;) {
            rl = SWAP16(*mdec_rl);
            mdec_rl++;
            if (rl == MDEC_END_OF_DATA)
                break;
            k += RLE_RUN(rl) + 1;
            if (k > 63) {
                break;
            }
            blk[zscan[k]] = SCALER(RLE_VAL(rl) * iqtab[k] * q_scale, AAN_EXTRA);
            used_col |= (zscan[k] > 7) ? 1 << (zscan[k] & 7) : 0;
        }
        if (k == 0)
            used_col = -1;
        idct(blk, used_col);
        blk += DSIZE2;
    }
    return mdec_rl;
}
void MDEC::putlinebw15(unsigned short *image, int *Yblk)
{
    int i;
    int A = (mdec.reg0 & MDEC0_STP) ? 0x8000 : 0;
    for (i = 0; i < 8; i++, Yblk++) {
        int Y    = *Yblk;
        image[i] = SWAP16((CLAMP5(Y >> 3) * 0x421) | A);
    }
}
void MDEC::putquadrgb15(unsigned short *image, int *Yblk, int Cr, int Cb)
{
    int Y, R, G, B;
    int A = (mdec.reg0 & MDEC0_STP) ? 0x8000 : 0;

    R = MULR(Cr);
    G = MULG2(Cb, Cr);
    B = MULB(Cb);
    Y = MULY(Yblk[0]);

    image[0]  = MAKERGB15(CLAMP_SCALE5(Y + R), CLAMP_SCALE5(Y + G), CLAMP_SCALE5(Y + B), A);
    Y         = MULY(Yblk[1]);
    image[1]  = MAKERGB15(CLAMP_SCALE5(Y + R), CLAMP_SCALE5(Y + G), CLAMP_SCALE5(Y + B), A);
    Y         = MULY(Yblk[8]);
    image[16] = MAKERGB15(CLAMP_SCALE5(Y + R), CLAMP_SCALE5(Y + G), CLAMP_SCALE5(Y + B), A);
    Y         = MULY(Yblk[9]);
    image[17] = MAKERGB15(CLAMP_SCALE5(Y + R), CLAMP_SCALE5(Y + G), CLAMP_SCALE5(Y + B), A);
}
void MDEC::yuv2rgb15(int *blk, unsigned short *image)
{
    int  x, y;
    int *Yblk  = blk + DSIZE2 * 2;
    int *Crblk = blk;
    int *Cbblk = blk + DSIZE2;
    if (!Config.Mdec) {
        for (y = 0; y < 16; y += 2, Crblk += 4, Cbblk += 4, Yblk += 8, image += 24) {
            if (y == 8)
                Yblk += DSIZE2;
            for (x = 0; x < 4; x++, image += 2, Crblk++, Cbblk++, Yblk += 2) {
                putquadrgb15(image, Yblk, *Crblk, *Cbblk);
                putquadrgb15(image + 8, Yblk + DSIZE2, *(Crblk + 4), *(Cbblk + 4));
            }
        }
    } else {
        for (y = 0; y < 16; y++, Yblk += 8, image += 16) {
            if (y == 8)
                Yblk += DSIZE2;
            putlinebw15(image, Yblk);
            putlinebw15(image + 8, Yblk + DSIZE2);
        }
    }
}
void MDEC::putlinebw24(unsigned char *image, int *Yblk)
{
    int           i;
    unsigned char Y;
    for (i = 0; i < 8 * 3; i += 3, Yblk++) {
        Y = CLAMP8(*Yblk);

        image[i + 0] = Y;
        image[i + 1] = Y;
        image[i + 2] = Y;
    }
}
void MDEC::putquadrgb24(unsigned char *image, int *Yblk, int Cr, int Cb)
{
    int Y, R, G, B;
    R = MULR(Cr);
    G = MULG2(Cb, Cr);
    B = MULB(Cb);
    Y = MULY(Yblk[0]);

    image[0 * 3 + 0]  = CLAMP_SCALE8(Y + R);
    image[0 * 3 + 1]  = CLAMP_SCALE8(Y + G);
    image[0 * 3 + 2]  = CLAMP_SCALE8(Y + B);
    Y                 = MULY(Yblk[1]);
    image[1 * 3 + 0]  = CLAMP_SCALE8(Y + R);
    image[1 * 3 + 1]  = CLAMP_SCALE8(Y + G);
    image[1 * 3 + 2]  = CLAMP_SCALE8(Y + B);
    Y                 = MULY(Yblk[8]);
    image[16 * 3 + 0] = CLAMP_SCALE8(Y + R);
    image[16 * 3 + 1] = CLAMP_SCALE8(Y + G);
    image[16 * 3 + 2] = CLAMP_SCALE8(Y + B);
    Y                 = MULY(Yblk[9]);
    image[17 * 3 + 0] = CLAMP_SCALE8(Y + R);
    image[17 * 3 + 1] = CLAMP_SCALE8(Y + G);
    image[17 * 3 + 2] = CLAMP_SCALE8(Y + B);
}
void MDEC::yuv2rgb24(int *blk, unsigned char *image)
{
    int  x, y;
    int *Yblk  = blk + DSIZE2 * 2;
    int *Crblk = blk;
    int *Cbblk = blk + DSIZE2;

    if (!Config.Mdec) {
        for (y = 0; y < 16; y += 2, Crblk += 4, Cbblk += 4, Yblk += 8, image += 24 * 3) {
            if (y == 8)
                Yblk += DSIZE2;
            for (x = 0; x < 4; x++, image += 6, Crblk++, Cbblk++, Yblk += 2) {
                putquadrgb24(image, Yblk, *Crblk, *Cbblk);
                putquadrgb24(image + 8 * 3, Yblk + DSIZE2, *(Crblk + 4), *(Cbblk + 4));
            }
        }
    } else {
        for (y = 0; y < 16; y++, Yblk += 8, image += 16 * 3) {
            if (y == 8)
                Yblk += DSIZE2;
            putlinebw24(image, Yblk);
            putlinebw24(image + 8 * 3, Yblk + DSIZE2);
        }
    }
}
void MDEC::mdecInit(void)
{
    mdec.rl   = (u16 *)&pcsx->psxMem->psxM[0x100000];
    mdec.reg0 = 0;
    mdec.reg1 = 0;
}
void MDEC::mdecWrite0(u32 data)
{
    mdec.reg0 = data;
}
u32 MDEC::mdecRead0(void)
{
    return mdec.reg0;
}
void MDEC::mdecWrite1(u32 data)
{
    if (data & MDEC1_RESET) {
        mdec.reg0 = 0;
        mdec.reg1 = 0;
    }
}
u32 MDEC::mdecRead1(void)
{
    u32 v = mdec.reg1;
    v |= (mdec.reg0 & MDEC0_STP) ? MDEC1_STP : 0;
    v |= (mdec.reg0 & MDEC0_RGB24) ? MDEC1_RGB24 : 0;
    return v;
}
void MDEC::psxDma0(u32 adr, u32 bcr, u32 chcr)
{
    int cmd = mdec.reg0;
    int size;

    if (chcr != 0x01000201) {
        return;
    }
    size = (bcr >> 16) * (bcr & 0xffff);

    switch (cmd >> 28) {
        case 0x3:
            mdec.rl     = (u16 *)PSXM(adr);
            mdec.rlsize = mdec.reg0 & MDEC0_SIZE_MASK;
            break;
        case 0x4: {
            u8 *p = (u8 *)PSXM(adr);
            iqtab_init(iq_y, p);
            iqtab_init(iq_uv, p + 64);
        } break;
        case 0x6:
            break;
        default:
            break;
    }
    HW_DMA0_CHCR &= SWAP32(~0x01000000);
    DMA_INTERRUPT(0);
}
void MDEC::psxDma1(u32 adr, u32 bcr, u32 chcr)
{
    int             blk[DSIZE2 * 6];
    unsigned short *image;
    int             size;

    if (chcr != 0x01000200)
        return;
    size  = (bcr >> 16) * (bcr & 0xffff);
    image = (u16 *)PSXM(adr);
    if (mdec.reg0 & MDEC0_RGB24) {
        MDECOUTDMA_INT(size / 4);
        size = size / ((16 * 16) / 2);
        for (; size > 0; size--, image += (16 * 16)) {
            mdec.rl = rl2blk(blk, mdec.rl);
            yuv2rgb15(blk, image);
        }
    } else {
        MDECOUTDMA_INT(size / 4);
        size = size / ((24 * 16) / 2);
        for (; size > 0; size--, image += (24 * 16)) {
            mdec.rl = rl2blk(blk, mdec.rl);
            yuv2rgb24(blk, (u8 *)image);
        }
    }
    mdec.reg1 |= MDEC1_BUSY;
}
void MDEC::mdec1Interrupt()
{
    if (HW_DMA1_CHCR & SWAP32(0x01000000)) {
        MDECOUTDMA_INT(PSXCLK / 1000 * BIAS);
        HW_DMA1_CHCR &= SWAP32(~0x01000000);
        DMA_INTERRUPT(1);
    } else {
        mdec.reg1 &= ~MDEC1_BUSY;
    }
}
int MDEC::mdecFreeze(gzFile f, int Mode)
{
    gzfreeze(&mdec, sizeof(mdec));
    gzfreeze(iq_y, sizeof(iq_y));
    gzfreeze(iq_uv, sizeof(iq_uv));
    return 0;
}
