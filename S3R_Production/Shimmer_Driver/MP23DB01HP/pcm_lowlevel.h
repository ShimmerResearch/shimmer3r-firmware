#ifndef PCM_LOWLEVEL_H
#define PCM_LOWLEVEL_H

#define PCM_OK    0
#define PCM_ERROR -1

/* Audio Frequencies */
#ifndef AUDIO_FREQUENCY_16K
#define AUDIO_FREQUENCY_16K 16000U
#endif

#define SaturaLH(N, L, H) (((N)<(L))?(L):(((N)>(H))?(H):(N)))
#endif //PCM_LOWLEVEL_H
