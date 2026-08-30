import { useState } from "react"
import { Button } from "@/components/ui/button"
import { 
	X,
	Maximize2 as MaximizeIcon,
	Minimize2 as MinimizeIcon,
} from "lucide-react"

export default function SubWindowHeader({
	windowTitle = "Sub Window",
	maximazable = false,
	Icon
}: {
	windowTitle?: string
	maximazable?: boolean
	Icon?: React.ComponentType<{ className?: string }>
}) {
	const [isMaximized, setIsMaximized] = useState(false)
	const closeWindow = () => {
		window.api.window.close()
	}
	const maxminWindow = async () => {
		const maximized = await window.api.window.isMaximized()
		if (maximized) {
			setIsMaximized(m => !m)
			window.api.window.unmaximize()
		} else {
			setIsMaximized(m => !m)
			window.api.window.maximize()
		}
	}



	return (
      <div className="h-10 flex items-center justify-between px-4 bg-muted/30 border-b drag-area select-none">
        <div className="flex items-center gap-2 opacity-70">
					 {Icon && <Icon className="h-3.5 w-3.5" />}
          <span className="text-[11px] font-medium uppercase tracking-wider">{windowTitle}</span>
        </div>
				<div>

				{
					maximazable && (
						<Button 
							variant="ghost" 
							size="icon" 
							className="h-6 w-6 no-drag-area bg-accent hover:text-icon-foreground transition-colors transform hover:scale-110 shadow-none ring-0"
							onClick={maxminWindow}
						>
							{/* You can replace this with a maximize icon */}
							{
								isMaximized ? (
									<MinimizeIcon className="h-3.5 w-3.5" />
								) : (
									<MaximizeIcon className="h-3.5 w-3.5" />
								)
							}
						</Button>
					)
				}
        <Button 
          variant="ghost" 
          size="icon" 
          className="h-6 w-6 no-drag-area hover:bg-destructive hover:text-destructive-foreground transition-colors ml-2 transform hover:scale-110 shadow-none"
          onClick={closeWindow}
        >
          <X className="h-3.5 w-3.5" />
        </Button>
				</div>
      </div>
	)
}