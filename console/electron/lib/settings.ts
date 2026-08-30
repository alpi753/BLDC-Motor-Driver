import { encode } from "cbor"

/** Matches `USB_MSG_SETTINGS` in firmware `bsp.h`. */
export const USB_MSG_SETTINGS = 1

/** CBOR map keys — must stay in sync with `settings_encode` / `settings_decode` in `telem.c`. */
export type SettingsRaw = {
  pp: number
  rs: number
  ls: number
  i_kp: number
  i_ki: number
  s_kp: number
  s_ki: number
  p_kp: number
  p_ki: number
  obs: number
  min_cl: number
  max_ol: number
  ramp: number
  align_t: number
  ol_ramp: number
  align: number
  ol_i: number
  ol_start: number
  ho_ae: number
  ho_conf: number
  rpm_t: number
  smode: number
  l_i: number
}

export type MotorSettings = SettingsRaw

export const DEFAULT_MOTOR_SETTINGS: MotorSettings = {
  pp: 7,
  rs: 0.05,
  ls: 0.000008,
  i_kp: 0.8,
  i_ki: 120,
  s_kp: 0.0025,
  s_ki: 0.05,
  p_kp: 80,
  p_ki: 2500,
  obs: 25,
  min_cl: 500,
  max_ol: 1200,
  ramp: 500,
  align_t: 150,
  ol_ramp: 60,
  align: 2,
  ol_i: 1.5,
  ol_start: 150,
  ho_ae: 25,
  ho_conf: 55,
  rpm_t: 0,
  smode: 0,
  l_i: 20,
}

const SETTINGS_KEYS = Object.keys(DEFAULT_MOTOR_SETTINGS) as (keyof MotorSettings)[]

export const isSettingsPayload = (value: unknown): value is SettingsRaw => {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    return false
  }

  const record = value as Record<string, unknown>
  return SETTINGS_KEYS.every((key) => typeof record[key] === "number")
}

export function encodeSettingsMessage(settings: MotorSettings): Buffer {
  const payload: SettingsRaw = { ...settings, smode: Math.round(settings.smode) }
  return Buffer.from(encode([USB_MSG_SETTINGS, payload]))
}