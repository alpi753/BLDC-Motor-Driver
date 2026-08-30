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