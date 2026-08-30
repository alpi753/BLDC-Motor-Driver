import React from "react"
import { Maximize2 } from "lucide-react"
import { Button } from "@/components/ui/button"
import { useI18n } from "@/components/i18n-provider"

interface CardWrapperProps {
  children: React.ReactNode
  title: string
  route: string
}

export function CardWrapper({ children, title, route }: CardWrapperProps) {
  const { t } = useI18n()
  const openSubWindow = (e: React.MouseEvent) => {
    e.preventDefault()
    if (window.api) {
      window.api.openNewWindow(route)
    } else {
      console.warn("Electron API not available")
    }
  }

  return (
    <div className="dashboard-card-slot group relative h-full overflow-visible [&>div:last-child]:rounded-none">
      <div className="absolute top-2 right-2 z-10 opacity-0 group-hover:opacity-100 transition-opacity">
        <Button
          variant="ghost"
          size="icon"
          className="h-7 w-7 border bg-background hover:bg-muted hover:text-foreground"
          onClick={openSubWindow}
          title={t("window.openCard", { title })}
        >
          <Maximize2 className="h-3.5 w-3.5" />
        </Button>
      </div>
      {children}
    </div>
  )
}
