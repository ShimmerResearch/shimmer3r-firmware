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
