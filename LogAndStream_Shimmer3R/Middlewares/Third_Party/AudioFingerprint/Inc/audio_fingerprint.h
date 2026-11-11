/*
 * Landmark audio fingerprinting for Shimmer firmware (DEV-885).
 *
 * A C port of the audfprint analyzer: find_peaks -> peaks2landmarks -> landmarks2hashes
 * in reference/audfprint_analyze.py, at the settings the phone app uses (density 60).
 * Every rule mirrors the Python line it came from. "Simplifying" one changes which peaks
 * survive, and the device's hashes then stop matching a database built on the PC.
 *
 * audfprint is Copyright (c) 2014-5 Dan Ellis, Columbia University, and Google, under the
 * MIT licence in reference/LICENSE. That notice must ship with every copy of this code:
 * in the source, and in the product's third-party notices.
 *
 * Memory: everything lives in one af_workspace that the caller provides (statically, on
 * the device). Nothing here allocates. sizeof(af_workspace) is about 0.4 MB in float at
 * the default AF_MAX_FRAMES.
 *
 * Use:
 *   af_init(ws);                                 once
 *   af_begin_i16(ws, pcm, n);                    per chunk
 *   while (af_step(ws, budget) == AF_BUSY) { }   or af_run(ws); other work can run between steps
 *   for (i = 0; i < ws->n_landmarks; i++) emit(ws->landmarks[i].t1, af_hash(&ws->landmarks[i]));
 *
 * Privacy (DEV-885): the PCM buffer is only read, during the first stage, and is never
 * copied. The workspace's spectrogram is NOT safe to export: a magnitude spectrogram can
 * be inverted back to intelligible speech. Only peaks, landmarks and hashes may leave.
 *
 * Build options (compile definitions, must match between library and caller):
 *   AF_MAX_FRAMES      frames per analysis, default 313 (5 s at 16 kHz)
 *   AF_DOUBLE          double arithmetic: the PC reference build, never the device
 *   AF_FFT_CMSIS       CMSIS-DSP arm_rfft_fast_f32 instead of the built-in radix-2 FFT
 *                      (float only)
 *   AF_MEAN_FLOAT_SUM  add the per-bin row totals of the spectrogram mean in af_real
 *                      instead of double (see log_row). For the precision study only.
 */
#ifndef AUDIO_FINGERPRINT_H
#define AUDIO_FINGERPRINT_H

#include <stdint.h>

#if defined(AF_FFT_CMSIS)
#if defined(AF_DOUBLE)
#error "AF_FFT_CMSIS is float only"
#endif
#include "arm_math.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef AF_MAX_FRAMES
#define AF_MAX_FRAMES 313 /* 1 + 80000 / 256: one 5 s chunk at 16 kHz */
#endif

#if defined(AF_DOUBLE)
typedef double af_real;
#else
typedef float af_real;
#endif

#if defined(AF_MEAN_FLOAT_SUM)
typedef af_real af_sum;
#else
typedef double af_sum;
#endif

#define AF_N_FFT 512
#define AF_N_HOP 256
#define AF_N_BINS (AF_N_FFT / 2)          /* bins kept: N_FFT/2 + 1, Nyquist dropped */
#define AF_MAX_PEAKS_PER_FRAME 5
#define AF_MAX_PAIRS_PER_PEAK 3
#define AF_MAX_PEAKS (AF_MAX_FRAMES * AF_MAX_PEAKS_PER_FRAME)
#define AF_MAX_LANDMARKS (AF_MAX_PEAKS * AF_MAX_PAIRS_PER_PEAK)

#if AF_MAX_FRAMES > 65535
#error "AF_MAX_FRAMES must fit in uint16_t"
#endif

/* Hash layout, identical to audfprint_analyze.landmarks2hashes:
 * [ f1 (7 bits) | f2 - f1 (6 bits) | dt (19 bits) ] */
#define AF_HASH_F1_BITS 7
#define AF_HASH_DF_BITS 6
#define AF_HASH_DT_BITS 19

typedef struct {
    uint16_t frame;
    uint8_t bin;
} af_peak;

/* (t1, f1, f2, dt) as audfprint's peaks2landmarks builds them. */
typedef struct {
    uint16_t t1;
    uint8_t f1;
    uint8_t f2;
    uint8_t dt;
} af_landmark;

typedef enum {
    AF_DONE = 0,
    AF_BUSY = 1,
    AF_ERR_TOO_SHORT = -1, /* fewer than N_FFT/2 + 1 samples: reflect padding is undefined */
    AF_ERR_TOO_LONG = -2,  /* more frames than AF_MAX_FRAMES */
    AF_ERR_STATE = -3      /* af_step without af_begin_* */
} af_status;

typedef struct {
    /* Input. Read only during the STFT stage, never copied. */
    const int16_t *pcm_i16;
    const float *pcm_f32;
    uint32_t n_samples;
    uint32_t n_frames;

    /* Progress through the stages; see af_step. */
    uint8_t stage;
    uint32_t pos;

    af_real sg_max;
    af_real sg_floor;
    af_sum sg_sum;
    af_real sg_mean;

    /* Spectrogram, [bin][frame]. Row AF_N_BINS is the Nyquist bin: it counts toward the
     * global max and mean, then is dropped, as audfprint does. */
    af_real sgram[AF_N_BINS + 1][AF_MAX_FRAMES];

    /* Masking threshold, one bit per (frame, bin) peak, and the column's candidates. */
    af_real thresh[AF_N_BINS];
    uint32_t peak_bits[AF_MAX_FRAMES][AF_N_BINS / 32];
    uint8_t cand[AF_N_BINS];

    /* FFT scratch. */
    af_real fft_re[AF_N_FFT];
#if defined(AF_FFT_CMSIS)
    float32_t fft_out[AF_N_FFT];
    arm_rfft_fast_instance_f32 rfft;
#else
    af_real fft_im[AF_N_FFT];
#endif

    /* Results, valid once af_step returns AF_DONE. */
    uint32_t n_peaks;
    af_peak peaks[AF_MAX_PEAKS];                  /* by frame, then bin */
    uint32_t frame_first_peak[AF_MAX_FRAMES + 1]; /* peaks of frame f: [f], [f + 1]) */
    uint32_t n_landmarks;
    af_landmark landmarks[AF_MAX_LANDMARKS];
} af_workspace;

/* Builds the window, Gaussian and twiddle tables. Call once before anything else. */
void af_init(af_workspace *ws);

/* Starts analysing one chunk. The buffer must stay valid until the STFT stage ends. */
af_status af_begin_i16(af_workspace *ws, const int16_t *pcm, uint32_t n_samples);
af_status af_begin_f32(af_workspace *ws, const float *pcm, uint32_t n_samples);

/* Does up to `budget` units of work (one unit is one STFT frame, one spectrogram row or
 * column, or one frame's peaks or landmarks: none is much longer than the others, so a
 * caller can yield between units). Returns AF_BUSY until the chunk is finished, then
 * AF_DONE. budget 0 runs to the end. ws->stage tells a profiler which stage a unit was in:
 * 1 STFT, 2 log, 3 high-pass, 4 forward mask, 5 backward mask, 6 peaks, 7 landmarks. */
af_status af_step(af_workspace *ws, uint32_t budget);

/* af_step until done. */
af_status af_run(af_workspace *ws);

/* Frames produced from n samples: 1 + n / N_HOP (reflect padding of N_FFT/2 each side). */
static inline uint32_t af_frames_for(uint32_t n_samples)
{
    return 1u + n_samples / AF_N_HOP;
}

static inline uint32_t af_hash(const af_landmark *lm)
{
    const uint32_t f1 = lm->f1 & ((1u << AF_HASH_F1_BITS) - 1u);
    const uint32_t df = (uint32_t)((int)lm->f2 - (int)lm->f1) & ((1u << AF_HASH_DF_BITS) - 1u);
    const uint32_t dt = lm->dt & ((1u << AF_HASH_DT_BITS) - 1u);
    return (f1 << (AF_HASH_DF_BITS + AF_HASH_DT_BITS)) | (df << AF_HASH_DT_BITS) | dt;
}

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_FINGERPRINT_H */
