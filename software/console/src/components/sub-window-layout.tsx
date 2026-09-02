import React from "react"
import SubWindowHeader from "@/components/sub-window-header"
import { LayoutDashboard } from "lucide-react"

interface SubWindowLayoutProps {
  children: React.ReactNode
  title: string
}

export default function SubWindowLayout({ children, title }: SubWindowLayoutProps) {
  return (
    <div className="flex h-screen flex-col overflow-hidden border bg-background">
      <SubWindowHeader windowTitle={title} maximazable={true} Icon={LayoutDashboard} />
      <div className="flex flex-1 items-center justify-center overflow-auto">
        <div className="flex size-full items-center justify-center [&>div]:!rounded-none [&>div]:!border-0 [&>div]:!shadow-none">
          {children}
        </div>
      </div>
    </div>
  )
}
