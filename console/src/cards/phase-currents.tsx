import { BarChart } from "@/components/charts/bar"

export type PhaseCurrentPoint = {
  phase: string
  current: number
}

const fallbackCurrentDrawData: PhaseCurrentPoint[] = [
  { phase: 'Phase A', current: 12.5 },
  { phase: 'Phase B', current: 14.2 },
  { phase: 'Phase C', current: 13.8 },
]

type PhaseCurrentsCardProps = {
  data?: PhaseCurrentPoint[]
  dataRevision?: string
}

export default function PhaseCurrentsCard({ data, dataRevision }: PhaseCurrentsCardProps) {
	const chartData = data && data.length > 0 ? data : fallbackCurrentDrawData

	return (
        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
          <div className="mb-4 space-y-1 shrink-0">
            <h2 className="text-sm font-medium">Phase Current</h2>
            <p className="text-xs text-muted-foreground">
              Real-time current distribution across motor phases.
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