# CDC CBOR telemetry

`cdc_cbor.c` emits one telemetry message every 250 ms after USB CDC is
configured. Each message is framed as `COBS(CBOR) + 0x00`, so host software
must split the byte stream on `0x00`, COBS-decode the segment, then CBOR-decode
the resulting payload.

The CBOR message is a map with compact integer keys:

| Key | Meaning |
| --- | --- |
| `0` | Protocol version (`1`) |
| `1` | Message type (`1` = telemetry) |
| `2` | Sequence number |
| `3` | Device uptime in milliseconds |
| `4` | Telemetry map |

Telemetry-map keys are `0` for DC bus voltage (mV), `1` for phase current
(mA), `2` for motor speed (RPM), and `3` for MOSFET temperature (0.1 °C).
Values are currently deterministic pseudo-random test data.

`cdc_usb_bridge.c` is the sole USB-Cube-facing adapter. The generated CDC file
contains only protected callback forwarding calls, so all application behavior
remains under `Core/App`.
