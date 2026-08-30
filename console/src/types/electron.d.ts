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
    device_id: string
    speed: {
      actual_rpm: number
      target_rpm: number
    }
    currents: {
      phase_a: number
      phase_b: number
      phase_c: number
      i_d: number
      i_q: number
    }
    voltages: {
      phase_a: number
      phase_b: number
      phase_c: number
      battery: number
    }
    angles: {
      mechanical_deg: number
      electrical_deg: number
      error_deg: number
    }
    observer: {
      bemf_strength: number
      confidence: number
      pll_lock: number
    }
    power: {
      battery_current: number
      energy_used_wh: number
      energy_remaining_wh: number
    }
    timestamp_ms: number
    temperature: number
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
