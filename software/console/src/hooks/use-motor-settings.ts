import { useCallback, useState } from "react"
import type { MotorSettings } from "@/types/settings"
import {
  DEFAULT_MOTOR_SETTINGS,
  loadMotorSettings,
  saveMotorSettings,
} from "@/lib/motor-settings"

export function useMotorSettings() {
  const [settings, setSettings] = useState<MotorSettings>(() => loadMotorSettings())
  const [dirty, setDirty] = useState(false)

  const updateField = useCallback(
    <K extends keyof MotorSettings>(key: K, value: MotorSettings[K]) => {
      setSettings((prev) => ({ ...prev, [key]: value }))
      setDirty(true)
    },
    []
  )

  const resetToDefaults = useCallback(() => {
    setSettings({ ...DEFAULT_MOTOR_SETTINGS })
    setDirty(true)
  }, [])

  const persistLocally = useCallback(() => {
    saveMotorSettings(settings)
    setDirty(false)
  }, [settings])

  return {
    settings,
    dirty,
    updateField,
    resetToDefaults,
    persistLocally,
    setSettings,
    setDirty,
  }
}