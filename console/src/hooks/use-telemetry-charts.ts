import type { AnglePoint } from "@/cards/elec-mech-angle"
import type { DQFramePoint } from "@/cards/dq-ref-frame"
import type { MotorSpeedPoint } from "@/cards/motor-speed"
import type { MotorXticPoint } from "@/cards/motor-xtics"
import type { PhaseCurrentPoint } from "@/cards/phase-currents"
import type { PhaseVoltagePoint } from "@/cards/phase-voltage"
import {
  TELEMETRY_HISTORY_LENGTH,
  chartDataSignature,
  formatRelativeTimeLabel,
  unwrapDegrees,
} from "@/lib/telemetry-series"
import * as React from "react"

export function useTelemetryCharts() {
  const [telemetry, setTelemetry] = React.useState<TelemetryData | null>(null)
  const [telemetryHistory, setTelemetryHistory] = React.useState<TelemetryData[]>([])

  React.useEffect(() => {
    let alive = true

    const applyHistory = (history: TelemetryData[]) => {
      if (!alive) return
      const slice = history.slice(-TELEMETRY_HISTORY_LENGTH)
      setTelemetryHistory(slice)
      setTelemetry(slice.at(-1) ?? null)
    }

    window.api.usb.getTelemetryHistory?.().then((history) => {
      if (alive && Array.isArray(history)) {
        applyHistory(history)
      }
    })

    const unsubscribeHistory = window.api.usb.onTelemetryHistory?.(applyHistory)

    return () => {
      alive = false
      unsubscribeHistory?.()
    }
  }, [])

  const chartRevision = React.useMemo(
    () =>
      chartDataSignature(
        telemetryHistory.length,
        telemetryHistory.at(-1)?.timestamp_ms,
      ),
    [telemetryHistory],
  )

  const speedData = React.useMemo<MotorSpeedPoint[]>(() => {
    if (telemetryHistory.length === 0) return []

    const baseTs = telemetryHistory[0]?.timestamp_ms ?? 0
    return telemetryHistory.map((item, index) => ({
      sample: index,
      timeLabel: formatRelativeTimeLabel(item.timestamp_ms, baseTs, index),
      rpm: item.speed.actual_rpm,
      target: item.speed.target_rpm,
    }))
  }, [telemetryHistory])

  const phaseCurrentData = React.useMemo<PhaseCurrentPoint[]>(() => {
    if (!telemetry) return []

    return [
      { phase: "Phase A", current: telemetry.currents.phase_a },
      { phase: "Phase B", current: telemetry.currents.phase_b },
      { phase: "Phase C", current: telemetry.currents.phase_c },
    ]
  }, [telemetry])

  const dqData = React.useMemo<DQFramePoint[]>(() => {
    if (telemetryHistory.length === 0) return []

    const baseTs = telemetryHistory[0]?.timestamp_ms ?? 0
    return telemetryHistory.map((item, index) => ({
      sample: index,
      timeLabel: formatRelativeTimeLabel(item.timestamp_ms, baseTs, index),
      id: item.currents.i_d,
      iq: item.currents.i_q,
    }))
  }, [telemetryHistory])

  const angleData = React.useMemo<AnglePoint[]>(() => {
    if (telemetryHistory.length === 0) return []

    const baseTs = telemetryHistory[0]?.timestamp_ms ?? 0
    const mechanical = unwrapDegrees(
      telemetryHistory.map((item) => item.angles.mechanical_deg),
    )
    const electrical = unwrapDegrees(
      telemetryHistory.map((item) => item.angles.electrical_deg),
    )

    return telemetryHistory.map((item, index) => ({
      sample: index,
      timeLabel: formatRelativeTimeLabel(item.timestamp_ms, baseTs, index),
      mechanical: mechanical[index],
      electrical: electrical[index],
    }))
  }, [telemetryHistory])

  const phaseVoltageData = React.useMemo<PhaseVoltagePoint[]>(() => {
    if (telemetryHistory.length === 0) return []

    const baseTs = telemetryHistory[0]?.timestamp_ms ?? 0
    return telemetryHistory.map((item, index) => ({
      sample: index,
      timeLabel: formatRelativeTimeLabel(item.timestamp_ms, baseTs, index),
      phaseA: item.voltages.phase_a,
      phaseB: item.voltages.phase_b,
      phaseC: item.voltages.phase_c,
    }))
  }, [telemetryHistory])

  const motorXticData = React.useMemo<MotorXticPoint[]>(() => {
    if (!telemetry) return []

    const clampPercent = (value: number) => Math.max(0, Math.min(100, value))
    const rpmFactor = clampPercent((telemetry.speed.actual_rpm / 6000) * 100)
    const angleStability = clampPercent(100 - Math.abs(telemetry.angles.error_deg) * 2)

    return [
      {
        attribute: "B-EMF Strength",
        phaseA: clampPercent((telemetry.observer.bemf_strength / 255) * 100),
        phaseB: clampPercent((telemetry.observer.bemf_strength / 255) * 100),
        phaseC: clampPercent((telemetry.observer.bemf_strength / 255) * 100),
      },
      {
        attribute: "Obs. Confidence",
        phaseA: clampPercent(telemetry.observer.confidence),
        phaseB: clampPercent(telemetry.observer.confidence - 1),
        phaseC: clampPercent(telemetry.observer.confidence - 2),
      },
      {
        attribute: "PLL Lock",
        phaseA: telemetry.observer.pll_lock ? 100 : 25,
        phaseB: telemetry.observer.pll_lock ? 98 : 24,
        phaseC: telemetry.observer.pll_lock ? 96 : 23,
      },
      {
        attribute: "Model Fit",
        phaseA: 88,
        phaseB: 88,
        phaseC: 88,
      },
      {
        attribute: "Z-Cross Stab.",
        phaseA: angleStability,
        phaseB: clampPercent(angleStability - 1),
        phaseC: clampPercent(angleStability - 2),
      },
      {
        attribute: "Low-Spd Rel.",
        phaseA: clampPercent(100 - rpmFactor),
        phaseB: clampPercent(100 - rpmFactor - 2),
        phaseC: clampPercent(100 - rpmFactor - 4),
      },
      {
        attribute: "High-Spd Stab.",
        phaseA: rpmFactor,
        phaseB: clampPercent(rpmFactor - 2),
        phaseC: clampPercent(rpmFactor - 1),
      },
      {
        attribute: "Noise Immun.",
        phaseA: 76,
        phaseB: 75,
        phaseC: 74,
      },
    ]
  }, [telemetry])

  return {
    telemetry,
    chartRevision,
    speedData,
    phaseCurrentData,
    dqData,
    angleData,
    phaseVoltageData,
    motorXticData,
    batteryUsedWh: telemetry?.power.energy_used_wh,
    batteryRemainingWh: telemetry?.power.energy_remaining_wh,
  }
}