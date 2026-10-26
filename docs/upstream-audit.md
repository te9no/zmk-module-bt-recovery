# Upstream audit — 2026-10-26

Checked the live GitHub issue/PR metadata and upstream source. Initial upstream
target: 4dd95d7840eb3d5a3f59bb29fbf1ab054715ef85, Zephyr 4.1 manifest.
These observations do not establish the cause of the MeKaBu/Ananta idle failure.

- [#3522](https://github.com/zmkfirmware/zmk/issues/3522): OPEN. At the target,
  disconnected() can turn -EINVAL into uint8_t 234 and enqueue a battery event;
  the battery setter lacks a bounds check. The patch checks the slot *before*
  release and validates the central event source before any dispatch/write.
  Host links and third-party central-role links with no split slot are ignored.
- [#3538](https://github.com/zmkfirmware/zmk/pull/3538): OPEN, not merged;
  head f31da0fc54551df1437e8ab5c3463c724725edef. Target start_scanning marks
  is_scanning before bt_le_scan_start and leaves it true on failure. Our pinned
  patch clears that flag and delegates bounded retry to this module. The PR's
  retry loop is NOT also applied.
- [#3539](https://github.com/zmkfirmware/zmk/pull/3539): OPEN, not merged.
  The target connected callback uses role only, which includes non-split BLE
  central connections. Our patch checks slot membership; role alone is not an
  identity test.
- [#3195](https://github.com/zmkfirmware/zmk/issues/3195): OPEN; report concerns
  Studio + deep sleep after the c06fa48c Zephyr 4.1 update on nice!nano.
  This is not established as the same failure as a failed scan/advertisement.
  Software retry cannot run while the CPU is powered off. No wake-pin or PM
  workaround is silently applied.

No fixed release/tag can be claimed for these open PRs/issues from the reviewed
metadata. The patch helper accepts only the exact target, not arbitrary 0.3/0.4
labels, forks or future main. Other versions are unsupported pending re-audit.

## Watchdog is a separate design, not implemented here

Zephyr's watchdog API (wdt_install_timeout, wdt_setup, wdt_feed) may detect
scheduler/CPU stalls when the hardware watchdog still runs. It does not diagnose
BLE health. Reset can lose unsaved settings and disrupt normal sleep; Nordic
watchdog sleep/debug pause behavior and any existing watchdog owner must be
reviewed per board. A future watchdog feature must be opt-in, single-owner,
sleep-aware and test reset reason/persistent crash evidence. This module never
takes ownership of a watchdog or automatically reboots.
