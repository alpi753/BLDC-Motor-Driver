import {
  Menubar,
  MenubarMenu,
  MenubarTrigger,
} from "@/components/ui/menubar"

import {
  DropdownMenu,
  DropdownMenuTrigger,
  DropdownMenuContent,
  DropdownMenuItem,
} from "@/components/ui/dropdown-menu"

import { useUsbDevices } from "@/hooks/use-devices"
import { Check, Languages, Menu, Minus, Monitor, Moon, Square, Sun, X } from "lucide-react"
import { cn } from "@/lib/utils"
import { DeviceListDropDown } from "./device-list"
import { useTheme } from "./theme-provider"
import { useI18n, type Locale } from "./i18n-provider"


import { useNavigate } from "react-router-dom"

export default function TopBar() {
  const navigate = useNavigate()
  const { theme, setTheme } = useTheme()
  const { locale, setLocale, t } = useI18n()
  const { devices, loading, onConnect, onRefresh, onDisconnect } = useUsbDevices()

  const toggleMaximize = async () => {
    if (await window.api.window.isMaximized()) {
      window.api.window.unmaximize()
    } else {
      window.api.window.maximize()
    }
  }

  return (
    <header className="drag-area flex h-9 w-full items-center border-b-3 pl-3 text-xs">
      
      {/* LEFT: Brand */}
      <div 
        className="no-drag-area flex cursor-pointer items-center gap-2 text-sm font-semibold hover:opacity-80"
        onClick={() => navigate('/')}
      >
        <span>BLDC Console</span>
      </div>

      {/* CENTER: Navigation */}
      <div className="no-drag-area ml-6 hidden md:flex">
        <Menubar className="border-none shadow-none">
          <MenubarMenu>
            <MenubarTrigger onClick={() => navigate('/')}>{t("nav.dashboard")}</MenubarTrigger>
          </MenubarMenu>

					<MenubarMenu>
            <MenubarTrigger onClick={() => navigate('/console')}>{t("nav.console")}</MenubarTrigger>
					</MenubarMenu>

					<MenubarMenu>
            <MenubarTrigger onClick={() => navigate('/settings')}>{t("nav.settings")}</MenubarTrigger>
					</MenubarMenu>
        </Menubar>
      </div>

      {/* RIGHT: Actions */}
      <div className="no-drag-area ml-auto flex h-full items-center gap-2">

        {/* Quick action */}
				<DeviceListDropDown devices={devices} onConnect={onConnect} onRefresh={onRefresh} onDisconnect={onDisconnect} loading={loading}/>

        <DropdownMenu>
          <DropdownMenuTrigger
            className="grid size-7 place-items-center rounded-md text-muted-foreground hover:bg-muted hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring"
            title={t("theme.label")}
            aria-label={t("theme.choose")}
          >
            {theme === "system" ? <Monitor className="size-3.5" /> : theme === "dark" ? <Moon className="size-3.5" /> : <Sun className="size-3.5" />}
          </DropdownMenuTrigger>
          <DropdownMenuContent align="end" className="w-36">
            {([
              ["system", t("theme.system"), Monitor],
              ["light", t("theme.light"), Sun],
              ["dark", t("theme.dark"), Moon],
            ] as const).map(([value, label, Icon]) => (
              <DropdownMenuItem key={value} onClick={() => setTheme(value)} className="gap-2">
                <Icon className="size-3.5" />
                <span>{label}</span>
                {theme === value ? <Check className="ml-auto size-3.5" /> : null}
              </DropdownMenuItem>
            ))}
          </DropdownMenuContent>
        </DropdownMenu>

        <DropdownMenu>
          <DropdownMenuTrigger
            className="flex h-7 items-center gap-1.5 rounded-md px-2 font-mono text-[10px] font-semibold text-muted-foreground hover:bg-muted hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-ring"
            title={t("language.label")}
            aria-label={t("language.label")}
          >
            <Languages className="size-3.5" />
            {locale.toUpperCase()}
          </DropdownMenuTrigger>
          <DropdownMenuContent align="end" className="w-36">
            {([[
              "en", t("language.english"),
            ], [
              "tr", t("language.turkish"),
            ]] as [Locale, string][]).map(([value, label]) => (
              <DropdownMenuItem key={value} onClick={() => setLocale(value)}>
                <span>{label}</span>
                {locale === value ? <Check className="ml-auto size-3.5" /> : null}
              </DropdownMenuItem>
            ))}
          </DropdownMenuContent>
        </DropdownMenu>

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
            <DropdownMenuItem>{t("menu.about")}</DropdownMenuItem>
          </DropdownMenuContent>
        </DropdownMenu>

        <div className="ml-1 flex h-full items-center border-l">
          <button
            type="button"
            className="grid h-full w-10 place-items-center text-muted-foreground hover:bg-muted hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-inset focus-visible:ring-ring"
            onClick={() => window.api.window.minimize()}
            title={t("window.minimize")}
            aria-label={t("window.minimize")}
          >
            <Minus className="size-4" strokeWidth={1.5} />
          </button>
          <button
            type="button"
            className="grid h-full w-10 place-items-center text-muted-foreground hover:bg-muted hover:text-foreground focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-inset focus-visible:ring-ring"
            onClick={toggleMaximize}
            title={t("window.maximize")}
            aria-label={t("window.maximize")}
          >
            <Square className="size-3" strokeWidth={1.5} />
          </button>
          <button
            type="button"
            className="grid h-full w-10 place-items-center text-destructive hover:bg-destructive/15 hover:text-destructive focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-inset focus-visible:ring-ring"
            onClick={() => window.api.window.close()}
            title={t("window.close")}
            aria-label={t("window.close")}
          >
            <X className="size-3.5" strokeWidth={1.5} />
          </button>
        </div>

      </div>
    </header>
  )
}
