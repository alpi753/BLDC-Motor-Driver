import { LineChart } from "@/components/charts/line"

export type MotorSpeedPoint = {
  sample: number
  timeLabel: string
  rpm: number
  target: number
}

const fallbackMotorSpeedData: MotorSpeedPoint[] = [
  { sample: 0, timeLabel: '+0.0s', rpm: 1240, target: 1200 },
  { sample: 1, timeLabel: '+0.5s', rpm: 1310, target: 1250 },
  { sample: 2, timeLabel: '+1.0s', rpm: 1385, target: 1325 },
  { sample: 3, timeLabel: '+1.5s', rpm: 1460, target: 1400 },
  { sample: 4, timeLabel: '+2.0s', rpm: 1425, target: 1400 },
  { sample: 5, timeLabel: '+2.5s', rpm: 1510, target: 1480 },
]

type MotorSpeedCardProps = {
  data?: MotorSpeedPoint[]
  dataRevision?: string
}

export default function MotorSpeedCard({ data, dataRevision }: MotorSpeedCardProps) {
	const chartData = data && data.length > 0 ? data : fallbackMotorSpeedData

	return (
        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm break-inside-avoid">
          <div className="mb-4 space-y-1 shrink-0">
            <h2 className="text-sm font-medium">Motor speed trend</h2>
            <p className="text-xs text-muted-foreground">
              A reusable line chart example for live telemetry and setpoint tracking.
            </p>
          </div>
          <div className="flex-1 min-h-60 w-full animate-in fade-in duration-500">
            <LineChart
              className="size-full"
              data={chartData}
              xKey="sample"
              dataRevision={dataRevision}
              tooltipLabelKey="timeLabel"
              series={[
                { dataKey: 'rpm', label: 'Actual RPM' },
                { dataKey: 'target', label: 'Target RPM', dashArray: '6 6' },
              ]}
              xTickFormatter={(value) => `${value}`}
            />
          </div>
        </div>
	);
}