# Composer 669 x64 Tracker
<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/ab20df2d-d789-4c80-b397-25482f963c01" />
<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/44989124-208e-4850-b9af-f4b50869c056" />
<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/863c5d4b-521c-4505-8cc4-2c3e8ce4d82d" />


Ein nativer 64-Bit-Windows-Musiktracker im Stil des klassischen DOS-Trackers **Composer 669**, vollständig in reinem x64-Assembler (NASM) ohne C-Runtime (CRT) und ohne externe Bibliotheken entwickelt. Die Anwendung nutzt ausschließlich direkte Windows-API-Aufrufe (`kernel32`, `user32`, `gdi32`, `winmm`, `comdlg32`) und kombiniert einen vollständigen 8-Kanal-Tracker mit einem Sample-/Instrument-Editor (inklusive Mikrofonaufnahme und ADSR-Synthesizer) sowie einem integrierten 4K-Demoscene-Visualizer.

---

## Funktionsumfang

### Audio-Engine & Dateiformate
* **8-Kanal-Echtzeit-Stereo-Mixer:** 44.100 Hz, 16-Bit Stereo über non-blocking WinMM-Streaming (4 Puffer à 512 Frames).
* **Synthesizer & PCM-Sampler:** 8 eingebaute prozedurale Standard-Instrumente (Kick, Snare, Hi-Hat, Oct Bass, Pulse Arp, Saw Lead, Pluck/Echo, Sinus Pad) sowie bis zu **64 Sample-Slots (`00..3F`)** mit jeweils bis zu 128 KB 8-Bit-PCM-Speicher und 48.16-Fixkomma-Resampling.
* **Echtes `.669`-Laden und -Speichern:** Unterstützt klassische `if`- und `JN`-Module mit bis zu **128 Patterns à 64 Zeilen**, **128 Orders**, originaler **Loop-Order**, **Pattern-Tempos**, **Pattern-Breaks**, 108-Byte-Song-Message sowie allen eingebetteten 8-Bit-PCM-Samples in originaler 669-Stimmung (8363 Hz auf Oktave 2, 31,25-Hz-Timer).
* **WAV-Sample-Import:** Lädt unkomprimierte 8-, 16- und 24-Bit-PCM- sowie 32-Bit-IEEE-Float-WAV-Dateien (Mono/Stereo, inkl. `WAVE_FORMAT_EXTENSIBLE`) auf jeden beliebigen Slot `00..3F` inklusive Loop-Schalter und sofortiger Vorschau auf Originaltonhöhe.
* **Alle 6 Composer-669-Effekte:** Proportionale Pitch-Slides (`a`, `b`), Portamento to Note (`c`), Frequency Adjust / Detune (`d`), Vibrato (`e`) und Tempo (`f`) – einheitlich für PCM-Samples und Synthesizer.

### Sample- & Instrument-Editor (`c669_samped.inc`)
* **Direktzugriff:** Umschaltbar über **`[I]`** in der Fußzeile oder **`Strg + E`** (funktioniert 1:1 auch im Vollbildmodus).
* **Interaktive Wellenform-Ansicht:** Hochauflösende Min/Max-Darstellung des aktiven Slots (`00..3F`) mit Auswahl-Markern (`Linksklick` = Start, `Rechtsklick` = Ende), visueller Loop-Anzeige (`Shift + Links/Rechts` setzt `LoopStart`/`LoopEnd`) und Live-Vorhören über die QWERTZ-Klaviatur.
* **Mikrofon- / Line-In-Aufnahme:** Direkte Aufnahme über WinMM `waveIn` (22.050 Hz, 8-Bit Mono, bis zu 128 KB / ca. 5,9 Sekunden) direkt in den aktiven Sample-Slot inklusive automatischer Puffer-Überwachung.
* **Schnitt- & Sample-Effekte:** Zuschneiden (`Trim`), Auswahl als Loop übernehmen (`Sel->Lp`), Loop ein/aus sowie destruktive Puffer-Effekte für den markierten Bereich: **Normalisieren**, **Fade In**, **Fade Out**, **Reverse**, **Boost (+50 %)** und **HalbLen** (2:1-Dezimierung / Lo-Fi).
* **Integrierter ADSR-Synthesizer / Instrument-Designer:**
  * **5 Wellenformen:** Sinus, Sägezahn, Puls (mit einstellbarer Pulsbreite 10–90 %), Dreieck und Rauschen.
  * **8 Parameter-Slider (0–100):** Attack, Decay, Sustain, Release, Dauer (0,1–2,0 s), Pitch-Sweep (abwärts/aufwärts), Detune/Chorus (2. Oszillator) und Tiefpassfilter (LP).
  * **6 Presets:** `808Kick`, `Snare`, `AcidSaw`, `ChipSqr`, `SoftPad`, `LaserFX`.
  * **PCM-Export in den Slot:** Rendert das gestaltete Instrument als One-Shot (`SYNTH -> SLOT RENDERN`) oder mit automatischem Sustain-Loop (`SYNTH + AUTO-LOOP PAD`) direkt als 8-Bit-PCM in den gewählten Slot – sofort im Tracker spielbar, nachbearbeitbar und beim `.669`-Speichern vollständig eingebettet.

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
| `c669_defs.inc` | Konstanten, Windows-API-Importe (inkl. `waveIn`), `.data`-Tabellen, Hilfetexte und `.bss`-Speicherlayout |
| `c669_audio.inc` | 8-Kanal-Stereo-Mixer (Synth + 64-Slot-PCM), alle 669-Effekte (`a`–`f`) und WinMM-Pufferverwaltung |
| `c669_io.inc` | `.669`-Parser/Exporter, WAV-Importer, Slot-Verwaltung und Löschroutinen mit Sicherheitsabfrage |
| `c669_ui.inc` | GDI-Renderer, Scroll-Logik, Tastatur-, Maus-, Copy/Paste-, Moduswechsel- und Hilfe-Overlay-Steuerung |
| `c669_samped.inc` | Sample- & Instrument-Editor (Wellenform, `waveIn`-Aufnahme, Trim/Loops, Effekte und ADSR-Synthesizer) |
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
Ein nativer 64-Bit-Windows-Musiktracker im Stil des klassischen DOS-Trackers **Composer 669**, vollständig in reinem x64-Assembler (NASM) ohne C-Runtime (CRT) und ohne externe Bibliotheken entwickelt. Die Anwendung nutzt ausschließlich direkte Windows-API-Aufrufe (`kernel32`, `user32`, `gdi32`, `winmm`, `comdlg32`) und kombiniert einen vollständigen 8-Kanal-Tracker mit einem integrierten 4K-Demoscene-Visualizer.

---
