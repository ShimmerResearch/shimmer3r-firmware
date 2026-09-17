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

Placement decides this. Code inside a `USER CODE` block that CubeMX's own template defines is
normally preserved; code outside one is not, and an invented marker pair is no help either — CubeMX
keeps only the blocks it knows about. **Normally, not always:** regeneration has been seen to
overwrite user sections, so a block lowers the risk rather than removing it, and the guard stays even
on code that sits in one.

Where the entries below could simply be moved into a block, they have been (DEV-1017). The rest edit
a *generated statement in place* — an assignment, a call argument, a declaration — which no block can
protect, and that is exactly what the guard is for.

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
- **Most `MX_*_Init()` absent from `main.c` is correct.** 15 of the 29 entries in `functionlistsort`
  carry *Do Not Generate Function Call* — the third field of each `N-MX_Foo_Init-PERIPH-<flag>-HAL-…`
  entry — because those peripherals are initialised lazily. All 15 are genuinely absent from `main.c`.
  The mismatch is by design; don't "fix" it.

Afterwards, `Release/` and `Debug/` hold stale generated `subdir.mk`, `objects.list` and `makefile`
referencing files you removed — rebuild in the IDE rather than from the command line, which
regenerates them. (They also hardcode absolute paths, so they go stale if the repo moves.)

## Release
CI only — `build-release-firmware.yml` via **workflow_dispatch** (major/minor/patch, Release/Debug).
Version bumping is delegated to the shared submodule: `log-and-stream-common/scripts/increment_version.sh`,
writing `LogAndStream_Shimmer3R/build/version.txt`. Tags are `LogAndStream_Shimmer3R_v*`;
the matching Jira version prefix is `L&S_S3R_v`.

## Testing
The shared submodule carries a host test suite that needs no STM32 toolchain and no device:

```
make -C LogAndStream_Shimmer3R/log-and-stream-common/Test/host platform-check   # compile for BOTH MCUs
make -C LogAndStream_Shimmer3R/log-and-stream-common/Test/host                  # build and run, ~10 s
```

`platform-check` is the one that matters most from here: it compiles the shared modules for
`-DSHIMMER3R` *and* `-DSHIMMER3`, which is the cheapest guard against a submodule change that builds
for one platform and not the other. Nothing in it covers CubeMX, USBX or FileX.

The full release procedure — gates, the radio bring-up matrix, per-model functional
tests, sign-off list — is
`log-and-stream-common/docs/SHIMMER3_TEST_PROCEDURE.md`. §3.3 is the CubeMX checklist.

## Formatting
`clang-format-check.yml` runs on every push with `inplace: True` and commits the reformatted result
back, so a badly formatted push is fixed on your branch rather than rejected. **Pull before your next
push, and fetch before tagging a release**, or the tag misses the formatting commit. That commit also
gets no CI run of its own — GitHub does not trigger workflows for `GITHUB_TOKEN` pushes.

**Run `.githooks\install.bat` (or `.githooks/install.sh`) once per clone and the bot commit never
appears.** The `pre-commit` hook clang-formats the `.c`/`.h` files staged for the commit and re-stages
them. Nothing needs installing: Git for Windows supplies the shell, and
`Extras/clang-format-all-win64/clang-format.exe` is already in the clone. It never blocks a commit,
and `git commit --no-verify` bypasses it — see `.githooks/README.md`.

The installer also configures the `log-and-stream-common` submodule, because commits made inside it
are its commits and need their own hook configuration.

`Extras/clang-format-all-win64/LogAndStream-Shimmer3R.bat` still formats the whole project in one go.
Its exclusion list matches both the workflow's and the hook's — the hook's is the `EXCLUDE_RE` at the
top of `.githooks/pre-commit`, and it covers `Drivers/`, `Middlewares/`, `FATFS/` and every vendor
`-pid/` submodule. **If you change one of the three, change the other two in the same commit.**

CI pins clang-format **17**, the bundled `clang-format.exe` is **18.1.8**, and the two currently agree
on this codebase — the difference is not a live problem, but keep them in mind before blaming churn
on it. The IDE profile is `STM32CubeIDE_Format_Profile.xml` at the repo root; `.clang-format` lives in
`LogAndStream_Shimmer3R/`, not at the repo root.

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
