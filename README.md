# Shimmer3R Firmware — LogAndStream

Firmware for the **Shimmer3R** wearable sensing platform. The main application (`LogAndStream_Shimmer3R`) implements the **LogAndStream** protocol, enabling simultaneous data logging to a microSD card and wireless streaming over Bluetooth (Classic Bluetooth and BLE).

---

## Hardware

| Item | Detail |
|---|---|
| MCU | STM32U5A5VJTXQ (ARM Cortex-M33, 160 MHz) |
| Bluetooth | Infineon CYW20820 (Classic Bluetooth & BLE, CYSPP protocol) |
| Storage | microSD via SDMMC |
| USB | USB-C (CDC-ACM virtual COM port) |

### On-board sensors

| Sensor | Type |
|---|---|
| LSM6DSV | IMU — accelerometer + gyroscope |
| LIS2MDL | Magnetometer |
| LIS3MDL | Magnetometer |
| LIS2DW12 | Low-power accelerometer |
| ADXL371 | High-g accelerometer |
| BMP390 (BMP3) | Barometric pressure + temperature |
| ADS7028/38 | 8-channel ADC (expansion connector) |

### Supported expansion boards (daughter cards)

| Board | Sensors / Function |
|---|---|
| SR47 (ExG) | ADS1292 — ECG / biopotential |
| SR48 (GSR+) | Galvanic skin response + PPG connector |
| SR49 (Bridge Amp) | Bridge amplifier for strain gauges |
| SR38 (Proto3 Deluxe) | General-purpose prototyping |

---

## Repository layout

```
shimmer3r-firmware/
├── LogAndStream_Shimmer3R/          # Main LogAndStream firmware project (STM32CubeIDE)
│   ├── Core/                # STM32 HAL-generated peripheral init code
│   ├── Shimmer_Driver/      # Sensor and board support drivers
│   │   ├── LSM6DSV/         # IMU driver + STMicroelectronics PID submodule
│   │   ├── LIS2MDL/         # Magnetometer driver + PID submodule
│   │   ├── LIS3MDL/         # Magnetometer driver + PID submodule
│   │   ├── LIS2DW12/        # Accelerometer driver + PID submodule
│   │   ├── ADXL371/         # High-g accelerometer driver
│   │   ├── BMP3/            # Pressure sensor driver + BMP3_SensorAPI submodule
│   │   ├── ADS7028_38/      # Expansion ADC driver
│   │   ├── EXG/             # ADS1292 ExG driver
│   │   ├── CYW20820/        # Bluetooth driver
│   │   ├── CAT24C16/        # EEPROM driver
│   │   ├── GSRTestRig/      # GSR test-rig support
│   │   ├── hal_Board.*      # Board-level GPIO and power macros
│   │   ├── hal_Power.*      # Power management
│   │   ├── hal_Infomem.*    # Info-memory (device config) read/write
│   │   └── version.h        # Auto-generated firmware version header
│   ├── log-and-stream-common/   # Shared LogAndStream protocol code (submodule)
│   ├── Middlewares/         # USB, FATFS, ThreadX/USBX middleware
│   └── LogAndStream_Shimmer3R_UBGA132_SMPS.ioc  # STM32CubeMX project file
├── test_proj/               # Standalone test and example projects
│   ├── ADC_DMA_Transfer/
│   ├── FatFs_STM32U5/
│   ├── USB_CDC_ACM/
│   └── dev_CYW20820/
├── Extras/
│   └── WsOtaUpgrade/        # Wireless (BLE) OTA firmware upgrade tool
└── .github/workflows/
    ├── build-release-firmware.yml   # CI: build + GitHub Release + Jira link
    └── clang-format-check.yml       # CI: code style check
```

---

## Getting started

### Prerequisites

- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) (used for building and flashing)
- Git with submodule support

### Clone with submodules

```bash
git clone --recurse-submodules https://github.com/ShimmerResearch/shimmer3r-firmware.git
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

### Build in STM32CubeIDE

1. Open STM32CubeIDE.
2. **File → Open Projects from File System…** and select the `LogAndStream_Shimmer3R` directory.
3. Choose the desired build configuration (**Debug** or **Release**).
4. Click **Build** (hammer icon) or press `Ctrl+B`.

The compiled `.hex` file is written to `LogAndStream_Shimmer3R/Debug/` or `LogAndStream_Shimmer3R/Release/`.

### Flash the device

Use STM32CubeIDE's built-in debug/flash runner, the **STM32CubeProgrammer**, or any ST-LINK-compatible tool:

```bash
STM32_Programmer_CLI -c port=SWD -w LogAndStream_Shimmer3R/Debug/LogAndStream_Shimmer3R.hex -rst
```

---

## Firmware versioning

The version is stored in `LogAndStream_Shimmer3R/build/version.txt` and reflected in `LogAndStream_Shimmer3R/Shimmer_Driver/version.h`.
The current version follows the `vMAJOR.MINOR.PATCH` scheme (e.g. `v1.00.058`).

Releases are created automatically by the **LogAndStream S3R Build & Release** GitHub Actions workflow, which:

1. Increments the chosen version component.
2. Builds the firmware with STM32CubeIDE.
3. Publishes a GitHub Release with the `.hex` artifact.
4. Tags both this repository and the `log-and-stream-common` submodule.
5. Links the release to the corresponding Jira version.

---

## Code style

The project uses **clang-format** for consistent C code formatting.  
The style profile is defined in `LogAndStream_Shimmer3R/.clang-format` and `STM32CubeIDE_Format_Profile.xml`.

To format all files locally (Windows):

```batch
Extras\clang-format-all-win64\clang-format-all.bat
```

A `clang-format` check runs automatically on every pull request via GitHub Actions.

---

## Submodules

| Submodule path | Upstream repository |
|---|---|
| `LogAndStream_Shimmer3R/Shimmer_Driver/LSM6DSV/lsm6dsv-pid` | [STMicroelectronics/lsm6dsv-pid](https://github.com/STMicroelectronics/lsm6dsv-pid) |
| `LogAndStream_Shimmer3R/Shimmer_Driver/LIS3MDL/lis3mdl-pid` | [STMicroelectronics/lis3mdl-pid](https://github.com/STMicroelectronics/lis3mdl-pid) |
| `LogAndStream_Shimmer3R/Shimmer_Driver/LIS2MDL/lis2mdl-pid` | [STMicroelectronics/lis2mdl-pid](https://github.com/STMicroelectronics/lis2mdl-pid) |
| `LogAndStream_Shimmer3R/Shimmer_Driver/LIS2DW12/lis2dw12-pid` | [STMicroelectronics/lis2dw12-pid](https://github.com/STMicroelectronics/lis2dw12-pid) |
| `LogAndStream_Shimmer3R/Shimmer_Driver/BMP3/BMP3_SensorAPI` | [ShimmerEngineering/BMP3_SensorAPI](https://github.com/ShimmerEngineering/BMP3_SensorAPI) |
| `LogAndStream_Shimmer3R/log-and-stream-common` | [ShimmerResearch/log-and-stream-common](https://github.com/ShimmerResearch/log-and-stream-common) |

---

## Contributing

1. Fork the repository and create a feature branch.
2. Follow the existing code style (run clang-format before committing).
3. Open a pull request — the CI checks (format + build) run automatically.

---

## License

Copyright © Shimmer Research, Ltd. All rights reserved.  
Redistribution and use are governed by the BSD-style license found in each source file header.
