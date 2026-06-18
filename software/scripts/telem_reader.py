#!/usr/bin/env python3
"""
Read BLDC motor telemetry from STM32F411 or STM32G431 firmware over USB CDC.

Both targets use the same protocol:
  - USB CDC serial at 115200 baud
  - CBOR frames: [message_type, payload]
    0 = telemetry map (short keys)
    1 = settings map
    2 = debug string
    3 = error code (uint)

Usage:
  pip install pyserial cbor2
  python telem_reader.py --list
  python telem_reader.py --port /dev/ttyACM0
"""

from __future__ import annotations

import argparse
import sys
import time
from io import BytesIO
from typing import Any

try:
    import cbor2
except ImportError:
    print("Missing dependency: pip install cbor2", file=sys.stderr)
    raise SystemExit(1)

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    print("Missing dependency: pip install pyserial", file=sys.stderr)
    raise SystemExit(1)


BAUD_RATE = 115200

USB_MSG_TELEMETRY = 0
USB_MSG_SETTINGS = 1
USB_MSG_DEBUG_STR = 2
USB_MSG_ERROR = 3

MSG_NAMES = {
    USB_MSG_TELEMETRY: "telemetry",
    USB_MSG_SETTINGS: "settings",
    USB_MSG_DEBUG_STR: "debug",
    USB_MSG_ERROR: "error",
}

TELEM_FIELDS = [
    ("rpm", "RPM actual"),
    ("rpm_t", "RPM target"),
    ("i_a", "Phase A current (A)"),
    ("i_b", "Phase B current (A)"),
    ("i_c", "Phase C current (A)"),
    ("v_a", "Phase A voltage (V)"),
    ("v_b", "Phase B voltage (V)"),
    ("v_c", "Phase C voltage (V)"),
    ("i_d", "I_d (A)"),
    ("i_q", "I_q (A)"),
    ("ang_m", "Mechanical angle (deg)"),
    ("ang_e", "Electrical angle (deg)"),
    ("ang_err", "Angle error (deg)"),
    ("v_bat", "Battery voltage (V)"),
    ("i_bat", "Battery current (A)"),
    ("e_used", "Energy used (Wh)"),
    ("e_rem", "Energy remaining (Wh)"),
    ("bemf", "BEMF strength"),
    ("obs", "Observer confidence"),
    ("pll", "PLL lock"),
    ("temp", "Temperature (C)"),
    ("ts", "Timestamp (ms)"),
]


def is_likely_usb_port(device: str) -> bool:
    device_lower = device.lower()
    return (
        "ttyacm" in device_lower
        or "ttyusb" in device_lower
        or device_lower.startswith("com")
        or "cu.usb" in device_lower
    )


def list_usb_ports() -> list[str]:
    return [p.device for p in list_ports.comports() if is_likely_usb_port(p.device)]


def pick_default_port() -> str | None:
    ports = list_usb_ports()
    return ports[0] if ports else None


def decode_one_cbor_frame(buffer: bytes) -> tuple[Any, int] | None:
    """Decode the first complete CBOR value from buffer. Returns (value, nbytes)."""
    stream = BytesIO(buffer)
    decoder = cbor2.CBORDecoder(stream)
    try:
        value = decoder.decode()
    except cbor2.CBORDecodeError:
        return None
    consumed = stream.tell()
    if consumed <= 0:
        return None
    return value, consumed


def format_number(value: Any) -> str:
    if isinstance(value, float):
        return f"{value:.3f}"
    return str(value)


def print_telemetry(payload: dict[str, Any]) -> None:
    print("--- telemetry ---")
    for key, label in TELEM_FIELDS:
        if key in payload:
            print(f"  {label:28} {format_number(payload[key])}")


def print_frame(msg_type: int, payload: Any) -> None:
    name = MSG_NAMES.get(msg_type, f"unknown({msg_type})")
    print(f"\n[{time.strftime('%H:%M:%S')}] {name}")

    if msg_type == USB_MSG_TELEMETRY and isinstance(payload, dict):
        print_telemetry(payload)
    elif msg_type == USB_MSG_SETTINGS and isinstance(payload, dict):
        print("  settings:")
        for key in sorted(payload):
            print(f"    {key}: {format_number(payload[key])}")
    elif msg_type == USB_MSG_DEBUG_STR:
        print(f"  {payload}")
    elif msg_type == USB_MSG_ERROR:
        print(f"  error code: {payload}")
    else:
        print(f"  raw payload: {payload!r}")


def run(port: str, baud: int) -> int:
    buffer = bytearray()

    with serial.Serial(port, baudrate=baud, timeout=0.1) as ser:
        print(f"Connected to {port} @ {baud} baud")
        print("Waiting for CBOR frames (Ctrl+C to stop)...")

        while True:
            chunk = ser.read(ser.in_waiting or 1)
            if not chunk:
                continue

            buffer.extend(chunk)

            while buffer:
                decoded = decode_one_cbor_frame(bytes(buffer))
                if decoded is None:
                    break

                frame, nbytes = decoded
                del buffer[:nbytes]

                if not isinstance(frame, list) or len(frame) < 2:
                    print(f"Unexpected frame: {frame!r}")
                    continue

                msg_type, payload = frame[0], frame[1]
                if not isinstance(msg_type, int):
                    print(f"Unexpected message type: {msg_type!r}")
                    continue

                print_frame(msg_type, payload)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Decode BLDC telemetry CBOR frames from STM32F411/STM32G431 USB CDC."
    )
    parser.add_argument(
        "--port",
        "-p",
        help="Serial port (e.g. /dev/ttyACM0, COM3). Auto-detected if omitted.",
    )
    parser.add_argument(
        "--baud",
        "-b",
        type=int,
        default=BAUD_RATE,
        help=f"Baud rate (default: {BAUD_RATE})",
    )
    parser.add_argument(
        "--list",
        "-l",
        action="store_true",
        help="List likely USB serial ports and exit",
    )
    args = parser.parse_args()

    if args.list:
        ports = list_usb_ports()
        if not ports:
            print("No USB serial ports found.")
            return 1
        print("USB serial ports:")
        for device in ports:
            print(f"  {device}")
        return 0

    port = args.port or pick_default_port()
    if port is None:
        print(
            "No port specified and no USB serial device found.\n"
            "Try: python telem_reader.py --list",
            file=sys.stderr,
        )
        return 1

    try:
        return run(port, args.baud)
    except serial.SerialException as exc:
        print(f"Serial error: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("\nStopped.")
        return 0


if __name__ == "__main__":
    raise SystemExit(main())