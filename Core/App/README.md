# CDC nanopb telemetry

`cdc_nanopb.c` emits one protobuf `bldc.Telemetry` message every 250 ms after
USB CDC is configured. Each message is framed as `COBS(protobuf) + 0x00`, so
host software must split the byte stream on `0x00`, COBS-decode the segment,
then protobuf-decode the resulting payload using `protocol/bldc.proto`.

The protobuf fields are:

| Field | Meaning |
| --- | --- |
| `protocol_version` | Protocol version (`1`) |
| `sequence` | Sequence number |
| `uptime_ms` | Device uptime in milliseconds |
| `bus_voltage_mv` | DC bus voltage in mV |
| `phase_current_ma` | Phase current in mA |
| `motor_rpm` | Motor speed in RPM |
| `mosfet_temperature_cdec` | MOSFET temperature in 0.1 °C |

Values are currently deterministic pseudo-random test data. Field numbers in
`bldc.proto` are wire-contract identifiers: do not renumber or reuse removed
field numbers. LEDA (PB14) toggles after each completed USB telemetry transfer.

## Host monitor

Install its only Python dependency, then read the default USB CDC device:

```sh
python3 -m pip install -r tools/requirements.txt
python3 tools/print_telemetry.py
```

Pass a device path explicitly when it is not `/dev/ttyACM0`:

```sh
python3 tools/print_telemetry.py /dev/ttyACM1
```

The monitor uses `protocol/bldc_pb2.py`, generated from the same `.proto` as
the firmware, and restores the TTY configuration when it exits.

`cdc_usb_bridge.c` is the sole USB-Cube-facing adapter. The generated CDC file
contains only protected callback forwarding calls, so all application behavior
remains under `Core/App`.
