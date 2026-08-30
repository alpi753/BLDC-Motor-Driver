import * as React from "react"
import {
  Bar,
  BarChart as RechartsBarChart,
  CartesianGrid,
  XAxis,
  YAxis,
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

const DEFAULT_BAR_COLORS = [
  "var(--chart-1)",
  "var(--chart-2)",
  "var(--chart-3)",
  "var(--chart-4)",
  "var(--chart-5)",
] as const

export type BarChartSeries = {
  dataKey: string
  label?: React.ReactNode
  color?: string
  radius?: number | [number, number, number, number]
  stackId?: string
}

export type BarChartProps<TData extends Record<string, unknown>> =
  React.ComponentProps<"div"> & {
    data: TData[]
    xKey: keyof TData & string
    series: BarChartSeries[]
    showLegend?: boolean
    showGrid?: boolean
    showTooltip?: boolean
    yAxisWidth?: number
    layout?: "horizontal" | "vertical"
    xTickFormatter?: (value: unknown) => string
    yTickFormatter?: (value: unknown) => string
    tooltipLabelFormatter?: (label: unknown) => React.ReactNode
  }

export function BarChart<TData extends Record<string, unknown>>({
  data,
  xKey,
  series,
  className,
  showLegend = true,
  showGrid = true,
  showTooltip = true,
  yAxisWidth = 44,
  layout = "horizontal",
  xTickFormatter,
  yTickFormatter,
  tooltipLabelFormatter,
  ...props
}: BarChartProps<TData>) {
  const config = React.useMemo<ChartConfig>(() => {
    return series.reduce<ChartConfig>((accumulator, item, index) => {
      const color =
        item.color ?? DEFAULT_BAR_COLORS[index % DEFAULT_BAR_COLORS.length]

      accumulator[item.dataKey] = {
        label: item.label ?? item.dataKey,
        color,
      }

      return accumulator
    }, {})
  }, [series])

  const formatTooltipLabel = React.useCallback(
    (value: unknown) => {
      if (tooltipLabelFormatter) {
        return tooltipLabelFormatter(value)
      }

      if (typeof value === "string" || typeof value === "number") {
        return value
      }

      return null
    },
    [tooltipLabelFormatter]
  )

  return (
    <ChartContainer config={config} className={cn("aspect-auto min-h-[150px]", className)} {...props}>
      <RechartsBarChart
        data={data}
        layout={layout}
        margin={{ top: 12, right: 8, left: 0, bottom: 0 }}
      >
        {showGrid && (
          <CartesianGrid
            vertical={layout === "vertical"}
            horizontal={layout === "horizontal"}
            strokeDasharray="4 4"
            className="stroke-muted"
          />
        )}
        <XAxis
          dataKey={layout === "horizontal" ? xKey as TypedDataKey<TData, unknown> : undefined}
          type={layout === "horizontal" ? "category" : "number"}
          tickLine={false}
          axisLine={false}
          tickMargin={8}
          tickFormatter={xTickFormatter}
          className="text-[10px] fill-muted-foreground"
          hide={layout === "vertical"}
        />
        <YAxis
          dataKey={layout === "vertical" ? xKey as TypedDataKey<TData, unknown>: undefined}
          type={layout === "vertical" ? "category" : "number"}
          width={yAxisWidth}
          tickLine={false}
          axisLine={false}
          tickMargin={8}
          tickFormatter={yTickFormatter}
          className="text-[10px] fill-muted-foreground"
        />
        {showTooltip && (
          <ChartTooltip
            content={
              <ChartTooltipContent
                labelFormatter={formatTooltipLabel}
              />
            }
          />
        )}
        {showLegend && (
          <ChartLegend content={<ChartLegendContent />} className="pt-4" />
        )}
        {series.map((item) => (
          <Bar
            key={item.dataKey}
            dataKey={item.dataKey}
            fill={`var(--color-${item.dataKey})`}
            radius={item.radius ?? [4, 4, 0, 0]}
            stackId={item.stackId}
            isAnimationActive={false}
          />
        ))}
      </RechartsBarChart>
    </ChartContainer>
  )
}
