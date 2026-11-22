# Composer 669 x64 Tracker

<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/ab20df2d-d789-4c80-b397-25482f963c01" />
<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/44989124-208e-4850-b9af-f4b50869c056" />
<img width="2560" height="1440" alt="image" src="https://github.com/user-attachments/assets/863c5d4b-521c-4505-8cc4-2c3e8ce4d82d" />

Ein nativer 64-Bit-Windows-Musiktracker im Stil des klassischen DOS-Trackers **Composer 669**, vollständig in reinem x64-Assembler (NASM) ohne C-Runtime (CRT) und ohne externe Bibliotheken entwickelt. Die Anwendung nutzt ausschließlich direkte Windows-API-Aufrufe (`kernel32`, `user32`, `gdi32`, `winmm`, `comdlg32`) und kombiniert einen vollständigen 8-Kanal-Tracker mit einem interaktiven Sample-/Instrument-Editor (Live-ADSR-Synthesizer, zerstörungsfreie Sample-FX, Mikrofonaufnahme mit Live-VU) sowie einem integrierten 4K-Demoscene-Visualizer.

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
* **Interaktive Wellenform & verschiebbare Loop-Marker:**
  * Min/Max-Darstellung des aktiven Slots (`00..3F`) mit Auswahl-Markern (`Linksklick` = Start, `Rechtsklick` = Ende) und Live-Vorhören über die QWERTZ-Klaviatur.
  * Bei aktivem Loop erscheinen greifbare Marker an beiden Loop-Linien (**`L`** oben = Loop-Start, **`E`** unten = Loop-Ende), die sich direkt mit gedrückter linker Maustaste flüssig verschieben lassen und bei Slider-Änderungen erhalten bleiben.
* **Mikrofon- / Line-In-Aufnahme mit Geräteauswahl & Live-VU:**
  * Streaming-Aufnahme über WinMM `waveIn` (22.050 Hz, 8-Bit Mono, 4×512-Byte-Puffer, bis zu 128 KB / ca. 5,9 Sekunden) direkt in den aktiven Slot.
  * Direkte **Geräteauswahl** in der Aufnahmeleiste (`Standard (Windows)` sowie alle erkannten Mikrofone und Line-In-Eingänge).
  * **Live-VU-Pegelbalken** in 3 Farbzonen (Cyan = normal, Gelb = laut, Pink = Clip) mit Peak-Hold-Linie, laufender Zeitanzeige (`0.0/5.9` s) und live mitwachsender Wellenform.
* **Schnitt- & Puffer-Effekte:** Zuschneiden (`Trim`), Auswahl als Loop übernehmen (`Sel->Lp`), Loop ein/aus (`Loop:ON`/`LoopOFF`), Zurücksetzen auf internen Synth/leer (`Syn`) sowie destruktive Effekte für den markierten Bereich: **Normalisieren**, **Fade In**, **Fade Out**, **Reverse**, **Boost (+50 %)** und **HalbLen** (2:1-Dezimierung / Lo-Fi).
* **Zwei Live-Modi & getrennte Speicherung pro Slot (`00..3F`):**
  * Jeder der 64 Slots speichert seinen eigenen Modus (`MOD:SYNTH` oder `MOD:SAMPLE`), Wellenform, Pulsbreite, Auto-Loop und alle **9 Parameter-Slider (0–100)**: **Attack**, **Decay**, **Sustain**, **Release**, **Dauer/Gate**, **Sweep**, **Detune/Chorus**, **Filter LP** und **Lautstärke**.
  * **Live-Update (`[LIVE:AN ]`):** Jedes Ziehen an einem Slider aktualisiert die Wellenform sofort in Echtzeit und spielt beim Loslassen automatisch eine Vorschau an.
  * **`[MOD:SYNTH ]` (ADSR-Synthesizer):** 5 Wellenformen (Sinus, Sägezahn, Puls mit einstellbarer Pulsbreite 10–90 %, Dreieck, Rauschen), optionaler Sustain-Auto-Loop (`[AUTOLP:AN ]`) und 6 Presets (`808Kick`, `Snare`, `AcidSaw`, `ChipSqr`, `SoftPad`, `LaserFX`).
  * **`[MOD:SAMPLE]` (Zerstörungsfreie Sample-FX):** Geladene `.669`-Samples, WAV-Dateien und Aufnahmen werden automatisch in einem Original-Backup gesichert. Alle 9 Slider lassen sich verlustfrei live auf das Sample anwenden (ADSR-Hüllkurve, Gate/Länge, Pitch-Sweep, Chorus, Tiefpassfilter, Lautstärke); über **`[Orig ]`** wird jederzeit das unbearbeitete Original wiederhergestellt.

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
| `c669main.asm` | Haupteinstiegspunkt, Fenstererstellung, Borderless-Fullscreen-Logik, `WndProc` (inkl. Maus-Drag) und Hauptschleife |
| `c669_defs.inc` | Konstanten, Windows-API-Importe (inkl. `waveIn`), `.data`-Tabellen, Hilfetexte und `.bss`-Speicherlayout (inkl. 64-Slot-Speicher & Original-Sample-Puffer) |
| `c669_audio.inc` | 8-Kanal-Stereo-Mixer (Synth + 64-Slot-PCM), alle 669-Effekte (`a`–`f`) und WinMM-Pufferverwaltung |
| `c669_io.inc` | `.669`-Parser/Exporter, WAV-Importer, Slot-Original-Backup und Löschroutinen mit Sicherheitsabfrage |
| `c669_ui.inc` | GDI-Renderer, Scroll-Logik, Tastatur-, Maus-, Copy/Paste-, Moduswechsel- und Hilfe-Overlay-Steuerung |
| `c669_samped.inc` | Sample- & Instrument-Editor (Wellenform, ziehbare Loop-Marker, `waveIn`-Streaming mit Gerätewahl & Live-VU, destruktive FX, zerstörungsfreie Sample-FX und Live-ADSR-Synth pro Slot) |
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