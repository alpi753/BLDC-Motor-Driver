import React from "react"
import { Maximize2 } from "lucide-react"
import { Button } from "@/components/ui/button"

interface CardWrapperProps {
  children: React.ReactNode
  title: string
  route: string
}

export function CardWrapper({ children, title, route }: CardWrapperProps) {
  const openSubWindow = (e: React.MouseEvent) => {
    e.preventDefault()
    if (window.api) {
      window.api.openNewWindow(route)
    } else {
      console.warn("Electron API not available")
    }
  }

  return (
    <div className="relative group overflow-visible">
      <div className="absolute top-2 right-2 z-10 opacity-0 group-hover:opacity-100 transition-opacity">
        <Button
          variant="ghost"
          size="icon"
          className="h-7 w-7 bg-background/80 backdrop-blur-sm border shadow-sm hover:bg-primary hover:text-primary-foreground"
          onClick={openSubWindow}
          title={`Open ${title} in new window`}
        >
          <Maximize2 className="h-3.5 w-3.5" />
        </Button>
      </div>
      {children}
    </div>
  )
}
