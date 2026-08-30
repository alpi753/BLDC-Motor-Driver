import PhaseCurrentsCard from "@/cards/phase-currents"
import PhaseVoltageCard from "@/cards/phase-voltage"
import BusVoltageCard from "@/cards/bus-voltage"
import TemperaturesCard from "@/cards/temperatures"
import SubWindowLayout from "@/components/sub-window-layout"
import { useTelemetryCharts } from "@/hooks/use-telemetry-charts"

export function PhaseCurrentsWindow() {
  const { phaseCurrentData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title="Phase Currents"><PhaseCurrentsCard data={phaseCurrentData} dataRevision={chartRevision} /></SubWindowLayout>
}

export function PhaseVoltageWindow() {
  const { phaseVoltageData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title="Phase Voltages"><PhaseVoltageCard data={phaseVoltageData} dataRevision={chartRevision} /></SubWindowLayout>
}

export function BusVoltageWindow() {
  const { busVoltageData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title="DC Bus Voltage"><BusVoltageCard data={busVoltageData} dataRevision={chartRevision} /></SubWindowLayout>
}

export function TemperaturesWindow() {
  const { temperatureData, chartRevision } = useTelemetryCharts()
  return <SubWindowLayout title="PCB Temperature"><TemperaturesCard data={temperatureData} dataRevision={chartRevision} /></SubWindowLayout>
}
