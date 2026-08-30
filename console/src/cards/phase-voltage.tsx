import { LineChart } from "@/components/charts/line"

export type PhaseVoltagePoint = {
  sample: number
  timeLabel: string
  phaseA: number
  phaseB: number
  phaseC: number
}

const generateFallbackPhaseData = (): PhaseVoltagePoint[] => {
  const data: PhaseVoltagePoint[] = []
  for (let i = 0; i < 360; i += 10) {
    const rad = (i * Math.PI) / 180
    data.push({
      sample: i / 10,
      timeLabel: `${i}°`,
      phaseA: Math.sin(rad) * 230,
      phaseB: Math.sin(rad - (2 * Math.PI) / 3) * 230,
      phaseC: Math.sin(rad - (4 * Math.PI) / 3) * 230,
    })
  }
  return data
}

const fallbackPhaseVoltageData = generateFallbackPhaseData()

type PhaseVoltageCardProps = {
  data?: PhaseVoltagePoint[]
  dataRevision?: string
}

export default function PhaseVoltageCard({ data, dataRevision }: PhaseVoltageCardProps) {
  const chartData = data && data.length > 0 ? data : fallbackPhaseVoltageData

  return (
    <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 space-y-1 shrink-0">
        <h2 className="text-sm font-medium">Phase Voltages (Sinusoidal)</h2>
        <p className="text-xs text-muted-foreground">
          Real-time waveform tracking for Phase A, B, and C.
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
            { dataKey: 'phaseA', label: 'Phase A', color: 'var(--chart-1)', type: 'monotone' },
            { dataKey: 'phaseB', label: 'Phase B', color: 'var(--chart-2)', type: 'monotone' },
            { dataKey: 'phaseC', label: 'Phase C', color: 'var(--chart-5)', type: 'monotone' },
          ]}
        />
      </div>
    </div>
  )
}