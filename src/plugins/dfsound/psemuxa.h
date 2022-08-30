//============================================
//=== Audio XA decoding
//=== Kazzuya
//============================================

#ifndef DECODEXA_H
#define DECODEXA_H
#ifdef __cplusplus

extern "C" {
#endif
typedef struct
{
    long y0, y1;
} ADPCM_Decode_t;

typedef struct
{
    int            freq;
    int            nbits;
    int            stereo;
    int            nsamples;
    ADPCM_Decode_t left, right;
    short          pcm[16384];
} xa_decode_t;
#ifdef __cplusplus
}
#endif
#endif
