/*
 * hal_AudioRecorder.h
 *
 * Audio recording while logging to SD (DEV-99): the Shimmer3R half of
 * log-and-stream-common's Audio/shimmer_audio.h. It implements PlatAudio_*
 * (declared in log_and_stream_externs.h), capturing through hal_Audio.c and
 * writing audio/<experiment>/<session>/<NNN>.WAV (raw audio) and <NNN>.AFP
 * (the fingerprint, Middlewares/Third_Party/AudioFingerprint) beside each data
 * file. log-and-stream-common's docs/SHIMMER3R_AUDIO.md has the file layouts
 * and the rules.
 */

#ifndef HAL_AUDIORECORDER_H_
#define HAL_AUDIORECORDER_H_

#include <stdint.h>

/* The 16 kHz capture: 2048-sample (128 ms) blocks, whose first two after a
 * start are the mic's start-up and the filters' settling */
#define AUDIO_REC_BLOCK_SAMPLES 2048U
#define AUDIO_REC_SETTLE_BLOCKS 2U

/* The capture is shared by recording and the sound level (hal_AudioLevel.c,
 * DEV-1413). The first owner to acquire it starts it, at the recorder's
 * settings, and the last to release it stops it. Acquire returns 0 if the
 * capture would not start, or is running for something else (the factory
 * test). */
#define AUDIO_OWNER_RECORDER    0x01U
#define AUDIO_OWNER_LEVEL       0x02U
uint8_t AudioRecorder_captureAcquire(uint8_t owner);
void AudioRecorder_captureRelease(uint8_t owner);
/* Capture block n's AUDIO_REC_BLOCK_SAMPLES samples, for n among the last
 * AUDIO_CAPTURE_MAX_BLOCKS - 1 completed (AudioCapture_blocksDone()). Call
 * AudioCapture_prepareRead() before reading them. */
const int16_t *AudioRecorder_captureBlock(uint32_t block);

/* For the main loop, before the task list: writes one block of audio, ahead of
 * every task, when the capture ring has passed half full. TASK_AUDIO is the
 * lowest-priority task, so this is what keeps a long run of other work from
 * overrunning the ring. Each use is counted in the file's closing summary. */
void AudioRecorder_drainIfUrgent(void);

/* Once, at boot: builds the fingerprint's tables, about 70 ms of double
 * arithmetic. At boot rather than when a session starts, because by then
 * sensors are sampling and a 70 ms stall would drop samples. */
void AudioRecorder_init(void);

#endif /* HAL_AUDIORECORDER_H_ */
