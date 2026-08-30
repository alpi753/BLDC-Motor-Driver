import { LineChart } from "@/components/charts/line"

export type BusVoltagePoint = { sample: number; timeLabel: string; voltage: number }

export default function BusVoltageCard({ data = [], dataRevision }: { data?: BusVoltagePoint[]; dataRevision?: string }) {
  return (
    <div className="flex size-full flex-col rounded-xl border bg-card p-4 shadow-sm">
      <div className="mb-4 space-y-1">
        <h2 className="text-sm font-medium">DC bus voltage</h2>
        <p className="text-xs text-muted-foreground">Voltage measured through the board-level bus divider.</p>
      </div>
      <div className="min-h-50 flex-1">
        <LineChart className="size-full" data={data} xKey="sample" dataRevision={dataRevision} tooltipLabelKey="timeLabel" series={[{ dataKey: "voltage", label: "Bus (V)", color: "var(--chart-2)" }]} yTickFormatter={(value) => `${Number(value).toFixed(1)}V`} />
      </div>
    </div>
  )
}
