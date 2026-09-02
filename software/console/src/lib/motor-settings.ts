export const MOTOR_SETTINGS_STORAGE_KEY = "bldc.settings"

export function saveMotorSettings(settings: unknown): void {
  localStorage.setItem(MOTOR_SETTINGS_STORAGE_KEY, JSON.stringify(settings))
}