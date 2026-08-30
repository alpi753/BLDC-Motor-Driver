import type { MotorSettings } from "@/types/settings"

export const MOTOR_SETTINGS_STORAGE_KEY = "bldc.motor-settings.v4"

/** Matches firmware `bldc_settings_init_defaults()` / `observer.c` fallbacks. */
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

export function loadMotorSettings(): MotorSettings {
  try {
    const raw =
      localStorage.getItem(MOTOR_SETTINGS_STORAGE_KEY) ??
      localStorage.getItem("bldc.motor-settings.v3") ??
      localStorage.getItem("bldc.motor-settings.v2") ??
      localStorage.getItem("bldc.motor-settings.v1")
    if (!raw) {
      return { ...DEFAULT_MOTOR_SETTINGS }
    }

    const parsed = JSON.parse(raw) as Partial<MotorSettings>
    return { ...DEFAULT_MOTOR_SETTINGS, ...parsed }
  } catch {
    return { ...DEFAULT_MOTOR_SETTINGS }
  }
}

export function saveMotorSettings(settings: MotorSettings): void {
  localStorage.setItem(MOTOR_SETTINGS_STORAGE_KEY, JSON.stringify(settings))
}