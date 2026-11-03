# Audio fingerprint

Landmark audio fingerprinting for the Shimmer3R's audio recording (DEV-99): a C
port of the audfprint analyzer, at the settings Shimmer's phone app uses
(density 60). In modes 1 and 2 of configuration byte 233,
`Shimmer_Driver/hal_AudioRecorder.c` runs it on 5 s of audio every 4 s while
the unit logs, and writes the hashes to `audio/.../NNN.AFP` beside each data
file. log-and-stream-common `docs/SHIMMER3R_AUDIO.md` has the file format.

| Path | What |
|---|---|
| `Inc/audio_fingerprint.h`, `Src/audio_fingerprint.c` | The port |
| `reference/` | audfprint's Python, which the port mirrors line by line: Dan Ellis, Columbia University and Google, MIT licence, [github.com/dpwe/audfprint](https://github.com/dpwe/audfprint). Each C function names the Python it came from. |
| `reference/LICENSE` | audfprint's MIT notice. It must ship with every copy of this code, firmware included. |

**Don't edit these files here.** The port is developed and checked against
golden vectors from the Python reference outside this repository. Change it
there and copy both files here unchanged, so the two stay identical. A rule
that looks simpler but is not the Python's changes which peaks survive, and
the device's hashes then stop matching hashes made on a PC.

## Building

The firmware's `.cproject` sets all of this, in both configurations:

- `AF_FFT_CMSIS` and `AF_MAX_FRAMES=313` for every file. They change
  `af_workspace`, so the library and its caller must agree. `AF_FFT_CMSIS`
  uses CMSIS-DSP's `arm_rfft_fast_f32` (`../CMSIS-DSP`).
- `-ffp-contract=off` on this folder and on `../CMSIS-DSP`. In GNU C mode GCC
  fuses multiply-adds on the Cortex-M33, which moves borderline peaks.
- Both folders' include paths.

## Memory, time and privacy

- **Memory.** One `af_workspace`, 372,952 bytes, static in
  `hal_AudioRecorder.c`. Nothing here allocates.
- **Time.** About 0.9 s of CPU per 5 s chunk at 48 MHz. `af_step()` works in
  units of a few milliseconds at most, so the recorder yields to other tasks
  between them.
- **Privacy.** The audio is read only during the first stage and never copied.
  The workspace's spectrogram can be inverted back to intelligible speech, so
  never log, store or transmit the workspace. Only the hashes leave.
