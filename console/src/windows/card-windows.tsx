import BatteryCapCard from "@/cards/battery-cap"
import DQRefFrameCard from "@/cards/dq-ref-frame"
import ElecMechAngleCard from "@/cards/elec-mech-angle"
import MotorSpeedCard from "@/cards/motor-speed"
import MotorXticsCard from "@/cards/motor-xtics"
import PhaseCurrentsCard from "@/cards/phase-currents"
import PhaseVoltageCard from "@/cards/phase-voltage"
import SubWindowLayout from "@/components/sub-window-layout"
import { useTelemetryCharts } from "@/hooks/use-telemetry-charts"

export function MotorSpeedWindow() {
  const { speedData, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="Motor Speed">
      <MotorSpeedCard data={speedData} dataRevision={chartRevision} />
    </SubWindowLayout>
  )
}

export function PhaseCurrentsWindow() {
  const { phaseCurrentData, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="Phase Currents">
      <PhaseCurrentsCard data={phaseCurrentData} dataRevision={chartRevision} />
    </SubWindowLayout>
  )
}

export function DQFrameWindow() {
  const { dqData, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="DQ Reference Frame">
      <DQRefFrameCard data={dqData} dataRevision={chartRevision} />
    </SubWindowLayout>
  )
}

export function ElecMechAngleWindow() {
  const { angleData, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="Angles">
      <ElecMechAngleCard data={angleData} dataRevision={chartRevision} />
    </SubWindowLayout>
  )
}

export function PhaseVoltageWindow() {
  const { phaseVoltageData, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="Phase Voltages">
      <PhaseVoltageCard data={phaseVoltageData} dataRevision={chartRevision} />
    </SubWindowLayout>
  )
}

export function MotorXticsWindow() {
  const { motorXticData, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="Motor Characteristics">
      <MotorXticsCard data={motorXticData} dataRevision={chartRevision} />
    </SubWindowLayout>
  )
}

export function BatteryCapWindow() {
  const { batteryUsedWh, batteryRemainingWh, chartRevision } = useTelemetryCharts()

  return (
    <SubWindowLayout title="Battery Capacity">
      <BatteryCapCard
        usedWh={batteryUsedWh}
        remainingWh={batteryRemainingWh}
        dataRevision={chartRevision}
      />
    </SubWindowLayout>
  )
}