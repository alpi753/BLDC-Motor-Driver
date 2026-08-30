import { LineChart } from "@/components/charts/line"

export type AnglePoint = {
  sample: number
  timeLabel: string
  mechanical: number
  electrical: number
}

const generateFallbackAngleTimeSeries = (): AnglePoint[] => {
  const data: AnglePoint[] = []
  const polePairs = 7
  const totalPoints = 50
  
  for (let i = 0; i < totalPoints; i++) {
    const mechanical = (i * 15) % 360 // Simulating rotation over time
    data.push({
      sample: i,
      timeLabel: `+${(i * 0.1).toFixed(1)}s`,
      mechanical: mechanical,
      electrical: (mechanical * polePairs) % 360,
    })
  }
  return data
}

const fallbackAngleTimeSeriesData = generateFallbackAngleTimeSeries()

type ElecMechAngleCardProps = {
  data?: AnglePoint[]
  dataRevision?: string
}

export default function ElecMechAngleCard({ data, dataRevision }: ElecMechAngleCardProps) {
  const chartData = data && data.length > 0 ? data : fallbackAngleTimeSeriesData

  return (
    <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 space-y-1 shrink-0">
        <h2 className="text-sm font-medium">Elec. vs Mech. Angle (Live)</h2>
        <p className="text-xs text-muted-foreground">
          Real-time time-series tracking of motor rotor position vs. magnetic field cycles.
        </p>
      </div>
      <div className="flex-1 min-h-50 w-full animate-in fade-in duration-500">
        <LineChart
          className="size-full"
          data={chartData}
          xKey="sample"
          dataRevision={dataRevision}
          tooltipLabelKey="timeLabel"
          series={[
            { dataKey: 'mechanical', label: 'Mech Angle', color: 'var(--chart-1)', type: 'monotone', strokeWidth: 2 },
            { dataKey: 'electrical', label: 'Elec Angle', color: 'var(--chart-2)', type: 'monotone', strokeWidth: 1.5 },
          ]}
          showGrid={true}
          yTickFormatter={(val) => `${Number(val).toFixed(0)}°`}
        />
      </div>
    </div>
  )
}