import { BarChart } from "@/components/charts/bar"

export type PhaseCurrentPoint = {
  phase: string
  current: number
}

type PhaseCurrentsCardProps = {
  data?: PhaseCurrentPoint[]
  dataRevision?: string
}

export default function PhaseCurrentsCard({ data, dataRevision }: PhaseCurrentsCardProps) {
	const chartData = data ?? []

	return (
        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
          <div className="mb-4 space-y-1 shrink-0">
            <h2 className="text-sm font-medium">Phase currents</h2>
            <p className="text-xs text-muted-foreground">
              ADC measurements after the DRV8323 current-sense amplifiers.
            </p>
          </div>
          <div className="flex-1 min-h-50 w-full">
            <BarChart
              key={dataRevision ?? "fallback"}
              className="size-full"
              data={chartData}
              xKey="phase"
              series={[
                { dataKey: 'current', label: 'Amps (A)', radius: [4, 4, 0, 0] },
              ]}
            />
          </div>
        </div>
	);
}
