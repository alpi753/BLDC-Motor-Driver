import TopBar from "@/components/top-bar"
import PhaseCurrentsCard from "@/cards/phase-currents"
import PhaseVoltageCard from "@/cards/phase-voltage"
import BusVoltageCard from "@/cards/bus-voltage"
import TemperaturesCard from "@/cards/temperatures"
import { CardWrapper } from "@/components/card-wrapper"
import { useTelemetryCharts } from "@/hooks/use-telemetry-charts"

export default function Main() {
  const { telemetry, chartRevision, phaseCurrentData, phaseVoltageData, busVoltageData, temperatureData } = useTelemetryCharts()

  return (
    <div className="min-h-screen bg-background text-foreground">
      <TopBar />

      <div className="px-6 pt-4">
        <p className="text-sm">
          Real-time motor-controller telemetry over USB serial.
        </p>
        <p className="mt-1 font-mono text-xs text-muted-foreground">
          {telemetry
            ? `protobuf v${telemetry.protocol_version} · frame ${telemetry.sequence} · ${(telemetry.uptime_ms / 1000).toFixed(1)} s uptime`
            : "Connect a controller to begin receiving telemetry."}
        </p>
      </div>

      <main className="grid min-w-0 grid-cols-1 gap-4 px-6 py-6 md:grid-cols-2 xl:grid-cols-3">
        <CardWrapper title="DC Bus Voltage" route="card/bus-voltage">
          <BusVoltageCard data={busVoltageData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Phase Currents" route="card/phase-currents">
          <PhaseCurrentsCard data={phaseCurrentData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Phase Voltages" route="card/phase-voltage">
          <PhaseVoltageCard data={phaseVoltageData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="PCB Temperature" route="card/temperatures">
          <TemperaturesCard data={temperatureData} dataRevision={chartRevision} />
        </CardWrapper>

      </main>
    </div>
  )
}
