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

## CubeMX code generation — it deletes hand-written code
`LogAndStream_Shimmer3R_UBGA132_SMPS.ioc` is the peripheral model, and **Generate Code** is the only
sane way to add or remove a peripheral: it renumbers the `Mcu.IPn` / `Mcu.Pinn` arrays, which are
dense manifests where a hand-edited gap silently truncates the list. Never hand-edit those.

But generation rewrites everything outside `USER CODE` blocks, and this project keeps a lot of
hand-written code in CubeMX-owned regions. A single CRC deactivation (DEV-1003) also deleted 165
lines from `main.c` and changed ten other files. Nothing warns you.

**Always generate on a branch, then read the whole diff before committing.** Not `git diff --stat` —
the real diff. Restore what you did not ask for.

`scripts/check_cubemx_guards.sh` catches the known cases and runs in CI on every push, so a
regeneration that eats one of them fails the build instead of shipping. It is a backstop, not a
substitute for reading the diff — it only knows about damage that has already happened once. **If you
add hand-written code to a CubeMX-owned region, add a guard for it in the same commit.**

**In this instance CubeMX was not at fault — placement was.** Checked location by location against a real
regeneration: of the eleven pieces of hand-written code it touched, **nine sat outside any `USER CODE`
block**, and one was wrapped in an invented `/* USER CODE BEGIN Manufacturer_String */` that the
template does not define. The only two inside genuine blocks — `EC` in `app_usbx_device.h`, `PD` in
`ux_device_msc.c` — both **survived**. CubeMX preserves what is correctly placed, and only blocks its
own template defines.

That is one regeneration, though, not a guarantee: **regeneration has been seen to overwrite user
sections in some files**, so a `USER CODE` block makes loss much less likely rather than impossible.
Correct placement and the guard are both worth having, and the guard stays on code that has been
moved. With that said, three of these were losing code for no better reason than sitting in the
wrong place, and are moved into the right block rather than left to the guard:

- **`main.c`'s LSE ladder** — 165 lines, the single biggest loss, sitting in plain CubeMX territory
  between `SystemClock_Config`'s doc comment and its body. `USER CODE BEGIN PD` / `PFP` / `0` exist
  for exactly this and are already in use elsewhere in the file.
- **`app_usbx_device.h`** — the override inside `EC` is fine. The problem is that the *generated*
  defines above it were deleted by hand, so regeneration restores them and the 640 KB value wins only
  by appearing later in the file. Restore the generated pair; keep the override.
- **`ux_device_msc.c`** — `#define BUFFER_WORDS_SIZE   /* USER CODE END PD */` is a malformed block
  terminator. CubeMX has already silently repaired it once.

The rest genuinely cannot move, and that is what the guard is for. `rtc.c`, `sdmmc.c`, `usb_otg.c`,
both `app_usbx_device.c` cases and `ux_device_descriptors.h` each **edit a generated statement in
place** — no `USER CODE` block can protect a line that has to differ from the one the template emits.
`ux_user.h` is a stock ST config header meant to be edited directly and has no block to move into.
`ux_device_descriptors.c` could plausibly fold into `String_Framework1`, which already overwrites the
product string that way, but the `count`/`len` bookkeeping makes that a real change rather than a
move.

Files known to carry hand-written code that generation removes (DEV-1017):

| File | What lives there |
|---|---|
| `Core/Src/main.c` | DEV-866 LSE drive ladder — `Lse_tryDriveLevel/walkDriveLadder/bringUp`. **Was misplaced; moved into `USER CODE BEGIN 0`** |
| `Core/Src/rtc.c` | DEV-866 LSI limp-home — without it a board with a dead LSE hangs at boot |
| `Core/Src/sdmmc.c` | The deliberate no-`Error_Handler()` path for hot-swap failures |
| `Core/Src/usb_otg.c`, `Core/Inc/usb_otg.h` | `USB_getPcdSpeed()`, NVIC priority |
| `USBX/App/app_usbx_device.c` | 32-byte D-cache line alignment of the USBX byte pool |
| `USBX/App/ux_user.h` | `UX_SLAVE_REQUEST_DATA_MAX_LENGTH` 64 KB — the main MSC throughput knob |
| `USBX/App/app_usbx_device.h` | `UX_DEVICE_APP_MEM_POOL_SIZE` 640 KB. Correctly inside `EC` and preserved; the *generated* defines were deleted, so regeneration re-adds them as a 128 KB duplicate. **Generated pair restored; override now `#undef`s first** |
| `USBX/App/ux_device_descriptors.h` | CDC interrupt-IN `bInterval` — the fix for Mac xHCI USB-C dropping the MSC interface |
| `USBX/App/ux_device_descriptors.c` | EEPROM brand string for the USB manufacturer descriptor |
| `USBX/App/ux_device_msc.c` | Block-size defines, inside `PD` and preserved. **Malformed terminator fixed** |

Three more things that show up in the diff and are **not** yours to keep:

- **`main.c` gaining `MX_USBX_Device_Init();`.** It is already called from `usb_otg.c`; taking the
  generated one initialises USBX twice. The `.ioc` says to generate it, `main.c` has never had it —
  genuine drift, not a correction.
- **Hundreds of files under `Drivers/` and `Middlewares/`.** Line-ending churn, zero content change.
  `git checkout --` them.
- **Most `MX_*_Init()` absent from `main.c` is correct.** 15 of 28 entries in `functionlistsort`
  carry *Do Not Generate Function Call*, because those peripherals are initialised lazily. The
  mismatch is by design; don't "fix" it.

Afterwards, `Release/` and `Debug/` hold stale generated `subdir.mk`, `objects.list` and `makefile`
referencing files you removed — rebuild in the IDE rather than from the command line, which
regenerates them. (They also hardcode absolute paths, so they go stale if the repo moves.)

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
