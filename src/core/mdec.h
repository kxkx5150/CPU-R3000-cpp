
#ifndef __MDEC_H__
#define __MDEC_H__

#ifdef __cplusplus
#include "../utils/common.h"
#include "r3000a.h"
#include "hw.h"
#include "dma.h"
extern "C" {
#endif

#define DSIZE  8
#define DSIZE2 (DSIZE * DSIZE)


class MDEC {
  public:
    PCSX *pcsx = nullptr;

    struct _mdec
    {
        u32             reg0;
        u32             reg1;
        unsigned short *rl;
        int             rlsize;
    } mdec;

  public:
    MDEC(PCSX *_pcsx);

    void mdecInit();
    void mdecWrite0(u32 data);
    void mdecWrite1(u32 data);

    u32 mdecRead0();
    u32 mdecRead1();

    void psxDma0(u32 madr, u32 bcr, u32 chcr);
    void psxDma1(u32 madr, u32 bcr, u32 chcr);

    void mdec1Interrupt();
    int  mdecFreeze(gzFile f, int Mode);

    unsigned short *rl2blk(int *blk, unsigned short *mdec_rl);

    void fillcol(int *blk, int val);
    void fillrow(int *blk, int val);
    void idct(int *block, int used_col);
    void iqtab_init(int *iqtab, unsigned char *iq_y);
    void putlinebw15(unsigned short *image, int *Yblk);
    void putquadrgb15(unsigned short *image, int *Yblk, int Cr, int Cb);
    void yuv2rgb15(int *blk, unsigned short *image);
    void putlinebw24(unsigned char *image, int *Yblk);
    void putquadrgb24(unsigned char *image, int *Yblk, int Cr, int Cb);
    void yuv2rgb24(int *blk, unsigned char *image);

  public:
    int iq_y[DSIZE2], iq_uv[DSIZE2];
    int zscan[DSIZE2]     = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
                             41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
                             30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
    int aanscales[DSIZE2] = {1048576, 1454417, 1370031, 1232995, 1048576, 823861,  567485,  289301,  1454417, 2017334,
                             1900287, 1710213, 1454417, 1142728, 787125,  401273,  1370031, 1900287, 1790031, 1610986,
                             1370031, 1076426, 741455,  377991,  1232995, 1710213, 1610986, 1449849, 1232995, 968758,
                             667292,  340183,  1048576, 1454417, 1370031, 1232995, 1048576, 823861,  567485,  289301,
                             823861,  1142728, 1076426, 968758,  823861,  647303,  445870,  227303,  567485,  787125,
                             741455,  667292,  567485,  445870,  307121,  156569,  289301,  401273,  377991,  340183,
                             289301,  227303,  156569,  79818};
};


#ifdef __cplusplus
}
#endif
#endif
