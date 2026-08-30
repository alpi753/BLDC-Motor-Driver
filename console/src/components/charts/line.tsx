import * as React from "react"
import {
	CartesianGrid,
	Line,
	LineChart as RechartsLineChart,
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
import { TELEMETRY_HISTORY_LENGTH } from "@/lib/telemetry-series"

const DEFAULT_LINE_COLORS = [
	"var(--chart-1)",
	"var(--chart-2)",
	"var(--chart-3)",
	"var(--chart-4)",
	"var(--chart-5)",
] as const

export type LineChartSeries = {
	dataKey: string
	label?: React.ReactNode
	color?: string
	strokeWidth?: number
	type?: "linear" | "monotone" | "natural" | "step"
	dot?: boolean
	connectNulls?: boolean
	dashArray?: string
}

export type LineChartProps<TData extends Record<string, unknown>> =
	React.ComponentProps<"div"> & {
		data: TData[]
		xKey: keyof TData & string
		series: LineChartSeries[]
		showLegend?: boolean
		showGrid?: boolean
		showTooltip?: boolean
		yAxisWidth?: number
		xAxisType?: "number" | "category"
		xDomain?: [number, number]
		dataRevision?: string
		tooltipLabelKey?: keyof TData & string
		xTickFormatter?: (value: unknown) => string
		yTickFormatter?: (value: unknown) => string
		tooltipLabelFormatter?: (label: unknown) => React.ReactNode
	}

function LineChart<TData extends Record<string, unknown>>({
	data,
	xKey,
	series,
	className,
	showLegend = true,
	showGrid = true,
	showTooltip = true,
	yAxisWidth = 44,
	xAxisType = "number",
	xDomain = [0, TELEMETRY_HISTORY_LENGTH - 1],
	dataRevision,
	tooltipLabelKey,
	xTickFormatter,
	yTickFormatter,
	tooltipLabelFormatter,
	...props
}: LineChartProps<TData>) {
	const config = React.useMemo<ChartConfig>(() => {
		return series.reduce<ChartConfig>((accumulator, item, index) => {
			const color =
				item.color ?? DEFAULT_LINE_COLORS[index % DEFAULT_LINE_COLORS.length]

			accumulator[item.dataKey] = {
				label: item.label ?? item.dataKey,
				color,
			}

			return accumulator
		}, {})
	}, [series])

	const formatTooltipLabel = React.useCallback(
		(value: unknown, payload?: readonly unknown[]) => {
			if (tooltipLabelFormatter) {
				return tooltipLabelFormatter(value)
			}

			if (tooltipLabelKey && payload?.length) {
				const row = payload[0] as { payload?: Record<string, unknown> }
				const label = row?.payload?.[tooltipLabelKey]
				if (typeof label === "string" || typeof label === "number") {
					return label
				}
			}

			if (typeof value === "string" || typeof value === "number") {
				return value
			}

			return null
		},
		[tooltipLabelFormatter, tooltipLabelKey]
	)

	const chartKey = dataRevision ?? `${data.length}-${String(data.at(-1)?.[xKey] ?? "")}`

	return (
		<ChartContainer config={config} className={cn("aspect-auto min-h-[150px]", className)} {...props}>
			<RechartsLineChart
				key={chartKey}
				data={data}
				margin={{ top: 12, right: 12, left: 0, bottom: 4 }}
			>
				{showGrid ? (
					<CartesianGrid vertical={false} strokeDasharray="3 3" />
				) : null}
				<XAxis
					dataKey={xKey as string}
					type={xAxisType}
					domain={xAxisType === "number" ? xDomain : undefined}
					allowDecimals={false}
					axisLine={false}
					tickLine={false}
					tickMargin={10}
					tickFormatter={xTickFormatter}
					minTickGap={24}
					tickCount={xAxisType === "number" ? 6 : undefined}
				/>
				<YAxis
					axisLine={false}
					tickLine={false}
					tickMargin={10}
					width={yAxisWidth}
					tickFormatter={yTickFormatter}
					domain={["auto", "auto"]}
				/>
				{showTooltip ? (
					<ChartTooltip
						cursor={false}
						content={
							<ChartTooltipContent
								labelFormatter={formatTooltipLabel}
								indicator="line"
							/>
						}
					/>
				) : null}
				{series.map((item, index) => {
					const color =
						item.color ?? DEFAULT_LINE_COLORS[index % DEFAULT_LINE_COLORS.length]

					return (
						<Line
							key={item.dataKey}
							dataKey={item.dataKey}
							type={item.type ?? "monotone"}
							stroke={color}
							strokeWidth={item.strokeWidth ?? 2}
							strokeDasharray={item.dashArray}
							dot={item.dot ?? false}
							connectNulls={item.connectNulls ?? true}
							isAnimationActive={false}
							activeDot={{ r: 4 }}
						/>
					)
				})}
				{showLegend ? <ChartLegend content={<ChartLegendContent />} /> : null}
			</RechartsLineChart>
		</ChartContainer>
	)
}

export { LineChart }
