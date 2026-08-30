import { Button } from "@/components/ui/button"
import TopBar from '@/components/top-bar'
import MotorSpeedCard from '@/cards/motor-speed'
import PhaseCurrentsCard from '@/cards/phase-currents'
import BatteryCapCard from '@/cards/battery-cap'
import MotorXticsCard from '@/cards/motor-xtics'
import PhaseVoltageCard from '@/cards/phase-voltage'
import ElecMechAngleCard from '@/cards/elec-mech-angle'
import DQRefFrameCard from '@/cards/dq-ref-frame'
import { CardWrapper } from '@/components/card-wrapper'
import { useTelemetryCharts } from '@/hooks/use-telemetry-charts'

export default function Main() {
  const {
    chartRevision,
    speedData,
    phaseCurrentData,
    dqData,
    angleData,
    phaseVoltageData,
    motorXticData,
    batteryUsedWh,
    batteryRemainingWh,
  } = useTelemetryCharts()

  const openSettingsWindow = () => {
    if (window.api) {
      window.api.openNewWindow('settings')
    } else {
      console.warn('Electron API is not available in the browser environment.')
    }
  }

  return (
		<div className="min-h-screen bg-background text-foreground">

		<TopBar/>
		<div className="flex px-6 pt-3 max-w-md min-w-0 flex-col gap-1 text-sm leading-loose w-full">
			  <p className="text-sm">
			    Real-time sensorless FOC diagnostics, motor telemetry, and ESC tuning interface.
			  </p>

			  <p className="text-xs text-muted-foreground">
			    By Alperen Sahin & Amar Jay
			  </p>

			  <Button onClick={openSettingsWindow} className="mt-1 w-max">
			    Connect ESC
			  </Button>
				<div className="font-mono text-xs text-muted-foreground">
			</div>
		</div>
    <div className="flex px-6">
      <div className="px-6 pt-6 grid md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 grid-cols-1 min-w-md gap-4 text-sm leading-loose w-full">
        <CardWrapper title="Motor Speed" route="card/motor-speed">
				  <MotorSpeedCard data={speedData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Phase Currents" route="card/phase-currents">
				  <PhaseCurrentsCard data={phaseCurrentData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="DQ Reference Frame" route="card/dq-frame">
          <DQRefFrameCard data={dqData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Electrical vs Mechanical Angle" route="card/elec-mech">
          <ElecMechAngleCard data={angleData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Phase Voltages" route="card/phase-voltage">
				  <PhaseVoltageCard data={phaseVoltageData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Motor Characteristics" route="card/motor-xtics">
				  <MotorXticsCard data={motorXticData} dataRevision={chartRevision} />
        </CardWrapper>

        <CardWrapper title="Battery Capacity" route="card/battery-cap">
				  <BatteryCapCard
						usedWh={batteryUsedWh}
						remainingWh={batteryRemainingWh}
						dataRevision={chartRevision}
					/>
        </CardWrapper>
				</div>
    </div>
		</div>
  )
}