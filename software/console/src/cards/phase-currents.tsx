import { BarChart } from "@/components/charts/bar"
import { useI18n } from "@/components/i18n-provider"
import { formatSignedAmpTick, symmetricCurrentDomainA } from "@/lib/telemetry-series"

export type PhaseCurrentPoint = {
  phase: string
  current: number
}

type PhaseCurrentsCardProps = {
  data?: PhaseCurrentPoint[]
  dataRevision?: string
}

export default function PhaseCurrentsCard({ data }: PhaseCurrentsCardProps) {
	const { t } = useI18n()
	const chartData = data ?? []
	const yDomain = symmetricCurrentDomainA(chartData.map((point) => point.current))

	return (
        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
          <div className="mb-4 flex shrink-0 items-start justify-between gap-4 pr-9">
            <div className="min-w-0 space-y-1">
              <h2 className="text-sm font-medium">{t("card.currents.title")}</h2>
              <p className="text-xs text-muted-foreground">
                {t("card.currents.description")}
              </p>
            </div>
            <div className="grid shrink-0 grid-cols-3 gap-2 text-right" aria-live="polite">
              {(["A", "B", "C"] as const).map((phase, index) => (
                <div key={phase}>
                  <div className="font-mono text-sm font-medium tabular-nums">{chartData[index]?.current.toFixed(2) ?? "—"}</div>
                  <div className="text-[9px] uppercase tracking-wider text-muted-foreground">{phase} · A</div>
                </div>
              ))}
            </div>
          </div>
          <div className="flex-1 min-h-50 w-full">
            <BarChart
              className="size-full"
              data={chartData}
              xKey="phase"
              yDomain={yDomain}
              showZeroLine
              yAxisWidth={56}
              yTickFormatter={formatSignedAmpTick}
              series={[
                { dataKey: 'current', label: t("card.currents.series"), radius: 0 },
              ]}
              cellColors={["var(--chart-1)", "var(--chart-2)", "var(--chart-5)"]}
            />
          </div>
        </div>
	);
}
