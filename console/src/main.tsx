import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { HashRouter } from 'react-router-dom'
import './index.css'
import App from './App.tsx'
import { ThemeProvider } from '@/components/theme-provider'
import { I18nProvider } from '@/components/i18n-provider'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <HashRouter>
		<ThemeProvider>
			<I18nProvider>
        <App />
			</I18nProvider>
		</ThemeProvider>
    </HashRouter>    
  </StrictMode>,
)
