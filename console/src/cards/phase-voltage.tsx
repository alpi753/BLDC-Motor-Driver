import { LineChart } from "@/components/charts/line"
import { useI18n } from "@/components/i18n-provider"

export type PhaseVoltagePoint = {
  sample: number
  timeLabel: string
  phaseA: number
  phaseB: number
  phaseC: number
}

type PhaseVoltageCardProps = {
  data?: PhaseVoltagePoint[]
  dataRevision?: string
}

export default function PhaseVoltageCard({ data, dataRevision }: PhaseVoltageCardProps) {
  const { t } = useI18n()
  const chartData = data ?? []
  const latest = chartData.at(-1)

  return (
    <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 flex shrink-0 items-start justify-between gap-4 pr-9">
        <div className="min-w-0 space-y-1">
          <h2 className="text-sm font-medium">{t("card.voltages.title")}</h2>
          <p className="text-xs text-muted-foreground">
            {t("card.voltages.description")}
          </p>
        </div>
        <div className="grid shrink-0 grid-cols-3 gap-2 text-right" aria-live="polite">
          {(["A", "B", "C"] as const).map((phase) => (
            <div key={phase}>
              <div className="font-mono text-sm font-medium tabular-nums">
                {latest ? latest[`phase${phase}`].toFixed(2) : "—"}
              </div>
              <div className="text-[9px] uppercase tracking-wider text-muted-foreground">{phase} · V</div>
            </div>
          ))}
        </div>
      </div>
      <div className="flex-1 min-h-50 w-full animate-in fade-in duration-500">
        <LineChart
          className="size-full"
          data={chartData}
          xKey="sample"
          dataRevision={dataRevision}
          tooltipLabelKey="timeLabel"
          yDomain={[0, 1]}
          yTickFormatter={(value) => `${Number(value).toFixed(1)}V`}
          series={[
            { dataKey: 'phaseA', label: t("card.phaseA"), color: 'var(--chart-1)', type: 'monotone' },
            { dataKey: 'phaseB', label: t("card.phaseB"), color: 'var(--chart-2)', type: 'monotone' },
            { dataKey: 'phaseC', label: t("card.phaseC"), color: 'var(--chart-5)', type: 'monotone' },
          ]}
        />
      </div>
    </div>
  )
}
