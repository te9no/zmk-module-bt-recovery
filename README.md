# ZMK Bluetooth Recovery

Experimental, bounded recovery for **failed split scan/advertisement starts**.
Not a general freeze detector and not a proven cure for long-idle keyboard
failures. No hardware endurance testing has been performed.

## Scope and safety

Default OFF. No periodic polling, new thread, heap allocation, bond deletion,
stack reinitialization, working-link disconnect or reset. Normal disconnects
remain ZMK's responsibility. A real failed start schedules up to five retries
at 1/2/4/8/16 seconds, after a 5-second boot/wake grace. Exhaustion stops module
retries without disabling standard reconnection. Duplicate errors do not reset
the budget or postpone the pending deadline. A successful start or transport
disable/enable begins a new episode.

Central and peripheral share only the bounded policy; each retries through its
own ZMK owner. Central distinguishes peers using ZMK split-slot membership, not
the BLE role alone. A host disconnect never becomes a peripheral event. Multiple
peers are tracked independently in a bitmap; another peer connecting does not
cancel a retry for the missing peer.

States: DISCONNECTED, CONNECTED, SCANNING/ADVERTISING, RETRY_PENDING, RECOVERING,
SUSPENDED, RECOVERY_FAILED. CONNECTED means a link event, not verified key delivery.
SCANNING/ADVERTISING means a successful start, not peer connectivity. No retry
is triggered just because nobody is typing or a peer is powered off.

## Installation (pinned integration required)

ZMK's split start functions are private; public events cannot reliably report
start failure or safely restart their owners. This module therefore requires
an explicit [version-locked bridge](patches/README.md), not invasive symbol hooks.

1. Pin upstream ZMK to `4dd95d7840eb3d5a3f59bb29fbf1ab054715ef85`.
2. Add this module to the west manifest with URL
   `https://github.com/te9no/zmk-module-bt-recovery` and pin the reviewed module
   commit. Run west update.
3. Run `python3 path/to/module/scripts/apply_bridge.py path/to/zmk`.
   Apply this step again after a fresh checkout/update, before west build.
4. Add to both central and peripheral config:

```conf
CONFIG_ZMK_BT_RECOVERY=y
# Optional, normally off:
CONFIG_ZMK_BT_RECOVERY_DIAGNOSTICS=y
CONFIG_ZMK_BT_RECOVERY_LOG_LEVEL_INF=y
```

CMake fails closed if enabled without the bridge. OFF compiles no module sources.
No hardware files, keymaps or existing user settings are changed.

## Configuration

| Symbol (CONFIG_ prefix) | Default | Range / meaning |
| --- | --- | --- |
| ZMK_BT_RECOVERY | n | split BLE only |
| ZMK_BT_RECOVERY_MAX_RETRIES | 5 | 1–10 |
| ZMK_BT_RECOVERY_RETRY_DELAY_MS | 1000 | 1000–60000 |
| ZMK_BT_RECOVERY_MAX_DELAY_MS | 16000 | 1000–300000, at least initial delay |
| ZMK_BT_RECOVERY_GRACE_MS | 5000 | 1000–120000 |
| ZMK_BT_RECOVERY_DIAGNOSTICS | n | start results, transitions, peer/reason logs |
| ZMK_BT_RECOVERY_LOG_LEVEL | standard Zephyr log choice | OFF/ERR/WRN/INF/DBG |

Supports up to 32 peer slots; invalid IDs are rejected before shifts/indexing.
Snapshots via `zmk_bt_recovery_snapshot()` include state, retry count, last
error, link bitmap, last connection and last received split event timestamp.
Zero timestamps mean unobserved, not healthy. Peripheral last_traffic currently
remains unknown; advertising and connection events do not prove data delivery.
Diagnostics logs are event-driven, never per-key. No new Studio RPC is provided.

## Sleep

ZMK activity SLEEP cancels pending work; normal IDLE is not a fault. Recovery
work uses sysworkq (the sleep-entry queue), and its owner rechecks SLEEP before
starting. Resume supplies grace but does not itself restart radio activity;
ZMK retains ownership. nRF52 system-off wake normally reboots and initializes
the policy anew. Sleep without a running scheduler, broken wake hardware,
silent connected-link stalls and host-side HID failures are outside scope.

See [upstream audit and watchdog boundary](docs/upstream-audit.md).

## Tests and known limits

`sh tests/run.sh` runs native C policy tests with ASan/UBSan.
The CI additionally applies the pinned patch and builds nRF52840 Corne
central/peripheral and OFF variants. Tests do not substitute for RF hardware,
deep-sleep power measurements or long-idle endurance trials.

Before deployment: keep known-good UF2s; test each half, host disconnect,
two peers, radio loss/reconnect, injected transient start failures, sleep/wake,
and overnight idle. Do not erase bonds as a recovery step. Do not treat a
successful build or start as evidence that the original freeze is fixed.
