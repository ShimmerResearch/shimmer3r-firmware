"""DEV-1363: read and decode the Shimmer3R gather diagnostic (gatherDiag, layout GDG2) over SWD.

The diagnostic build (branch DEV-1363_gather_diag, GATHER_DIAG=1) records every
gather's stage completions, the SPI and I2C completion detail, every interrupt in
between, and a snapshot of the sensor buses at each packet-ring fail-safe stall
(Core/Inc/gather_diag.h). This reads the struct with STM32_Programmer_CLI and
prints it. The layout must match the build: bump GDIAG_MAGIC and this reader's
field lists together.

    python gather_diag_read.py --elf <LogAndStream_Shimmer3R.elf> --sn <ST-Link serial> [--save dump.bin]
    python gather_diag_read.py --elf <elf> --file dump.bin        (decode a saved dump)

A hot-plug SWD read leaves the unit's Bluetooth RX unserviced until the MCU is
reset: pass --reset to reset it afterwards, which also clears the struct. The tool
paths below are the ones on the bench laptop; change them to match yours.
"""
from __future__ import annotations

import argparse
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

CLI = Path("C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe")
TOOLS = Path("C:/ST/STM32CubeIDE_1.15.1/STM32CubeIDE/plugins/"
             "com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/"
             "tools/bin")
NM = TOOLS / "arm-none-eabi-nm.exe"
ADDR2LINE = TOOLS / "arm-none-eabi-addr2line.exe"

MAGIC = 0x32474447
TRACE_LEN = 16
EVENTS = 8
ISR_LEN = 24
SNAP_FIELDS = ["rtcNow", "stallCount", "slotTs", "cbFlags", "i2cBus", "spiBus", "i2c1Sens",
               "spi1Sens", "spi2Sens", "hi2c1State", "hi2c1Err", "i2c1Isr", "i2c1Cr1", "i2c1Cr2",
               "hspi1State", "hspi1Tx", "hspi1Err", "spi1Sr", "spi1Cr1", "hspi2",
               "dmaI2c1Rx0", "dmaI2c1Rx1", "dmaI2c1Rx2", "dmaSpi1Rx0", "dmaSpi1Rx1", "dmaSpi1Rx2",
               "dmaSpi1Tx0", "dmaSpi1Tx1", "dmaSpi1Tx2", "gpio", "i2cStale", "i2cSeqErr", "ring", "ring2"]
SNAP_WORDS = len(SNAP_FIELDS)               # 34
TRACE_FIELDS = ["startCyc", "slotTs", "adcCyc", "i2cCyc", "spiCyc", "doneCyc", "ipsr", "flagsSeen",
                "rtcPc", "rtcLr", "execTask", "spiPollCyc", "spiBus1Cyc", "spiBus2Cyc",
                "spiIpsrBefore", "spiAfterDone", "spiCounts", "i2cStartCyc", "i2cRxCyc", "i2cBytes", "nIsr"]
TRACE_WORDS = len(TRACE_FIELDS) + ISR_LEN   # 45
EVENT_WORDS = 2 + 2 * SNAP_WORDS + TRACE_LEN * TRACE_WORDS
HEAD_FIELDS = ["magic", "size", "sessions", "sessionStartRtc", "gathers", "completes", "busyTicks",
               "lateCompletes", "maxLateStall", "stallResets", "syncAdc", "syncI2c", "syncSpi",
               "asyncAdc", "asyncI2c", "asyncSpi", "lostUpdates", "i2cErrors", "i2cLastErr",
               "i2cLastErrRtc", "spiErrors", "spiLastErr", "spiLastErrRtc", "nEvents", "rtcMsp", "lateStarts"]
TOTAL_WORDS = len(HEAD_FIELDS) + SNAP_WORDS + 1 + TRACE_LEN * TRACE_WORDS + EVENTS * EVENT_WORDS

STAGES = {0x01: "ADC", 0x02: "I2C", 0x08: "SPI"}
I2C_STAT = {0: "IDLE", 1: "LIS2MDL_MAG_GET"}
SPI_SENSOR = ["EMPTY", "LSM6DSV_GYRO_AND_ACCEL", "LSM6DSV_ACCEL_ONLY", "LSM6DSV_GYRO_ONLY", "ADXL371",
              "PRESSURE_TEMP", "ADS7028_INT_EXP0", "ADS7028_INT_EXP1", "ADS7028_INT_EXP2",
              "ADS7028_INT_EXP3", "ADS7028_EXT_EXP0", "ADS7028_EXT_EXP1", "ADS7028_EXT_EXP2",
              "ADS7028_VBATT", "LIS2DW12", "LIS3MDL", "ADS1292R_EXG1", "ADS1292R_EXG2"]
SPI_STAT = ["IDLE", "LSM6DSV_STATUS_GET", "LSM6DSV_GYRO_AND_ACCEL_GET", "LSM6DSV_ACCEL_GET",
            "LSM6DSV_GYRO_GET", "ADXL371_GET", "PRESSURE_TEMP_GET", "ADS7028_INT_EXP0_GET",
            "ADS7028_INT_EXP1_GET", "ADS7028_INT_EXP2_GET", "ADS7028_INT_EXP3_GET",
            "ADS7028_EXT_EXP0_GET", "ADS7028_EXT_EXP1_GET", "ADS7028_EXT_EXP2_GET",
            "ADS7028_VBATT_GET", "LIS2DW12_GET", "LIS3MDL_GET", "EXG1_GET", "EXG2_GET"]
HAL_I2C_STATE = {0x00: "RESET", 0x20: "READY", 0x24: "BUSY", 0x21: "BUSY_TX", 0x22: "BUSY_RX",
                 0x28: "LISTEN", 0x60: "ABORT", 0xA0: "TIMEOUT", 0xE0: "ERROR"}
HAL_SPI_STATE = {0: "RESET", 1: "READY", 2: "BUSY", 3: "BUSY_TX", 4: "BUSY_RX", 5: "BUSY_TX_RX",
                 6: "ERROR", 7: "ABORT"}
# GPDMA CSR (RM0456 17.8.7): bit 0 IDLEF, 8 TCF, 9 HTF, 10 DTEF, 11 ULEF, 12 USEF, 13 SUSPF, 14 TOF
DMA_CSR = {0: "IDLE", 8: "TC", 9: "HT", 10: "DTE", 11: "ULE", 12: "USE", 13: "SUSP", 14: "TO"}
# Exception numbers (IRQn + 16) on the STM32U5A5, stm32u5a5xx.h
EXC = {15: "SysTick", 18: "RTC", 27: "EXTI0", 28: "EXTI1", 29: "EXTI2", 30: "EXTI3", 31: "EXTI4",
       32: "EXTI5", 33: "EXTI6", 34: "EXTI7(LSM6DSV)", 35: "EXTI8(BT_WAKE)", 36: "EXTI9(VBUS)",
       37: "EXTI10(LIS2MDL)", 38: "EXTI11(BMP)", 39: "EXTI12(LIS2DW12)", 40: "EXTI13", 41: "EXTI14(BT_CONN)",
       42: "EXTI15(BT_CYSPP)", 45: "DMA0(BT_RX)", 46: "DMA1(BT_TX)", 47: "DMA2", 49: "DMA4(SPI1_RX)",
       50: "DMA5(SPI1_TX)", 51: "DMA6(SPI2_RX)", 52: "DMA7(SPI2_TX)", 53: "ADC1_2", 65: "TIM6",
       71: "I2C1_EV", 72: "I2C1_ER", 75: "SPI1", 76: "SPI2", 77: "USART1", 79: "USART3", 89: "OTG_HS",
       94: "SDMMC1", 96: "DMA8", 97: "DMA9", 98: "DMA10(I2C1_RX)", 99: "DMA11", 115: "SPI3", 128: "ADF1"}


def flags(v: int) -> str:
    return "+".join(n for b, n in STAGES.items() if v & b) or "-"


def csr(v: int) -> str:
    return ",".join(n for b, n in DMA_CSR.items() if v >> b & 1) or "0"


def name(table, i):
    if isinstance(table, dict):
        return table.get(i, f"?{i:#x}")
    return table[i] if 0 <= i < len(table) else f"?{i}"


def exc(n: int) -> str:
    return EXC.get(n, f"exc{n}")


def symbol_address(elf: Path) -> int:
    out = subprocess.run([str(NM), str(elf)], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == "gatherDiag":
            return int(parts[0], 16)
    raise SystemExit(f"gatherDiag not in {elf}: is it a GATHER_DIAG build?")


def where(elf: Path | None, pc: int) -> str:
    if not elf or not pc:
        return f"{pc:#010x}"
    r = subprocess.run([str(ADDR2LINE), "-f", "-s", "-e", str(elf), hex(pc & ~1)],
                       capture_output=True, text=True)
    lines = r.stdout.split()
    return f"{pc:#010x} {lines[0]} ({lines[1]})" if len(lines) >= 2 else f"{pc:#010x}"


def swd_read(addr: int, size: int, sn: str, dest: Path) -> bytes:
    args = [str(CLI), "-c", "port=SWD", f"sn={sn}", "mode=HOTPLUG", "-u", hex(addr), str(size), str(dest)]
    r = subprocess.run(args, capture_output=True, text=True, timeout=120)
    if r.returncode != 0 or not dest.exists():
        raise SystemExit(f"SWD read failed:\n{r.stdout[-1500:]}\n{r.stderr[-500:]}")
    return dest.read_bytes()


def swd_reset(sn: str) -> None:
    subprocess.run([str(CLI), "-c", "port=SWD", f"sn={sn}", "mode=HOTPLUG", "-rst"],
                   capture_output=True, text=True, timeout=120)


def snap_lines(s: dict, label: str) -> list[str]:
    i2c = s["i2c1Sens"]
    out = [f"  {label}: stall {s['stallCount']}, slot stamp {s['slotTs']:#010x}, "
           f"flags {flags(s['cbFlags'] & 0xFF)} of {flags(s['cbFlags'] >> 8)}, "
           f"I2C bus {s['i2cBus'] & 0xFF:#x}/{s['i2cBus'] >> 8:#x}, SPI bus {s['spiBus'] & 0xFF:#x}/{s['spiBus'] >> 8:#x}"]
    out.append(f"    i2c1Sens {name(I2C_STAT, i2c & 0xFF)} cnt {i2c >> 8 & 0xFF}/{i2c >> 16 & 0xFF}; "
               f"hi2c1 {name(HAL_I2C_STATE, s['hi2c1State'] & 0xFF)} mode {s['hi2c1State'] >> 8 & 0xFF:#x} "
               f"xfer {s['hi2c1State'] >> 16} err {s['hi2c1Err']:#x}; I2C1 ISR {s['i2c1Isr']:#010x}")
    for bus in ("spi1Sens", "spi2Sens"):
        v = s[bus]
        out.append(f"    {bus} {name(SPI_STAT, v & 0xFF)} cnt {v >> 8 & 0xFF}/{v >> 16 & 0xFF} "
                   f"at {name(SPI_SENSOR, v >> 24)}")
    out.append(f"    hspi1 {name(HAL_SPI_STATE, s['hspi1State'] & 0xFF)} err {s['hspi1Err']:#x}; "
               f"hspi2 {name(HAL_SPI_STATE, s['hspi2'] & 0xFF)} err {s['hspi2'] >> 16:#x}; "
               f"DMA I2C1 RX {csr(s['dmaI2c1Rx0'])} SPI1 RX {csr(s['dmaSpi1Rx0'])} SPI1 TX {csr(s['dmaSpi1Tx0'])}")
    g = s["gpio"]
    out.append(f"    pins LSM6DSV INT1 {g & 1}, LIS2MDL DRDY {g >> 1 & 1}, BMP INT {g >> 2 & 1}; "
               f"i2c stale {s['i2cStale']} seqErr {s['i2cSeqErr']}; ring stallResets {s['ring'] & 0xFFFF}")
    return out


def us(c: int, mhz: float) -> str:
    return f"{c / mhz:6.1f}" if c else "     -"


def trace_lines(t: dict, isr: list[int], mhz: float, elf: Path | None, full: bool, period: int | None) -> list[str]:
    ip = t["ipsr"].to_bytes(4, "little")
    fs = t["flagsSeen"].to_bytes(4, "little")
    sb = t["spiIpsrBefore"].to_bytes(4, "little")    # ipsr1, ipsr2, before1, before2
    sa = t["spiAfterDone"].to_bytes(4, "little")     # after1, after2, done1, done2
    sc = t["spiCounts"].to_bytes(4, "little")        # cplt1, cplt2, calls, spare
    ib = t["i2cBytes"].to_bytes(4, "little")         # halRet, rxStatus, rxIpsr, rxCount
    late = f" period {period} cyc" if period is not None else ""
    out = [f"    stamp {t['slotTs']:#010x}{late}  I2C {us(t['i2cCyc'], mhz)} ({exc(ip[1])}, saw {flags(fs[1])})  "
           f"SPI {us(t['spiCyc'], mhz)} ({exc(ip[2])}, saw {flags(fs[2])})  done {us(t['doneCyc'], mhz)} us"]
    if not full:
        return out
    out.append(f"      RTC preempted {where(elf, t['rtcPc'])}, lr {t['rtcLr']:#010x}; main task {t['execTask']:#x}")
    out.append(f"      SPI poll {us(t['spiPollCyc'], mhz)}; bus1 done {us(t['spiBus1Cyc'], mhz)} in {exc(sb[0])} "
               f"flags {sb[2]}->{sa[0]} done {sa[2]}; bus2 done {us(t['spiBus2Cyc'], mhz)} in {exc(sb[1])} "
               f"flags {sb[3]}->{sa[1]} done {sa[3]}; calls {sc[2]}, SPI1 cplt {sc[0]}, SPI2 cplt {sc[1]}")
    out.append(f"      I2C start {us(t['i2cStartCyc'], mhz)} hal {ib[0]}; rx {us(t['i2cRxCyc'], mhz)} in {exc(ib[2])} "
               f"found {name(I2C_STAT, ib[1])} (rx count {ib[3]})")
    ev = []
    for w in isr[:t["nIsr"]]:
        ev.append(f"{'-' if w >> 31 else '+'}{exc(w >> 23 & 0xFF)}@{(w & 0x7FFFFF) / mhz:.1f}")
    out.append("      irqs: " + " ".join(ev))
    return out


def decode(raw: bytes, mhz: float, elf: Path | None) -> None:
    if len(raw) < TOTAL_WORDS * 4:
        raise SystemExit(f"dump is {len(raw)} bytes, expected {TOTAL_WORDS * 4}")
    w = struct.unpack_from(f"<{TOTAL_WORDS}I", raw)
    pos = 0
    head = dict(zip(HEAD_FIELDS, w[pos:pos + len(HEAD_FIELDS)]))
    pos += len(HEAD_FIELDS)
    if head["magic"] != MAGIC:
        raise SystemExit(f"magic {head['magic']:#x}: no session has run since reset, or not a GDG2 build")
    if head["size"] != TOTAL_WORDS * 4:
        raise SystemExit(f"firmware struct is {head['size']} bytes, this reader expects {TOTAL_WORDS * 4}")
    pos += SNAP_WORDS + 1 + TRACE_LEN * TRACE_WORDS        # pending, liveHead, live
    print(f"sessions {head['sessions']}, gathers {head['gathers']}, completed {head['completes']}, "
          f"stall resets {head['stallResets']}, lost updates {head['lostUpdates']}, late starts {head['lateStarts']}")
    print(f"busy ticks {head['busyTicks']}, gathers completed late {head['lateCompletes']} "
          f"(most busy ticks {head['maxLateStall']})")
    print(f"stage completions in the RTC interrupt: ADC {head['syncAdc']} I2C {head['syncI2c']} SPI {head['syncSpi']}; "
          f"elsewhere: ADC {head['asyncAdc']} I2C {head['asyncI2c']} SPI {head['asyncSpi']}")
    print(f"I2C error callbacks {head['i2cErrors']}, SPI error callbacks {head['spiErrors']}")
    for n in range(min(head["nEvents"], EVENTS)):
        ev = w[pos + n * EVENT_WORDS: pos + (n + 1) * EVENT_WORDS]
        session, into = ev[0], ev[1]
        first = dict(zip(SNAP_FIELDS, ev[2:2 + SNAP_WORDS]))
        reset = dict(zip(SNAP_FIELDS, ev[2 + SNAP_WORDS:2 + 2 * SNAP_WORDS]))
        base = 2 + 2 * SNAP_WORDS
        traces = []
        for i in range(TRACE_LEN):
            tw = ev[base + i * TRACE_WORDS: base + (i + 1) * TRACE_WORDS]
            traces.append((dict(zip(TRACE_FIELDS, tw[:len(TRACE_FIELDS)])), list(tw[len(TRACE_FIELDS):])))
        lost = traces[-1][0]
        expected = reset["cbFlags"] >> 8
        seen = (0x01 if lost["adcCyc"] else 0) | (0x02 if lost["i2cCyc"] else 0) | (0x08 if lost["spiCyc"] else 0)
        missing = expected & ~seen
        verdict = (f"stage(s) never reached ShimSens_stageCompleteCb: {flags(missing)}" if missing
                   else "every stage arrived but the flags never matched: a lost flag update")
        print(f"\nstall {n + 1}: session {session}, {into / 32768:.2f} s after sensing start; {verdict}")
        print("\n".join(snap_lines(first, "first busy tick")))
        print("\n".join(snap_lines(reset, "at the reset")))
        print("  last gathers, oldest first (the last is the lost one; the last three in full):")
        prev = None
        for i, (t, isr) in enumerate(traces):
            if not (t["startCyc"] or t["slotTs"]):
                continue
            period = (t["startCyc"] - prev) & 0xFFFFFFFF if prev is not None else None
            prev = t["startCyc"]
            print("\n".join(trace_lines(t, isr, mhz, elf, i >= TRACE_LEN - 3, period)))
    if head["nEvents"] > EVENTS:
        print(f"\n{head['nEvents'] - EVENTS} later stalls were counted but not kept")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", type=Path, required=True)
    ap.add_argument("--sn", help="ST-Link serial")
    ap.add_argument("--file", type=Path, help="decode a saved dump instead of reading the unit")
    ap.add_argument("--save", type=Path, help="keep the raw dump here")
    ap.add_argument("--reset", action="store_true", help="reset the unit after reading")
    ap.add_argument("--mhz", type=float, default=48.65, help="core clock (MSI range 0, measured), for cycle counts")
    a = ap.parse_args()
    if a.file:
        raw = a.file.read_bytes()
    else:
        if not a.sn:
            raise SystemExit("--sn is needed to read the unit")
        addr = symbol_address(a.elf)
        dest = a.save or Path(tempfile.mkdtemp()) / "gather_diag.bin"
        raw = swd_read(addr, TOTAL_WORDS * 4, a.sn, dest)
        print(f"read {len(raw)} bytes from {addr:#010x} into {dest}")
        if a.reset:
            swd_reset(a.sn)
            print("unit reset")
    decode(raw, a.mhz, a.elf)


if __name__ == "__main__":
    sys.exit(main())
