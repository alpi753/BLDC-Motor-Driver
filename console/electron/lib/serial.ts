import { SerialPort } from "serialport"
import type { BLDCTelemetry } from "./telemetry"

const isLikelyUsbDevice = (path: string) =>
  path.includes("ttyUSB") || path.includes("ttyACM") ||
  path.includes("cu.usb") || path.startsWith("COM")

export async function listDevices() {
  const ports = await SerialPort.list()
  return ports.filter((port) => port.path && isLikelyUsbDevice(port.path)).map((port) => ({
    path: port.path, manufacturer: port.manufacturer ?? undefined,
    serialNumber: port.serialNumber ?? undefined, pnpId: port.pnpId ?? undefined,
    locationId: port.locationId ?? undefined, productId: port.productId ?? undefined,
    vendorId: port.vendorId ?? undefined,
  }))
}

export async function connectDevice(path: string): Promise<SerialPort> {
  const port = new SerialPort({ path, baudRate: 115200, autoOpen: false })
  return new Promise((resolve, reject) => {
    port.open((error) => error ? reject(error) : resolve(port))
  })
}

const initializedPorts = new Set<SerialPort>()
type PortReaderHandlers = {
  onMessage?: (message: string) => void
  onTelemetry?: (telemetry: BLDCTelemetry) => void
}
type ProtoValues = Partial<Record<number, number>>

function decodeCobs(frame: Buffer): Buffer {
  const output: number[] = []
  let cursor = 0
  while (cursor < frame.length) {
    const code = frame[cursor++]
    if (code === 0) throw new Error("zero byte inside COBS frame")
    const end = cursor + code - 1
    if (end > frame.length) throw new Error("truncated COBS frame")
    while (cursor < end) output.push(frame[cursor++])
    if (code !== 0xff && cursor < frame.length) output.push(0)
  }
  return Buffer.from(output)
}

function readVarint(bytes: Buffer, start: number): [number, number] {
  let value = 0
  let multiplier = 1
  let cursor = start
  for (let count = 0; count < 10 && cursor < bytes.length; count += 1) {
    const byte = bytes[cursor++]
    value += (byte & 0x7f) * multiplier
    if ((byte & 0x80) === 0) return [value, cursor]
    multiplier *= 128
  }
  throw new Error("invalid protobuf varint")
}

function skipField(bytes: Buffer, cursor: number, wireType: number): number {
  if (wireType === 0) return readVarint(bytes, cursor)[1]
  if (wireType === 1) return cursor + 8
  if (wireType === 2) {
    const [length, next] = readVarint(bytes, cursor)
    return next + length
  }
  if (wireType === 5) return cursor + 4
  throw new Error(`unsupported protobuf wire type ${wireType}`)
}

function decodeTelemetry(payload: Buffer): BLDCTelemetry {
  const fields: ProtoValues = {}
  let cursor = 0
  while (cursor < payload.length) {
    const [tag, afterTag] = readVarint(payload, cursor)
    cursor = afterTag
    const fieldNumber = Math.floor(tag / 8)
    const wireType = tag & 0x07
    if (wireType === 0) {
      const [value, next] = readVarint(payload, cursor)
      fields[fieldNumber] = value
      cursor = next
    } else {
      cursor = skipField(payload, cursor, wireType)
    }
    if (cursor > payload.length) throw new Error("truncated protobuf field")
  }

  const zigZag32 = (value: number) => ((value >>> 1) ^ -(value & 1)) | 0
  const cdecOrNull = (value: number) => {
    const signed = zigZag32(value)
    return signed === -2147483648 ? null : signed / 10
  }
  return {
    protocol_version: fields[1] ?? 0,
    sequence: fields[2] ?? 0,
    uptime_ms: fields[3] ?? 0,
    bus_voltage_v: (fields[4] ?? 0) / 1000,
    phase_current_a: zigZag32(fields[5] ?? 0) / 1000,
    motor_rpm: fields[6] ?? 0,
    mosfet_temperature_c: zigZag32(fields[7] ?? 0) / 10,
    ntc_pcb_temperature_c: fields[8] === undefined ? null : cdecOrNull(fields[8]),
    currents_a: {
      phase_a: (fields[9] ?? 0) / 1000,
      phase_b: (fields[10] ?? 0) / 1000,
      phase_c: (fields[11] ?? 0) / 1000,
    },
    voltages_v: {
      phase_a: (fields[12] ?? 0) / 1000,
      phase_b: (fields[13] ?? 0) / 1000,
      phase_c: (fields[14] ?? 0) / 1000,
    },
  }
}

export async function sendDataToPort(port: SerialPort, data: string): Promise<void> {
  const payload = data.endsWith("\n") ? data : `${data}\n`
  return new Promise((resolve, reject) => {
    port.write(payload, (error) => {
      if (error) return reject(error)
      port.drain((drainError) => drainError ? reject(drainError) : resolve())
    })
  })
}

export function setupPortReader(port: SerialPort, handlers: PortReaderHandlers) {
  if (initializedPorts.has(port)) return
  initializedPorts.add(port)
  let buffer = Buffer.alloc(0)
  const onData = (data: Buffer) => {
    buffer = Buffer.concat([buffer, data])
    let delimiter = buffer.indexOf(0)
    while (delimiter >= 0) {
      const frame = buffer.subarray(0, delimiter)
      buffer = buffer.subarray(delimiter + 1)
      if (frame.length > 0) {
        try {
          handlers.onTelemetry?.(decodeTelemetry(decodeCobs(frame)))
        } catch (error) {
          handlers.onMessage?.(`Frame rejected: ${error instanceof Error ? error.message : String(error)}`)
        }
      }
      delimiter = buffer.indexOf(0)
    }
  }
  const cleanup = () => {
    port.off("data", onData); port.off("error", cleanup); port.off("close", cleanup)
    buffer = Buffer.alloc(0); initializedPorts.delete(port)
  }
  port.on("data", onData); port.on("error", cleanup); port.on("close", cleanup)
}
