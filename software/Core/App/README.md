# CDC nanopb telemetry

`telemetry.c` emits one protobuf `bldc.Telemetry` message every 250 ms after
USB CDC is configured. Each message is framed as `COBS(protobuf) + 0x00`, so
host software must split the byte stream on `0x00`, COBS-decode the segment,
then protobuf-decode the resulting payload using [`protocol/bldc.proto`](../../protocol/bldc.proto).

All published measurement fields are live ADC readings. Field numbers
in `bldc.proto` are wire-contract identifiers: do not renumber or reuse
removed field numbers. LEDC (PB12) toggles after completed USB telemetry
transfers, at most once every 500 ms.

`VM` uses a 330 kΩ / 10 kΩ divider (34:1); its 3.3 V zener protection clamp is
well above the expected 10–12 V operating range. Each phase-voltage input uses
a 91 kΩ / 4.7 kΩ divider (approximately 20.36:1). `NTC_PCB` uses a 10 kΩ,
β3435 NTC and a 1 kΩ fixed resistor, driven from
3.3 V. The firmware measures VDDA through the STM32 VREFINT calibration value
before converting the NTC reading, so it remains correct when VDDA differs
from the 3.3 V divider supply. MCU die temperature is sampled from
`ADC_CHANNEL_TEMPSENSOR_ADC1` with a long sampling time using the factory TS_CAL values. The result is published as tenths of a degree C.

The DRV8323 current-sense amplifiers run in bidirectional mode:
each `SOx` output is biased at `VREF/2` (3.3V/2 analog rail) at zero
current. At 20 V/V and a 2 mΩ phase shunt that is 25 mA (-12.5 to 12.5) per millivolt.

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

The generated CDC file contains only protected callback forwarding calls; all
telemetry behavior, ADC sampling, framing, and transmit state remain in
`Core/App/telemetry.c`.
