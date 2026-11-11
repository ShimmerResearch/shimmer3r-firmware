/*
 * hal_AudioLevel.h
 *
 * The sound-level channel (DEV-1413): the Shimmer3R half of
 * log-and-stream-common's Audio/shimmer_audio_level.h. It implements
 * PlatAudio_levelStart/Stop/Process (log_and_stream_externs.h): while the unit
 * senses with configuration byte 233 bit 4 set, it holds the microphone
 * capture (shared with recording, hal_AudioRecorder.h) and works out the
 * A-weighted level of each 128 ms block as it completes, in TASK_AUDIO_LEVEL.
 * SPI_pollSensors() copies the latest into every packet's SOUND_LEVEL slot.
 * log-and-stream-common's docs/SHIMMER3R_AUDIO.md has what the value means.
 */

#ifndef HAL_AUDIOLEVEL_H_
#define HAL_AUDIOLEVEL_H_

#include <stdint.h>

/* The latest level: hundredths of a dB(A) SPL, or AUDIO_LEVEL_NONE before the
 * first block after the mic's start-up, and while the capture is not running.
 * Read from the sampling interrupt: an aligned 16-bit read, so never torn. */
int16_t AudioLevel_get(void);

#endif /* HAL_AUDIOLEVEL_H_ */
