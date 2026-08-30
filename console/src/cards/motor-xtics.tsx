import { RadarChart } from "@/components/charts/radar"

export type MotorXticPoint = {
  attribute: string
  phaseA: number
  phaseB: number
  phaseC: number
}

type MotorXticsCardProps = {
  data?: MotorXticPoint[]
  dataRevision?: string
}

export default function MotorXticsCard({ data, dataRevision }: MotorXticsCardProps) {
	const chartData = data ?? []

	return (
			        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
          <div className="mb-4 space-y-1 shrink-0">
            <h2 className="text-sm font-medium">Motor Characteristic Comparison</h2>
            <p className="text-xs text-muted-foreground">
              Radar view for efficiency, torque, and thermal benchmarks.
            </p>
          </div>
          <div className="flex-1 min-h-62.5 w-full flex items-center justify-center">
            <RadarChart
              key={dataRevision ?? "fallback"}
              className="size-full"
              data={chartData}
              indexKey="attribute"
              series={[
                { dataKey: 'phaseA', label: 'Phase A', fillOpacity: 0.5, color: "var(--chart-1)" },
                { dataKey: 'phaseB', label: 'Phase B', fillOpacity: 0.5, color: "var(--chart-2)" },
                { dataKey: 'phaseC', label: 'Phase C', fillOpacity: 0.5, color: "var(--chart-5)" },
              ]}
            />
          </div>
        </div>
	);
}
