export const TELEMETRY_HISTORY_LENGTH = 80


/** Renderer-facing representation of protocol/bldc.proto in engineering units. */
export type BLDCTelemetry = {
  protocol_version: number
  sequence: number
  uptime_ms: number
  bus_voltage_v: number
  mosfet_temperature_c: number | null
  pcb_temperature_c: number | null
  currents_a: { phase_a: number; phase_b: number; phase_c: number }
  voltages_v: { phase_a: number; phase_b: number; phase_c: number }
}
