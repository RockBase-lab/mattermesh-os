"""Apply the upstream NimBLE host-timer shutdown race fix to NimBLE-Arduino 2.3.6.

NimBLE can dequeue ble_hs_timer.ev after nimble_port_stop() has cleared its
callback. Keep the event valid until ble_hs_deinit(), after the host task has
left nimble_port_run(). The matching change is in current esp-nimble.
"""

from pathlib import Path

Import("env")

source = (
    Path(env.subst("$PROJECT_LIBDEPS_DIR"))
    / env.subst("$PIOENV")
    / "NimBLE-Arduino/src/nimble/nimble/host/src/ble_hs.c"
)
old = """    if (!ble_hs_is_enabled()) {
        ble_npl_callout_stop(&ble_hs_timer);
        ble_npl_callout_deinit(&ble_hs_timer);
    } else {"""
new = """    if (!ble_hs_is_enabled()) {
        ble_npl_callout_stop(&ble_hs_timer);
    } else {"""

if not source.is_file():
    raise RuntimeError(f"NimBLE source not found: {source}")

text = source.read_text(encoding="utf-8")
if old in text:
    if text.count(old) != 1:
        raise RuntimeError(f"Unexpected NimBLE host-timer definitions: {source}")
    source.write_text(text.replace(old, new), encoding="utf-8")
    print("Applied NimBLE host-timer shutdown race fix")
elif new not in text:
    raise RuntimeError(
        f"NimBLE host-timer code changed; review the shutdown patch: {source}"
    )
