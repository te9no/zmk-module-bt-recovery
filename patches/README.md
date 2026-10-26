# Pinned core integration

Target: upstream zmkfirmware/zmk `4dd95d7840eb3d5a3f59bb29fbf1ab054715ef85`.
Apply explicitly with `python3 scripts/apply_bridge.py /path/to/zmk`.
The helper refuses other HEADs and is idempotent. It does not reset user changes.
Nothing patches sources at configure time.

The patch contains two kinds of change:
1. Core correctness prerequisites: clear failed scan state; reject non-slot
   connections in connected/disconnected callbacks; reject out-of-range source
   IDs before dispatch (including battery writes).
2. Optional bridge: report actual start results, valid split link events and
   received BLE key-state packets (after length validation); expose a role-specific retry entry guarded by the
   existing transport-enabled/connection state and activity Sleep state.

The core prerequisites remain with the module OFF. They are intentional fixes
to unsafe upstream behavior, not background recovery. All optional hook calls
compile away when OFF. A completely unpatched upstream build with the module
OFF is also supported. Never apply this patch to a newer revision by force.

No weak symbols, linker wrapping, static-function replacement, bt_disable(),
bond erasure, or periodic reboot. We call the *existing owner's* start function,
preserving its connection filters, slot allocation, advertising data, bond
selection and low-duty fallback.

On a future upstream version containing #3538/#3539, port the bridge and remove
overlapping retry ownership; do not stack two retry engines. No compatibility
claim is made for that version until separately built and tested.
