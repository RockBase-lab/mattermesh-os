# RockBase NM-EPD-420-BW

An independent Free firmware target derived from the T-Deck Max application.
The board has an ESP32-S3 N16R8, SX1262, 400x300 monochrome e-paper panel and two buttons.
It has no keyboard, touchscreen, GPS receiver, frontlight or vibration motor.
Audio and offline dictation are disabled; the codec, amplifier and AHT20 are powered off.

## Build and flash

From the repository root:

```sh
bash scripts/fetch-deps.sh
(cd nm-epd-420 && pio run -e nm-epd-420-bw-free)
(cd nm-epd-420 && pio run -e nm-epd-420-bw-free -t upload)
```

The upload target flashes the bootloader, partition table, OTA data and application.
For an application-only update on an already compatible partition layout, flash
`.pio/build/nm-epd-420-bw-free/firmware.bin` at `0x10000`.
Community prebuilt images are available from [RockBase](https://flash.rockbaseiot.com).
Source builds have no default OTA server.

Host regression tests (C++17 compiler and Bash required):

```sh
(cd nm-epd-420 && bash tools/test_release.sh)
```

The suite includes NM landscape geometry and two-button unread-message browsing,
plus the inherited application tests. Physical checks are still needed for flashing,
panel waveforms, LoRa receive during standby, radio reconnects and battery current.

## Hardware and behavior

The NM-EPD-420 panel uses native partial refresh for changed regions, with periodic full-refresh maintenance. NM-EPD-420 defaults to External Antenna. Its reference board has no software-controlled RF selection pin, so the antenna choice is a persisted UI/configuration preference rather than an XL9555 hardware switch. Battery voltage is sampled from GPIO3 through the 2:1 divider; GPIO43 is enabled only during sampling. The serial command `battery status` reports `mv`, `percent`, raw `adc` and `pin_mv` for hardware verification.

On NM-EPD-420, locking the screen suspends idle Wi-Fi without changing the saved preference; an active TCP Companion link or an NTP, weather or OTA request keeps a temporary Wi-Fi lease. Idle BLE advertising stops immediately and BLE is deinitialized after 60 seconds, then restored on unlock. With both radios asleep, NM enters Light Sleep for up to 100 ms per pass while the SX1262 remains in continuous RX. DIO1, either button or the service timer wakes the CPU; an attached USB host prevents Light Sleep to keep diagnostics and Companion links stable. The serial commands `sleep status`, `wifi status` and the `WIFI_STANDBY` / `BLE_STANDBY` logs expose sleep counters and radio state.

On the first NM-EPD-420 boot, **Connect to Wi-Fi** starts a temporary WPA2 setup hotspot instead of requiring password entry with two buttons. **Settings → Wi-Fi** opens the same hotspot for later reconfiguration. Join the `MatterMesh-Setup-XXXX` network using the fixed password `12345678`, then open `http://192.168.4.1`. The phone form configures the Wi-Fi SSID/password, 868/900 MHz band, country/network preset, time zone, Companion Name and 1–3 byte Path hash size. Saving opens a **Ready** page: hold USER on the device to stop the hotspot and reconnect Wi-Fi immediately, or hold BOOT to restart. Hold USER before saving to cancel the hotspot and return to the previous screen.

NM-EPD-420 controls:

- Navigation: BOOT moves down/next, USER moves up/previous; hold BOOT to confirm and hold USER to go back.
  Hold USER on Home to lock. On Maps, short presses zoom out/in.
- Text editing: BOOT/USER cycle a provisional character forwards/backwards. Hold BOOT to keep that character;
  with no provisional character, hold BOOT again to confirm the field or send. Hold USER deletes the provisional
  character or backspaces; on an empty field it exits.
- Lock screen: BOOT/USER browse older/newer messages, hold BOOT to unlock, and hold USER to open Quick Reply.

