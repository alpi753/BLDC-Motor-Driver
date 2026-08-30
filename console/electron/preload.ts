/* eslint-disable @typescript-eslint/no-explicit-any */
import { contextBridge, ipcRenderer } from 'electron'
import type { MotorSettings } from './lib/settings'

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld("api", {
  openNewWindow: (path: string) => ipcRenderer.send('open-new-window', path),
	file: {
		saveFile: (data: ArrayBuffer, filePath: string): Promise<void> => ipcRenderer.invoke('file:save-file', data, filePath),
	},
  window: {
    close: () => ipcRenderer.send('window:close'),
    maximize: () => ipcRenderer.send('window:maximize'),
    unmaximize: () => ipcRenderer.send('window:unmaximize'),
    isMaximized: () => ipcRenderer.invoke('window:is-maximized'),
  },
  usb: {
    list: () => ipcRenderer.invoke("usb:list"),
    connect: (id: string) => ipcRenderer.invoke("usb:connect", id),
		disconnect: (id: string) => ipcRenderer.invoke("usb:disconnect", id),
    refresh: () => ipcRenderer.invoke("usb:refresh"),
    sendData: (data: string) => ipcRenderer.invoke("usb:send-data", data),
    sendSettings: (settings: MotorSettings) =>
      ipcRenderer.invoke("usb:send-settings", settings),
    onUpdate: (cb: (devices: any[]) => void) => {
      const handler = (_: any, devices: any[]) => cb(devices)

      ipcRenderer.on("usb:update", handler)
      ipcRenderer.on("usb:on-update", handler)

      return () => {
        ipcRenderer.removeListener("usb:update", handler)
        ipcRenderer.removeListener("usb:on-update", handler)
      }
    },
	  setupPortReader: () => ipcRenderer.invoke("usb:setup-port-reader"),
	  onData: (callback: (msg: string) => void) => {
      const handler = (_: unknown, msg: string) => callback(msg)
      ipcRenderer.on("usb:data", handler)
      return () => ipcRenderer.removeListener("usb:data", handler)
	  },
	  offData: () => {
	    ipcRenderer.removeAllListeners("usb:data")
    },
    getTelemetryHistory: () =>
      ipcRenderer.invoke("usb:get-telem-history") as Promise<unknown[]>,
    onTelemetry: (callback: (telem: unknown) => void) => {
      const handler = (_: unknown, telem: unknown) => callback(telem)
      ipcRenderer.on("usb:telem", handler)
      return () => ipcRenderer.removeListener("usb:telem", handler)
    },
    onTelemetryHistory: (callback: (history: unknown[]) => void) => {
      const handler = (_: unknown, history: unknown[]) => callback(history)
      ipcRenderer.on("usb:telem-history", handler)
      return () => ipcRenderer.removeListener("usb:telem-history", handler)
    },
    offTelemetry: () => {
      ipcRenderer.removeAllListeners("usb:telem")
	  },
    offTelemetryHistory: () => {
      ipcRenderer.removeAllListeners("usb:telem-history")
    },
  },
})