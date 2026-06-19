# Software caveats

Known gaps, risks, and incomplete paths in `software/` as of the current tree. Use this before bench tuning, demos, or production assumptions.

For architecture and build instructions see [`../AGENTS.md`](../AGENTS.md).

---

## What is in good shape

| Area | Status |
|------|--------|
| Phase currents, Vbus, FET temp, phase voltages (duty × Vbus), energy used | Real ADC on both boards when `CONFIG_BLDC_TELEM_USE_DEMO=n` |
| USB CDC telemetry | CBOR `[0, map]` at 115200 baud, ~1 kHz (`CONFIG_TELEM_TX_HZ=1000`) |
| Settings host → device | CBOR `[1, map]` decoded in `telem.c`; live PI apply via `bldc_foc_apply_settings()` |
| G431 sensorless FOC path | TIM1 OC4REF → ADC1 injected, 10 kHz loop, idle until `rpm_t > 0` |
| G431 DRV8323 SPI | 8-bit, soft NSS, ~1.3 Mbit/s (`stm32g431.ioc` + `main.c`) |
| Console dashboard | Live charts when connected; settings window sends CBOR |
| Shared BSP | Single public header: `shared/bsp/bsp.h` |

---

## Board defaults (`board.conf`)

| Flag | STM32F411 | STM32G431 |
|------|-----------|-----------|
| `CONFIG_FOC_ENABLE` | **n** | **y** |
| `CONFIG_BLDC_TELEM_USE_DEMO` | n | n |
| `CONFIG_TELEM_TX_HZ` | 1000 | 1000 |
| `CONFIG_FOC_LOOP_HZ` | 10000 | 10000 |
| `CONFIG_BLDC_HAS_HW_ACCEL` | no | yes (CORDIC, FMAC) |
| ADC | 6-ch DMA circular (ADC1) | Dual ADC: injected phase + polled slow |

Build either target:

```bash
cd software/stm32f411   # or stm32g431
cmake --preset Debug && cmake --build --preset Debug
```

---

## Critical — verify before spin

### F411: FOC is off and not PWM-synced

With `CONFIG_FOC_ENABLE=y` on F411, `bsp_foc_sample_sensors()` reads the **telem DMA snapshot**, not PWM-aligned samples. G431 uses hardware injection on TIM1 TRGO. Do not expect F411 FOC parity without new ADC timing work.

**Path:** `boards/stm32f411/board.c`, `boards/stm32f411/board.conf`

### DroneCAN: protocol only, no bus I/O

`dronecan.c` initializes libcanard and publishes from `TelemThread`, but:

- `bldc_dronecan_update()` has no FDCAN RX/TX (no `HAL_FDCAN_Start`, no IRQ bridge).
- `handle_RawCommand()` ignores motor commands.
- ESC status uses placeholder values (12 V, 0 A, 0 RPM).
- Param get/set and execute-opcode handlers are empty.
- Node ID is hardcoded to **42**; DNA handlers exist but `should_accept_transfer()` does not accept DNA broadcasts.
- F411: `bldc_dronecan_init()` is **commented out** in `main.c`.

**Path:** `shared/bsp/dronecan.c`, `stm32g431/Core/Src/main.c`, `stm32f411/Core/Src/main.c`

### CubeMX regeneration risks (G431)

| Item | Risk |
|------|------|
| **TIM1 CH4** | Internal OC4REF for ADC trigger is in `/* USER CODE BEGIN TIM1_Init 2 */` in `main.c`. CubeMX often omits CH4 when it has no pin — regen can break FOC ADC while leaving `TIM_TRGO_OC4REF`. |
| **SPI1 DataSize** | Must stay `SPI_DATASIZE_8BIT` in `.ioc` / `main.c` for DRV8323 16-bit frames over two bytes. |
| **ADC1_2_IRQHandler** | Must live only in `stm32g4xx_it.c`, not `boards/stm32g431/board.c` (duplicate symbol). |
| HAL outside USER CODE | Edits to generated `MX_*_Init()` / `*_hal_msp.c` are overwritten — change `.ioc` instead. |

### Motor spins only when commanded

`rpm_target` defaults to **0** (idle). Setting `rpm_t > 0` in settings and sending over USB starts the startup sequence. There is no separate “arm” flag beyond that.

---

## FOC and motor control

### Observer model

- Simplified sliding-mode current observer + PLL in `observer.c` — not a full Luenberger/PLL reference design.
- `Id` target is hardcoded **0** (no field weakening).
- Motor model params `rs`, `ls`, `obs` must be reasonable; wrong values cause hand-off failure or instability.
- No ADC **zero-offset calibration** at boot; standstill current assumes `PHASE_CURRENT_ZERO_V = 1.65 V`.

### Current / voltage limits

- Current PI output clamps use **`FOC_DEFAULT_VBUS` (24 V)**, not measured `v_bat`, at init (`foc_pi_init`). High pack voltage can saturate the current loop early.
- Only **`l_i` (`max_phase_current`)** is enforced in closed-loop on `Iq`. Alignment and open-loop currents are capped by `l_i` only where `foc_open_loop_current()` applies; no thermal or bus-voltage shutdown.

### G431 Vbus in the FOC loop

Phase currents are PWM-synchronous; **Vbus is cached ~1 Hz** from ADC2 slow scan during FOC (`g431_foc_refresh_vbus_cache`). Decoupling and duty scaling can lag real bus transients.

**Path:** `boards/stm32g431/board.c`

### PWM frequency

TIM1 defaults (CubeMX): `Period = 65535`, `Prescaler = 0` → PWM frequency in the **low kHz** range (clock-dependent). Audible noise and current ripple may be high vs a typical 20–40 kHz ESC target.

### Startup and hand-off (recent behavior)

All modes eventually require **observer readiness** before closed-loop speed control:

| Criterion | Setting (CBOR key) | Default |
|-----------|------------------|---------|
| Open-loop RPM ≥ min CL | `min_cl` | 500 |
| PLL locked | telemetry `pll` | — |
| Observer confidence ≥ threshold | `ho_conf` | 55 |
| Angle error ≤ threshold | `ho_ae` | 25° |

| `smode` | Behavior |
|---------|----------|
| 0 | Align → open-loop from `ol_start` → hand-off → closed-loop |
| 1 | Skip align; open-loop from `ol_start` |
| 2 | Skip align; open-loop starts at **`min_cl` RPM** (not instant closed-loop) |

- `ramp` (ms) limits **acceleration time** only; after timeout, RPM holds until hand-off criteria are met (no blind hand-off on timeout).
- `ol_i` — open-loop torque current (separate from `align`).
- `ol_start` — entry RPM after align / mode 1 (capped by `min_cl`, `max_ol`).

Changing `pp` / `rs` / `ls` over USB updates observer gains but **does not reset** observer state; stop (`rpm_t = 0`) and restart after large motor-param changes.

### Fault handling

`foc_gate_driver_ok()` reads DRV8323 faults and calls `bldc_drv8323r_reset_faults()` — faults are cleared automatically, not surfaced on USB. Persistent hardware faults may be masked.

### Tasking

| Task | Priority | Stack (G431) | Role |
|------|----------|--------------|------|
| `commTask` | Above normal | 512×4 | FOC loop when enabled |
| `telemTask` | Low | 512×4 | USB TX + DroneCAN pub |
| `defaultTask` | Normal | 128×4 | USB init + idle loop |

FOC at 10 kHz with trig/`sqrtf` on a 2 KB stack — watch for overflow under load.

### F411 without FOC

`CommThread` runs **trapezoidal** 6-step commutation at fixed duty (`BLDC_COMM_DUTY_PERCENT`). RPM comes from settings `max_ol` if set, else defaults in `commutation.c`.

---

## Telemetry and sensing

| Field | Notes |
|-------|--------|
| `i_a`–`i_c` | Shunt scaling `PHASE_CURRENT_V_PER_A = 0.1 V/A`, zero 1.65 V |
| `v_bat` | Divider × 11; F411 PA5, G431 PB2 (slow ADC) |
| `v_a`–`v_c` | `duty × v_bat`; no dead-time / neutral shift (see `telem.c` TODO) |
| `i_bat` | **Approximation**: mean \|phase current\|, not bus shunt |
| `e_used` | Integrated from approx `i_bat` × `v_bat` |
| `e_rem` | Always **0**; `BATTERY_CAPACITY_WH` unused |
| `temp` | FET NTC (PA6) only; motor NTC (PA7) sampled but **not telemetered** |
| `rpm`, `i_d`/`i_q`, angles, observer | Populated when `CONFIG_FOC_ENABLE`; else zero / stubs |
| `id` | 16-byte UID in CBOR key `"id"` |

Telem snapshot on G431 during FOC reads the **FOC ADC cache** when HW trigger is active; otherwise polled injected ranks.

**Path:** `shared/bsp/telem.c`, board `board.c` files

---

## USB and settings protocol

- **Encode:** `usb_telem_encode` / `settings_encode` in `telem.c`.
- **RX:** `usb_msg_rx` in `telem.c` → `bldc_foc_apply_settings()` when FOC enabled.
- **TX blocking:** `usb_send_blocking()` retries with `osDelay` — backpressure can add jitter to `telemTask`.
- **Double USB init:** `bsp_usb_init()` in `bldc_telem_init()` (pre-scheduler) and `MX_USB_Device_Init()` again in `defaultTask` / `MainTask`.
- **Settings not stored in flash** — reboot restores firmware defaults until host re-sends.
- **No settings readback** from MCU to host (one-way apply).

### Active settings keys (23)

`pp`, `rs`, `ls`, `i_kp`, `i_ki`, `s_kp`, `s_ki`, `p_kp`, `p_ki`, `obs`, `min_cl`, `max_ol`, `ramp`, `align_t`, `ol_ramp`, `align`, `ol_i`, `ol_start`, `ho_ae`, `ho_conf`, `rpm_t`, `smode`, `l_i`

Removed from protocol (unused or misleading): `kv`, `idt`, `bemf` filter, `l_v`, `l_t`, `l_cd`.

Console local storage key: `bldc.motor-settings.v4` (`console/src/lib/motor-settings.ts`).

---

## Console (`software/console/`)

### Demo fallbacks mask disconnect

All dashboard cards (`src/cards/*.tsx`) show **synthetic demo data** when history is empty. Charts can look “live” with no device connected — verify `device_id` / connection state before tuning.

### Telemetry validation is partial

`electron/lib/serial.ts` `isTelemetryPayload()` checks only rpm, currents, voltages, and timestamp — not `id`, observer fields, or `i_d`/`i_q`.

### No in-dashboard RPM setpoint

Target RPM is **`rpm_t` in Settings** → Save & Send. No throttle slider on the main dashboard.

### Lint / CI

`npm run lint` fails on pre-existing issues (generated `dist-electron/`, shadcn refresh rules, etc.) — not a clean gate today.

### IPC gaps

- `file:save-file` exists in main process; no dashboard wiring for session capture.
- Settings RX from device (`onSettings`) is typed but not used for MCU → host sync.

---

## Build and tooling

| Topic | Caveat |
|-------|--------|
| **clangd** | Root `.clangd` defaults to F411 build; G431 FOC work may need `compile_commands.json` from `stm32g431/build/Debug`. |
| **DroneCAN DSDL** | Generated sources under `Middlewares/Third_Party/dsdl_generated/`; run `make dsdl_gen_build` if missing. |
| **libcanard** | Submodule under each target; `canard_stm32` driver exists but is **not linked** into the app HAL path. |
| **`#include "dronecan.c"`** | DroneCAN compiled into `telem.c` translation unit — unusual for tests and linking boundaries. |
| **F411 DRV8323** | `bldc_drv8323r_init()` commented in `main.c`; commutation/FOC paths call it from threads when used. |

---

## Scripts

`software/scripts/telem_reader.py` — standalone CBOR telemetry decoder for bring-up without the Electron app. Settings TX and FOC control not covered.

---

## Suggested bring-up order

1. Flash G431 (or F411 trapezoidal only with `CONFIG_FOC_ENABLE=n`).
2. Connect console; confirm **real** `i_a`–`i_c`, `v_bat`, `temp` (ignore demo card data until history populates).
3. Send settings with **`rpm_t = 0`**; confirm motor stays idle.
4. Set conservative startup: `smode=0`, low `align` / `ol_i`, `ol_start=150`, `min_cl=500`, `ho_ae=30`, `ho_conf=50`.
5. Set `rpm_t` > 0 and send; watch `pll`, `obs`, `ang_err` until hand-off.
6. Tune `obs`, PLL, then current/speed loops.
7. Do not rely on DroneCAN, `e_rem`, or `i_bat` for safety decisions until implemented.

---

## High-leverage fixes (not done)

1. FDCAN RX/TX bridge + wire `RawCommand` → `rpm_t` / torque.
2. Remove or gate dashboard demo fallbacks when disconnected.
3. Live Vbus in current PI limits and faster G431 Vbus in FOC.
4. Thermal / bus over-voltage protection (real limits, not removed settings).
5. F411 PWM-sync ADC if FOC on F411 is required.
6. Persist settings to flash or echo settings map on request.
7. ADC zero calibration at boot.

---

*Update this file when closing a gap or changing defaults in `board.conf`, CBOR keys, or startup logic.*