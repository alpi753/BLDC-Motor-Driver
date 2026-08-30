import type { MotorSettings } from "./settings"

export {}

declare global {
  interface Window {
    api: {
      openNewWindow: (path: string) => void
			file: {
				saveFile: (data: ArrayBuffer, filePath: string) => Promise<void>
			},
      window: {
        close: () => void
        minimize: () => void
        maximize: () => void
        unmaximize: () => void
        isMaximized: () => Promise<boolean>
      }
      usb: {
        list: () => Promise<Device[]>
				disconnect: (id: string) => Promise<void>
        connect: (id: string) => Promise<Device>
        refresh: () => Promise<Device[]>
        onUpdate: (cb: (devices: Device[]) => void) => () => void 
        sendData?: (data: string) => Promise<unknown>
        sendSettings?: (settings: MotorSettings) => Promise<void>
        setupPortReader?: () => Promise<unknown>
        onData?: (cb: (msg: string) => void) => () => void
        offData?: () => void
        getTelemetryHistory?: () => Promise<TelemetryData[]>
        onTelemetry?: (cb: (telem: TelemetryData) => void) => () => void
        onTelemetryHistory?: (cb: (history: TelemetryData[]) => void) => () => void
        offTelemetry?: () => void
        offTelemetryHistory?: () => void
      }
    }
  }

  type TelemetryData = {
    protocol_version: number
    sequence: number
    uptime_ms: number
    bus_voltage_v: number
    ntc_pcb_temperature_c: number | null
    currents_a: {
      phase_a: number
      phase_b: number
      phase_c: number
    }
    voltages_v: {
      phase_a: number
      phase_b: number
      phase_c: number
    }
  }

	/** define a usb device type */ 
	type Device = {
	  connected?: boolean
		path: string
	  manufacturer?: string
	  serialNumber?: string
	  pnpId?: string
	  locationId?: string
	  productId?: string
	  vendorId?: string
	}
}
