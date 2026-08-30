import { Input } from "@/components/ui/input"
import { Label } from "@/components/ui/label"

type SettingsFieldProps = {
  id: string
  label: string
  hint?: string
  value: number
  step?: string | number
  min?: number
  onChange: (value: number) => void
}

export function SettingsField({
  id,
  label,
  hint,
  value,
  step = "any",
  min,
  onChange,
}: SettingsFieldProps) {
  return (
    <div className="space-y-2 text-left">
      <Label htmlFor={id}>{label}</Label>
      <Input
        id={id}
        type="number"
        step={step}
        min={min}
        value={Number.isFinite(value) ? value : 0}
        onChange={(event) => onChange(Number(event.target.value))}
        className="no-drag-area"
      />
      {hint ? (
        <p className="text-[12px] text-muted-foreground">{hint}</p>
      ) : null}
    </div>
  )
}