import TopBar from "@/components/top-bar"
import PhaseCurrentsCard from "@/cards/phase-currents"
import PhaseVoltageCard from "@/cards/phase-voltage"
import BusVoltageCard from "@/cards/bus-voltage"
import TemperaturesCard from "@/cards/temperatures"
import { CardWrapper } from "@/components/card-wrapper"
import { useTelemetryCharts } from "@/hooks/use-telemetry-charts"
import { useI18n } from "@/components/i18n-provider"

export default function Main() {
  const { t } = useI18n()
  const { telemetry, chartRevision, phaseCurrentData, phaseVoltageData, busVoltageData, temperatureData } = useTelemetryCharts()

  return (
    <div className="min-h-screen bg-background text-foreground">
      <TopBar />

      <div className="px-6 pt-4">
        <p className="text-sm">
          {t("dashboard.intro")}
        </p>
        <p className="mt-1 font-mono text-xs text-muted-foreground">
          {telemetry
            ? t("dashboard.status", { version: telemetry.protocol_version, frame: telemetry.sequence, uptime: (telemetry.uptime_ms / 1000).toFixed(1) })
            : t("dashboard.empty")}
        </p>
      </div>

      <main className="dashboard-card-grid grid min-w-0 grid-cols-1 gap-0 px-6 py-6 md:grid-cols-2 xl:grid-cols-3">
        <CardWrapper title={t("card.bus.title")} route="card/bus-voltage">
          <BusVoltageCard data={busVoltageData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title={t("card.currents.title")} route="card/phase-currents">
          <PhaseCurrentsCard data={phaseCurrentData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title={t("card.voltages.title")} route="card/phase-voltage">
          <PhaseVoltageCard data={phaseVoltageData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title={t("card.temperature.title")} route="card/temperatures">
          <TemperaturesCard data={temperatureData} dataRevision={chartRevision} />
        </CardWrapper>

      </main>
    </div>
  )
}
