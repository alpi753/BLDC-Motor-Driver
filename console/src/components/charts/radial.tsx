import * as React from "react"
import {
  Label,
  PolarRadiusAxis,
  RadialBar,
  RadialBarChart as RechartsRadialBarChart,
} from "recharts"
import {
  ChartContainer,
  ChartTooltip,
  ChartTooltipContent,
  type ChartConfig,
} from "@/components/ui/chart"
import { cn } from "@/lib/utils"

const DEFAULT_RADIAL_COLORS = [
  "var(--chart-1)",
  "var(--chart-2)",
  "var(--chart-3)",
  "var(--chart-4)",
  "var(--chart-5)",
] as const

export type RadialChartSeries = {
  dataKey: string
  label?: React.ReactNode
  color?: string
  stackId?: string
  cornerRadius?: number
}

export type RadialChartProps<TData extends Record<string, unknown>> =
  React.ComponentProps<"div"> & {
    data: TData[]
    series: RadialChartSeries[]
    innerRadius?: number
    outerRadius?: number
    startAngle?: number
    endAngle?: number
    centerLabel?: string
    centerValue?: string | number
    showTooltip?: boolean
  }

export function RadialChart<TData extends Record<string, unknown>>({
  data,
  series,
  innerRadius = 80,
  outerRadius = 110,
  startAngle = 90,
  endAngle = 450,
  centerLabel,
  centerValue,
  showTooltip = true,
  className,
  ...props  // ✅ now safely spread onto the outer <div>
}: RadialChartProps<TData>) {
  const config = React.useMemo<ChartConfig>(() => {
    return series.reduce<ChartConfig>((accumulator, item, index) => {
      const color =
        item.color ?? DEFAULT_RADIAL_COLORS[index % DEFAULT_RADIAL_COLORS.length]
      accumulator[item.dataKey] = {
        label: item.label ?? item.dataKey,
        color,
      }
      return accumulator
    }, {})
  }, [series])

  return (
    <div className={cn("mx-auto w-full h-full min-h-0 overflow-hidden relative", className)} {...props}>
      <ChartContainer config={config} className="size-full">
        <RechartsRadialBarChart
          data={data}
          startAngle={startAngle}
          endAngle={endAngle}
          innerRadius={innerRadius}
          outerRadius={outerRadius}
          cx="50%"
          cy="70%"
        >
            {showTooltip && (
              <ChartTooltip
                cursor={false}
                content={<ChartTooltipContent hideLabel />}
              />
            )}

            {series.map((item) => (
              <RadialBar
                key={item.dataKey}
                dataKey={item.dataKey}
                fill={`var(--color-${item.dataKey})`}
                stackId={item.stackId ?? item.dataKey}
                cornerRadius={item.cornerRadius ?? 5}
                className="stroke-transparent stroke-2 pt-0"
                isAnimationActive={false}
              />
            ))}

            {(centerLabel || centerValue) && (
              <PolarRadiusAxis tick={false} tickLine={false} axisLine={false}>
                <Label
                  content={({ viewBox }) => {
                    if (viewBox && "cx" in viewBox && "cy" in viewBox) {
                      return (
                        <text x={viewBox.cx} y={viewBox.cy} textAnchor="middle">
                          <tspan
                            x={viewBox.cx}
                            y={(viewBox.cy || 0) - 10}
                            fontSize={24}
                            fontWeight="bold"
                            fill="var(--foreground)"
                          >
                            {typeof centerValue === "number"
                              ? centerValue.toLocaleString()
                              : centerValue}
                          </tspan>
                          <tspan
                            x={viewBox.cx}
                            y={(viewBox.cy || 0) + 14}
                            fontSize={10}
                            fill="var(--muted-foreground)"
                          >
                            {centerLabel}
                          </tspan>
                        </text>
                      )
                    }
                  }}
                />
              </PolarRadiusAxis>
            )}
          </RechartsRadialBarChart>
        </ChartContainer>
    </div>
  )
}

