# BLDC Console

Electron + React desktop dashboard for the BLDC ESC firmware. Connects over USB serial (115200 baud), decodes **CBOR** telemetry frames, and renders live motor diagnostics in chart cards.

Project-wide architecture, protocol details, and firmware board differences are documented in the repo root **[AGENTS.md](../../AGENTS.md)** and **[README.md](../../README.md)**.

## Quick start

```bash
cd software/console
npm install
npm run dev          # Vite + Electron (DevTools open in dev mode)
```

Linux serial access:

```bash
sudo usermod -a -G dialout $USER   # re-login required
```

Production build / package:

```bash
npm run build
npm run build:linux   # AppImage + deb
npm run build:mac
npm run build:win
```

Dev mode on Linux runs Electron with `--no-sandbox --ozone-platform=x11`.

## Tech stack

| Layer | Choice |
|-------|--------|
| Desktop shell | Electron 42 (`contextIsolation`, no `nodeIntegration`) |
| UI | React 19, TypeScript 5.9, Vite 8 |
| Styling | Tailwind CSS 4, shadcn/ui |
| Charts | Recharts |
| Device I/O | serialport + cbor (main process only) |

## Protocol

USB CDC at **115200 baud**. Frames are CBOR arrays: `[message_type, payload]`.

| Type | Name | Direction |
|------|------|-----------|
| `0` | Telemetry | MCU → host |
| `1` | Settings | Bidirectional (console TX not wired yet) |
| `2` | Debug string | MCU → host |
| `3` | Error code | MCU → host |

Telemetry uses short map keys (`rpm`, `i_a`, `v_bat`, `temp`, `ts`, …). Parsing and normalization live in **`electron/lib/serial.ts`** — all CBOR/serial logic stays in the main process.

### What the dashboard shows today

Data is **real from firmware ADC** when the target has `CONFIG_BLDC_TELEM_USE_DEMO=n` (default on both F411 and G431):

- Phase currents, bus voltage, phase voltages (duty × Vbus), FET temperature, energy used

Still **stubbed to zero** (or placeholder values) in firmware:

- RPM, FOC `i_d`/`i_q`, mechanical/electrical angles, observer diagnostics
- Energy remaining

Publish rate from firmware is about **1 Hz**.

## Process model

```
SerialPort (main)  →  serial.ts CBOR decode  →  ipcMain  →  preload  →  React cards
```

- **`electron/main.ts`** — port open/close, reader setup, IPC handlers
- **`electron/preload.ts`** — exposes `window.api` to the renderer
- **`src/`** — React SPA; never imports `serialport` directly

### `window.api` (IPC)

| API | Purpose |
|-----|---------|
| `usb.list` / `usb.refresh` | Enumerate serial devices |
| `usb.connect` / `usb.disconnect` | Open/close at 115200 |
| `usb.setup-port-reader` | Start CBOR stream parser |
| `usb.onTelemetry` | Live telemetry events |
| `usb.sendData` | Raw string TX (console window) |
| `openNewWindow` | Detached card/settings windows |
| `file.saveFile` | Export to `~/Documents/BLDC/` |

Types: `src/types/electron.d.ts`.

## UI layout

| File | Role |
|------|------|
| `src/windows/main.tsx` | Main dashboard, 40-sample telemetry history |
| `src/cards/*.tsx` | Motor speed, phase currents/voltages, dq frame, battery, angles, … |
| `src/windows/settings.tsx` | Motor/FOC form — **visual only, no settings TX** |
| `src/windows/console.tsx` | Raw serial console |
| `src/components/top-bar.tsx` | Device picker and navigation |
| `src/components/card-wrapper.tsx` | Pop-out button per card |
| `App.tsx` | Routes (`#/settings`, `#/card/motor-speed`, …) |

Cards include fallback demo series when no device is connected.

## Development without hardware

1. Use card fallback demo data in the renderer (default when telemetry is empty), or
2. Enable `CONFIG_BLDC_TELEM_USE_DEMO=y` in `software/shared/bsp/boards/<board>/board.conf` and rebuild firmware for synthetic CBOR streams.

## Lint

```bash
npm run lint
```