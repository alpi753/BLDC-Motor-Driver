import { useMemo, useState } from "react"
import { Button } from "@/components/ui/button"
import {
  Card,
  CardContent,
  CardDescription,
  CardHeader,
  CardTitle,
} from "@/components/ui/card"
import { Label } from "@/components/ui/label"
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select"
import {
  Settings as SettingsIcon,
  Cpu,
  Gauge,
  Radar,
  Play,
  Save,
  Trash2,
  Upload,
} from "lucide-react"
import { Separator } from "@/components/ui/separator"
import { Tabs, TabsContent, TabsList, TabsTrigger } from "@/components/ui/tabs"
import SubWindowHeader from "@/components/sub-window-header"
import { SettingsField } from "@/components/settings-field"
import { useMotorSettings } from "@/hooks/use-motor-settings"
import { useUsbDevices } from "@/hooks/use-devices"
import { toast } from "sonner"

export default function Settings() {
  const { devices } = useUsbDevices()
  const {
    settings,
    dirty,
    updateField,
    resetToDefaults,
    persistLocally,
  } = useMotorSettings()
  const [sending, setSending] = useState(false)

  const connectedDevice = useMemo(
    () => devices.find((device) => device.connected),
    [devices]
  )

  const handleSave = async () => {
    persistLocally()

    if (!connectedDevice) {
      toast.warning("Saved locally", {
        description: "Connect a device to send settings to firmware.",
      })
      return
    }

    if (!window.api.usb.sendSettings) {
      toast.error("Settings transport is unavailable in this build.")
      return
    }

    setSending(true)
    try {
      await window.api.usb.sendSettings(settings)
      toast.success("Settings sent to firmware", {
        description: connectedDevice.path,
      })
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error)
      toast.error("Failed to send settings", { description: message })
    } finally {
      setSending(false)
    }
  }

  return (
    <div className="min-h-screen bg-background border border-accent-foreground/30 shadow-2xl overflow-hidden flex flex-col rounded-2xl">
      <SubWindowHeader windowTitle="System Settings" Icon={SettingsIcon} />

      <div className="flex-1 overflow-y-auto p-6 pt-4">
        <div className="max-w-4xl mx-auto space-y-6">
          <div className="flex items-center justify-between gap-4">
            <div>
              <h1 className="text-2xl font-semibold tracking-tight text-left">
                Motor Settings
              </h1>
              <p className="text-sm text-muted-foreground text-left">
                Edit FOC and ESC parameters, then send them to the connected
                firmware over USB CBOR.
              </p>
            </div>
            <Button
              size="sm"
              className="gap-2 no-drag-area shrink-0"
              onClick={() => void handleSave()}
              disabled={sending}
            >
              {connectedDevice ? (
                <Upload className="h-4 w-4" />
              ) : (
                <Save className="h-4 w-4" />
              )}
              {sending
                ? "Sending…"
                : connectedDevice
                  ? "Save & Send"
                  : "Save Locally"}
            </Button>
          </div>

          <Card>
            <CardContent className="pt-6 flex flex-wrap items-center justify-between gap-3 text-sm">
              <div className="text-left">
                <p className="font-medium">Device</p>
                <p className="text-muted-foreground">
                  {connectedDevice
                    ? `${connectedDevice.manufacturer ?? "USB"} — ${connectedDevice.path}`
                    : "No device connected — settings will be stored locally only."}
                </p>
              </div>
              {dirty ? (
                <span className="text-xs text-amber-500">Unsaved changes</span>
              ) : (
                <span className="text-xs text-muted-foreground">Up to date</span>
              )}
            </CardContent>
          </Card>
        </div>

        <Tabs defaultValue="motor" className="w-full max-w-4xl mx-auto mt-6">
          <TabsList className="grid w-full grid-cols-4 mb-8">
            <TabsTrigger value="motor" className="gap-2">
              <Cpu className="h-4 w-4" /> Motor
            </TabsTrigger>
            <TabsTrigger value="control" className="gap-2">
              <Gauge className="h-4 w-4" /> Control
            </TabsTrigger>
            <TabsTrigger value="observer" className="gap-2">
              <Radar className="h-4 w-4" /> Observer
            </TabsTrigger>
            <TabsTrigger value="startup" className="gap-2">
              <Play className="h-4 w-4" /> Startup
            </TabsTrigger>
          </TabsList>

          <TabsContent value="motor" className="space-y-4">
            <Card>
              <CardHeader className="text-left">
                <CardTitle>Motor parameters</CardTitle>
                <CardDescription>
                  Physical motor constants used by the FOC observer and limits.
                </CardDescription>
              </CardHeader>
              <CardContent className="grid grid-cols-2 gap-4">
                <SettingsField
                  id="pp"
                  label="Pole pairs"
                  hint="CBOR key: pp"
                  value={settings.pp}
                  step={1}
                  min={1}
                  onChange={(value) => updateField("pp", value)}
                />
                <SettingsField
                  id="rs"
                  label="Phase resistance (Ω)"
                  hint="CBOR key: rs"
                  value={settings.rs}
                  min={0}
                  onChange={(value) => updateField("rs", value)}
                />
                <SettingsField
                  id="ls"
                  label="Phase inductance (H)"
                  hint="CBOR key: ls"
                  value={settings.ls}
                  min={0}
                  onChange={(value) => updateField("ls", value)}
                />
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="control" className="space-y-4">
            <Card>
              <CardHeader className="text-left">
                <CardTitle>Current & speed loops</CardTitle>
                <CardDescription>PI gains and d-axis current target.</CardDescription>
              </CardHeader>
              <CardContent className="grid grid-cols-2 gap-4">
                <SettingsField
                  id="rpm_t"
                  label="Target RPM"
                  hint="CBOR key: rpm_t — 0 keeps the motor idle"
                  value={settings.rpm_t}
                  min={0}
                  step={50}
                  onChange={(value) => updateField("rpm_t", value)}
                />
                <SettingsField
                  id="i_kp"
                  label="Current Kp"
                  value={settings.i_kp}
                  onChange={(value) => updateField("i_kp", value)}
                />
                <SettingsField
                  id="i_ki"
                  label="Current Ki"
                  value={settings.i_ki}
                  onChange={(value) => updateField("i_ki", value)}
                />
                <SettingsField
                  id="s_kp"
                  label="Speed Kp"
                  value={settings.s_kp}
                  onChange={(value) => updateField("s_kp", value)}
                />
                <SettingsField
                  id="s_ki"
                  label="Speed Ki"
                  value={settings.s_ki}
                  onChange={(value) => updateField("s_ki", value)}
                />
                <SettingsField
                  id="l_i"
                  label="Max phase current (A)"
                  hint="CBOR key: l_i — iq ceiling in closed loop"
                  value={settings.l_i}
                  min={0}
                  onChange={(value) => updateField("l_i", value)}
                />
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="observer" className="space-y-4">
            <Card>
              <CardHeader className="text-left">
                <CardTitle>Observer & PLL</CardTitle>
                <CardDescription>
                  Sensorless observer tuning (active when FOC is implemented).
                </CardDescription>
              </CardHeader>
              <CardContent className="grid grid-cols-2 gap-4">
                <SettingsField
                  id="p_kp"
                  label="PLL Kp"
                  value={settings.p_kp}
                  onChange={(value) => updateField("p_kp", value)}
                />
                <SettingsField
                  id="p_ki"
                  label="PLL Ki"
                  value={settings.p_ki}
                  onChange={(value) => updateField("p_ki", value)}
                />
                <SettingsField
                  id="obs"
                  label="Observer gain"
                  value={settings.obs}
                  onChange={(value) => updateField("obs", value)}
                />
                <SettingsField
                  id="min_cl"
                  label="Min closed-loop RPM"
                  value={settings.min_cl}
                  min={0}
                  onChange={(value) => updateField("min_cl", value)}
                />
                <SettingsField
                  id="max_ol"
                  label="Max open-loop RPM"
                  value={settings.max_ol}
                  min={0}
                  onChange={(value) => updateField("max_ol", value)}
                />
              </CardContent>
            </Card>
          </TabsContent>

          <TabsContent value="startup" className="space-y-4">
            <Card>
              <CardHeader className="text-left">
                <CardTitle>Startup sequence</CardTitle>
                <CardDescription>
                  Open-loop ramp, alignment, and observer hand-off thresholds.
                </CardDescription>
              </CardHeader>
              <CardContent className="grid grid-cols-2 gap-4">
                <SettingsField
                  id="ramp"
                  label="Open-loop ramp window (ms)"
                  hint="CBOR key: ramp — max duration to accelerate; hand-off needs observer ready"
                  value={settings.ramp}
                  min={0}
                  onChange={(value) => updateField("ramp", value)}
                />
                <SettingsField
                  id="align_t"
                  label="Alignment time (ms)"
                  hint="CBOR key: align_t"
                  value={settings.align_t}
                  min={0}
                  onChange={(value) => updateField("align_t", value)}
                />
                <SettingsField
                  id="ol_ramp"
                  label="Open-loop ramp (RPM/s)"
                  hint="CBOR key: ol_ramp"
                  value={settings.ol_ramp}
                  min={0}
                  onChange={(value) => updateField("ol_ramp", value)}
                />
                <SettingsField
                  id="align"
                  label="Alignment current (A)"
                  hint="CBOR key: align — align phase only"
                  value={settings.align}
                  min={0}
                  onChange={(value) => updateField("align", value)}
                />
                <SettingsField
                  id="ol_i"
                  label="Open-loop torque current (A)"
                  hint="CBOR key: ol_i"
                  value={settings.ol_i}
                  min={0}
                  onChange={(value) => updateField("ol_i", value)}
                />
                <SettingsField
                  id="ol_start"
                  label="Open-loop start RPM"
                  hint="CBOR key: ol_start — capped by min CL / max OL"
                  value={settings.ol_start}
                  min={0}
                  onChange={(value) => updateField("ol_start", value)}
                />
                <SettingsField
                  id="ho_ae"
                  label="Hand-off max angle error (°)"
                  hint="CBOR key: ho_ae"
                  value={settings.ho_ae}
                  min={0}
                  onChange={(value) => updateField("ho_ae", value)}
                />
                <SettingsField
                  id="ho_conf"
                  label="Hand-off min observer confidence"
                  hint="CBOR key: ho_conf (0–100)"
                  value={settings.ho_conf}
                  min={0}
                  max={100}
                  onChange={(value) => updateField("ho_conf", value)}
                />
                <div className="space-y-2 text-left col-span-2">
                  <Label htmlFor="smode">Startup mode</Label>
                  <Select
                    value={String(settings.smode)}
                    onValueChange={(value) => updateField("smode", Number(value))}
                  >
                    <SelectTrigger id="smode" className="no-drag-area">
                      <SelectValue placeholder="Select mode" />
                    </SelectTrigger>
                    <SelectContent>
                      <SelectItem value="0">Alignment + open-loop hand-off</SelectItem>
                      <SelectItem value="1">Open-loop from start RPM (skip align)</SelectItem>
                      <SelectItem value="2">Open-loop from min CL RPM (skip align)</SelectItem>
                    </SelectContent>
                  </Select>
                  <p className="text-[12px] text-muted-foreground">
                    CBOR key: smode (uint8)
                  </p>
                </div>
              </CardContent>
            </Card>
          </TabsContent>
        </Tabs>

        <Card className="max-w-4xl mx-auto mt-6">
          <CardContent className="pt-6">
            <div className="flex items-center justify-between text-destructive">
              <div className="space-y-0.5 text-left">
                <Label className="text-destructive font-semibold">
                  Reset motor defaults
                </Label>
                <p className="text-[12px] text-destructive/80">
                  Restore firmware parameter defaults in this form.
                </p>
              </div>
              <Button
                variant="outline"
                size="sm"
                className="border-destructive/30 hover:bg-destructive/10 text-destructive h-8 gap-2 no-drag-area"
                onClick={resetToDefaults}
              >
                <Trash2 className="h-3.5 w-3.5" /> Reset
              </Button>
            </div>
          </CardContent>
        </Card>

        <Separator className="max-w-4xl mx-auto my-6" />
        <p className="max-w-4xl mx-auto text-xs text-muted-foreground text-left pb-4">
          Protocol: CBOR array{" "}
          <code className="font-mono">[1, {"{ pp, rs, … }"}]</code> at 115200
          baud. Firmware applies each key via <code className="font-mono">usb_msg_rx</code>{" "}
          in <code className="font-mono">telem.c</code>.
        </p>
      </div>
    </div>
  )
}