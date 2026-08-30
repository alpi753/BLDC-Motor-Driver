import { LineChart } from "@/components/charts/line"

export type TemperaturePoint = { sample: number; timeLabel: string; pcb: number | null }

export default function TemperaturesCard({ data = [], dataRevision }: { data?: TemperaturePoint[]; dataRevision?: string }) {
  return (
    <div className="flex size-full flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 space-y-1">
        <h2 className="text-sm font-medium">PCB temperature</h2>
        <p className="text-xs text-muted-foreground">Live reading from the board's 10 kΩ NTC thermistor.</p>
      </div>
      <div className="min-h-50 flex-1">
        <LineChart
          className="size-full"
          data={data}
          xKey="sample"
          dataRevision={dataRevision}
          tooltipLabelKey="timeLabel"
          series={[{ dataKey: "pcb", label: "PCB NTC", color: "var(--chart-2)" }]}
          yDomain={[0, 100]}
          yTickFormatter={(value) => `${Number(value).toFixed(0)}°C`}
        />
      </div>
    </div>
  )
}
