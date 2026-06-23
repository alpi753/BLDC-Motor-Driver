# BLDC Motor Driver

Open-source BLDC ESC firmware for STM32 targets plus a desktop telemetry console. This  two contains firmware builds (for both STM32F411 and STM32G431), and an Electron dashboard that decodes live motor data over USB (CAN in the future).

## Quick start

### Console

Linux USB serial access usually requires membership in the `dialout` group:

```bash
sudo usermod -a -G dialout $USER   # re-login required
```
Then: 

```bash
cd software/console
npm install
npm run dev
```

### Firmware

```bash
cd software/<board>
cmake --preset Debug -DBLDC_BOARD=<board>
cmake --build --preset Debug
make flash    # optional — ST-Link + OpenOCD
```

## How it fits together

```
┌─────────────────────┐  Trapezoid Comm./┌─────────────────────┐                              ┌──────────────────────┐
│  BLDC Motor         │  FOC Commutation │  STM32F411 / G431   │  CBOR [type, payload](TX/RX) │  Electron nodejs     │
│                     │◄─────────────────│  Telem     (~1Hz)   │ ───────────────────────────► │  decoder             │
│                     │◄─────────────────│                     │ ◄─────────────────────────── │      ipc → UI        │
└─────────────────────┘◄─────────────────└─────────────────────┘                              └──────────────────────┘
                              P.W.M          │
                                             ├── TIM1 complementary PWM → DRV8323 → 3-phase bridge
                                             ├── SPI1 → DRV8323R (enable, faults, gain, OC)
                                             └── ADC → phase currents, bus voltage, NTC temperature
```
           
Connect the board over USB, open the console, pick the CDC serial port, and dashboard cards update from live telemetry.

Telemetry rate is about **1 Hz** (one CBOR frame per second from `TelemThread`).

## What's not done yet

- Sensorless FOC and speed/position observers (RPM, angles, `i_d`/`i_q` stay at zero)
- DroneCAN not fully implemented yet.

For UI development without hardware, set `CONFIG_BLDC_TELEM_USE_DEMO=y` in the target's `software/shared/bsp/boards/<board>/board.conf` and rebuild.
