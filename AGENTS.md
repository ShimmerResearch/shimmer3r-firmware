# shimmer3r-firmware

LogAndStream firmware for the **Shimmer3R**. STM32U5A5VJTXQ (Cortex-M33) with an Infineon CYW20820
for Classic Bluetooth and BLE. Logs to microSD and streams over BT simultaneously.

## The submodule is shared — check before you edit
`LogAndStream_Shimmer3R/log-and-stream-common` is its own repo
(`ShimmerResearch/log-and-stream-common`) and is **also a submodule of `shimmer3-firmware`**, the
MSP430 Shimmer3 firmware. A change there ships to both platforms.

Before editing anything under `log-and-stream-common/`, confirm the change is platform-agnostic.
Platform-specific behaviour belongs behind the abstraction declared in
`log-and-stream-common/log_and_stream_externs.h` and implemented here.

## Build
**STM32CubeIDE 1.15.1** (`C:\ST\STM32CubeIDE_1.15.1`). The workspace binds this project by absolute
path, so don't relocate the repo.

## Release
CI only — `build-release-firmware.yml` via **workflow_dispatch** (major/minor/patch, Release/Debug).
Version bumping is delegated to the shared submodule: `log-and-stream-common/scripts/increment_version.sh`,
writing `LogAndStream_Shimmer3R/build/version.txt`. Tags are `LogAndStream_Shimmer3R_v*`;
the matching Jira version prefix is `L&S_S3R_v`.

## Formatting
`clang-format-check.yml` gates pushes. The IDE profile is `STM32CubeIDE_Format_Profile.xml` at the repo root.
Unlike verisense-firmware, this one **checks** rather than reformatting in place — a badly formatted
push fails CI instead of being silently fixed.

## Do not edit
Vendor sensor libraries under `LogAndStream_Shimmer3R/Shimmer_Driver/*/` that are submodules
(`lsm6dsv-pid`, `lis3mdl-pid`, `lis2mdl-pid`, `lis2dw12-pid`, `BMP3_SensorAPI`).

## Keep the docs in step with the code
This repo has no `docs/` of its own — the reference documentation lives in the
`log-and-stream-common` submodule under `docs/`, and covers both platforms.

The rule there applies to changes made from here: **change behaviour in a subsystem, update its doc
in the same PR.** The doc is named after the subsystem directory (`Comms/` →
`SHIMMER3_BT_COMMUNICATION_PROTOCOL.md`, and so on) — see that repo's `AGENTS.md` for the full
mapping. Scope it to behaviour, not renames or refactors.

A doc change in the submodule is its own PR plus a pointer bump here, so allow for two PRs when the
change spans both.
