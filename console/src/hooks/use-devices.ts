import { useEffect, useState, useCallback } from "react"
import { toast } from "sonner"

export const useUsbDevices = () => {
  const [devices, setDevices] = useState<Device[]>([])
  const [loading, setLoading] = useState(false)

  const onConnect = useCallback(async (device: Device) => {
    setLoading(true)
    try {
      await window.api.usb.connect(device.path)
      const freshDevices = await window.api.usb.list()
      setDevices(freshDevices)
      toast.success(`Connected to ${device.manufacturer || device.path}`)
    } catch (error) {
      const res = await window.api.usb.list()
      setDevices(res)
      const message = error instanceof Error ? error.message : String(error)
      toast.error(`Connection failed: ${message}`)
    } finally {
      setLoading(false)
    }
  }, [])

  const onRefresh = useCallback(async () => {
    setLoading(true)
    try {
      const res = await window.api.usb.refresh()
      setDevices(res)
      toast.info("Device list refreshed")
    } catch {
      toast.error("Failed to refresh devices")
    } finally {
      setLoading(false)
    }
  }, [])

  const onDisconnect = useCallback(async (path: string) => {
    setLoading(true)
    try {
      await window.api.usb.disconnect(path)
      
      // Force update the local state with the returned devices
      const freshDevices = await window.api.usb.list()
      setDevices(freshDevices)
      
      toast.info("Disconnected")
    } catch (error) {
      toast.error("Failed to disconnect", error instanceof Error ? { description: error.message } : undefined)
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    let alive = true

    const init = async () => {
      setLoading(true)
      try {
        const res = await window.api.usb.list()
        if (alive) setDevices(res)
      } finally {
        if (alive) setLoading(false)
      }
    }

    init()

    const unsubscribe = window.api.usb.onUpdate((updated) => {
      if (alive) {
        setDevices(updated)
        setLoading(false)
      }
    })

    return () => {
      alive = false
      unsubscribe?.()
    }
  }, [])

  return {
    devices,
    loading,
    onConnect,
    onRefresh,
    onDisconnect,
  }
}