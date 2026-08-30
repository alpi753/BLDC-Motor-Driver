
import { RadialChart } from "@/components/charts/radial"

type BatteryCapCardProps = {
  usedWh?: number
  remainingWh?: number
  dataRevision?: string
}

export default function BatteryCapCard({
  usedWh = 640,
  remainingWh = 360,
  dataRevision,
}: BatteryCapCardProps) {
	const safeUsed = Math.max(usedWh, 0)
	const safeRemaining = Math.max(remainingWh, 0)
	const batteryData = [
	  { name: 'Power', used: safeUsed, remaining: safeRemaining }
	]

	return (
        <div className="size-full flex flex-col rounded-xl border bg-card p-4 shadow-sm break-inside-avoid">
          <div className="mb-4 space-y-1 shrink-0">
            <h2 className="text-sm font-medium">Battery Capacity</h2>
            <p className="text-xs text-muted-foreground">
              Total energy consumption and charge remaining.
            </p>
          </div>
          <div className="flex-1 min-h-50 w-full relative">
            <RadialChart
              key={dataRevision ?? "fallback"}
              className="size-full"
              data={batteryData}
              centerLabel="Wh remaining"
              centerValue={Math.round(safeRemaining)}
              series={[
                { dataKey: 'remaining', label: 'Remaining', stackId: 'a', color: 'var(--chart-2)' },
                { dataKey: 'used', label: 'Used', stackId: 'a', color: 'var(--muted)' },
              ]}
              innerRadius={80}
              outerRadius={110}
              startAngle={180}
              endAngle={0}
            />
          </div>
        </div>
	);
}