# Validation record

Initial implementation, 2026-10-26. Target upstream ZMK
4dd95d7840eb3d5a3f59bb29fbf1ab054715ef85 plus the explicit bridge.
Local Zephyr: 10ba6d0cb38bc3d258775d27982f707599320085 (4.1).

## Local native tests

ASan/UBSan C policy tests pass: normal link/disconnect does not schedule recovery;
injected scan/advertising errors; exponential delays; duplicate errors and
deadline preservation; five attempts and exhaustion; standard success after
exhaustion; multiple peer slots; negative/234 slot rejection; suspend/cancel,
wake grace, transport disable, saturating delay.

Actual source-function mock tests pass for pinned start_scanning,
split_central_connected/disconnected and advertising_cb: failed scan clears
the cached flag; healthy scan is not restarted; host and non-split central-role
connections are excluded; valid peer disconnect queues the correct battery
index; fully occupied slots do not scan; connected/disabled peripherals do not
start advertising; -EALREADY is treated as already advertising, not a retry storm.

These are deterministic mocks, not RF/Zephyr scheduler tests.

## Local Zephyr firmware builds

nice_nano//zmk (nRF52840), Corne:
| Build | Flash bytes | RAM bytes | Result |
| --- | ---: | ---: | --- |
| central, recovery + diagnostics + sleep | 233484 | 52408 | PASS |
| peripheral, recovery + diagnostics + sleep | 193408 | 36568 | PASS |
| central, recovery OFF | 231120 | 52176 | PASS |

Local builds reuse the existing workspace dependency checkout (additional ZMK
modules are discovered); they are not a clean-room dependency validation.
CI separately fetches the pinned upstream manifest and builds these variants.
The approximate local central delta is 2364 flash / 232 RAM bytes, including
diagnostics and linker-layout effects, not a universal memory guarantee.
No additional thread stack is allocated by recovery.

## Hardware acceptance still required

All RF and endurance items remain **UNTESTED**: actual host/peripheral reconnect,
start-failure fault injection on hardware, concurrent multiple peers, overnight
idle, DeepSleep power-off/wake, current draw, and interaction with other modules.
No keyboard has been flashed as part of this task. No claim is made that the
MeKaBu/Ananta long-idle freeze is fixed.

Other ZMK commits/forks are **unsupported**, not tested compatible. The installer
refuses them rather than guessing that similarly named releases are equivalent.
