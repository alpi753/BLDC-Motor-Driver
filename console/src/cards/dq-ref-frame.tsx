import { LineChart } from "@/components/charts/line"

export type DQFramePoint = {
  sample: number
  timeLabel: string
  id: number
  iq: number
}

const generateFallbackDQData = (): DQFramePoint[] => {
  const data: DQFramePoint[] = []
  for (let i = 0; i < 50; i++) {
    data.push({
      sample: i,
      timeLabel: `+${(i * 0.1).toFixed(1)}s`,
      id: 5 + Math.sin(i / 6) * 0.2,
      iq: 12 + Math.cos(i / 7) * 0.5,
    })
  }
  return data
}

const fallbackDQData = generateFallbackDQData()

type DQRefFrameCardProps = {
  data?: DQFramePoint[]
  dataRevision?: string
}

export default function DQRefFrameCard({ data, dataRevision }: DQRefFrameCardProps) {
  const chartData = data && data.length > 0 ? data : fallbackDQData

  return (
    <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 space-y-1 shrink-0">
        <h2 className="text-sm font-medium">DQ Reference Frame</h2>
        <p className="text-xs text-muted-foreground">
          Real-time tracking of torque (Iq) and flux (Id) components.
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
            { dataKey: 'iq', label: 'Iq (Torque)', color: 'var(--chart-2)', type: 'monotone' },
            { dataKey: 'id', label: 'Id (Flux)', color: 'var(--chart-1)', type: 'monotone' },
          ]}
          showGrid={true}
          yTickFormatter={(val) => `${val}A`}
        />
      </div>
    </div>
  )
}