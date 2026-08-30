import * as React from "react"

export function useDeviceId() {
  const [deviceId, setDeviceId] = React.useState<string | null>(null)

  React.useEffect(() => {
    let alive = true

    const apply = (telem: TelemetryData) => {
      if (!alive || !telem.device_id) return
      setDeviceId((prev) => (prev === telem.device_id ? prev : telem.device_id))
    }

    const applyHistory = (history: TelemetryData[]) => {
      if (history.length === 0) {
        setDeviceId(null)
        return
      }

      const latest = history.at(-1)
      if (latest?.device_id) {
        apply(latest)
      }
    }

    const applyDevices = (devices: Device[]) => {
      if (!devices.some((device) => device.connected)) {
        setDeviceId(null)
      }
    }

    window.api.usb.getTelemetryHistory?.().then((history) => {
      if (alive && Array.isArray(history)) {
        applyHistory(history as TelemetryData[])
      }
    })

    window.api.usb.list?.().then((devices) => {
      if (alive && Array.isArray(devices)) {
        applyDevices(devices)
      }
    })

    const unsubscribeTelemetry = window.api.usb.onTelemetry?.(apply)
    const unsubscribeHistory = window.api.usb.onTelemetryHistory?.((history) => {
      if (Array.isArray(history)) {
        applyHistory(history as TelemetryData[])
      }
    })
    const unsubscribeDevices = window.api.usb.onUpdate?.(applyDevices)

    return () => {
      alive = false
      unsubscribeTelemetry?.()
      unsubscribeHistory?.()
      unsubscribeDevices?.()
    }
  }, [])

  return deviceId
}