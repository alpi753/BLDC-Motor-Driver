import {
  DropdownMenu,
  DropdownMenuTrigger,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
} from "@/components/ui/dropdown-menu"
import { RefreshCw, Usb } from "lucide-react"

import { cn } from "@/lib/utils"
import { buttonVariants } from "@/components/ui/button"

interface Device {
  path: string
  manufacturer?: string
  connected?: boolean
}

interface DeviceDropdownProps {
  devices: Device[]
  onConnect: (device: Device) => void
  onDisconnect: (path: string) => void
  onRefresh?: () => void
  loading?: boolean
}

export function DeviceListDropDown({
  devices,
  onConnect,
  onDisconnect,
  onRefresh,
  loading = false,
}: DeviceDropdownProps) {
  const connectedDevice = devices.find((d) => d.connected)

  return (
    <DropdownMenu>
      <DropdownMenuTrigger
        className={cn(
          buttonVariants({ variant: "ghost", size: "sm" }),
          "text-xs gap-2"
        )}
      >
        <Usb className="w-3.5 h-3.5" />
        {connectedDevice ? "Connected" : "Connect"}
      </DropdownMenuTrigger>

      <DropdownMenuContent align="end" className="w-56 text-xs">

        <div className="px-2 py-1 text-[10px] text-muted-foreground">
          USB Devices
        </div>

        <DropdownMenuSeparator />

        {/* EMPTY STATE */}
        {devices.length === 0 && !loading && (
          <div className="px-2 py-3 text-xs text-muted-foreground">
            No devices found
          </div>
        )}

        {/* LOADING STATE */}
        {loading && (
          <div className="px-2 py-3 text-xs text-muted-foreground">
            Scanning for devices...
          </div>
        )}

        {/* DEVICE LIST */}
        {devices.map((device) => {
          const isConnected = device.connected

          return (
            <DropdownMenuItem
              key={device.path}
              className={cn(
                "flex flex-col items-start gap-1 text-xs",
                loading && "pointer-events-none opacity-50"
              )}
              onClick={() => {
                if (isConnected) {
                  onDisconnect(device.path)
                } else {
                  onConnect(device)
                }
              }}
            >
              <div className="flex w-full justify-between items-center">
                <span className="font-medium">
                  {device.manufacturer ?? "Unknown Device"}
                </span>

                {isConnected && (
                  <span className="text-[10px] text-green-500">
                    connected
                  </span>
                )}
              </div>

              <span className="text-[10px] text-muted-foreground font-mono">
                {device.path}
              </span>
            </DropdownMenuItem>
          )
        })}

        <DropdownMenuSeparator />

        {/* REFRESH */}
        <DropdownMenuItem onClick={onRefresh} className="text-xs gap-2">
          <RefreshCw className="w-3.5 h-3.5" />
          Rescan devices
        </DropdownMenuItem>
      </DropdownMenuContent>
    </DropdownMenu>
  )
}
