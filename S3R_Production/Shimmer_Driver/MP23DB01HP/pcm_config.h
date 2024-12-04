#ifndef PCM_CONFIG_H
#define PCM_CONFIG_H

//#include "app_config.h"
#include <pcm_lowlevel.h>

#define N_MS_PER_INTERRUPT              (1)

/* Default Audio IN sampling frequency */
#ifndef AUDIO_IN_SAMPLING_FREQUENCY
  #define AUDIO_IN_SAMPLING_FREQUENCY      AUDIO_FREQUENCY_16K
#endif

/* Default Audio IN internal buffer size */
#define DEFAULT_AUDIO_IN_BUFFER_SIZE     (AUDIO_IN_SAMPLING_FREQUENCY/1000)*2U*N_MS_PER_INTERRUPT

/* Default Audio IN application buffer size */
#define PCM_REC_BUFF_SIZE (DEFAULT_AUDIO_IN_BUFFER_SIZE / 2U)

#endif /* PCM_CONFIG_H */
