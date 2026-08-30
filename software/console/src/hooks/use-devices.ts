import { useEffect, useState, useCallback } from "react"
import { toast } from "sonner"
import { useI18n } from "@/components/i18n-provider"

export const useUsbDevices = () => {
  const { t } = useI18n()
  const [devices, setDevices] = useState<Device[]>([])
  const [loading, setLoading] = useState(false)

  const onConnect = useCallback(async (device: Device) => {
    setLoading(true)
    try {
      await window.api.usb.connect(device.path)
      const freshDevices = await window.api.usb.list()
      setDevices(freshDevices)
      toast.success(t("device.connectedTo", { device: device.manufacturer || device.path }))
    } catch (error) {
      const res = await window.api.usb.list()
      setDevices(res)
      const message = error instanceof Error ? error.message : String(error)
      toast.error(t("device.connectionFailed", { message }))
    } finally {
      setLoading(false)
    }
  }, [t])

  const onRefresh = useCallback(async () => {
    setLoading(true)
    try {
      const res = await window.api.usb.refresh()
      setDevices(res)
      toast.info(t("device.refreshed"))
    } catch {
      toast.error(t("device.refreshFailed"))
    } finally {
      setLoading(false)
    }
  }, [t])

  const onDisconnect = useCallback(async (path: string) => {
    setLoading(true)
    try {
      await window.api.usb.disconnect(path)
      
      // Force update the local state with the returned devices
      const freshDevices = await window.api.usb.list()
      setDevices(freshDevices)
      
      toast.info(t("device.disconnected"))
    } catch (error) {
      toast.error(t("device.disconnectFailed"), error instanceof Error ? { description: error.message } : undefined)
    } finally {
      setLoading(false)
    }
  }, [t])

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
