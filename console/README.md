# BLDC Console

Electron + React instrumentation dashboard for this STM32G431 BLDC controller.

## Run it

```bash
npm install
npm run dev
```

The app lists USB serial devices, connects at 115200 baud, and keeps the latest 80 telemetry samples in the Electron main process so the dashboard and detached card windows share one stream.

## Wire protocol

The firmware sends `protocol/bldc.proto` `Telemetry` messages encoded with nanopb. Each protobuf payload is COBS encoded and terminated with `0x00`:

```text
USB CDC → split on 0x00 → COBS decode → protobuf decode → engineering units → IPC → React
```

The first dashboard intentionally exposes only ADC-backed values:

- DC bus voltage
- phase A/B/C currents
- phase A/B/C voltages
- PCB NTC temperature
- protocol version, frame sequence, and controller uptime

Prototype-only random telemetry fields were removed from the schema and their field numbers are reserved. Settings transmission remains disabled until command messages are added to the protobuf schema and handled by firmware.

## Verification

```bash
npm run lint
npm run build
```

The Electron renderer runs with context isolation and without Node integration. Serial access remains in the main process and is exposed through the narrow API in `electron/preload.ts`.
