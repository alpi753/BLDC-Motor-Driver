import { Routes, Route} from 'react-router-dom'
import './App.css'
import Settings from './windows/settings'
import Main from './windows/main'
import { Toaster } from "@/components/ui/sonner"
import Console from './windows/console'
import {
  BatteryCapWindow,
  DQFrameWindow,
  ElecMechAngleWindow,
  MotorSpeedWindow,
  MotorXticsWindow,
  PhaseCurrentsWindow,
  PhaseVoltageWindow,
} from './windows/card-windows'


function App() {
  return (
    <>
      <Routes>
        <Route path="/" element={<Main />} />
        <Route path="/settings" element={<Settings />} />
				<Route path="/console" element={<Console/>} />

        <Route path="/card/motor-speed" element={<MotorSpeedWindow />} />
        <Route path="/card/phase-currents" element={<PhaseCurrentsWindow />} />
        <Route path="/card/motor-xtics" element={<MotorXticsWindow />} />
        <Route path="/card/battery-cap" element={<BatteryCapWindow />} />
        <Route path="/card/phase-voltage" element={<PhaseVoltageWindow />} />
        <Route path="/card/elec-mech" element={<ElecMechAngleWindow />} />
        <Route path="/card/dq-frame" element={<DQFrameWindow />} />
      </Routes>
      <Toaster duration={1000} />
    </>
  )
}

export default App