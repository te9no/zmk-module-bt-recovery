# ZMK Bluetooth Recovery (experimental)

External Zephyr module for investigating BLE reconnection failures on ZMK split keyboards.

**Status: scaffold only.** No automatic recovery is implemented yet. Enabling the Kconfig option does not alter BLE behavior.

## Goals
- Diagnose central/peripheral disconnects and failed scan restarts.
- Avoid interfering with normal idle/deep sleep and pairing bonds.
- Implement bounded retries only where ZMK exposes safe APIs.
- Prevent duplicate work with upstream fixes (ZMK issues #3522 and PRs #3538/#3539).
- Support reproducible tests before device deployment.

## Integration
Add this repository to the ZMK west manifest under `projects` with `name: zmk-module-bt-recovery`, `url: https://github.com/te9no/zmk-module-bt-recovery`, and a pinned `revision`. Then enable `CONFIG_ZMK_BT_RECOVERY=y` in the appropriate central-side configuration.

This is a **no-op** until implementation and testing are complete. Do not rely on it to recover stuck keyboards.

## Safety constraints
Never clear bonding data automatically. Do not forcibly reset a working BLE connection. Avoid polling during suspend/deep sleep. Rate-limit retries and record diagnostic events. Any fix requiring changes to ZMK internal code should be a separate version-pinned patch, not a fragile hook.

## Next steps
1. Pin target ZMK/Zephyr revision and verify existing upstream fixes.
2. Reproduce the disconnect and capture logs from both halves.
3. Add tests for disconnection, scan failure, deep sleep, and repeated retries.
4. Implement guarded recovery using supported APIs; validate on hardware.
