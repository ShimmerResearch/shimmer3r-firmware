/*
 * hal_AudioRecorder.h
 *
 * Audio recording while logging to SD (DEV-99): the Shimmer3R half of
 * log-and-stream-common's Audio/shimmer_audio.h. It implements PlatAudio_*
 * (declared in log_and_stream_externs.h), capturing through hal_Audio.c and
 * writing audio/<experiment>/<session>/<NNN>.WAV beside each data file.
 * log-and-stream-common's docs/SHIMMER3R_AUDIO.md has the file layout and the
 * rules.
 */

#ifndef HAL_AUDIORECORDER_H_
#define HAL_AUDIORECORDER_H_

/* For the main loop, before the task list: writes one block of audio, ahead of
 * every task, when the capture ring has passed half full. TASK_AUDIO is the
 * lowest-priority task, so this is what keeps a long run of other work from
 * overrunning the ring. Each use is counted in the file's closing summary. */
void AudioRecorder_drainIfUrgent(void);

#endif /* HAL_AUDIORECORDER_H_ */
