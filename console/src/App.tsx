import { Routes, Route} from 'react-router-dom'
import './App.css'
import Main from './windows/main'
import { Toaster } from "@/components/ui/sonner"
import Console from './windows/console'
import {
  BusVoltageWindow,
  PhaseCurrentsWindow,
  PhaseVoltageWindow,
  TemperaturesWindow,
} from './windows/card-windows'


function App() {
  return (
    <>
      <Routes>
        <Route path="/" element={<Main />} />
				<Route path="/console" element={<Console/>} />

        <Route path="/card/bus-voltage" element={<BusVoltageWindow />} />
        <Route path="/card/phase-currents" element={<PhaseCurrentsWindow />} />
        <Route path="/card/phase-voltage" element={<PhaseVoltageWindow />} />
        <Route path="/card/temperatures" element={<TemperaturesWindow />} />
      </Routes>
      <Toaster duration={1000} />
    </>
  )
}

export default App
