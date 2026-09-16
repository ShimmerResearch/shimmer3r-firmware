#!/usr/bin/env bash
#
# Guards hand-written code against CubeMX regeneration.
#
# Regenerating from the .ioc rewrites everything CubeMX owns. This project keeps
# hand-written code inside those regions, so an ordinary Generate Code deletes
# it silently - DEV-1017 records one CRC deactivation that also removed 165
# lines from main.c and changed ten other files with no warning.
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
# Loss is not always deletion. Regenerating app_usbx_device.h left the 640 KB
# pool define in place and ADDED the stock 128 KB one beside it - a redefinition
# where whichever comes last wins. Hence require_once, which fails on two
# definitions as well as on none.
#
# Adding an entry: pick the shortest string unique to the hand-written version,
# not to the generated one, so the check fails when generation wins. Prefer an
# identifier or a define name over exact spacing, which a reformat can move.

set -uo pipefail

cd "$(dirname "$0")/.." || exit 2
B=LogAndStream_Shimmer3R
fail=0

report() { # status, file, what, [detail]
  printf '  %-12s %-38s %s\n' "$1" "$(basename "$2")" "$3"
  [ -n "${4:-}" ] && printf '               %s\n' "$4"
}

require() { # file, fixed string, what it protects
  [ -f "$1" ] || { report "MISSING FILE" "$1" "$3" "$1"; fail=1; return; }
  if grep -qF -- "$2" "$1"; then report ok "$1" "$3"
  else report LOST "$1" "$3" "$1 no longer contains: $2"; fail=1; fi
}

require_re() { # file, extended regex, what it protects
  [ -f "$1" ] || { report "MISSING FILE" "$1" "$3" "$1"; fail=1; return; }
  if grep -qE -- "$2" "$1"; then report ok "$1" "$3"
  else report LOST "$1" "$3" "$1 has no line matching: $2"; fail=1; fi
}

require_once() { # file, extended regex, what it protects - exactly one match
  [ -f "$1" ] || { report "MISSING FILE" "$1" "$3" "$1"; fail=1; return; }
  n=$(grep -cE -- "$2" "$1")
  if [ "$n" -eq 1 ]; then report ok "$1" "$3"
  elif [ "$n" -eq 0 ]; then report LOST "$1" "$3" "$1 has no line matching: $2"; fail=1
  else report DUPLICATED "$1" "$3" "$1 has $n lines matching: $2 (regeneration added the stock one)"; fail=1; fi
}

forbid() { # file, fixed string, why it must not be there
  if grep -qF -- "$2" "$1" 2>/dev/null; then report REAPPEARED "$1" "$3" "$1 contains: $2"; fail=1
  else report ok "$1" "$3"; fi
}

echo "Hand-written code that CubeMX regeneration is known to delete:"
require      "$B/Core/Src/main.c"    "LSE_START_TIMEOUT_MS"          "DEV-866 LSE drive ladder"
require      "$B/Core/Src/rtc.c"     "Boot_rtcIsOnLsiFallback"       "DEV-866 LSI limp-home (else a dead LSE hangs boot)"
require      "$B/Core/Src/sdmmc.c"   "hsd1.Instance = NULL"          "hot-swap tolerance, no Error_Handler() on SD init"
require      "$B/Core/Src/usb_otg.c" "Init.speed = USB_getPcdSpeed"  "PCD speed selection"
require      "$B/USBX/App/app_usbx_device.c" "ALIGN_32BYTES"         "32-byte D-cache line alignment of the USBX pool"
require      "$B/USBX/App/app_usbx_device.c" "UX_DEVICE_APP_MEM_POOL_SIZE, UX_NULL" \
                                                                     "correct pool size passed to ux_system_initialize()"
require_once "$B/USBX/App/app_usbx_device.h" '^#define UX_DEVICE_APP_MEM_POOL_SIZE[[:space:]]' \
                                                                     "one pool-size define (regen adds a stock 128 KB one)"
require      "$B/USBX/App/app_usbx_device.h" "640U"                  "640 KB pool (else UX_MEMORY_INSUFFICIENT at 64 KB MSC)"
require      "$B/USBX/App/ux_device_descriptors.c" "ShimEeprom_getBrandUsbManufacturer" \
                                                                     "EEPROM brand string in the USB descriptor"
require      "$B/USBX/App/ux_device_descriptors.h" "USBD_CDCACM_EPINCMD_FS_BINTERVAL 0x10U" \
                                                                     "CDC int-IN bInterval FS (Mac xHCI MSC-drop fix)"
require      "$B/USBX/App/ux_device_descriptors.h" "USBD_CDCACM_EPINCMD_HS_BINTERVAL 0x09U" \
                                                                     "CDC int-IN bInterval HS (Mac xHCI MSC-drop fix)"
require_re   "$B/USBX/App/ux_user.h" '^#define UX_SLAVE_REQUEST_DATA_MAX_LENGTH[[:space:]]' \
                                                                     "active 64 KB MSC transfer define (stock is commented out)"

echo
echo "Generated code that must NOT come back:"
forbid       "$B/Core/Src/main.c"    "MX_USBX_Device_Init"           "duplicate init - usb_otg.c already calls it"

echo
if [ "$fail" -ne 0 ]; then
  cat <<'MSG'
FAILED. This almost always means someone regenerated from CubeMX and committed
the result without reading the diff. The lost code is in git - restore it from
the parent commit. See the CubeMX section of AGENTS.md, and DEV-1017.
MSG
  exit 1
fi
echo "All guards intact."
