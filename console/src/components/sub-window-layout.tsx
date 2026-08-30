import React from "react"
import SubWindowHeader from "@/components/sub-window-header"
import { LayoutDashboard } from "lucide-react"

interface SubWindowLayoutProps {
  children: React.ReactNode
  title: string
}

export default function SubWindowLayout({ children, title }: SubWindowLayoutProps) {
  return (
    <div className="flex flex-col h-screen overflow-hidden bg-background border rounded-lg shadow-2xl">
      <SubWindowHeader windowTitle={title} maximazable={true} Icon={LayoutDashboard} />
      <div className="flex-1 overflow-auto p-4 flex items-center justify-center">
        <div className="size-full flex items-center justify-center">
          {children}
        </div>
      </div>
    </div>
  )
}
