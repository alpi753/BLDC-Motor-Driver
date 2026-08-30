import { LineChart } from "@/components/charts/line"

export type MotorSpeedPoint = {
  sample: number
  timeLabel: string
  rpm: number
}

type MotorSpeedCardProps = {
  data?: MotorSpeedPoint[]
  dataRevision?: string
}

export default function MotorSpeedCard({ data, dataRevision }: MotorSpeedCardProps) {
	const chartData = data ?? []

	return (
        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm break-inside-avoid">
          <div className="mb-4 space-y-1 shrink-0">
            <h2 className="text-sm font-medium">Motor speed trend</h2>
            <p className="text-xs text-muted-foreground">
              Requires a measured RPM field in the telemetry schema.
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
                { dataKey: 'rpm', label: 'RPM' },
              ]}
              xTickFormatter={(value) => `${value}`}
            />
          </div>
        </div>
	);
}
