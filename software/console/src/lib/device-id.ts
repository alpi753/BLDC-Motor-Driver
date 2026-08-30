export function formatDeviceIdShort(hex: string): string {
  if (!hex) return "—"
  if (hex.length <= 16) return hex
  return `${hex.slice(0, 8)}…${hex.slice(-8)}`
}

export function formatDeviceIdGrouped(hex: string): string {
  if (!hex) return "—"
  return hex.match(/.{1,2}/g)?.join(" ") ?? hex
}