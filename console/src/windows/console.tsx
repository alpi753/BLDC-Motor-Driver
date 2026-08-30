import * as React from "react"
import { Terminal as TerminalIcon, Trash2, Download, Play, Square } from "lucide-react"
import { Button } from "@/components/ui/button"
import { Input } from "@/components/ui/input"
import { ScrollArea } from "@/components/ui/scroll-area"
import { cn } from "@/lib/utils"
import TopBar from "@/components/top-bar"
import { useDeviceId } from "@/hooks/use-device-id"
import { formatDeviceIdGrouped, formatDeviceIdShort } from "@/lib/device-id"
import { toast } from "sonner"

interface ConsoleMessage {
  id: string
  timestamp: string
  type: "in" | "out" | "error" | "info"
  text: string
}

const MAX_CONSOLE_MESSAGES = 1000
const MAX_PENDING_MESSAGES = 5000
const FLUSH_INTERVAL_MS = 33
const MAX_BATCH_SIZE = 200
const MAX_MESSAGE_TEXT_LENGTH = 1024

const appendCappedMessages = (
  prev: ConsoleMessage[],
  next: ConsoleMessage[]
): ConsoleMessage[] => {
  const updated = [...prev, ...next]
  if (updated.length <= MAX_CONSOLE_MESSAGES) return updated
  return updated.slice(updated.length - MAX_CONSOLE_MESSAGES)
}

const sanitizeMessage = (msg: ConsoleMessage): ConsoleMessage => {
  if (msg.text.length <= MAX_MESSAGE_TEXT_LENGTH) return msg
  return {
    ...msg,
    text: `${msg.text.slice(0, MAX_MESSAGE_TEXT_LENGTH)} ...[truncated]`,
  }
}

export default function Console() {
  const deviceId = useDeviceId()
  const [messages, setMessages] = React.useState<ConsoleMessage[]>([ //demo
    // start empty; real data will arrive from IPC
  ])
  const [inputValue, setInputValue] = React.useState("")
  const [isPaused, setIsPaused] = React.useState(false)
  const scrollRef = React.useRef<HTMLDivElement>(null)
  const unsubscribeDataRef = React.useRef<(() => void) | null>(null)
  const unsubscribeTelemetryRef = React.useRef<(() => void) | null>(null)
  const pendingMessagesRef = React.useRef<ConsoleMessage[]>([])
  const flushTimerRef = React.useRef<ReturnType<typeof setTimeout> | null>(null)

  const flushMessages = React.useCallback(() => {
    flushTimerRef.current = null

    if (pendingMessagesRef.current.length === 0) {
      return
    }

    const batch = pendingMessagesRef.current.splice(0, MAX_BATCH_SIZE)
    setMessages((prev) => appendCappedMessages(prev, batch))

    if (pendingMessagesRef.current.length > 0) {
      flushTimerRef.current = setTimeout(flushMessages, FLUSH_INTERVAL_MS)
    }
  }, [])

  const enqueueMessage = React.useCallback((message: ConsoleMessage) => {
    pendingMessagesRef.current.push(sanitizeMessage(message))

    if (pendingMessagesRef.current.length > MAX_PENDING_MESSAGES) {
      pendingMessagesRef.current = pendingMessagesRef.current.slice(
        pendingMessagesRef.current.length - MAX_PENDING_MESSAGES
      )
    }

    if (!flushTimerRef.current) {
      flushTimerRef.current = setTimeout(flushMessages, FLUSH_INTERVAL_MS)
    }
  }, [flushMessages])

  React.useEffect(() => {
    return () => {
      if (flushTimerRef.current) {
        clearTimeout(flushTimerRef.current)
        flushTimerRef.current = null
      }
      pendingMessagesRef.current = []
    }
  }, [])

  // Auto-scroll logic
  React.useEffect(() => {
    if (!isPaused && scrollRef.current) {
      const scrollContainer = scrollRef.current.querySelector('[data-slot="scroll-area-viewport"]')
      if (scrollContainer) {
        scrollContainer.scrollTop = scrollContainer.scrollHeight
      }
    }
  }, [messages, isPaused])

  const handleSend = (e?: React.FormEvent) => {
    e?.preventDefault()
    if (!inputValue.trim()) return

    const newMessage: ConsoleMessage = {
      id: Date.now().toString(),
      timestamp: new Date().toLocaleTimeString([], { hour12: false }),
      type: "out",
      text: inputValue
    }

    enqueueMessage(newMessage)
    setInputValue("")
    // send over IPC to the main process; incoming responses will be delivered
    // via the `usb:data` channel and appended by the onData handler below
    if (window.api?.usb?.sendData) {
      window.api.usb.sendData(inputValue).catch((err) => {
        enqueueMessage({
          id: (Date.now() + 2).toString(),
          timestamp: new Date().toLocaleTimeString([], { hour12: false }),
          type: "error",
          text: String(err?.message ?? err),
        })
      })
    }
  }

  // Subscribe to incoming serial data + telemetry from the main process.
  // Subscriptions are managed via refs so we can unsubscribe while paused
  // to avoid queuing messages and leaking handlers.
  React.useEffect(() => {
    if (!window.api?.usb?.onData && !window.api?.usb?.onTelemetry) return

    const handler = (msg: string) => {
      enqueueMessage({
        id: Date.now().toString(),
        timestamp: new Date().toLocaleTimeString([], { hour12: false }),
        type: "in",
        text: msg,
      })
    }

    const telemetryHandler = (telem: TelemetryData) => {
      enqueueMessage({
        id: Date.now().toString(),
        timestamp: new Date().toLocaleTimeString([], { hour12: false }),
        type: "info",
        text: `TELEM rpm=${telem.speed.actual_rpm.toFixed(1)} target=${telem.speed.target_rpm.toFixed(1)} vbat=${telem.voltages.battery.toFixed(2)} ts=${telem.timestamp_ms}`,
      })
    }

    const subscribe = () => {
      unsubscribeDataRef.current?.()
      unsubscribeTelemetryRef.current?.()
      unsubscribeDataRef.current = window.api.usb.onData?.(handler) ?? null
      unsubscribeTelemetryRef.current = window.api.usb.onTelemetry?.(telemetryHandler) ?? null
    }

    const unsubscribe = () => {
      unsubscribeDataRef.current?.()
      unsubscribeTelemetryRef.current?.()
      unsubscribeDataRef.current = null
      unsubscribeTelemetryRef.current = null
    }

    if (!isPaused) {
      subscribe()
    } else {
      unsubscribe()
    }

    return () => {
      unsubscribe()
    }
  }, [isPaused, enqueueMessage])

  const onPauseToggle = () => {
    setIsPaused(i => !i)
  }

  const onDownload = async () => {
		if (messages.length === 0) return
    const logContent = messages.map(msg => `[${msg.timestamp}] ${msg.text}`).join("\n")
    const blob = new Blob([logContent], { type: "text/plain" })
    const safeTs = new Date().toISOString().replace(/[:.]/g, "-")
    const filename = `logs_${safeTs}.txt`
    const arrayBuffer = await blob.arrayBuffer()
    await window.api.file.saveFile(arrayBuffer, filename).then(() => {
			toast.success("Logs saved", { duration: 5000 });
		}).catch(() => {
			toast.error("Failed to save logs")
		})
  }
  const onClear = () => {
    pendingMessagesRef.current = []
    if (flushTimerRef.current) {
      clearTimeout(flushTimerRef.current)
      flushTimerRef.current = null
    }
    setMessages([])
  }

  return (
    <div className="flex flex-col h-screen overflow-hidden">
      <TopBar />
      
      <div className="flex-1 flex flex-col min-h-0">
        <div className="flex flex-col min-h-0 h-full border bg-card shadow-sm">
          {/* Console Header */}
          <div className="flex items-center justify-between border-b bg-muted/30">
            <div className="flex items-center gap-2 px-3 py-1.5 min-w-0">
              <TerminalIcon className="w-4 h-4 text-primary shrink-0" />
              <div className="text-left min-w-0">
                <h2 className="text-sm font-semibold tracking-tight">Serial Console</h2>
                <div className="flex items-center gap-2 mt-0.5 min-w-0">
                  <span className="text-[10px] text-muted-foreground font-mono shrink-0">
                    115200 bps
                  </span>
                  <span className="text-[10px] text-muted-foreground shrink-0">·</span>
                  <span className="text-[10px] text-muted-foreground shrink-0">Device ID</span>
                  <button
                    type="button"
                    className={cn(
                      "text-[10px] font-mono truncate max-w-[min(28rem,50vw)] text-left",
                      deviceId
                        ? "text-foreground hover:text-primary transition-colors"
                        : "text-muted-foreground cursor-default"
                    )}
                    title={deviceId ? formatDeviceIdGrouped(deviceId) : "Waiting for telemetry"}
                    disabled={!deviceId}
                    onClick={async () => {
                      if (!deviceId) return
                      try {
                        await navigator.clipboard.writeText(deviceId)
                        toast.success("Device ID copied")
                      } catch {
                        toast.error("Failed to copy device ID")
                      }
                    }}
                  >
                    {deviceId ? formatDeviceIdShort(deviceId) : "—"}
                  </button>
                </div>
              </div>
            </div>

            <div className="flex items-center gap-1">
              <Button 
                variant="ghost" 
                size="icon" 
                className="h-8 w-8" 
                onClick={onPauseToggle}
                title={isPaused ? "Resume scrolling" : "Pause scrolling"}
              >
                {isPaused ? <Play className="h-4 w-4" /> : <Square className="h-4 w-4" />}
              </Button>
              <Button variant="ghost" size="icon" className="h-8 w-8" onClick={onClear} title="Clear console" disabled={messages.length === 0}>
                <Trash2 className="h-4 w-4" />
              </Button>
              <Button variant="ghost" size="icon" className="h-8 w-8" title="Download log" onClick={onDownload} disabled={messages.length === 0}>
                <Download className="h-4 w-4" />
              </Button>
            </div>
          </div>

          {/* Messages Area */}
          <div className="flex-1 min-h-0 font-mono text-[13px] leading-relaxed bg-background">
            <ScrollArea ref={scrollRef} className="h-full">
              <div className="pl-1 space-y-1">
                {messages.map((msg) => (
                <div key={msg.id} className="group flex items-start gap-2 hover:bg-muted/30 px-0 py-0.5 transition-colors">
                  <span className="text-muted-foreground/50 shrink-0 select-none w-18.75">
                    [{msg.timestamp}] 
                  </span>
                  <span className={cn(
                    "font-medium break-all",
                    msg.type === "in" && "text-muted-foreground",
                    msg.type === "out" && "text-green-500",
                    msg.type === "error" && "text-destructive",
                    msg.type === "info" && "text-blue-400"
                  )}>
                    {msg.text}
                  </span>
                </div>
              ))}
              {messages.length === 0 && (
                <div className="h-full flex flex-col items-center justify-center text-muted-foreground py-20">
                  <TerminalIcon className="w-8 h-8 opacity-20 mb-2" />
                  <p className="text-sm">Waiting for data...</p>
                </div>
              )}
              </div>
            </ScrollArea>
          </div>

          {/* Input Area */}
          <div className="p-3 border-t bg-muted/20">
            <form onSubmit={handleSend} className="flex gap-2">
              <div className="relative flex-1 group">
                <Input
                  value={inputValue}
                  onChange={(e) => setInputValue(e.target.value)}
                  placeholder="Type command (at+config...)"
                  className="font-mono text-sm bg-background border-muted-foreground/20 focus-visible:ring-primary/30"
                />
                <div className="absolute right-3 top-1/2 -translate-y-1/2 flex items-center gap-2 opacity-0 group-focus-within:opacity-100 transition-opacity">
                  <kbd className="pointer-events-none hidden h-5 select-none items-center gap-1 rounded border bg-muted px-1.5 font-mono text-[10px] font-medium opacity-100 sm:flex">
                    <span className="text-xs">↵</span>
                  </kbd>
                </div>
              </div>
              <Button type="submit" size="sm" className="px-4">
                Send
              </Button>
            </form>
            {/* <div className="flex items-center justify-between mt-2 px-1"> */}
                {/* <div className="flex gap-3">
                    <label className="flex items-center gap-1.5 text-[10px] text-muted-foreground cursor-pointer hover:text-foreground transition-colors">
                        <input type="checkbox" className="rounded-sm border-muted" defaultChecked />
                        <span>Append CR</span>
                    </label>
                    <label className="flex items-center gap-1.5 text-[10px] text-muted-foreground cursor-pointer hover:text-foreground transition-colors">
                        <input type="checkbox" className="rounded-sm border-muted" defaultChecked />
                        <span>Append LF</span>
                    </label>
                </div> */}
                {/* <span className="text-[10px] text-muted-foreground">
                    History: Use <kbd className="font-sans">↑</kbd> <kbd className="font-sans">↓</kbd>
                </span> */}
            {/* </div> */}
          </div>
        </div>
      </div>
    </div>
  )
}