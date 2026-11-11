/*
 * Landmark audio fingerprinting: a C port of the audfprint analyzer.
 *
 * audfprint is Copyright (c) 2014-5 Dan Ellis, Columbia University, and Google.
 * MIT licence: see reference/LICENSE, which must ship with every copy of this code.
 *
 * Each function names the reference/audfprint_analyze.py (or stft.py) code it mirrors.
 * Evaluation order follows the reference where it decides rounding, and the double build
 * must reproduce golden/phase3-11025hz-30s exactly. Keep it that way.
 */
#include "audio_fingerprint.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

/* audfprint_analyze.py constants, at the phone app's settings. */
#define DENSITY 60.0     /* Analyzer(density=60); upstream default is 20 */
#define HPF_POLE 0.98    /* HPF_POLE ** (1 / OVERSAMP), OVERSAMP = 1 */
#define F_SD 30.0        /* self.f_sd: Gaussian spread of the masking threshold, in bins */
#define MIN_DT 2         /* self.mindt */
#define TARGET_DT 63     /* self.targetdt */
#define TARGET_DF 31     /* self.targetdf */
#define INIT_FRAMES 10   /* forward threshold is seeded from the first 10 frames */

#define AF_PI 3.14159265358979323846

enum { ST_IDLE, ST_STFT, ST_LOG, ST_HPF, ST_FWD, ST_BWD, ST_PEAKS, ST_LANDMARKS, ST_DONE };

#if defined(AF_DOUBLE)
#define AF_LOG log
#define AF_SQRT sqrt
#else
#define AF_LOG logf
#define AF_SQRT sqrtf
#endif

/* Tables, built in double and rounded once to af_real. */
static af_real s_window[AF_N_FFT];
static af_real s_gauss[2 * AF_N_BINS + 1];
static af_real s_a_dec;
static af_real s_hpf_pole;
#if !defined(AF_FFT_CMSIS)
static af_real s_tw_re[AF_N_FFT / 2];
static af_real s_tw_im[AF_N_FFT / 2];
#endif

void af_init(af_workspace *ws)
{
    int i;

    /* np.hanning(n_fft + 2)[1:-1], computed the way numpy does:
     * 0.5 + 0.5 * cos(pi * n / (M - 1)), n = 1 - M, 3 - M, ..., M = n_fft + 2. */
    for (i = 0; i < AF_N_FFT; i++) {
        const double n = (double)(2 * i - (AF_N_FFT - 1));
        s_window[i] = (af_real)(0.5 + 0.5 * cos(AF_PI * n / (double)(AF_N_FFT + 1)));
    }

    /* Analyzer.spreadpeaks: np.exp(-0.5 * ((np.arange(-npoints, npoints + 1) / width) ** 2)) */
    for (i = 0; i <= 2 * AF_N_BINS; i++) {
        const double x = (double)(i - AF_N_BINS) / F_SD;
        s_gauss[i] = (af_real)exp(-0.5 * (x * x));
    }

    /* find_peaks: a_dec = (1 - 0.01 * (density * np.sqrt(n_hop / 352.8) / 35)) ** (1 / OVERSAMP) */
    s_a_dec = (af_real)(1.0 - 0.01 * (DENSITY * sqrt((double)AF_N_HOP / 352.8) / 35.0));
    s_hpf_pole = (af_real)HPF_POLE;

#if defined(AF_FFT_CMSIS)
    arm_rfft_fast_init_512_f32(&ws->rfft);
#else
    for (i = 0; i < AF_N_FFT / 2; i++) {
        s_tw_re[i] = (af_real)cos(-2.0 * AF_PI * i / AF_N_FFT);
        s_tw_im[i] = (af_real)sin(-2.0 * AF_PI * i / AF_N_FFT);
    }
#endif

    ws->stage = ST_IDLE;
    ws->n_frames = 0;
    ws->n_peaks = 0;
    ws->n_landmarks = 0;
}

static af_status begin(af_workspace *ws, uint32_t n_samples)
{
    const uint32_t n_frames = af_frames_for(n_samples);

    ws->stage = ST_IDLE;
    ws->n_peaks = 0;
    ws->n_landmarks = 0;
    /* np.pad(mode='reflect') needs more samples than the pad. */
    if (n_samples < AF_N_FFT / 2 + 1)
        return AF_ERR_TOO_SHORT;
    if (n_frames > AF_MAX_FRAMES)
        return AF_ERR_TOO_LONG;

    ws->n_samples = n_samples;
    ws->n_frames = n_frames;
    ws->sg_max = 0;
    ws->sg_floor = 0;
    ws->sg_sum = 0;
    ws->sg_mean = 0;
    memset(ws->peak_bits, 0, sizeof(ws->peak_bits[0]) * n_frames);
    ws->stage = ST_STFT;
    ws->pos = 0;
    return AF_BUSY;
}

af_status af_begin_i16(af_workspace *ws, const int16_t *pcm, uint32_t n_samples)
{
    ws->pcm_i16 = pcm;
    ws->pcm_f32 = NULL;
    return begin(ws, n_samples);
}

af_status af_begin_f32(af_workspace *ws, const float *pcm, uint32_t n_samples)
{
    ws->pcm_i16 = NULL;
    ws->pcm_f32 = pcm;
    return begin(ws, n_samples);
}

/* ------------------------------------------------------------------------------------ */
/* Spectrogram: stft.stft (reflect padding, framing, window, rfft) then np.abs            */
/* ------------------------------------------------------------------------------------ */

/* Sample k of the reflect-padded signal: np.pad(d, n_fft // 2, mode='reflect'). */
static af_real padded_sample(const af_workspace *ws, int32_t k)
{
    const int32_t pad = AF_N_FFT / 2;
    const int32_t n = (int32_t)ws->n_samples;
    int32_t i;

    if (k < pad)
        i = pad - k;
    else if (k >= pad + n)
        i = 2 * n + pad - 2 - k;
    else
        i = k - pad;

    if (ws->pcm_i16 != NULL)
        return (af_real)ws->pcm_i16[i] * (af_real)(1.0 / 32768.0); /* exact: power of two */
    return (af_real)ws->pcm_f32[i];
}

static void store_magnitude(af_workspace *ws, uint32_t bin, uint32_t frame, af_real re, af_real im)
{
    const af_real m = AF_SQRT(re * re + im * im);
    ws->sgram[bin][frame] = m;
    if (m > ws->sg_max)
        ws->sg_max = m;
}

#if !defined(AF_FFT_CMSIS)
/* In-place radix-2 complex FFT, as in the Android port that matched the golden vectors. */
static void fft_radix2(af_real *re, af_real *im)
{
    const uint32_t n = AF_N_FFT;
    uint32_t i, j = 0, m, size, half, step, k;

    for (i = 0; i < n - 1; i++) {
        if (i < j) {
            af_real t = re[i];
            re[i] = re[j];
            re[j] = t;
            t = im[i];
            im[i] = im[j];
            im[j] = t;
        }
        m = n >> 1;
        while (j >= m) {
            j -= m;
            m >>= 1;
        }
        j += m;
    }
    for (size = 2; size <= n; size <<= 1) {
        half = size >> 1;
        step = n / size;
        for (i = 0; i < n; i += size) {
            for (k = 0; k < half; k++) {
                const af_real wr = s_tw_re[k * step];
                const af_real wi = s_tw_im[k * step];
                const uint32_t a = i + k;
                const uint32_t b = a + half;
                const af_real xr = re[b] * wr - im[b] * wi;
                const af_real xi = re[b] * wi + im[b] * wr;
                re[b] = re[a] - xr;
                im[b] = im[a] - xi;
                re[a] += xr;
                im[a] += xi;
            }
        }
    }
}
#endif

static void stft_frame(af_workspace *ws, uint32_t frame)
{
    const int32_t start = (int32_t)(frame * AF_N_HOP);
    uint32_t j, b;

    for (j = 0; j < AF_N_FFT; j++)
        ws->fft_re[j] = padded_sample(ws, start + (int32_t)j) * s_window[j];

#if defined(AF_FFT_CMSIS)
    /* Packed output: [X0.re, X(N/2).re, X1.re, X1.im, ...]. Unscaled, like np.fft.rfft. */
    arm_rfft_fast_f32(&ws->rfft, ws->fft_re, ws->fft_out, 0);
    store_magnitude(ws, 0, frame, ws->fft_out[0], 0);
    store_magnitude(ws, AF_N_BINS, frame, ws->fft_out[1], 0);
    for (b = 1; b < AF_N_BINS; b++)
        store_magnitude(ws, b, frame, ws->fft_out[2 * b], ws->fft_out[2 * b + 1]);
#else
    for (j = 0; j < AF_N_FFT; j++)
        ws->fft_im[j] = 0;
    fft_radix2(ws->fft_re, ws->fft_im);
    for (b = 0; b <= AF_N_BINS; b++)
        store_magnitude(ws, b, frame, ws->fft_re[b], ws->fft_im[b]);
#endif
}

/* find_peaks: sgram = np.log(np.maximum(sgram, np.max(sgram) / 1e6)), summed for the mean.
 * The sum has two levels: each row (one bin, n_frames values) in af_real, then the 257 row
 * totals in af_sum. A float sum over all ~80,000 values drifts, while a double sum of all
 * of them costs ~80,000 software double additions on the Cortex-M33, whose FPU is single
 * precision. Two levels keep almost all of the accuracy for 257 double additions. */
static void log_row(af_workspace *ws, uint32_t bin)
{
    af_real *row = ws->sgram[bin];
    af_real row_sum = 0;
    uint32_t f;

    for (f = 0; f < ws->n_frames; f++) {
        const af_real v = AF_LOG(row[f] > ws->sg_floor ? row[f] : ws->sg_floor);
        row[f] = v;
        row_sum += v;
    }
    ws->sg_sum += (af_sum)row_sum;
}

/* find_peaks: sgram - np.mean(sgram), then scipy.signal.lfilter([1, -1], [1, -HPF_POLE]) along
 * time with zero initial state. lfilter's direct form II transposed evaluates
 * y[n] = x[n] + z, z = -x[n] + pole * y[n], and that order is kept here. */
static void hpf_row(af_workspace *ws, uint32_t bin)
{
    af_real *row = ws->sgram[bin];
    af_real z = 0;
    uint32_t f;

    for (f = 0; f < ws->n_frames; f++) {
        const af_real x = row[f] - ws->sg_mean;
        const af_real y = x + z;
        row[f] = y;
        z = s_hpf_pole * y - x;
    }
}

/* ------------------------------------------------------------------------------------ */
/* Peak picking: _decaying_threshold_fwd_prune and _decaying_threshold_bwd_prune_peaks    */
/* ------------------------------------------------------------------------------------ */

/* locmax: v[i] >= v[i-1] (or i == 0), and not v[i+1] >= v[i]. */
static int is_locmax(const af_real *v, uint32_t i, uint32_t n)
{
    const int left = (i == 0) || (v[i] >= v[i - 1]);
    const int right_blocks = (i + 1 < n) && (v[i + 1] >= v[i]);
    return left && !right_blocks;
}

/* thresh = np.maximum(thresh, value * gaussian centred on pos) */
static void spread_into(af_real *thresh, uint32_t pos, af_real value)
{
    const af_real *g = &s_gauss[AF_N_BINS - pos];
    uint32_t i;

    for (i = 0; i < AF_N_BINS; i++) {
        const af_real v = value * g[i];
        if (v > thresh[i])
            thresh[i] = v;
    }
}

/* spreadpeaksinvector: Gaussians at every local max of v, over a zero base. */
static void spread_peaks_in_vector(af_real *out, const af_real *v)
{
    uint32_t i;

    for (i = 0; i < AF_N_BINS; i++)
        out[i] = 0;
    for (i = 0; i < AF_N_BINS; i++)
        if (is_locmax(v, i, AF_N_BINS))
            spread_into(out, i, v[i]);
}

static void decay(af_real *thresh)
{
    uint32_t i;

    for (i = 0; i < AF_N_BINS; i++)
        thresh[i] *= s_a_dec;
}

static int peak_get(const af_workspace *ws, uint32_t frame, uint32_t bin)
{
    return (ws->peak_bits[frame][bin >> 5] >> (bin & 31u)) & 1u;
}

static void peak_set(af_workspace *ws, uint32_t frame, uint32_t bin)
{
    ws->peak_bits[frame][bin >> 5] |= 1u << (bin & 31u);
}

static void peak_clear(af_workspace *ws, uint32_t frame, uint32_t bin)
{
    ws->peak_bits[frame][bin >> 5] &= ~(1u << (bin & 31u));
}

/* Python's sorted(zip(vals, pos), reverse=True): value descending, then position descending. */
static int ranks_before(af_real va, uint32_t pa, af_real vb, uint32_t pb)
{
    return va > vb || (va == vb && pa > pb);
}

/* Column `frame` of the spectrogram, copied to the FFT scratch (free after the STFT). */
static af_real *column(af_workspace *ws, uint32_t frame)
{
    af_real *col = ws->fft_re;
    uint32_t b;

    for (b = 0; b < AF_N_BINS; b++)
        col[b] = ws->sgram[b][frame];
    return col;
}

static void fwd_init(af_workspace *ws)
{
    af_real *first = ws->fft_re;
    const uint32_t n = ws->n_frames < INIT_FRAMES ? ws->n_frames : INIT_FRAMES;
    uint32_t b, c;

    /* np.max(sgram[:, :np.minimum(10, scols)], axis=1) */
    for (b = 0; b < AF_N_BINS; b++) {
        af_real m = ws->sgram[b][0];
        for (c = 1; c < n; c++)
            if (ws->sgram[b][c] > m)
                m = ws->sgram[b][c];
        first[b] = m;
    }
    spread_peaks_in_vector(ws->thresh, first);
}

static void fwd_column(af_workspace *ws, uint32_t frame)
{
    const af_real *col = column(ws, frame);
    uint32_t n_cand = 0, b, k, j;

    /* Candidates are fixed before any threshold update in this column. */
    for (b = 0; b < AF_N_BINS; b++)
        if (is_locmax(col, b, AF_N_BINS) && col[b] > ws->thresh[b])
            ws->cand[n_cand++] = (uint8_t)b;

    /* valspeaks[:maxpksperframe], taken in rank order */
    for (k = 0; k < AF_MAX_PEAKS_PER_FRAME && n_cand > 0; k++) {
        uint32_t best = 0;
        for (j = 1; j < n_cand; j++)
            if (ranks_before(col[ws->cand[j]], ws->cand[j], col[ws->cand[best]], ws->cand[best]))
                best = j;
        b = ws->cand[best];
        spread_into(ws->thresh, b, col[b]);
        peak_set(ws, frame, b);
        ws->cand[best] = ws->cand[--n_cand];
    }
    decay(ws->thresh);
}

static void bwd_init(af_workspace *ws)
{
    spread_peaks_in_vector(ws->thresh, column(ws, ws->n_frames - 1));
}

/* One step of `for col in range(scols, 0, -1)`: examines the peaks of frame col - 1. */
static void bwd_column(af_workspace *ws, uint32_t col)
{
    const uint32_t frame = col - 1;
    uint32_t n_cand = 0, b, i, j;

    for (b = 0; b < AF_N_BINS; b++)
        if (peak_get(ws, frame, b))
            ws->cand[n_cand++] = (uint8_t)b;

    /* sorted(..., reverse=True): insertion sort, at most AF_MAX_PEAKS_PER_FRAME entries */
    for (i = 1; i < n_cand; i++) {
        const uint8_t c = ws->cand[i];
        for (j = i; j > 0 && ranks_before(ws->sgram[c][frame], c,
                                          ws->sgram[ws->cand[j - 1]][frame], ws->cand[j - 1]); j--)
            ws->cand[j] = ws->cand[j - 1];
        ws->cand[j] = c;
    }

    for (i = 0; i < n_cand; i++) {
        const uint32_t pos = ws->cand[i];
        const af_real val = ws->sgram[pos][frame];
        if (val >= ws->thresh[pos]) {
            spread_into(ws->thresh, pos, val);
            /* "Delete any following peak": the reference clears column `col`, not col - 1. */
            if (col < ws->n_frames)
                peak_clear(ws, col, pos);
        } else {
            peak_clear(ws, frame, pos);
        }
    }
    decay(ws->thresh);
}

/* ------------------------------------------------------------------------------------ */
/* peaks2landmarks                                                                        */
/* ------------------------------------------------------------------------------------ */

/* The surviving peaks of frame f, appended in bin order. One frame per step: done for the
 * whole chunk at once, this was the longest single step on the device (~30 ms at 48 MHz). */
static void collect_peaks_frame(af_workspace *ws, uint32_t f)
{
    uint32_t w, b;

    ws->frame_first_peak[f] = ws->n_peaks;
    for (w = 0; w < AF_N_BINS / 32; w++) {
        const uint32_t bits = ws->peak_bits[f][w];
        if (bits == 0)
            continue; /* most words hold no peak */
        for (b = 0; b < 32; b++) {
            if ((bits >> b) & 1u) {
                ws->peaks[ws->n_peaks].frame = (uint16_t)f;
                ws->peaks[ws->n_peaks].bin = (uint8_t)(w * 32u + b);
                ws->n_peaks++;
            }
        }
    }
}

/* Landmarks starting at frame `col`. scols is the last peak's frame + 1. */
static void landmarks_from(af_workspace *ws, uint32_t col, uint32_t scols)
{
    const uint32_t end = (col + TARGET_DT < scols) ? col + TARGET_DT : scols;
    uint32_t p, col2, q;

    for (p = ws->frame_first_peak[col]; p < ws->frame_first_peak[col + 1]; p++) {
        const int peak = ws->peaks[p].bin;
        uint32_t pairs = 0;
        for (col2 = col + MIN_DT; col2 < end && pairs < AF_MAX_PAIRS_PER_PEAK; col2++) {
            for (q = ws->frame_first_peak[col2]; q < ws->frame_first_peak[col2 + 1]; q++) {
                const int peak2 = ws->peaks[q].bin;
                const int df = peak2 > peak ? peak2 - peak : peak - peak2;
                if (df < TARGET_DF && pairs < AF_MAX_PAIRS_PER_PEAK) {
                    af_landmark *lm = &ws->landmarks[ws->n_landmarks++];
                    lm->t1 = (uint16_t)col;
                    lm->f1 = (uint8_t)peak;
                    lm->f2 = (uint8_t)peak2;
                    lm->dt = (uint8_t)(col2 - col);
                    pairs++;
                }
            }
        }
    }
}

/* ------------------------------------------------------------------------------------ */
/* Stage machine                                                                          */
/* ------------------------------------------------------------------------------------ */

static void enter(af_workspace *ws, uint8_t stage)
{
    ws->stage = stage;
    ws->pos = 0;
}

af_status af_step(af_workspace *ws, uint32_t budget)
{
    uint32_t left = budget ? budget : UINT32_MAX;

    if (ws->stage == ST_IDLE)
        return AF_ERR_STATE;

    while (left > 0 && ws->stage != ST_DONE) {
        left--;
        switch (ws->stage) {
        case ST_STFT:
            stft_frame(ws, ws->pos++);
            if (ws->pos == ws->n_frames) {
                ws->pcm_i16 = NULL; /* the audio is not needed again */
                ws->pcm_f32 = NULL;
                if (ws->sg_max > 0) {
                    ws->sg_floor = ws->sg_max / (af_real)1e6;
                    enter(ws, ST_LOG);
                } else {
                    /* Silent input: the reference skips the log and the mean. */
                    enter(ws, ST_HPF);
                }
            }
            break;

        case ST_LOG:
            log_row(ws, ws->pos++);
            if (ws->pos == AF_N_BINS + 1) {
                ws->sg_mean = (af_real)(ws->sg_sum / (af_sum)((AF_N_BINS + 1) * ws->n_frames));
                enter(ws, ST_HPF);
            }
            break;

        case ST_HPF:
            hpf_row(ws, ws->pos++); /* the Nyquist row is dropped here */
            if (ws->pos == AF_N_BINS)
                enter(ws, ST_FWD);
            break;

        case ST_FWD:
            if (ws->pos == 0)
                fwd_init(ws);
            fwd_column(ws, ws->pos++);
            if (ws->pos == ws->n_frames)
                enter(ws, ST_BWD);
            break;

        case ST_BWD:
            if (ws->pos == 0)
                bwd_init(ws);
            bwd_column(ws, ws->n_frames - ws->pos);
            if (++ws->pos == ws->n_frames) {
                ws->n_peaks = 0;
                enter(ws, ST_PEAKS);
            }
            break;

        case ST_PEAKS:
            collect_peaks_frame(ws, ws->pos++);
            if (ws->pos == ws->n_frames) {
                ws->frame_first_peak[ws->n_frames] = ws->n_peaks;
                ws->n_landmarks = 0;
                enter(ws, ws->n_peaks == 0 ? ST_DONE : ST_LANDMARKS);
            }
            break;

        case ST_LANDMARKS:
            {
                const uint32_t scols = ws->peaks[ws->n_peaks - 1].frame + 1u;
                landmarks_from(ws, ws->pos++, scols);
                if (ws->pos == scols)
                    enter(ws, ST_DONE);
            }
            break;

        default:
            return AF_ERR_STATE;
        }
    }
    return ws->stage == ST_DONE ? AF_DONE : AF_BUSY;
}

af_status af_run(af_workspace *ws)
{
    return af_step(ws, 0);
}
