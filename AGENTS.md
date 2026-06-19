# BLDC Motor Driver — Agent & Contributor Guide

Guidelines for AI agents and humans working in this repository.

## What this project is

| Component | Path | Role |
|-----------|------|------|
| **Hardware** | `hardware/` | KiCad ESC/MCU designs (`untitled.kicad_pcb`, `Ana_ESC.kicad_pcb`, etc.) |
| **Shared BSP** | `software/shared/bsp/` | Motor control, telemetry, USB/CBOR, DroneCAN — compiled into both MCU targets |
| **STM32F411 firmware** | `software/stm32f411/` | Single-ADC ESC firmware (CubeMX + CMake) |
| **STM32G431 firmware** | `software/stm32g431/` | Dual-ADC ESC firmware with HW math accelerators |
| **Console** | `software/console/` | Electron + React dashboard; decodes CBOR over USB CDC |

Long-term goal: sensorless FOC diagnostics, ESC tuning, and DroneCAN. Several paths exist but are only partially connected — see [Current state & gaps](#current-state--gaps).

**Do not edit KiCad PCB/schematic files unless the user explicitly requests a hardware change.**

---

## Architecture & data flow

```
┌──────────────────────┐     USB CDC 115200 baud     ┌──────────────────────┐
│  STM32F411 / G431    │  CBOR [type, payload]       │  Electron main       │
│  TelemThread (~1Hz)  │ ──────────────────────────► │  electron/lib/       │
│                      │                             │    serial.ts         │
│  CommThread          │ ◄────────────────────────── │       │              │
│  (trapezoidal PWM)   │   settings CBOR (RX)        │  ipcMain → renderer  │
└──────────────────────┘                             └──────────────────────┘
         │
         ├── TIM1 CH1/2/3 (+ CH2N/CH3N) → DRV8323 → 3-phase bridge
         ├── SPI1 → DRV8323R (CS, EN, FAULT, registers)
         └── ADC → phase currents, VBus, NTC (board-specific routing)
```

### FreeRTOS tasks (`main.c`, both targets)

| Task | Priority | Role |
|------|----------|------|
| `defaultTask` / `MainTask` | Normal | `MX_USB_DEVICE_Init()`, idle loop |
| `telemTask` | Low | `TelemThread` — ADC snapshot, CBOR encode, USB TX, DroneCAN pub |
| `commTask` | Above normal | `CommThread` — trapezoidal commutation via TIM1 |

`bsp_init()`, `bldc_comm_init()`, and `bldc_telem_init()` run in `main()` before the scheduler starts.

### Board selection (CMake)

Shared BSP is wired via `software/shared/bsp/cmake/bldc_bsp.cmake`:

```bash
cmake -DBLDC_BOARD=stm32f411 ..   # or stm32g431 (cache default)
```

Board options live in `software/shared/bsp/boards/<name>/board.conf` and `board.h`. CMake generates `build/.../generated/bsp_autoconf.h`.

### Electron process model

- **Main** (`electron/main.ts`): owns `SerialPort`, parses CBOR, broadcasts `usb:telem` / `usb:data`.
- **Preload** (`electron/preload.ts`): exposes `window.api` with `contextIsolation` — renderer never gets raw `ipcRenderer`.
- **Renderer** (`src/`): React SPA, hash routing (`react-router-dom`). Pop-out card windows load routes like `#/card/motor-speed`.

---

## Board comparison — ADC & telemetry

Both targets share `bldc_telem_update()` in `telem.c` when `BLDC_TELEM_USE_DEMO=0`.

### Build flags (`board.conf`)

| Flag | F411 | G431 |
|------|------|------|
| `CONFIG_BLDC_TELEM_USE_DEMO` | **n** | **n** |
| `CONFIG_BLDC_HAS_USB_TELEM` | y | y |
| `CONFIG_BLDC_ADC_DMA_CHANNELS` | **6** | **5** |
| `CONFIG_BLDC_HAS_HW_ACCEL` | no | **y** (CORDIC, FMAC, RNG) |

### ADC architecture

**STM32F411** — single ADC1, 6-channel circular DMA scan:

| DMA rank | Pin | Signal |
|----------|-----|--------|
| 1–3 | PA0–PA2 | Phase currents |
| 4 | **PA5** | VBus (`V_Bus_Sense`, `ADC1_IN5`) |
| 5–6 | PA6–PA7 | NTC FET / NTC motor |

Snapshot: tear-free `memcpy` of DMA buffer (`boards/stm32f411/board.c`).

**STM32G431** — dual ADC, software-triggered sampling each telem tick:

| ADC | Mode | Pins | Signal |
|-----|------|------|--------|
| ADC1 | Injected (EXTI15 SW trigger) | PA0–PA2 | Phase currents |
| ADC2 | Regular 3-ch polled scan | PA6, PA7, PB2 | NTC FET, NTC motor, VBus |

Snapshot: `boards/stm32g431/board.c` assembles a unified 5-sample buffer.

### `bldc_telemetry_t` — what is real vs stubbed

| Field | CBOR key | F411 | G431 |
|-------|----------|------|------|
| Phase currents | `i_a`–`i_c` | Real (DMA) | Real (injected) |
| Bus voltage | `v_bat` | Real (PA5 × 11 divider) | Real (PB2 × 11 divider) |
| Phase voltages | `v_a`–`v_c` | Real (duty × Vbus) | Real (duty × Vbus) |
| FET temperature | `temp` | Real (PA6, software log + IIR) | Real (PA6, CORDIC + FMAC IIR) |
| Battery current | `i_bat` | Approx (mean \|i_phase\|) | Same |
| Energy used | `e_used` | Integrated | Integrated |
| Timestamp | `ts` | `millis32()` | `millis32()` |
| RPM, target RPM | `rpm`, `rpm_t` | **0** | **0** |
| FOC currents | `i_d`, `i_q` | **0** | **0** |
| Angles | `ang_m`, `ang_e`, `ang_err` | **0** | **0** |
| Observer | `bemf`, `obs`, `pll` | **0 / 100 / 0** | **0 / 100 / 0** |
| Energy remaining | `e_rem` | **0** | **0** |

**Sampled but not telemetered:** PA7 motor NTC on both boards (`ADC_IDX_TEMP_MTR` / `ADC_SLOW_RANK_NTC_MTR`).

**Demo mode:** `CONFIG_BLDC_TELEM_USE_DEMO=y` replaces all fields with `bldc_telem_fake()` random data and skips ADC init.

---

## USB / CBOR protocol

115200 baud USB CDC. Payloads are CBOR arrays: `[message_type, payload]`.

### Message types (`bldc.h`)

| Value | Name | Direction | Payload |
|-------|------|-----------|---------|
| `0` | `USB_MSG_TELEMETRY` | MCU → host | CBOR map (short keys) |
| `1` | `USB_MSG_SETTINGS` | Both | CBOR map of motor/FOC parameters |
| `2` | `USB_MSG_DEBUG_STR` | MCU → host | CBOR text string |
| `3` | `USB_MSG_ERROR` | MCU → host | CBOR uint error code |

Encoding/decoding lives in `software/shared/bsp/telem.c` (`usb_telem_encode`, `usb_msg_tx`, `usb_msg_rx`). CDC receive calls `usb_msg_rx` from `usbd_cdc_if.c`.

### Telemetry map keys

| Key | Field | Notes |
|-----|-------|-------|
| `rpm`, `rpm_t` | actual / target RPM | Stubbed 0 — no observer |
| `i_a`, `i_b`, `i_c` | phase currents (A) | Shunt amps, PA0–PA2 |
| `v_a`, `v_b`, `v_c` | phase voltages (V) | PWM duty × `v_bat` |
| `i_d`, `i_q` | FOC d/q currents | Stubbed 0 |
| `ang_m`, `ang_e` | mechanical / electrical angle (°) | Stubbed 0 |
| `ang_err` | angle error (°) | Stubbed 0 |
| `v_bat`, `i_bat` | battery voltage / current | Real Vbus; `i_bat` approximated |
| `e_used`, `e_rem` | energy used / remaining (Wh) | `e_used` integrated; `e_rem` always 0 |
| `bemf`, `obs`, `pll` | observer diagnostics (uint8) | Stubbed |
| `temp` | FET temperature (°C) | NTC Beta + IIR; PA6 only |
| `ts` | timestamp (ms) | `millis32()` via DWT |

When adding a telemetry field, update **`bldc.h`** (`bldc_telemetry_t`), **`telem.c`** (populate + `usb_telem_encode`), **`electron/lib/telemetry.ts`**, **`electron/lib/serial.ts`** (`isTelemetryPayload` + `mapTelemetry`), **`src/types/electron.d.ts`**, and the relevant dashboard card.

### Settings map keys

Short keys: `pp`, `kv`, `rs`, `ls`, `i_kp`, `i_ki`, `s_kp`, `s_ki`, `idt`, `p_kp`, `p_ki`, `bemf`, `obs`, `min_cl`, `max_ol`, `ramp`, `align`, `smode`, `l_i`, `l_v`, `l_t`, `l_cd`.

RX path in `usb_msg_rx` decodes into `bldc_get_settings()` in `telem.c`. **Console settings UI does not send settings over CBOR yet.**

---

## Firmware guide

### Where to put code

| Change type | Location |
|-------------|----------|
| Motor control, sensing, protocols | `software/shared/bsp/*.c` and `bldc.h` |
| Board-specific ADC snapshot, pins, timers | `software/shared/bsp/boards/<board>/board.c`, `board.h` |
| Board Kconfig options | `software/shared/bsp/boards/<board>/board.conf` |
| CubeMX user hooks (init, tasks, callbacks) | `/* USER CODE BEGIN/END */` in `main.c`, `freertos.c`, `usbd_cdc_if.c`, etc. |
| Pin / clock / DMA / peripheral config | **`<target>/<target>.ioc`** via STM32CubeMX — not by hand-editing `MX_*_Init()` or `*_hal_msp.c` |
| New third-party C sources | `<target>/CMakeLists.txt` |

**CubeMX rule:** Code outside `USER CODE` blocks is deleted on regeneration. Warn the user and point them to the `.ioc` file for pin or peripheral changes.

### Shared module responsibilities (`software/shared/bsp/`)

| File | Role |
|------|------|
| `bsp.c` / `bsp.h` | Init orchestration, motor handle, telem ADC hooks |
| `boards/<board>/board.c` | PWM handle binding, ADC snapshot implementation |
| `boards/<board>/board.h` | ADC indices, divider constants, phase timer macros |
| `boards/stm32g431/hw_accel.c` | CORDIC sin/log, FMAC IIR, HW RNG (G431 only) |
| `bsp_math.c` | Portable wrappers; software fallback on F411 |
| `commutation.c` | `bldc_comm_init/enable/disable/set_duty/commutate`, TIM1 trapezoidal drive |
| `drv8323r.c` | SPI register access, fault decode, OC config |
| `telem.c` | `bldc_telem_update`, CBOR USB codec, `TelemThread`, settings decode |
| `dronecan.c` | libcanard init, DNA, ESC status — **CAN HAL TX/RX not implemented** |
| `utils.c` | `micros64()` / `millis32()` via DWT, `get_device_id()`, `rand32()` |

### Hardware constants (`board.h` / `bldc.h`)

- `ADC_REF_VOLT` = 3.3 V, `ADC_MAX_COUNT` = 4095
- `PHASE_CURRENT_ZERO_V` = 1.65 V, `PHASE_CURRENT_V_PER_A` = 0.100 V/A
- `BUS_VOLTAGE_DIVIDER_RATIO` = 11.0
- Thermistor: `THERMISTOR_PULLUP` / `THERMISTOR_R25` = 10 kΩ, `THERMISTOR_BETA` = 3950
- `IIR_FILTER_ALPHA` = 0.1 (temperature low-pass)

### Build & flash

```bash
cd software/stm32f411   # or stm32g431
cmake --preset Debug
cmake --build --preset Debug
# Output: build/Debug/<target>.elf
make flash              # ST-Link + OpenOCD
```

**clangd:** `build/Debug/compile_commands.json` is generated on configure. Root `.clangd` points at the F411 build by default.

**DroneCAN DSDL codegen** (if `Middlewares/Third_Party/dsdl_generated/` is missing):

```bash
make dsdl_gen_build
```

### Compile flags

- Application + BSP sources: `-O3 -Wall -Wextra`
- Third-party (libcanard, NanoCBOR, DSDL generated): `-w`

---

## Console guide

### Tech stack

- **Vite 8** + **React 19** + **TypeScript 5.9**
- **Tailwind CSS 4** + **shadcn/ui** (`@/` path alias)
- **Recharts** for dashboard charts
- **Electron 42** (`contextIsolation: true`, `nodeIntegration: false`)
- **serialport** + **cbor**

### IPC surface (`window.api`)

Defined in `electron/preload.ts`, typed in `src/types/electron.d.ts`:

| Channel | Type | Purpose |
|---------|------|---------|
| `usb:list` / `usb:refresh` | invoke | Enumerate USB serial devices |
| `usb:connect` / `usb:disconnect` | invoke | Open/close port at 115200 |
| `usb:send-data` | invoke | Write string to port |
| `usb:setup-port-reader` | invoke | Attach CBOR stream parser |
| `usb:telem` / `usb:data` | event | Parsed telemetry / raw messages |
| `usb:update` / `usb:on-update` | event | Device list changes |
| `open-new-window` | send | Spawn sub-window at `/#/{path}` |
| `file:save-file` | invoke | Write binary to `~/Documents/BLDC/{name}` |

**All serial/CBOR logic belongs in `electron/lib/serial.ts`.** Do not use `serialport` in renderer code.

### UI structure

| Path | Role |
|------|------|
| `src/windows/main.tsx` | Dashboard; subscribes to `usb:telem`, 40-sample history |
| `src/cards/*.tsx` | Chart widgets; fallback demo data when empty |
| `src/components/top-bar.tsx` | Device connect dropdown |
| `src/windows/settings.tsx` | Settings form — **UI only, no CBOR TX** |
| `src/windows/console.tsx` | Raw serial console |
| `src/components/card-wrapper.tsx` | Pop-out card windows |
| `App.tsx` | Routes including `/card/...` |

### Build & run

```bash
cd software/console
npm install
npm run dev
npm run build
npm run build:linux
```

Linux dev mode uses `--no-sandbox --ozone-platform=x11`.

---

## Current state & gaps

| Area | Status |
|------|--------|
| Phase currents, Vbus, phase voltages, FET temp, energy used | **Working** on both boards (real ADC) |
| Motor NTC (PA7) | Sampled in ADC; **not in telemetry map** |
| RPM, angles, `i_d`/`i_q`, observer fields | Stubbed in `bldc_telem_update()` |
| `i_bat` | Approximated from phase currents, not a bus shunt |
| `e_rem` | Always 0; `BATTERY_CAPACITY_WH` unused |
| Telemetry publish rate | ~**1 Hz** (`TelemThread` 1 s loop) |
| Settings UI → firmware | Not connected; firmware RX exists |
| DroneCAN bus I/O | Protocol layer only |
| `TelemetryData` in `electron.d.ts` | Missing `temperature` field (mapped in `serial.ts` as `BLDCTelemetry.temperature`) |
| Demo telemetry | Off on both boards; enable via `board.conf` for UI-only dev |

When implementing FOC, wire observer outputs into `bldc_telem_update()` and keep CBOR keys in sync with the console.

---

## Common tasks

### Add a telemetry field

1. `bldc_telemetry_t` in `bldc.h`
2. Populate in `telem.c` (`bldc_telem_update` or `bldc_telem_fake`)
3. `usb_telem_encode` in `telem.c`
4. `electron/lib/telemetry.ts` types
5. `isTelemetryPayload` + `mapTelemetry` in `serial.ts`
6. `TelemetryData` in `electron.d.ts` + dashboard card

### Add a dashboard card

1. `src/cards/my-card.tsx`
2. Import in `src/windows/main.tsx`
3. Wrap in `CardWrapper` with route string
4. Add route in `App.tsx`

### Add firmware motor logic

1. New or existing file under `software/shared/bsp/`
2. Declare API in `bldc.h`
3. Call from `main.c` USER CODE or an RTOS task
4. Keep ISR work minimal; defer to tasks

### Change a pin or peripheral

1. Edit `<target>/<target>.ioc` in STM32CubeMX
2. Regenerate code
3. Update `board.h` ADC indices if scan order changed
4. Re-verify USER CODE blocks and rebuild

### Enable VBus on F411

VBus must land on an ADC pin (currently **PA5**). Update CubeMX, then align `board.h` indices and `CONFIG_BLDC_ADC_DMA_CHANNELS` in `board.conf`. **Do not reroute nets in KiCad without explicit user approval.**

---

## Testing & verification

- **Firmware:** `cmake --build --preset Debug` (zero errors). Flash and confirm CDC port appears.
- **Console:** `npm run lint`. Connect device; confirm cards update from `usb:telem`.
- **Without hardware:** `CONFIG_BLDC_TELEM_USE_DEMO=y` in `board.conf`, or card fallback demo data in the renderer.

---

## Do not

- Edit KiCad PCB/schematic files unless the user explicitly asks.
- Edit STM32 HAL init or `*_hal_msp.c` for configuration changes — use CubeMX.
- Put `serialport` or filesystem access in React renderer code.
- Add code outside CubeMX USER CODE blocks in generated files (except shared BSP and top-level `CMakeLists.txt`).
- Assume settings forms or DroneCAN motor commands work without verifying the data path.
- Create markdown documentation files unless explicitly asked.