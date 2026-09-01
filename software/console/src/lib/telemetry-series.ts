export const TELEMETRY_HISTORY_LENGTH = 40

export function formatRelativeTimeLabel(
  timestampMs: number,
  baseTimestampMs: number,
  index: number,
): string {
  if (
    Number.isFinite(timestampMs) &&
    Number.isFinite(baseTimestampMs) &&
    baseTimestampMs > 0
  ) {
    return `+${((timestampMs - baseTimestampMs) / 1000).toFixed(1)}s`
  }

  return `t${index + 1}`
}

/** Unwrap 0-360 degree samples so line charts don't spike across the plot. */
export function unwrapDegrees(values: number[]): number[] {
  if (values.length === 0) return []

  const result = [values[0]]
  for (let i = 1; i < values.length; i++) {
    let delta = values[i] - values[i - 1]
    if (delta > 180) delta -= 360
    if (delta < -180) delta += 360
    result.push(result[i - 1] + delta)
  }

  return result
}

export function chartDataSignature(
  length: number,
  timestampMs?: number,
): string {
  if (length === 0) return "empty"
  return `${length}-${timestampMs ?? 0}`
}

/** Symmetric Y range around 0 so the zero axis stays centered. */
export function symmetricCurrentDomainA(
  values: number[],
  minAmp = 1,
): [number, number] {
  let peak = minAmp
  for (const value of values) {
    if (!Number.isFinite(value)) continue
    const magnitude = Math.abs(value)
    if (magnitude > peak) peak = magnitude
  }

  const steps = [1, 2, 5, 10, 20, 25, 50]
  const limit = steps.find((step) => peak <= step) ?? Math.ceil(peak / 10) * 10
  return [-limit, limit]
}

/** Keep +/− visible on a narrow Y-axis; 0 stays unsigned. */
export function formatSignedAmpTick(value: unknown): string {
  const amps = Number(value)
  if (!Number.isFinite(amps) || amps === 0) return "0A"

  const sign = amps > 0 ? "+" : "−"
  const magnitude = Math.abs(amps)
  const digits = Number.isInteger(magnitude) ? 0 : 1
  return `${sign}${magnitude.toFixed(digits)}A`
}

/** Pin a readable °C window that still expands for a hot MCU die. */
export function temperatureDomainC(
  values: Array<number | null | undefined>,
  minFloor = 10,
  minCeiling = 40,
): [number, number] {
  let min = minFloor
  let max = minCeiling
  for (const value of values) {
    if (value == null || !Number.isFinite(value)) continue
    if (value < min) min = value
    if (value > max) max = value
  }
  return [Math.floor(min / 5) * 5, Math.ceil(max / 5) * 5]
}