import { SerialPort } from "serialport"
import { join } from "node:path"
import protobuf from "protobufjs"
import type { BLDCTelemetry } from "./telemetry"

const telemetryMessage = protobuf
  .loadSync(join(__dirname, "../protocol/bldc.proto"))
  .lookupType("bldc.Telemetry")

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

function decodeTelemetry(payload: Buffer): BLDCTelemetry {
  const decoded = telemetryMessage.toObject(telemetryMessage.decode(payload), {
    defaults: true,
    longs: Number,
  }) as Record<string, number>
  const cdecToCelsius = (cdec: number | undefined): number | null =>
    cdec === undefined || cdec === -2147483648 ? null : cdec / 10

  return {
    protocol_version: decoded.protocolVersion,
    sequence: decoded.sequence,
    uptime_ms: decoded.uptimeMs,
    bus_voltage_v: decoded.busVoltageMv / 1000,
    ntc_pcb_temperature_c: cdecToCelsius(decoded.ntcPcbTemperatureCdec),
    mcu_temperature_c: cdecToCelsius(decoded.mcuTemperatureCdec),
    currents_a: {
      phase_a: decoded.currAMa / 1000,
      phase_b: decoded.currBMa / 1000,
      phase_c: decoded.currCMa / 1000,
    },
    voltages_v: {
      phase_a: decoded.voltAMv / 1000,
      phase_b: decoded.voltBMv / 1000,
      phase_c: decoded.voltCMv / 1000,
    },
  }
}

export function decodeTelemetryFrame(frame: Buffer): BLDCTelemetry {
  return decodeTelemetry(decodeCobs(frame))
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
          handlers.onTelemetry?.(decodeTelemetryFrame(frame))
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
