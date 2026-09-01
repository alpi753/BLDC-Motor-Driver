import type { PhaseCurrentPoint } from "@/cards/phase-currents"
import type { PhaseVoltagePoint } from "@/cards/phase-voltage"
import type { BusVoltagePoint } from "@/cards/bus-voltage"
import type { TemperaturePoint } from "@/cards/temperatures"
import { TELEMETRY_HISTORY_LENGTH, chartDataSignature, formatRelativeTimeLabel } from "@/lib/telemetry-series"
import * as React from "react"
import { useI18n } from "@/components/i18n-provider"

const historyPoint = (item: TelemetryData, sample: number, baseUptime: number) => ({
  sample,
  timeLabel: formatRelativeTimeLabel(item.uptime_ms, baseUptime, sample),
})

export function useTelemetryCharts() {
  const { t } = useI18n()
  const [telemetryHistory, setTelemetryHistory] = React.useState<TelemetryData[]>([])

  React.useEffect(() => {
    let alive = true
    const applyHistory = (history: TelemetryData[]) => {
      if (alive) setTelemetryHistory(history.slice(-TELEMETRY_HISTORY_LENGTH))
    }
    window.api.usb.getTelemetryHistory?.().then((history) => {
      if (Array.isArray(history)) applyHistory(history)
    })
    const unsubscribe = window.api.usb.onTelemetryHistory?.(applyHistory)
    return () => { alive = false; unsubscribe?.() }
  }, [])

  const telemetry = telemetryHistory.at(-1) ?? null
  const chartRevision = chartDataSignature(telemetryHistory.length, telemetry?.sequence)
  const phaseCurrentData = React.useMemo<PhaseCurrentPoint[]>(() => telemetry ? [
    { phase: t("card.phaseA"), current: telemetry.currents_a.phase_a },
    { phase: t("card.phaseB"), current: telemetry.currents_a.phase_b },
    { phase: t("card.phaseC"), current: telemetry.currents_a.phase_c },
  ] : [], [telemetry, t])
  const phaseVoltageData = React.useMemo<PhaseVoltagePoint[]>(() => {
    const base = telemetryHistory[0]?.uptime_ms ?? 0
    return telemetryHistory.map((item, sample) => ({
      sample, timeLabel: formatRelativeTimeLabel(item.uptime_ms, base, sample),
      phaseA: item.voltages_v.phase_a,
      phaseB: item.voltages_v.phase_b,
      phaseC: item.voltages_v.phase_c,
    }))
  }, [telemetryHistory])

  const baseUptime = telemetryHistory[0]?.uptime_ms ?? 0
  const busVoltageData = React.useMemo<BusVoltagePoint[]>(() => telemetryHistory.map((item, sample) => ({ ...historyPoint(item, sample, baseUptime), voltage: item.bus_voltage_v })), [telemetryHistory, baseUptime])
  const temperatureData = React.useMemo<TemperaturePoint[]>(() => telemetryHistory.map((item, sample) => ({
    ...historyPoint(item, sample, baseUptime),
    pcb: item.ntc_pcb_temperature_c,
    mcu: item.mcu_temperature_c,
  })), [telemetryHistory, baseUptime])

  return { telemetry, telemetryHistory, chartRevision, phaseCurrentData, phaseVoltageData, busVoltageData, temperatureData }
}
