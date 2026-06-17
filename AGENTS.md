# BLDC Motor Firmware Project

Guidelines for AI agents and contributors working in this repository.
Current repository is for the Console 

## What This Project Is

2. **`console/`** — **Electron + React + TypeScript** desktop dashboard. Connects to the firmware over serial, decodes **CBOR** telemetry frames, and renders live motor diagnostics in chart cards.

The long-term goal is sensorless FOC diagnostics, ESC tuning, and DroneCAN integration. Several UI and protocol paths exist but are only partially connected — see [Current State & Gaps](#current-state--gaps).

---
## Architecture & Data Flow

```
┌─────────────────────┐     USB CDC (115200 baud)      ┌──────────────────────┐
│  STM32F411          │  CBOR frames: [type, payload]  │  Electron main       │
│                     │ ─────────────────────────────► │  serial.ts decoder   │
│  TelemThread (10Hz) │                                │       │              │
│    → usbQueue       │ ◄───────────────────────────── │  ipcMain → renderer  │
│  UsbThread          │  settings CBOR (RX, partial)   │  dashboard cards     │
└─────────────────────┘                                └──────────────────────┘
         │
         ├── TIM3 PWM → 3-phase outputs (commutation)
         ├── SPI1   → DRV8323R (enable, faults, gain, OC config)
         ├── ADC1   → phase currents, bus voltage, NTC temperature (DMA)
         └── (future) CAN → DroneCAN via libcanard
```

### FreeRTOS Tasks (`main.c`)

| Task | Priority | Role |
|------|----------|------|
| `defaultTask` | Normal | Initializes USB device (`MX_USB_DEVICE_Init`), idle loop |
| `telemTask` | Low | Samples ADC, publishes telemetry to `usbQueue` at ~10 Hz, calls `bldc_dronecan_pub()` |
| `usbTask` | Low | Dequeues messages, CBOR-encodes, transmits over CDC |

Inter-task messaging uses `usbQueue` (FreeRTOS message queue). Messages are `usb_msg_t` structs defined in `bldc.h`.

### Electron Process Model

- **Main process** (`electron/main.ts`): owns the `SerialPort` connection, parses CBOR, broadcasts `usb:telem` / `usb:data` to all windows.
- **Preload** (`electron/preload.ts`): exposes `window.api` via `contextIsolation` — renderer never gets raw `ipcRenderer` or Node APIs.
- **Renderer** (`src/`): React SPA with hash routing (`react-router-dom`). Sub-windows are separate `BrowserWindow` instances loading routes like `#/settings` or `#/card/motor-speed`.

---

## USB / CBOR Protocol

Firmware and console communicate over USB CDC at **115200 baud**. Payloads are **CBOR arrays** of `[message_type, payload]`.

### Message Types (`bldc.h`)

| Value | Name | Direction | Payload |
|-------|------|-----------|---------|
| `0` | `USB_MSG_TELEMETRY` | MCU → host | CBOR map (short keys) |
| `1` | `USB_MSG_SETTINGS` | Both | CBOR map of FOC/motor parameters |
| `2` | `USB_MSG_DEBUG_STR` | MCU → host | CBOR text string |
| `3` | `USB_MSG_ERROR` | MCU → host | CBOR uint error code |

### Telemetry Map Keys

Firmware encodes in `usb.c` (`usb_telem_encode`); console decodes in `electron/lib/serial.ts` (`mapTelemetry`):

| Key | Field | Notes |
|-----|-------|-------|
| `rpm`, `rpm_t` | actual / target RPM | Currently stubbed to 0 in firmware (no observer yet) |
| `i_a`, `i_b`, `i_c` | phase currents (A) | From ADC shunt amplifiers |
| `v_a`, `v_b`, `v_c` | phase voltages (V) | Derived from PWM duty × bus voltage |
| `i_d`, `i_q` | FOC d/q currents | Stubbed to 0 |
| `ang_m`, `ang_e` | mechanical / electrical angle (deg) | Stubbed to 0 |
| `ang_err` | angle error (deg) | Stubbed to 0 |
| `v_bat`, `i_bat` | battery voltage / current | `i_bat` approximated from phase currents |
| `e_used`, `e_rem` | energy used / remaining (Wh) | `e_rem` not computed yet |
| `bemf`, `obs`, `pll` | observer diagnostics (uint8) | Stubbed |
| `temp` | temperature (°C) | NTC via Beta equation; wired end-to-end |
| `ts` | timestamp (ms) | `millis32()` from DWT cycle counter |

When adding a new telemetry field, update **both** `usb.c` (encode), `bldc.h` (`bldc_telemetry_t`), `electron/lib/telemetry.ts` (types), and `electron/lib/serial.ts` (`isTelemetryPayload` + `mapTelemetry`).

### Settings Map Keys

Encoded/decoded in `usb.c`. Short keys: `pp`, `kv`, `rs`, `ls`, `i_kp`, `i_ki`, `s_kp`, `s_ki`, `idt`, `p_kp`, `p_ki`, `bemf`, `obs`, `min_cl`, `max_ol`, `ramp`, `align`, `smode`, `l_i`, `l_v`, `l_t`, `l_cd`. RX path writes into `bldc_get_settings()` in `telem.c`. **The console settings UI does not send settings over CBOR yet** — it is a visual placeholder.

---

## Firmware Guide

### Where to Put Code

| Change type | Location |
|-------------|----------|
| Motor control, sensing, protocols | `firmware/Core/Src/bldc/*.c` and `firmware/Core/Inc/bldc.h` |
| CubeMX user hooks (init calls, handles, task bodies) | `/* USER CODE BEGIN/END */` blocks in `main.c`, `freertos.c`, `usbd_cdc_if.c`, etc. |
| Pin/peripheral/clock/DMA/timer changes | **`firmware/demo_stm32f411.ioc`** via STM32CubeMX — **not** by hand-editing `MX_*_Init()` or `stm32f4xx_hal_msp.c` |
| New third-party C sources | `firmware/CMakeLists.txt` (`target_sources`, include dirs, compile flags) |

**Crucial CubeMX rule:** User code must stay inside `/* USER CODE BEGIN ... */` / `/* USER CODE END ... */` comment pairs. Regenerating from CubeMX deletes anything outside those blocks. If asked to change HAL pin assignments, clock trees, DMA, or interrupt vectors, **warn the user** and direct them to edit `demo_stm32f411.ioc` instead.

### Hardware Constants (`bldc.h`)

Key defines agents should know before changing sensing math:

- `ADC_REF_VOLT` = 3.3 V, `ADC_MAX_COUNT` = 4095
- `PHASE_CURRENT_ZERO_V` = 1.65 V (mid-rail shunt amp bias)
- `PHASE_CURRENT_V_PER_A` = 0.100 V/A
- `BUS_VOLTAGE_DIVIDER_RATIO` = 11.0
- Thermistor: `THERMISTOR_PULLUP` / `THERMISTOR_R25` = 10 kΩ, `THERMISTOR_BETA` = 3950
- `BLDC_COMPLEMENTARY_DRIVE` — optional macro for TIM1/TIM8 complementary PWM (off by default)
- `BLDC_TELEM_USE_DEMO` — enables `gen_demo_telemetry()` in `utils.c` for UI dev without hardware

### BLDC Module Responsibilities

- **`commutation.c`** — `bldc_comm_init/enable/disable/set_duty`, trapezoidal 6-step via `bldc_comm_trapeziod()` (note: header declares `bldc_comm_commutate()` — name mismatch, fix if linking fails). Uses `TIM3` channels mapped in `main.c`.
- **`drv8323r.c`** — SPI register read/write, fault decode, OC gain/mode, DC calibration.
- **`telem.c`** — ADC DMA (5 channels), IIR temperature filter, energy integration, `TelemThread`.
- **`usb.c`** — CBOR codec, `UsbThread`, `usb_msg_rx` called from CDC receive callback.
- **`dronecan.c`** — libcanard init, DNA allocation handler, ESC RawCommand/Status/NodeStatus. **CAN HAL TX/RX not implemented** — `bldc_dronecan_update()` is a stub.
- **`utils.c`** — `micros64()` / `millis32()` via DWT, `get_device_id()` from STM32 UID, `rand32()`.

### Build & Flash

**Prerequisites:** `arm-none-eabi-gcc` on PATH, `cmake`, `ninja`.

```bash
cd firmware
cmake --preset Debug
cmake --build --preset Debug
# Output: firmware/build/Debug/demo_stm32f411.elf
```

Or from `firmware/`: `make build` (runs preset + build). Flash with ST-Link + OpenOCD: `make flash`.

**clangd:** After configuring, `firmware/build/Debug/compile_commands.json` is generated. Root `.clangd` expects this path.

**DroneCAN DSDL codegen** (required before first build if `dsdl_generated/` is missing):

```bash
cd firmware
make dsdl_gen_build   # clones DSDL + dronecan_dsdlc, generates into Middlewares/Third_Party/dsdl_generated/
```

Git submodules at `firmware/Middlewares/Third_Party/DSDL` and `dronecan_dsdlc` are the canonical sources; the Makefile clones fresh copies for generation.

### Firmware Compile Flags

- `Core/Src/*.c` and `Core/Src/bldc/*.c`: `-O3 -Wall -Wextra` (strict)
- Third-party (libcanard, NanoCBOR, DSDL generated): `-w` (warnings suppressed)

---

## Console Guide

### Tech Stack

- **Vite 8** + **React 19** + **TypeScript 5.9**
- **Tailwind CSS 4** + **shadcn/ui** components (`components.json`, `@/` path alias)
- **Recharts** for dashboard charts
- **Electron 42** with `contextIsolation: true`, `nodeIntegration: false`
- **serialport** + **cbor** for device I/O

### IPC Surface (`window.api`)

Defined in `electron/preload.ts`, typed in `src/types/electron.d.ts`:

| Channel | Type | Purpose |
|---------|------|---------|
| `usb:list` / `usb:refresh` | invoke | Enumerate USB serial devices |
| `usb:connect` / `usb:disconnect` | invoke | Open/close port at 115200 |
| `usb:send-data` | invoke | Write newline-terminated string to port |
| `usb:setup-port-reader` | invoke | Attach CBOR stream parser |
| `usb:telem` / `usb:data` | event (main→renderer) | Parsed telemetry / raw messages |
| `usb:update` / `usb:on-update` | event | Device list changes |
| `open-new-window` | send | Spawn sub-window at `/#/{path}` |
| `file:save-file` | invoke | Write binary to `~/Documents/BLDC/{name}` |

**All serial/CBOR logic belongs in `electron/lib/serial.ts`.** Do not add `serialport` usage in renderer code.

### UI Structure

- **`src/windows/main.tsx`** — Dashboard; subscribes to `usb:telem`, maintains 40-sample history, feeds cards.
- **`src/cards/*.tsx`** — Presentational chart widgets; accept typed `data` props, include fallback demo data when empty.
- **`src/components/top-bar.tsx`** — Device connect dropdown (`useUsbDevices`), navigation.
- **`src/windows/settings.tsx`** — Motor/FOC settings form (**UI only, not wired to CBOR settings TX**).
- **`src/windows/console.tsx`** — Raw serial console via `usb:send-data` / `usb:data`.
- **`src/components/card-wrapper.tsx`** — Pop-out button opens detached card window.

Cards can be opened as pop-out windows via routes in `App.tsx` (e.g. `/card/motor-speed`).

### Build & Run

**Prerequisites:** Node.js, npm. Linux serial access requires `dialout` group:

```bash
sudo usermod -a -G dialout $USER   # re-login required
```

```bash
cd console
npm install
npm run dev          # Vite + Electron (opens DevTools in dev mode)
npm run build        # Production React + electron tsc
npm run build:linux  # Package AppImage/deb via electron-builder
```

Dev mode runs Electron with `--no-sandbox --ozone-platform=x11` (Linux).

### Console Code Style

- Functional components + hooks; no class components.
- Use existing shadcn/ui primitives under `src/components/ui/` — don't reinvent buttons, dialogs, etc.
- Global types (`TelemetryData`, `Device`) live in `src/types/electron.d.ts`.
- Prefer `useMemo` for chart data transforms (see `main.tsx`).
- Toast notifications via `sonner` (`use-devices.ts` pattern).

---

## Current State & Gaps

Agents should treat these as known incomplete areas — don't assume features work end-to-end:

| Area | Status |
|------|--------|
| Phase current / voltage / temp / battery telemetry | Working from real ADC |
| RPM, angles, i_d/i_q, observer fields | Stubbed to 0 in `telem.c` (no FOC observer) |
| Settings UI → firmware | Not connected; firmware RX handler exists |
| `usbQueue` size in `main.c` | Created with `sizeof(uint16_t)` but stores `usb_msg_t` — likely bug |
| `bldc_comm_commutate` vs `bldc_comm_trapeziod` | Header/implementation name mismatch |
| DroneCAN CAN bus I/O | Protocol layer only; no HAL CAN driver hooked up |
| `BLDC_TELEM_USE_DEMO` | Uncomment in `bldc.h` for synthetic telemetry without hardware |
| `TelemetryData` type | Missing `temperature` field in `electron.d.ts` (present in `BLDCTelemetry`) |
| Settings window | Static form controls, no state persistence or IPC |

When implementing FOC, wire observer outputs into `bldc_telem_update()` and ensure CBOR keys stay in sync with the console.

---

## Common Agent Tasks

### Add a telemetry field
1. Add to `bldc_telemetry_t` in `bldc.h`
2. Populate in `telem.c` (`bldc_telem_update` or demo generator)
3. Encode in `usb.c` (`usb_telem_encode`)
4. Add to `TelemetryRaw` / `BLDCTelemetry` in `electron/lib/telemetry.ts`
5. Update `isTelemetryPayload` + `mapTelemetry` in `serial.ts`
6. Add to `TelemetryData` in `electron.d.ts` and wire into relevant card

### Add a dashboard card
1. Create `src/cards/my-card.tsx` (props + chart, fallback data)
2. Import in `src/windows/main.tsx`, derive data from `telemetry` / `telemetryHistory`
3. Wrap in `CardWrapper` with a route string
4. Add route in `App.tsx` under `/card/...` with `SubWindowLayout`

### Add firmware motor logic
1. Implement in `Core/Src/bldc/` (new `.c` file auto-picked up by CMake `GLOB`)
2. Declare public API in `bldc.h`
3. Call from `main.c` USER CODE blocks or an RTOS task
4. Keep ISR work minimal; defer to tasks

### Change a pin or peripheral
1. Edit `firmware/demo_stm32f411.ioc` in STM32CubeMX
2. Regenerate code
3. Re-verify USER CODE blocks preserved
4. Rebuild firmware

---

## Testing & Verification

- **Firmware:** No automated test suite. Verify with `cmake --build --preset Debug` (zero errors). Flash and confirm CDC port appears.
- **Console:** `npm run lint` (ESLint). Manual test: connect device, confirm cards update from `usb:telem` events.
- **Without hardware:** Enable `BLDC_TELEM_USE_DEMO` in `bldc.h` and rebuild firmware, or rely on card fallback demo data in the renderer.

---

## Do Not

- Edit STM32 HAL init functions, `stm32f4xx_hal_msp.c`, or `startup_*.s` directly for configuration changes — use CubeMX.
- Put `serialport` or filesystem access in React renderer code.
- Add code outside CubeMX USER CODE blocks in generated files (except `bldc/` and top-level `CMakeLists.txt`).
- Assume settings forms or DroneCAN motor commands are functional without verifying the data path.
- Create markdown documentation files unless explicitly asked.