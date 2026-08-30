import { LineChart } from "@/components/charts/line"
import { useI18n } from "@/components/i18n-provider"

export type TemperaturePoint = { sample: number; timeLabel: string; pcb: number | null }

export default function TemperaturesCard({ data = [], dataRevision }: { data?: TemperaturePoint[]; dataRevision?: string }) {
  const { t } = useI18n()
  const latestTemperature = data.at(-1)?.pcb

  return (
    <div className="flex size-full flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 flex items-start justify-between gap-4 pr-9">
        <div className="min-w-0 space-y-1">
          <h2 className="text-sm font-medium">{t("card.temperature.title")}</h2>
          <p className="text-xs text-muted-foreground">{t("card.temperature.description")}</p>
        </div>
        <div className="shrink-0 text-right" aria-live="polite">
          <div className="font-mono text-xl font-medium tabular-nums">
            {latestTemperature == null ? "—" : latestTemperature.toFixed(1)}
          </div>
          <div className="text-[10px] uppercase tracking-wider text-muted-foreground">°C</div>
        </div>
      </div>
      <div className="min-h-50 flex-1">
        <LineChart
          className="size-full"
          data={data}
          xKey="sample"
          dataRevision={dataRevision}
          tooltipLabelKey="timeLabel"
          series={[{ dataKey: "pcb", label: t("card.temperature.series"), color: "var(--chart-4)" }]}
          yDomain={[10, 35]}
          yTickFormatter={(value) => `${Number(value).toFixed(0)}°C`}
        />
      </div>
    </div>
  )
}
