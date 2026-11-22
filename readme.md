# Composer 669 x64 Tracker
<img width="1446" height="945" alt="image" src="https://github.com/user-attachments/assets/4c7cf9b3-7026-4590-9aac-ac0dfdd6c04d" />
Ein nativer 64-Bit-Windows-Musiktracker im Stil des klassischen DOS-Trackers **Composer 669**, vollständig in reinem x64-Assembler (NASM) ohne C-Runtime (CRT) und ohne externe Bibliotheken entwickelt. Die Anwendung nutzt ausschließlich direkte Windows-API-Aufrufe (`kernel32`, `user32`, `gdi32`, `winmm`, `comdlg32`) und kombiniert einen vollständigen 8-Kanal-Tracker mit einem integrierten 4K-Demoscene-Visualizer.

---

## Funktionsumfang

### Audio-Engine & Dateiformate
* **8-Kanal-Echtzeit-Stereo-Mixer:** 44.100 Hz, 16-Bit Stereo über non-blocking WinMM-Streaming (4 Puffer à 512 Frames).
* **Synthesizer & PCM-Sampler:** 8 eingebaute prozedurale Instrumente (Kick, Snare, Hi-Hat, Oct Bass, Pulse Arp, Saw Lead, Pluck/Echo, Sinus Pad) sowie bis zu **64 Sample-Slots (`00..3F`)** mit jeweils bis zu 128 KB 8-Bit-PCM-Speicher und 48.16-Fixkomma-Resampling.
* **Echtes `.669`-Laden und -Speichern:** Unterstützt klassische `if`- und `JN`-Module mit bis zu **128 Patterns à 64 Zeilen**, **128 Orders**, originaler **Loop-Order**, **Pattern-Tempos**, **Pattern-Breaks**, 108-Byte-Song-Message sowie allen eingebetteten 8-Bit-PCM-Samples in originaler 669-Stimmung (8363 Hz auf Oktave 2, 31,25-Hz-Timer).
* **WAV-Sample-Import:** Lädt unkomprimierte 8-, 16- und 24-Bit-PCM- sowie 32-Bit-IEEE-Float-WAV-Dateien (Mono/Stereo, inkl. `WAVE_FORMAT_EXTENSIBLE`) auf jeden beliebigen Slot `00..3F` inklusive Loop-Schalter und sofortiger Vorschau auf Originaltonhöhe.
* **Alle 6 Composer-669-Effekte:** Proportionale Pitch-Slides (`a`, `b`), Portamento to Note (`c`), Frequency Adjust / Detune (`d`), Vibrato (`e`) und Tempo (`f`) – einheitlich für PCM-Samples und Synthesizer.

### Oberfläche, Editor & Steuerung
* **80×50-Zeichenraster (960×600 nativ):** Klassische Composer-669-Farbpalette mit Live-VU-Metern, scrollender 64-Sample-Liste, 128-Order-Anzeige, 64-Zeilen-Pattern-Editor und markierter Break-Zeile.
* **Borderless Fullscreen:** Umschaltbar per `Alt + Enter` oder `Strg + F` mit automatischer Skalierung unter Beibehaltung des 16:10-Seitenverhältnisses.
* **Tastatur- & Maussteuerung:** 2-Oktaven-QWERTZ-Klaviatur mit Live-Vorhören, direkter Mausklick-Navigation, Rechtsklick-Vorschau, Mausrad-Scrolling für Sample-Liste, Order-Liste und Pattern-Grid sowie klickbarer Fußzeile (Linksklick `+`, Rechtsklick `-`).
* **Copy/Paste & Sicherheitsabfragen:** Getrennte Zwischenablagen für einzelne Spuren (64 Zeilen) und komplette Patterns (8×64 Zeilen inkl. Break und Tempo) sowie vier Löschfunktionen mit Windows-Bestätigungsdialog (`Ja`/`Nein`).
* **Integrierte Tasten- & Maushilfe:** Jederzeit über `[?]` oder `Strg + H` einblendbar.

### Integrierter 4K-Demoscene-Visualizer
* Umschaltbar über `[D]` oder `Strg + D` (auch im Vollbildmodus), live gekoppelt an die aktuell geladene und laufende `.669`-Musik.
* **3 umschaltbare Effekte (`Tab` / Mausrad):** Beat-reaktives Interferenz-Plasma, perspektivischer 3D-Tunnel mit Tiefennebel (FPU-LUT) und 8 kanalgekoppelte Copper-Rasterbars.
* Kombiniert mit 3D-Sternenfeld, Echtzeit-Stereo-Oszilloskop aus dem Audio-Puffer, 8-Kanal-Spektrum-Equalizer und einem Sinus-Scroller aus der Song-Message der geladenen `.669`-Datei.

---

## Projektstruktur

| Datei | Beschreibung |
|---|---|
| `c669main.asm` | Haupteinstiegspunkt, Fenstererstellung, Borderless-Fullscreen-Logik, `WndProc` und Hauptschleife |
| `c669_defs.inc` | Konstanten, Windows-API-Importe, `.data`-Tabellen, Hilfetexte und `.bss`-Speicherlayout |
| `c669_audio.inc` | 8-Kanal-Stereo-Mixer (Synth + 64-Slot-PCM), alle 669-Effekte (`a`–`f`) und WinMM-Pufferverwaltung |
| `c669_io.inc` | `.669`-Parser/Exporter, WAV-Importer, Slot-Verwaltung und Löschroutinen mit Sicherheitsabfrage |
| `c669_ui.inc` | GDI-Renderer, Scroll-Logik, Tastatur-, Maus-, Copy/Paste- und Hilfe-Overlay-Steuerung |
| `c669_demo.inc` | 320×200-Framebuffer-Visualizer (Plasma, 3D-Tunnel, Rasterbars, Oszilloskop, Equalizer, Scroller) |

---

## Build & Voraussetzungen

### Benötigte Werkzeuge (Windows x64)
* **NASM** (`nasm.exe`, 64-Bit-Assembler)
* **GoLink** (`GoLink.exe`, Windows-Linker ohne CRT-Abhängigkeit)

### Kompilieren und Linken (PowerShell)
```powershell
.\nasm.exe -f win64 c669main.asm -o c669main.obj
.\GoLink.exe /entry start /fo c669main.exe c669main.obj kernel32.dll user32.dll gdi32.dll winmm.dll comdlg32.dll
.\c669main.exe

