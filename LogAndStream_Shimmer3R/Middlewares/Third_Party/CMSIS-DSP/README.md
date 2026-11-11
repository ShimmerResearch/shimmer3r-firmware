# CMSIS-DSP: the real FFT only

[CMSIS-DSP](https://github.com/ARM-software/CMSIS-DSP) at tag `v1.18.0`
(`VERSION`), Apache-2.0 (`LICENSE`), unmodified. It holds only what
`arm_rfft_fast_f32` needs, which the audio fingerprint (`../AudioFingerprint`)
uses for its 512-point FFT:

- the headers, whole, because `arm_math.h` includes every `dsp/*.h`;
- `Source/CommonTables/`, of which the linker keeps only the 512-point tables;
- six files from `Source/TransformFunctions/`.

It is built with `-ffp-contract=off`, like the fingerprint (a folder setting
in the `.cproject`). To update it, replace the files with the same subset of a
newer tag, and check that the fingerprint's hashes still match the PC's.
