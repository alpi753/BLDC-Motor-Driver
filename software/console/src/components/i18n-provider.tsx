/* eslint-disable react-refresh/only-export-components */
import * as React from "react"

export type Locale = "en" | "tr"

const translations = {
  en: {
    "nav.dashboard": "Dashboard",
    "nav.overview": "Overview",
    "nav.analytics": "Analytics",
    "nav.console": "Console",
    "nav.settings": "Settings",
    "menu.about": "About",
    "language.label": "Language",
    "language.english": "English",
    "language.turkish": "Türkçe",
    "theme.label": "Theme",
    "theme.choose": "Choose theme",
    "theme.system": "System",
    "theme.light": "Light",
    "theme.dark": "Dark",
    "window.minimize": "Minimize",
    "window.maximize": "Maximize or restore",
    "window.close": "Close",
    "window.openCard": "Open {title} in new window",
    "device.connect": "Connect",
    "device.connected": "Connected",
    "device.usbDevices": "USB devices",
    "device.none": "No devices found",
    "device.scanning": "Scanning for devices…",
    "device.unknown": "Unknown device",
    "device.rescan": "Rescan devices",
    "device.connectedTo": "Connected to {device}",
    "device.connectionFailed": "Connection failed: {message}",
    "device.refreshed": "Device list refreshed",
    "device.refreshFailed": "Failed to refresh devices",
    "device.disconnected": "Disconnected",
    "device.disconnectFailed": "Failed to disconnect",
    "dashboard.intro": "Real-time motor-controller telemetry over USB serial.",
    "dashboard.status": "protobuf v{version} · frame {frame} · {uptime} s uptime",
    "dashboard.empty": "Connect a controller to begin receiving telemetry.",
    "card.bus.title": "DC bus voltage",
    "card.bus.description": "Voltage measured through the board-level bus divider.",
    "card.bus.volts": "volts",
    "card.bus.series": "Bus (V)",
    "card.currents.title": "Phase currents",
    "card.currents.description": "Measurements sampled from the current-sense amplifiers.",
    "card.currents.series": "Amps (A)",
    "card.voltages.title": "Phase voltages",
    "card.voltages.description": "Divider measurements sampled from all three switching nodes.",
    "card.phaseA": "Phase A",
    "card.phaseB": "Phase B",
    "card.phaseC": "Phase C",
    "card.temperature.title": "Temperatures",
    "card.temperature.description": "Temperatures measured in degrees Celsius.",
    "card.temperature.mosfet": "MOSFET",
    "card.temperature.pcb": "PCB",
    "card.temperature.series": "MOSFET",
    "console.title": "Serial console",
    "console.resume": "Resume scrolling",
    "console.pause": "Pause scrolling",
    "console.clear": "Clear console",
    "console.download": "Download log",
    "console.waiting": "Waiting for data…",
    "console.placeholder": "Type command (at+config…)",
    "console.send": "Send",
    "console.saved": "Logs saved",
    "console.saveFailed": "Failed to save logs",
  },
  tr: {
    "nav.dashboard": "Gösterge paneli",
    "nav.overview": "Genel bakış",
    "nav.analytics": "Analizler",
    "nav.settings": "Ayarlar",
    "nav.console": "Konsol",
    "menu.about": "Hakkında",
    "language.label": "Dil",
    "language.english": "English",
    "language.turkish": "Türkçe",
    "theme.label": "Tema",
    "theme.choose": "Tema seç",
    "theme.system": "Sistem",
    "theme.light": "Açık",
    "theme.dark": "Koyu",
    "window.minimize": "Küçült",
    "window.maximize": "Büyüt veya geri yükle",
    "window.close": "Kapat",
    "window.openCard": "{title} kartını yeni pencerede aç",
    "device.connect": "Bağlan",
    "device.connected": "Bağlı",
    "device.usbDevices": "USB aygıtları",
    "device.none": "Aygıt bulunamadı",
    "device.scanning": "Aygıtlar taranıyor…",
    "device.unknown": "Bilinmeyen aygıt",
    "device.rescan": "Aygıtları yeniden tara",
    "device.connectedTo": "{device} aygıtına bağlandı",
    "device.connectionFailed": "Bağlantı başarısız: {message}",
    "device.refreshed": "Aygıt listesi yenilendi",
    "device.refreshFailed": "Aygıtlar yenilenemedi",
    "device.disconnected": "Bağlantı kesildi",
    "device.disconnectFailed": "Bağlantı kesilemedi",
    "dashboard.intro": "USB seri bağlantısı üzerinden gerçek zamanlı motor denetleyici telemetrisi.",
    "dashboard.status": "protobuf v{version} · kare {frame} · {uptime} sn çalışma süresi",
    "dashboard.empty": "Telemetri almak için bir denetleyiciye bağlanın.",
    "card.bus.title": "DC bara gerilimi",
    "card.bus.description": "Kart üzerindeki bara gerilim bölücüsünden ölçülür.",
    "card.bus.volts": "volt",
    "card.bus.series": "Bara (V)",
    "card.currents.title": "Faz akımları",
    "card.currents.description": "Akım algılama yükselteçlerinden örneklenen ölçümler.",
    "card.currents.series": "Amper (A)",
    "card.voltages.title": "Faz gerilimleri",
    "card.voltages.description": "Üç anahtarlama düğümünden örneklenen bölücü ölçümleri.",
    "card.phaseA": "Faz A",
    "card.phaseB": "Faz B",
    "card.phaseC": "Faz C",
    "card.temperature.title": "Sıcaklıklar",
    "card.temperature.description": "Sıcaklıklar derece Santigrat cinsinden ölçülmüştür.",
    "card.temperature.mosfet": "MOSFET",
    "card.temperature.pcb": "PCB",
    "card.temperature.series": "MOSFET",
    "console.title": "Seri konsol",
    "console.resume": "Kaydırmayı sürdür",
    "console.pause": "Kaydırmayı duraklat",
    "console.clear": "Konsolu temizle",
    "console.download": "Günlüğü indir",
    "console.waiting": "Veri bekleniyor…",
    "console.placeholder": "Komut yazın (at+config…)",
    "console.send": "Gönder",
    "console.saved": "Günlük kaydedildi",
    "console.saveFailed": "Günlük kaydedilemedi",
  },
} as const

export type TranslationKey = keyof typeof translations.en
type Replacements = Record<string, string | number>

type I18nContextValue = {
  locale: Locale
  setLocale: (locale: Locale) => void
  t: (key: TranslationKey, replacements?: Replacements) => string
}

const STORAGE_KEY = "bldc-console-locale"
const I18nContext = React.createContext<I18nContextValue | undefined>(undefined)

const detectLocale = (): Locale => {
  const saved = localStorage.getItem(STORAGE_KEY)
  if (saved === "en" || saved === "tr") return saved
  return navigator.language.toLocaleLowerCase().startsWith("tr") ? "tr" : "en"
}

export function I18nProvider({ children }: { children: React.ReactNode }) {
  const [locale, setLocaleState] = React.useState<Locale>(detectLocale)

  const setLocale = React.useCallback((nextLocale: Locale) => {
    localStorage.setItem(STORAGE_KEY, nextLocale)
    setLocaleState(nextLocale)
  }, [])

  React.useEffect(() => {
    document.documentElement.lang = locale
  }, [locale])

  React.useEffect(() => {
    const syncLocale = (event: StorageEvent) => {
      if (event.key === STORAGE_KEY && (event.newValue === "en" || event.newValue === "tr")) {
        setLocaleState(event.newValue)
      }
    }
    window.addEventListener("storage", syncLocale)
    return () => window.removeEventListener("storage", syncLocale)
  }, [])

  const t = React.useCallback((key: TranslationKey, replacements: Replacements = {}) => {
    return Object.entries(replacements).reduce(
      (text, [name, value]) => text.replaceAll(`{${name}}`, String(value)),
      translations[locale][key]
    )
  }, [locale])

  const value = React.useMemo(() => ({ locale, setLocale, t }), [locale, setLocale, t])

  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>
}

export function useI18n() {
  const context = React.useContext(I18nContext)
  if (!context) throw new Error("useI18n must be used within I18nProvider")
  return context
}
