import { LineChart } from "@/components/charts/line"
import { useI18n } from "@/components/i18n-provider"
import { temperatureDomainC } from "@/lib/telemetry-series"

export type TemperaturePoint = {
  sample: number
  timeLabel: string
  pcb: number | null
  mcu: number | null
}

const formatCelsius = (value: number | null | undefined) =>
  value == null ? "—" : value.toFixed(1)

export default function TemperaturesCard({ data = [], dataRevision }: { data?: TemperaturePoint[]; dataRevision?: string }) {
  const { t } = useI18n()
  const latest = data.at(-1)
  const yDomain = temperatureDomainC(data.flatMap((point) => [point.pcb, point.mcu]))

  return (
    <div className="flex size-full flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 flex items-start justify-between gap-4 pr-9">
        <div className="min-w-0 space-y-1">
          <h2 className="text-sm font-medium">{t("card.temperature.title")}</h2>
          <p className="text-xs text-muted-foreground">{t("card.temperature.description")}</p>
        </div>
        <div className="grid shrink-0 grid-cols-2 gap-3 text-right" aria-live="polite">
          <div>
            <div className="font-mono text-sm font-medium tabular-nums">{formatCelsius(latest?.pcb)}</div>
            <div className="text-[9px] uppercase tracking-wider text-muted-foreground">{t("card.temperature.pcb")} · °C</div>
          </div>
          <div>
            <div className="font-mono text-sm font-medium tabular-nums">{formatCelsius(latest?.mcu)}</div>
            <div className="text-[9px] uppercase tracking-wider text-muted-foreground">{t("card.temperature.mcu")} · °C</div>
          </div>
        </div>
      </div>
      <div className="min-h-50 flex-1">
        <LineChart
          className="size-full"
          data={data}
          xKey="sample"
          dataRevision={dataRevision}
          tooltipLabelKey="timeLabel"
          yDomain={yDomain}
          yTickFormatter={(value) => `${Number(value).toFixed(0)}°C`}
          series={[
            { dataKey: "pcb", label: t("card.temperature.pcb"), color: "var(--chart-4)" },
            { dataKey: "mcu", label: t("card.temperature.mcu"), color: "var(--chart-1)" },
          ]}
        />
      </div>
    </div>
  )
}
