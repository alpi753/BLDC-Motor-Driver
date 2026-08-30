import * as React from "react"
import {
  PolarAngleAxis,
  PolarGrid,
  Radar,
  RadarChart as RechartsRadarChart,
} from "recharts"
import {
  ChartContainer,
  ChartLegend,
  ChartLegendContent,
  ChartTooltip,
  ChartTooltipContent,
  type ChartConfig,
} from "@/components/ui/chart"
import { cn } from "@/lib/utils"
import type { TypedDataKey } from "recharts/types/util/typedDataKey"

const DEFAULT_RADAR_COLORS = [
  "var(--chart-1)",
  "var(--chart-2)",
  "var(--chart-3)",
  "var(--chart-4)",
  "var(--chart-5)",
] as const

export type RadarChartSeries = {
  dataKey: string
  label?: React.ReactNode
  color?: string
  fillOpacity?: number
  strokeWidth?: number
}

export type RadarChartProps<TData extends Record<string, unknown>> =
  React.ComponentProps<"div"> & {
    data: TData[]
    indexKey: keyof TData & string
    series: RadarChartSeries[]
    showGrid?: boolean
    showTooltip?: boolean
    showLegend?: boolean
    showPolarAngleAxis?: boolean
  }

export function RadarChart<TData extends Record<string, unknown>>({
  data,
  indexKey,
  series,
  className,
  showGrid = true,
  showTooltip = true,
  showLegend = true,
  showPolarAngleAxis = true,
  ...props
}: RadarChartProps<TData>) {
  const config = React.useMemo<ChartConfig>(() => {
    return series.reduce<ChartConfig>((accumulator, item, index) => {
      const color =
        item.color ?? DEFAULT_RADAR_COLORS[index % DEFAULT_RADAR_COLORS.length]
      accumulator[item.dataKey] = {
        label: item.label ?? item.dataKey,
        color,
      }
      return accumulator
    }, {})
  }, [series])

  return (
    <div className={cn("flex flex-col", className)} {...props}>
      <ChartContainer config={config} className="w-full h-full aspect-square">
        <RechartsRadarChart data={data} cx="50%" cy="50%" outerRadius="80%">
          {showGrid && (
            <PolarGrid strokeDasharray="3 3" className="stroke-muted" />
          )}
          {showPolarAngleAxis && (
            <PolarAngleAxis
              dataKey={indexKey as TypedDataKey<TData, unknown>}
              tick={{ fontSize: 10, fill: "var(--muted-foreground)" }}
            />
          )}
          {showTooltip && (
            <ChartTooltip
              cursor={false}
              content={<ChartTooltipContent indicator="line" />}
            />
          )}
          {series.map((item) => (
            <Radar
              key={item.dataKey}
              name={item.dataKey}
              dataKey={item.dataKey}
              stroke={`var(--color-${item.dataKey})`}
              fill={`var(--color-${item.dataKey})`}
              fillOpacity={item.fillOpacity ?? 0.5}
              strokeWidth={item.strokeWidth ?? 2}
              isAnimationActive={false}
            />
          ))}
          {showLegend && (
            <ChartLegend content={<ChartLegendContent />} />
          )}
        </RechartsRadarChart>
      </ChartContainer>
    </div>
  )
}
