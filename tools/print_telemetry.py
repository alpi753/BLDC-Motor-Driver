#!/usr/bin/env python3
"""Print COBS-framed nanopb telemetry from a USB CDC ACM device."""

import argparse
import os
import pathlib
import sys
import termios
import tty

from google.protobuf.message import DecodeError

PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT))

from protocol import bldc_pb2


def cobs_decode(frame: bytes) -> bytes:
    """Return the decoded payload for one COBS frame (without its 0x00 delimiter)."""
    decoded = bytearray()
    index = 0

    while index < len(frame):
        code = frame[index]
        if code == 0:
            raise ValueError("zero byte inside a COBS frame")
        index += 1

        block_end = index + code - 1
        if block_end > len(frame):
            raise ValueError("COBS block exceeds frame length")

        decoded.extend(frame[index:block_end])
        index = block_end
        if code != 0xFF and index < len(frame):
            decoded.append(0)

    return bytes(decoded)


def format_telemetry(telemetry: bldc_pb2.Telemetry) -> str:
    return (
        f"protocol_version={telemetry.protocol_version} seq={telemetry.sequence} "
        f"uptime_ms={telemetry.uptime_ms} "
        f"bus_voltage_mv={telemetry.bus_voltage_mv} "
        f"phase_current_ma={telemetry.phase_current_ma} "
        f"motor_rpm={telemetry.motor_rpm} "
        f"mosfet_temperature_cdec={telemetry.mosfet_temperature_cdec} "
        f"ntc_pcb_adc_raw={telemetry.ntc_pcb_adc_raw}"
    )


def monitor(device: str, max_frame_size: int) -> None:
    fd = os.open(device, os.O_RDONLY | os.O_NOCTTY)
    original_attributes = termios.tcgetattr(fd)
    frame = bytearray()

    try:
        tty.setraw(fd)
        while True:
            for byte in os.read(fd, 256):
                if byte != 0:
                    frame.append(byte)
                    if len(frame) > max_frame_size:
                        print("discarding oversized frame", file=sys.stderr)
                        frame.clear()
                    continue

                if not frame:
                    continue

                try:
                    payload = cobs_decode(bytes(frame))
                    telemetry = bldc_pb2.Telemetry()
                    telemetry.ParseFromString(payload)
                    print(format_telemetry(telemetry), flush=True)
                except (DecodeError, ValueError) as error:
                    print(f"discarding invalid frame: {error}", file=sys.stderr)
                finally:
                    frame.clear()
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, original_attributes)
        os.close(fd)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("device", nargs="?", default="/dev/ttyACM0")
    parser.add_argument(
        "--max-frame-size",
        type=int,
        default=256,
        help="discard encoded frames longer than this many bytes (default: 256)",
    )
    arguments = parser.parse_args()

    if arguments.max_frame_size < 1:
        parser.error("--max-frame-size must be positive")

    monitor(arguments.device, arguments.max_frame_size)


if __name__ == "__main__":
    main()
