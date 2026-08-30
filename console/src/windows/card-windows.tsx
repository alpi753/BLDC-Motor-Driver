import PhaseCurrentsCard from "@/cards/phase-currents"
import PhaseVoltageCard from "@/cards/phase-voltage"
import BusVoltageCard from "@/cards/bus-voltage"
import TemperaturesCard from "@/cards/temperatures"
import SubWindowLayout from "@/components/sub-window-layout"
import { useTelemetryCharts } from "@/hooks/use-telemetry-charts"
import { useI18n } from "@/components/i18n-provider"

export function PhaseCurrentsWindow() {
  const { t } = useI18n()
  const { phaseCurrentData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title={t("card.currents.title")}><PhaseCurrentsCard data={phaseCurrentData} dataRevision={chartRevision} /></SubWindowLayout>
}

export function PhaseVoltageWindow() {
  const { t } = useI18n()
  const { phaseVoltageData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title={t("card.voltages.title")}><PhaseVoltageCard data={phaseVoltageData} dataRevision={chartRevision} /></SubWindowLayout>
}

export function BusVoltageWindow() {
  const { t } = useI18n()
  const { busVoltageData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title={t("card.bus.title")}><BusVoltageCard data={busVoltageData} dataRevision={chartRevision} /></SubWindowLayout>
}

export function TemperaturesWindow() {
  const { t } = useI18n()
  const { temperatureData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title={t("card.temperature.title")}><TemperaturesCard data={temperatureData} dataRevision={chartRevision} /></SubWindowLayout>
}
