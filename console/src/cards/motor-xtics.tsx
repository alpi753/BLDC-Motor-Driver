import { RadarChart } from "@/components/charts/radar"

export type MotorXticPoint = {
  attribute: string
  phaseA: number
  phaseB: number
  phaseC: number
}

const fallbackPerformanceData: MotorXticPoint[] = [
  {
    attribute: "B-EMF Strength",
    phaseA: 88,
    phaseB: 86,
    phaseC: 87,
  },
  {
    attribute: "Obs. Confidence",
    phaseA: 92,
    phaseB: 90,
    phaseC: 91,
  },
  {
    attribute: "PLL Lock",
    phaseA: 85,
    phaseB: 83,
    phaseC: 84,
  },
  {
    attribute: "Model Fit",
    phaseA: 89,
    phaseB: 88,
    phaseC: 90,
  },
  {
    attribute: "Z-Cross Stab.",
    phaseA: 81,
    phaseB: 79,
    phaseC: 80,
  },
  {
    attribute: "Low-Spd Rel.",
    phaseA: 62,
    phaseB: 65,
    phaseC: 64,
  },
  {
    attribute: "High-Spd Stab.",
    phaseA: 93,
    phaseB: 91,
    phaseC: 92,
  },
  {
    attribute: "Noise Immun.",
    phaseA: 77,
    phaseB: 75,
    phaseC: 76,
  },
]

type MotorXticsCardProps = {
  data?: MotorXticPoint[]
  dataRevision?: string
}

export default function MotorXticsCard({ data, dataRevision }: MotorXticsCardProps) {
	const chartData = data && data.length > 0 ? data : fallbackPerformanceData

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