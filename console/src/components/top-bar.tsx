import {
  Menubar,
  MenubarMenu,
  MenubarTrigger,
  MenubarContent,
  MenubarItem,
} from "@/components/ui/menubar"

import {
  DropdownMenu,
  DropdownMenuTrigger,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuSeparator,
} from "@/components/ui/dropdown-menu"

import { useUsbDevices } from "@/hooks/use-devices"
import { Menu } from "lucide-react"
import { cn } from "@/lib/utils"
import { DeviceListDropDown } from "./device-list"


import { useNavigate } from "react-router-dom"

export default function TopBar() {
  const navigate = useNavigate()
  const { devices, loading, onConnect, onRefresh, onDisconnect } = useUsbDevices()

  const openSettingsWindow = () => {
    if (window.api) {
      window.api.openNewWindow('settings')
    } else {
      console.warn('Electron API is not available in the browser environment.')
    }
  }

  const navigateToConsole = () => {
    navigate('/console')
  }

  const navigateToDashboard = () => {
    navigate('/')
  }

  return (
    <header className="w-full h-8 border-b flex items-center px-3 bg-background text-xs">
      
      {/* LEFT: Brand */}
      <div 
        className="flex items-center gap-2 font-semibold text-sm cursor-pointer hover:opacity-80"
        onClick={navigateToDashboard}
      >
        <span>BLDC Console</span>
      </div>

      {/* CENTER: Navigation */}
      <div className="ml-6 hidden md:flex">
        <Menubar className="border-none shadow-none">
          <MenubarMenu>
            <MenubarTrigger onClick={navigateToDashboard}>Dashboard</MenubarTrigger>
            <MenubarContent>
              <MenubarItem onClick={navigateToDashboard}>Overview</MenubarItem>
              <MenubarItem>Analytics</MenubarItem>
            </MenubarContent>
          </MenubarMenu>

          <MenubarMenu>
            <MenubarTrigger>Devices</MenubarTrigger>
            <MenubarContent>
              <MenubarItem>All Devices</MenubarItem>
              <MenubarItem>Motor Config</MenubarItem>
              <MenubarItem>Logs</MenubarItem>
            </MenubarContent>
          </MenubarMenu>
					<MenubarMenu>
            <MenubarTrigger onClick={navigateToConsole}>Console</MenubarTrigger>
					</MenubarMenu>

          <MenubarMenu>
            <MenubarTrigger onClick={openSettingsWindow}>Settings</MenubarTrigger>
            {/* <MenubarContent>
              <MenubarItem>Profile</MenubarItem>
              <MenubarItem>System</MenubarItem>
            </MenubarContent>
         */}
						</MenubarMenu> 
        </Menubar>
      </div>

      {/* RIGHT: Actions */}
      <div className="ml-auto flex items-center gap-2">

        {/* Quick action */}
				<DeviceListDropDown devices={devices} onConnect={onConnect} onRefresh={onRefresh} onDisconnect={onDisconnect} loading={loading}/>

        {/* Dropdown menu */}
        <DropdownMenu>
          <DropdownMenuTrigger
            className={cn(
              "group/button inline-flex shrink-0 items-center justify-center rounded-md border border-transparent bg-clip-padding text-xs/relaxed font-medium whitespace-nowrap transition-all outline-none select-none focus-visible:border-ring focus-visible:ring-2 focus-visible:ring-ring/30 active:not-aria-[haspopup]:translate-y-px disabled:pointer-events-none disabled:opacity-50 aria-invalid:border-destructive aria-invalid:ring-2 aria-invalid:ring-destructive/20 dark:aria-invalid:border-destructive/50 dark:aria-invalid:ring-destructive/40 [&_svg]:pointer-events-none [&_svg]:shrink-0 [&_svg:not([class*='size-'])]:size-4 hover:bg-muted hover:text-foreground aria-expanded:bg-muted aria-expanded:text-foreground dark:hover:bg-muted/50 size-7 [&_svg:not([class*='size-'])]:size-3.5"
            )}
          >
            <Menu className="w-5 h-5" />
          </DropdownMenuTrigger>

          <DropdownMenuContent align="end">
            {/* <DropdownMenuItem>New Device</DropdownMenuItem> */}
            <DropdownMenuItem>Import Config</DropdownMenuItem>
            <DropdownMenuItem>Settings</DropdownMenuItem>
            <DropdownMenuSeparator />
            <DropdownMenuItem>About</DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>

        {/* User */}
        {/* <DropdownMenu>
          <DropdownMenuTrigger asChild={true}>
            <Avatar className="h-7 w-7 cursor-pointer">
              <AvatarImage src="" />
              <AvatarFallback>MN</AvatarFallback>
            </Avatar>
          </DropdownMenuTrigger>

          <DropdownMenuContent align="end">
            <DropdownMenuItem>Profile</DropdownMenuItem>
            <DropdownMenuItem>Billing</DropdownMenuItem>
            <DropdownMenuSeparator />
            <DropdownMenuItem className="text-red-500">
              Logout
            </DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu> */}

      </div>
    </header>
  )
}