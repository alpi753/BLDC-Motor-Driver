# BLDC Motor Driver

Open-source BLDC ESC firmware for STM32 targets plus a desktop telemetry console. The repo contains KiCad hardware designs, two firmware builds (STM32F411 and STM32G431), and an Electron dashboard that decodes live motor data over USB.

Part of the broader [BLDC-Motor-Driver](https://github.com/alpi753/BLDC-Motor-Driver) project.

## Repository layout

```
PCB/
├── hardware/              KiCad schematics and PCBs (ESC, MCU, MOSFET boards)
├── software/
│   ├── console/           Electron + React telemetry dashboard
│   ├── stm32f411/         STM32F411CE firmware (CubeMX + CMake)
│   ├── stm32g431/         STM32G431CB firmware (CubeMX + CMake)
│   ├── shared/bsp/        Shared motor control, telemetry, USB/CBOR, DroneCAN
│   └── scripts/           Host-side utilities (e.g. telem_reader.py)
└── AGENTS.md              Contributor and AI-agent guide (architecture, protocol, gaps)
```

## Quick start

### Console (dashboard)

```bash
cd software/console
npm install
npm run dev
```

Linux USB serial access usually requires membership in the `dialout` group:

```bash
sudo usermod -a -G dialout $USER   # re-login required
```

### Firmware

Prerequisites: `arm-none-eabi-gcc`, `cmake`, `ninja` (and `openocd` for flashing).

**STM32F411**

```bash
cd software/stm32f411
cmake --preset Debug
cmake --build --preset Debug
make flash    # optional — ST-Link + OpenOCD
```

**STM32G431**

```bash
cd software/stm32g431
cmake --preset Debug
cmake --build --preset Debug
make flash
```

Select the board at configure time with `-DBLDC_BOARD=stm32f411` or `-DBLDC_BOARD=stm32g431` (default is `stm32g431` in the shared BSP CMake).

DroneCAN DSDL codegen (first build only, if `dsdl_generated/` is missing):

```bash
cd software/stm32f411   # or stm32g431
make dsdl_gen_build
```

## How it fits together

```
┌─────────────────────┐     USB CDC 115200 baud      ┌──────────────────────┐
│  STM32F411 / G431   │  CBOR [type, payload]        │  Electron main       │
│  TelemThread (~1Hz) │ ───────────────────────────► │  serial.ts decoder   │
│                     │ ◄─────────────────────────── │  ipc → React cards   │
└─────────────────────┘   settings CBOR (RX only)   └──────────────────────┘
         │
         ├── TIM1 complementary PWM → DRV8323 → 3-phase bridge
         ├── SPI1 → DRV8323R (enable, faults, gain, OC)
         └── ADC → phase currents, bus voltage, NTC temperature
```

Connect the board over USB, open the console, pick the CDC serial port, and dashboard cards update from live telemetry.

## Telemetry today

Both boards publish the same CBOR telemetry map when `CONFIG_BLDC_TELEM_USE_DEMO=n` (current default on both).

| Data | F411 | G431 |
|------|------|------|
| Phase currents (`i_a`–`i_c`) | Real — ADC1 DMA, PA0–PA2 | Real — ADC1 injected, PA0–PA2 |
| Bus voltage (`v_bat`) | Real — PA5 (`ADC1_IN5`) | Real — PB2 (`ADC2_IN12`) |
| Phase voltages (`v_a`–`v_c`) | Real — PWM duty × Vbus | Real — PWM duty × Vbus |
| FET temperature (`temp`) | Real — PA6 NTC, IIR filtered | Real — PA6 NTC, IIR + CORDIC/FMAC |
| Energy used (`e_used`) | Real — integrated V×I | Real — integrated V×I |
| Battery current (`i_bat`) | Approx — mean \|phase currents\| | Same |
| RPM, FOC (`i_d`/`i_q`), angles, observer fields | Stubbed 0 | Stubbed 0 |
| Energy remaining (`e_rem`) | Always 0 | Always 0 |
| Motor NTC (PA7) | Sampled, not telemetered | Sampled, not telemetered |

Telemetry rate is about **1 Hz** (one CBOR frame per second from `TelemThread`).

## What's not done yet

- Sensorless FOC and speed/position observers (RPM, angles, `i_d`/`i_q` stay at zero)
- Console settings UI → firmware settings over CBOR
- DroneCAN on the wire (libcanard layer exists; CAN HAL TX/RX not hooked up)
- `energy_rem` from battery capacity

For UI development without hardware, set `CONFIG_BLDC_TELEM_USE_DEMO=y` in the target's `software/shared/bsp/boards/<board>/board.conf` and rebuild.

## Documentation

- **[AGENTS.md](AGENTS.md)** — Full architecture, protocol reference, board differences, coding rules, and common tasks for contributors and AI agents.
- **[software/console/README.md](software/console/README.md)** — Console-specific setup, IPC API, and UI structure.

## License

No root-level LICENSE file is present yet. Check individual components and submodules for their terms.