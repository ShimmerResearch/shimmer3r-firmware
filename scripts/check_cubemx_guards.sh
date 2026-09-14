#!/usr/bin/env bash
#
# Guards hand-written code against CubeMX regeneration.
#
# Regenerating from the .ioc rewrites everything CubeMX owns. This project keeps
# hand-written code inside those regions, so an ordinary Generate Code deletes
# it silently - DEV-1017 records one CRC deactivation that also removed 165
# lines from main.c and changed seven other files with no warning.
#
# Why this is a checker and not a refactor: almost none of the at-risk code can
# be moved into a USER CODE block, because it is not code sitting BESIDE
# generated lines - it IS a generated line, edited in place. rtc.c changes the
# RTCClockSelection assignment CubeMX emits; sdmmc.c replaces the Error_Handler()
# call inside a generated if; app_usbx_device.c changes the buffer declaration
# and an argument to ux_system_initialize(). A USER CODE block cannot protect a
# statement that has to differ from the one the template produces. So instead of
# pretending it can be made safe, this makes the loss loud.
#
# Adding an entry: pick the shortest string unique to the hand-written version,
# not to the generated one, so the check fails when generation wins.

set -uo pipefail

cd "$(dirname "$0")/.." || exit 2
B=LogAndStream_Shimmer3R
fail=0

require() { # file, pattern, what it protects
  if [ ! -f "$1" ]; then
    printf '  MISSING FILE  %s\n' "$1"; fail=1; return
  fi
  if grep -qF -- "$2" "$1"; then
    printf '  ok            %-38s %s\n' "$(basename "$1")" "$3"
  else
    printf '  LOST          %-38s %s\n' "$(basename "$1")" "$3"
    printf '                  %s no longer contains: %s\n' "$1" "$2"
    fail=1
  fi
}

forbid() { # file, pattern, why it must not be there
  if grep -qF -- "$2" "$1" 2>/dev/null; then
    printf '  REAPPEARED    %-38s %s\n' "$(basename "$1")" "$3"
    printf '                  %s contains: %s\n' "$1" "$2"
    fail=1
  else
    printf '  ok            %-38s %s\n' "$(basename "$1")" "$3"
  fi
}

echo "Hand-written code that CubeMX regeneration is known to delete:"
require "$B/Core/Src/main.c"   "LSE_START_TIMEOUT_MS"    "DEV-866 LSE drive ladder"
require "$B/Core/Src/rtc.c"    "Boot_rtcIsOnLsiFallback" "DEV-866 LSI limp-home (else a dead LSE hangs boot)"
require "$B/Core/Src/sdmmc.c"  "hsd1.Instance = NULL"    "hot-swap tolerance, no Error_Handler() on SD init"
require "$B/Core/Src/usb_otg.c" "Init.speed = USB_getPcdSpeed" "PCD speed selection"
require "$B/USBX/App/app_usbx_device.c" "ALIGN_32BYTES"  "32-byte D-cache line alignment of the USBX pool"
require "$B/USBX/App/app_usbx_device.c" "UX_DEVICE_APP_MEM_POOL_SIZE, UX_NULL" \
                                                         "correct pool size to ux_system_initialize()"
require "$B/USBX/App/ux_device_descriptors.c" "ShimEeprom_getBrandUsbManufacturer" \
                                                         "EEPROM brand string in the USB descriptor"
require "$B/USBX/App/ux_user.h" "UX_SLAVE_REQUEST_DATA_MAX_LENGTH (64 * 1024)" \
                                                         "64 KB MSC transfer size (default is 2048)"

echo
echo "Generated code that must NOT come back:"
forbid "$B/Core/Src/main.c" "MX_USBX_Device_Init" \
       "duplicate init - usb_otg.c already calls it"

echo
if [ "$fail" -ne 0 ]; then
  cat <<'MSG'
FAILED. This almost always means someone regenerated from CubeMX and committed
the result without reading the diff. The lost code is in git - restore it from
the parent commit. See the CubeMX section of CLAUDE.md, and DEV-1017.
MSG
  exit 1
fi
echo "All guards intact."
