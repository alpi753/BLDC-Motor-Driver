import { app, BrowserWindow, ipcMain } from 'electron'
import { join } from 'path'
import isDev from 'electron-is-dev'
import { connectDevice, listDevices, sendDataToPort, setupPortReader } from './lib/serial'
import type { MotorSettings } from './lib/settings'
import { Menu } from 'electron'
import { SerialPort } from 'serialport'
import type { BLDCTelemetry } from './lib/telemetry'
import { TELEMETRY_HISTORY_LENGTH } from './lib/telemetry'
import { saveFile } from './lib/file'

// Multi-window support
const windows = new Set<BrowserWindow>()

export function createWindow(urlPath: string = '') {
  const isSubWindow = urlPath !== ''
  
  const win = new BrowserWindow({
    width: isSubWindow ? 700 : 1024,
    height: isSubWindow ? 570 : 768,
    frame: false,
    transparent: true,
    backgroundColor: '#00000000',
    hasShadow: false,
    titleBarStyle: 'hidden',
    webPreferences: {
      preload: join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
    },
  })

	Menu.setApplicationMenu(null)

  const url = isDev
    ? `http://localhost:5173/#/${urlPath}`
    : `file://${join(__dirname, '../dist/index.html')}#/${urlPath}`

  win.loadURL(url)

  if (isDev) {
    win.webContents.openDevTools({ mode: 'detach' })
  }

  windows.add(win)

  win.webContents.once('did-finish-load', () => {
    if (telemetryHistory.length > 0) {
      win.webContents.send('usb:telem-history', telemetryHistory)
    }
  })

  win.on('closed', () => {
    windows.delete(win)
  })

  return win
}

// ---- usb related IPC handlers (example) ----

// eslint-disable-next-line @typescript-eslint/no-explicit-any
let devices: any[] = []
let connectedPort: SerialPort | null = null
let connectedPath: string | null = null
let telemetryHistory: BLDCTelemetry[] = []

const broadcastTelemetryHistory = () => {
  BrowserWindow.getAllWindows().forEach((win) => {
    win.webContents.send('usb:telem-history', telemetryHistory)
  })
}

const pushTelemetry = (telem: BLDCTelemetry) => {
  telemetryHistory = [...telemetryHistory, telem].slice(-TELEMETRY_HISTORY_LENGTH)
  BrowserWindow.getAllWindows().forEach((win) => {
    win.webContents.send('usb:telem', telem)
    win.webContents.send('usb:telem-history', telemetryHistory)
  })
}

const clearTelemetryHistory = () => {
  telemetryHistory = []
  broadcastTelemetryHistory()
}

const setupConnectedPortReader = () => {
  if (!connectedPort) {
    throw new Error("No device connected")
  }

  if (!connectedPort.readable) {
    throw new Error("Port is not readable")
  }

  setupPortReader(connectedPort, {
    onMessage: (msg: string) => {
      BrowserWindow.getAllWindows().forEach(win => {
        win.webContents.send("usb:data", msg)
      })
    },
    onTelemetry: (telem: BLDCTelemetry) => {
      pushTelemetry(telem)
    },
  })
}

ipcMain.handle('usb:get-telem-history', () => telemetryHistory)

ipcMain.handle("file:save-file", async (_, data: ArrayBuffer, filePath: string) => {
	if (!filePath) throw new Error("File path is required")
	if (!data) throw new Error("No data provided to save")
	return saveFile(data, filePath) 
})

ipcMain.handle("usb:send-data", async (_, data: string) => {
	if (!connectedPort) throw new Error("No device connected")
	if (!connectedPort.writable) throw new Error("Port is not writable")

	return sendDataToPort(connectedPort, data)
})

ipcMain.handle("usb:send-settings", async (_, settings: MotorSettings) => {
  void settings
  throw new Error("Settings are not part of the current nanopb protocol")
})

ipcMain.handle("usb:setup-port-reader", async () => {
  setupConnectedPortReader()
})

ipcMain.handle("usb:list", async () => {
  const freshDevices = await listDevices()
  devices = freshDevices.map((d) => ({
    ...d,
    connected: d.path === connectedPath,
  }))
  return devices
})

ipcMain.handle("usb:connect", async (_, path: string) => {
  try {
    if (connectedPath === path) return devices.find((d) => d.path === path)

    if (connectedPort && connectedPath) {
      // close current connection before opening new one
      await new Promise<void>((resolve, reject) => {
        connectedPort?.close((err: unknown) => {
          if (err) reject(err)
          else resolve()
        })
      })
      connectedPort = null
      connectedPath = null
      clearTelemetryHistory()
    }

    connectedPort = await connectDevice(path)
    connectedPath = path

    // Reader setup belongs to the successful connection lifecycle.
    setupConnectedPortReader()

    devices = devices.map((d) =>
      d.path === path ? { ...d, connected: true } : { ...d, connected: false }
    )

    // Notify all windows about the change
    windows.forEach(win => {
      win.webContents.send("usb:update", devices)
      win.webContents.send("usb:on-update", devices)
    })

    return devices.find((d) => d.path === path)
  } catch (error) {
    console.error("USB Connect Error:", error)
    throw error
  }
})

ipcMain.handle("usb:refresh", async () => {
  const freshDevices = await listDevices()
  devices = freshDevices.map((d) => ({
    ...d,
    connected: d.path === connectedPath,
  }))
  return devices
})

ipcMain.handle("usb:disconnect", async (_, path: string) => {
  try {
    if (connectedPort && connectedPath === path) {
      // Assuming SerialPort instance has a close method
      await new Promise<void>((resolve, reject) => {
        connectedPort?.close((err: unknown) => {
          if (err) reject(err)
          else resolve()
        })
      })
      connectedPort = null
      connectedPath = null
      clearTelemetryHistory()
    }

    devices = devices.map((d) =>
      d.path === path ? { ...d, connected: false } : d
    )

    // Notify all windows about the change
    windows.forEach(win => {
      win.webContents.send("usb:update", devices)
      win.webContents.send("usb:on-update", devices)
    })

    return devices.find((d) => d.path === path)
  } catch (error) {
    console.error("USB Disconnect Error:", error)
    throw error
  }
})
// -----------------------------------------



app.whenReady().then(() => {
  createWindow()

  // Example IPC for opening new windows from frontend
  ipcMain.on('open-new-window', (_event, path) => {
    createWindow(path)
  })

  ipcMain.on('window:close', (event) => {
    const win = BrowserWindow.fromWebContents(event.sender)
    win?.close()
  })

  ipcMain.on('window:minimize', (event) => {
    const win = BrowserWindow.fromWebContents(event.sender)
    win?.minimize()
  })

  ipcMain.on('window:maximize', (event) => {
    const win = BrowserWindow.fromWebContents(event.sender)
    win?.maximize()
  })

  ipcMain.on('window:unmaximize', (event) => {
    const win = BrowserWindow.fromWebContents(event.sender)
    win?.unmaximize()
  })

  ipcMain.handle('window:is-maximized', (event) => {
    const win = BrowserWindow.fromWebContents(event.sender)
    return win?.isMaximized() ?? false
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', () => {
  if (windows.size === 0) {
    createWindow()
  }
})
