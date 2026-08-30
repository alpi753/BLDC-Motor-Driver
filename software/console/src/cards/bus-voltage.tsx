import { LineChart } from "@/components/charts/line"
import { useI18n } from "@/components/i18n-provider"

export type BusVoltagePoint = { sample: number; timeLabel: string; voltage: number }

export default function BusVoltageCard({ data = [], dataRevision }: { data?: BusVoltagePoint[]; dataRevision?: string }) {
  const { t } = useI18n()
  const latestVoltage = data.at(-1)?.voltage

  return (
    <div className="flex size-full flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 flex items-start justify-between gap-4 pr-9">
        <div className="min-w-0 space-y-1">
          <h2 className="text-sm font-medium">{t("card.bus.title")}</h2>
          <p className="text-xs text-muted-foreground">{t("card.bus.description")}</p>
        </div>
        <div className="shrink-0 text-right" aria-live="polite">
          <div className="font-mono text-xl font-medium tabular-nums">
            {latestVoltage === undefined ? "—" : latestVoltage.toFixed(2)}
          </div>
          <div className="text-[10px] uppercase tracking-wider text-muted-foreground">{t("card.bus.volts")}</div>
        </div>
      </div>
      <div className="min-h-50 flex-1">
        <LineChart className="size-full" data={data} xKey="sample" dataRevision={dataRevision} tooltipLabelKey="timeLabel" series={[{ dataKey: "voltage", label: t("card.bus.series"), color: "var(--chart-3)" }]} yTickFormatter={(value) => `${Number(value).toFixed(1)}V`} />
      </div>
    </div>
  )
}
