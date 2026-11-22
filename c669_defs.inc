; Konstanten, Windows-API-Importe sowie .data und .bss fuer Composer 669 x64 Tracker

; Raster- und Fensterkonstanten (80x50 Zeichen a 12x12 Pixel)
CHAR_W          equ 12
CHAR_H          equ 12
WIN_W           equ 960
WIN_H           equ 600
DEMO_W          equ 320
DEMO_H          equ 200

; Tracker-Datenmodell (128 Patterns x 64 Zeilen, 128 Orders, 64 Sample-Slots a 128 KB)
MAX_PATS        equ 128
PAT_ROWS        equ 64
VIEW_ROWS       equ 32
HELP_LINES      equ 35
MAX_ORDERS      equ 128
NUM_CH          equ 8
MAX_SAMPS       equ 64
CELL_SIZE       equ 4                   ; Byte 0=Note, 1=Inst (0..63), 2=Vol, 3=FX
ROW_BYTES       equ NUM_CH * CELL_SIZE  ; 32 Byte pro Zeile
PAT_BYTES       equ PAT_ROWS * ROW_BYTES; 2048 Byte pro Pattern
VOICE_SIZE      equ 64
MAX_SAMP_BYTES  equ 131072              ; 128 KB pro Slot (64 Slots = 8 MB in .bss)
FILE_BUF_SIZE   equ 8912896             ; 8,5 MB Dateipuffer fuer .669 und .wav I/O

; Echtzeit-Audio-Streaming (44100 Hz, 16-Bit Stereo, 4 Puffer a 512 Frames)
SAMPLE_RATE     equ 44100
BUF_COUNT       equ 4
BUF_FRAMES      equ 512
BUF_BYTES       equ BUF_FRAMES * 4
TICKS_PER_TEMPO equ 1411                ; 44100 Hz / 31,25 Hz Original-669-Timer

; Windows-Konstanten
WS_TRACKER      equ 0x10CA0000
WS_POPUP_VIS    equ 0x90000000
GWL_STYLE       equ -16
SWP_FRAMECHANGED equ 0x0020
SRCCOPY         equ 0x00CC0020
BLACKNESS       equ 0x00000042
COLORONCOLOR    equ 3
ETO_OPAQUE      equ 2
TRANSPARENT     equ 1
WHDR_DONE       equ 0x00000001
IDC_ARROW       equ 32512
MB_CONFIRM      equ 0x00000134          ; MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
IDYES           equ 6

GENERIC_READ          equ 0x80000000
GENERIC_WRITE         equ 0x40000000
FILE_SHARE_READ       equ 1
CREATE_ALWAYS         equ 2
OPEN_EXISTING         equ 3
FILE_ATTRIBUTE_NORMAL equ 0x80
OFN_OPEN_FLAGS        equ 0x00001808
OFN_SAVE_FLAGS        equ 0x0000080A

WM_DESTROY      equ 0x0002
WM_CLOSE        equ 0x0010
WM_KEYDOWN      equ 0x0100
WM_SYSKEYDOWN   equ 0x0104
WM_SYSCHAR      equ 0x0106
WM_LBUTTONDOWN  equ 0x0201
WM_RBUTTONDOWN  equ 0x0204
WM_MOUSEWHEEL   equ 0x020A

VK_BACK         equ 0x08
VK_TAB          equ 0x09
VK_RETURN       equ 0x0D
VK_SHIFT        equ 0x10
VK_CONTROL      equ 0x11
VK_ESCAPE       equ 0x1B
VK_SPACE        equ 0x20
VK_PRIOR        equ 0x21
VK_NEXT         equ 0x22
VK_END          equ 0x23
VK_HOME         equ 0x24
VK_LEFT         equ 0x25
VK_UP           equ 0x26
VK_RIGHT        equ 0x27
VK_DOWN         equ 0x28
VK_INSERT       equ 0x2D
VK_DELETE       equ 0x2E
VK_F1           equ 0x70
VK_F2           equ 0x71
VK_F3           equ 0x72
VK_F4           equ 0x73
VK_F5           equ 0x74
VK_F6           equ 0x75
VK_F7           equ 0x76
VK_F8           equ 0x77
VK_F9           equ 0x78
VK_F10          equ 0x79
VK_F11          equ 0x7A
VK_F12          equ 0x7B

; Composer-669-Farbpalette (GDI COLORREF = 0x00BBGGRR)
COL_BG          equ 0x00624222
COL_BLACK       equ 0x00000000
COL_BEAT        equ 0x00443424
COL_PLAYROW     equ 0x00584814
COL_EDITROW     equ 0x007A5018
COL_CURSOR      equ 0x00F0D020
COL_CYAN        equ 0x00F0F000
COL_TEAL        equ 0x00B0B000
COL_HDR_ON      equ 0x00786400
COL_HDR_OFF     equ 0x00282848
COL_NOTE        equ 0x00F0D8B8
COL_WHITE       equ 0x00FFFFFF
COL_DOT         equ 0x0070583C
COL_PURP        equ 0x00900050
COL_PURPHI      equ 0x00F048B8
COL_MUTED       equ 0x00504040

; kernel32.dll
extern GetModuleHandleA
extern GetTickCount
extern ExitProcess
extern Sleep
extern CreateFileA
extern ReadFile
extern WriteFile
extern CloseHandle

; user32.dll
extern RegisterClassA
extern LoadCursorA
extern AdjustWindowRect
extern CreateWindowExA
extern DefWindowProcA
extern GetDC
extern ReleaseDC
extern PeekMessageA
extern TranslateMessage
extern DispatchMessageA
extern GetKeyState
extern GetSystemMetrics
extern GetWindowRect
extern SetWindowLongA
extern SetWindowPos
extern ScreenToClient
extern MessageBoxA

; gdi32.dll
extern CreateCompatibleDC
extern CreateCompatibleBitmap
extern SelectObject
extern CreateFontA
extern SetBkMode
extern SetBkColor
extern SetTextColor
extern ExtTextOutA
extern BitBlt
extern StretchBlt
extern StretchDIBits
extern SetStretchBltMode
extern PatBlt

; winmm.dll
extern waveOutOpen
extern waveOutPrepareHeader
extern waveOutWrite
extern waveOutReset
extern waveOutClose
extern waveInOpen
extern waveInPrepareHeader
extern waveInUnprepareHeader
extern waveInAddBuffer
extern waveInStart
extern waveInStop
extern waveInReset
extern waveInClose

; comdlg32.dll
extern GetOpenFileNameA
extern GetSaveFileNameA

; ============================================================================
; Initialisierte Daten
; ============================================================================
section .data

clsName      db "C669TrackerCls", 0
winTitle     db "Composer 669 x64 Tracker", 0
fontName     db "Consolas", 0
hexTbl       db "0123456789ABCDEF"
noteNames    db "C C#D D#E F F#G G#A A#B "
txtOn        db "On "
txtOff       db "Off"
txtEmptySlot db " (leer)   "
txtNewFile   db "_NEUER_SONG     "
txtRecName   db "REC 00 22kHz"
txtMinus     db "[-]"
txtPlus      db "[+]"

flt669       db "Composer 669 (*.669)", 0, "*.669", 0, "Alle Dateien (*.*)", 0, "*.*", 0, 0
defExt       db "669", 0
defSaveName  db "song.669", 0
dlgOpenTitle db "669-Modul laden", 0
dlgSaveTitle db "669-Modul speichern", 0

fltWav       db "WAV-Audio (*.wav)", 0, "*.wav", 0, "Alle Dateien (*.*)", 0, "*.*", 0, 0
defWavExt    db "wav", 0
dlgWavTitle  db "WAV-Sample auf aktiven Slot (00..3F) laden", 0

; Dialogtexte fuer Sicherheitsabfragen beim Loeschen
capClrPat     db "Pattern loeschen", 0
msgClrPat     db "Aktuelles Pattern (alle 64 Zeilen) wirklich loeschen?", 0
capClrAllPats db "Alle Patterns loeschen", 0
msgClrAllPats db "Alle 128 Patterns und die Order-Liste wirklich leeren? (Samples bleiben erhalten)", 0
capClrInsts   db "Alle Instrumente loeschen", 0
msgClrInsts   db "Alle 64 Sample-Slots leeren? (Slots 00..07 werden auf Synth zurueckgesetzt)", 0
capClrAll     db "Neuer Song / Alles loeschen", 0
msgClrAll     db "Wirklich ALLES leeren (alle Patterns, Orders, Titel und alle 64 Samples)?", 0

stSaved      db "SAVED  "
stLoaded     db "LOADED "
stError      db "ERROR! "
stBad669     db "BAD669 "
stWavOk      db "WAV OK "
stBadWav     db "BADWAV "
stLoopOn     db "LOOPON "
stLoopOff    db "LOOPOFF"
stSynth      db "SYNTH  "
stBreak      db "BREAK  "
stClrPat     db "PAT CLR"
stClrPats    db "PATSCLR"
stClrInst    db "INSTCLR"
stNewSong    db "NEWSONG"
stCpCh       db "CH COPY"
stPsCh       db "CH PAST"
stCpPat      db "PAT CPY"
stPsPat      db "PAT PST"
stNoClip     db "NO CLIP"
stRecOn      db "REC... "
stRecOk      db "REC OK "
stRecErr     db "REC ERR"
stSynOk      db "SYN OK "
stFxNorm     db "NORM OK"
stFxFi       db "FADE IN"
stFxFo       db "FADEOUT"
stFxRev      db "REVERSE"
stFxBst      db "BOOST +"
stFxHlf      db "HALFLEN"
stFxTrm      db "TRIM OK"

topBar      db " Composer 669 x64 Tracker        File: _4KSYNTH          Mode: PAT-LOOP        "
clrBar      db " Loeschen: [Akt.Pat] [Alle Pat] [Alle Inst] [Alles/Neu] [SpC] [SpV] [PtC] [PtV] "
botBar      db " Tmp:5 Oct:2 Ch:0 Pat:00 Row:00/3F Ins:00 Stp:1 [SPACE=Play] [Brk] [I] [D] [?]  "
demoTopBar  db "4K:PLASMA File:                  Ord:00 Pat:00 Row:00 Tmp:5  [TAB=FX ESC=Back] "
demoBotBar  db " [TAB/Rad=Effekt] [Klick/ESC=Tracker] [R-Klick/SPACE=Play] [Strg+O=669] [FS]   "
demoFxNames db "PLASMATUNNELRASTER"
demoSep     db "   *** 669 x64 ***  "

; Texte und Layouts fuer den Sample- und Instrument-Editor
seTopBar    db " SAMPLE-EDITOR  Slot:00              Len:00000  Loop:OFF  [ESC=Tracker]        "
seInfoBar   db " Sel:00000-00000  Loop:00000-FFFFF   [L=SelStart R=SelEnde Shift+L/R=Loop]     "
seActBar    db " [WAV Laden] [REC Start] [Vorhoeren] [StopTon] [Loop:ON] [Sel->Lp] [Trim] [Syn]"
seFxBar     db " FX: [Normalis] [Fade In] [FadeOut] [Reverse] [Boost +] [HalbLen] [SelAll]     "
seSynthHdr  db " SYNTHESIZER / INSTRUMENT-DESIGNER (ADSR, Wellenform, Sweep & Filter -> PCM)  "
seWaveBar   db "   Welle: [0:Sinus] [1:Saege] [2:Puls ] [3:Dreie] [4:Rausc]                    "
sePulseTxt  db "Pulsbreite: 50% "
sePresetBar db "   Presets: [808Kick] [Snare  ] [AcidSaw] [ChipSqr] [SoftPad] [LaserFX]        "
seRenderBar db "   [ SYNTH -> SLOT RENDERN  ]  [ SYNTH + AUTO-LOOP PAD ]  [ Slot - / Slot + ]  "
seRecBanner db "   *** AUFNAHME LAEUFT (22050 Hz, 8-Bit Mono) - [REC Stop!] zum Beenden ***   "
seKeyHint   db " QWERTZ-Klaviatur (Y..M / Q..I): Aktiven Slot live spielen | Mausrad: Slot    "
seBotBar    db " Slot:00  Oct:2  [SPACE=Play Song]  [Strg+E / ESC = Zurueck Tracker]       [?] "
seEmptyMsg  db " Slot nutzt internen Synth / leer - WAV laden, REC oder SYNTH nutzen"

txtRecBtnOn  db "[REC Start]"
txtRecBtnOff db "[REC Stop!]"
txtLpBtnOn   db "[Loop:ON]"
txtLpBtnOff  db "[LoopOFF]"

; Startspalte und Breite der 7 Effekt-Buttons in Zeile 24
seFxBtnPos:
    db  5, 10
    db 16,  9
    db 26,  9
    db 36,  9
    db 46,  9
    db 56,  9
    db 66,  8

seParamNames:
    db "Attack    "
    db "Decay     "
    db "Sustain   "
    db "Release   "
    db "Dauer     "
    db "Sweep     "
    db "Detune/Cho"
    db "Filter LP "

seParamHints:
    db "Einschwing "
    db "Abfallzeit "
    db "Haltepegel "
    db "Ausklingen "
    db "0.1..2.0 s "
    db "<50Dn >50Up"
    db "2.Oszillat."
    db "Tiefpass   "

seWaveSlotNames:
    db "SYN Sinus   "
    db "SYN Saege   "
    db "SYN Puls    "
    db "SYN Dreieck "
    db "SYN Rauschen"

; 6 Synth-Presets: Wave, Pulse, A, D, S, R, Dur, Sweep, Detune, Filter, AutoLoop
sePresets:
    db 0, 50,  0, 28,  0, 20, 18,  8,  0, 80, 0   ; 808Kick
    db 4, 50,  0, 32,  0, 18, 14, 35,  0, 75, 0   ; Snare
    db 1, 50,  2, 40, 45, 15, 35, 50, 20, 55, 0   ; AcidSaw
    db 2, 25,  0, 20, 70, 10, 30, 50, 10, 95, 1   ; ChipSqr
    db 3, 50, 35, 25, 80, 40, 75, 50, 35, 65, 1   ; SoftPad
    db 1, 50,  0, 45, 10, 20, 25,  5,  0, 90, 0   ; LaserFX

titleBox:
    db "  Composer 669 Live Tracker (x64)   "
    db "  QWERTZ=Piano  Pfeile/Tab  Del/BS  "
    db " Ctrl+O/S:669 Ctrl+E:Inst Ctrl+D:4K "

defSampNames:
    db "00 Kick Drum"
    db "01 Snare 669"
    db "02 Hi-Hat HP"
    db "03 Oct Bass "
    db "04 Pulse Arp"
    db "05 Saw Lead "
    db "06 Pluck/Ech"
    db "07 Sinus Pad"

align 4
saveSampLen:
    dd 2048, 2048, 1024, 128, 128, 128, 128, 128
saveSampStart:
    dd 0, 0, 0, 0, 0, 0, 0, 0
saveSampEnd:
    dd 0xFFFFF, 0xFFFFF, 0xFFFFF, 128, 128, 128, 128, 128

statLines:
    db "Vol:   F"
    db "Ord:  00"
    db "Pat:  00"
    db "Row:  00"
    db "Step: 01"
    db "Inst: 00"

align 4
f_2pi_256   dd 0.024543693
f_127       dd 127.0
f_63_5      dd 63.5
f_tun_scale dd 2560.0
f_128_pi    dd 40.743665

; Copper-Rasterbars: 8 Basisfarben (0x00RRGGBB) und 17-Zeilen-Glanzprofil
align 4
rasterCols:
    dd 0x0000D0FF, 0x00FF20B8, 0x007838FF, 0x002078FF
    dd 0x00FFA800, 0x0020E088, 0x00FF3848, 0x0068E8FF
barCurve:
    db 24, 52, 88, 128, 170, 210, 238, 255, 255, 255, 238, 210, 170, 128, 88, 52, 24
barSpec:
    db 0, 0, 0, 0, 0, 16, 48, 96, 130, 96, 48, 16, 0, 0, 0, 0, 0

; BITMAPINFOHEADER fuer 320x200 32-Bit Top-Down DIB (4K-Demo-Framebuffer)
align 4
demoBmi:
    dd 40
    dd DEMO_W
    dd -DEMO_H
    dw 1
    dw 32
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

; 12-TET Phaseninkremente fuer Oktave 2 (C-2 bis B-2 bei 44100 Hz)
align 4
noteTbl:
    dd  6370030,  6748811,  7150117,  7575285
    dd  8025735,  8502969,  9008583,  9544261
    dd 10111791, 10713070, 11350103, 12025015

; 16-Bit-Fixkomma-Multiplikatoren 2^(s/12) * 65536 fuer PCM-Resampling
align 4
semiMulTbl:
    dd  65536,  69433,  73562,  77936
    dd  82570,  87480,  92682,  98193
    dd 104032, 110218, 116772, 123715

; Hardware-Scancode (0..63) -> Halbton-Offset (0..24) fuer QWERTZ/QWERTY-Klaviatur
scanToNote:
    db 0xFF, 0xFF, 0xFF,   13,   15, 0xFF,   18,   20
    db   22, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    db   12,   14,   16,   17,   19,   21,   23,   24
    db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,    1
    db    3, 0xFF,    6,    8,   10, 0xFF, 0xFF, 0xFF
    db 0xFF, 0xFF, 0xFF, 0xFF,    0,    2,    4,    5
    db    7,    9,   11,   12, 0xFF, 0xFF, 0xFF, 0xFF
    db 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF

; 6 Sub-Spalten je Zelle: Note(3), InstHi(1), InstLo(1), Vol(1), FxCmd(1), FxVal(1)
subColOff db 0, 3, 4, 5, 6, 7
subColWid db 3, 1, 1, 1, 1, 1

demoChords:
    db  9, 21, 24, 28
    db  5, 17, 21, 24
    db  0, 12, 16, 19
    db  7, 19, 23, 26
demoArpIdx db 0, 1, 2, 1
demoLead:
    db 33, 0xFF, 28, 0xFF, 24, 28, 33, 0xFF
    db 29, 0xFF, 28, 0xFF, 24, 0xFF, 21, 24
    db 28, 0xFF, 31, 0xFF, 28, 24, 28, 31
    db 26, 0xFF, 31, 0xFF, 35, 0xFF, 31, 0xFF

; Ausgabeformat: 44100 Hz, 16-Bit Stereo
align 2
wfx:
    dw 1
    dw 2
    dd SAMPLE_RATE
    dd SAMPLE_RATE * 4
    dw 4
    dw 16
    dw 0

; Aufnahmeformat: 22050 Hz, 8-Bit Mono (direkt passend fuer 8-Bit-PCM-Slots)
align 2
wfxRec:
    dw 1
    dw 1
    dd 22050
    dd 22050
    dw 1
    dw 8
    dw 0

; Makro fuer exakt 64 Zeichen breite Hilfezeilen
%macro HLINE 1
%%s: db %1
     times 64 - ($ - %%s) db ' '
%endmacro

helpLines:
    HLINE "        COMPOSER 669 x64 TRACKER - TASTEN- & MAUSHILFE"
    HLINE "----------------------------------------------------------------"
    HLINE " WIEDERGABE & NAVIGATION"
    HLINE "  Leertaste       : Play / Stop ab Cursorzeile"
    HLINE "  F5 / F6         : Start ab Anfang / PAT-LOOP <-> SONG-ORD"
    HLINE "  Pfeile / Tab    : Cursor bewegen / naechster Kanal"
    HLINE "  Pos1 / Ende     : Zeile 00 / Pattern-Break anspringen"
    HLINE "  Bild Auf / Ab   : Pattern vor / zurueck (00..7F)"
    HLINE " "
    HLINE " EDITOR, KLAVIATUR & EFFEKTE"
    HLINE "  Y..M / Q..I     : QWERTZ-Piano (2 Oktaven, Live-Vorhoeren)"
    HLINE "  0-9 / A-F       : Instrument (00..3F), Volume, Effekt setzen"
    HLINE "  Entf / Backsp.  : Feld oder Zeile loeschen"
    HLINE "  Strg+C / +V     : Aktuelle Spur (64 Zeilen) kopieren/einf."
    HLINE "  Strg+Shift+C/V  : Ganzes Pattern (8 Spuren) kopieren/einf."
    HLINE "  F1 / F2         : Oktave runter / hoch"
    HLINE "  F3 / F4         : Sample-Slot / Instrument waehlen (00..3F)"
    HLINE "  F7 / F8         : Tempo runter / hoch (1..F)"
    HLINE "  F9 / F10        : Edit-Step (1,2,4,0) / Kanal Mute (On/Off)"
    HLINE "  Effekte (a..f)  : a:Up b:Down c:Porta d:Detune e:Vib f:Tempo"
    HLINE " "
    HLINE " DATEIEN, SAMPLE-EDITOR, LOESCHEN & DEMO"
    HLINE "  Strg+O / F11    : .669-Modul laden"
    HLINE "  Strg+S / F12    : .669-Modul speichern"
    HLINE "  Strg+W / Einfg  : WAV-Sample auf aktiven Slot laden"
    HLINE "  Strg+E / [I]    : Sample- & Instrument-Editor (REC/FX/ADSR)"
    HLINE "  Strg+L / Strg+U : Sample-Loop umschalten / Slot auf Synth"
    HLINE "  Strg+B          : Pattern-Break auf Cursorzeile setzen"
    HLINE "  Strg+Entf / +N  : Akt. Pattern leeren / Alles leer (Neu)"
    HLINE "  Strg+Shift+P / I: Alle Patterns / Alle 64 Samples leeren"
    HLINE "  Strg+D / [D]    : 4K-Demo (TAB: Plasma / 3D-Tunnel / Raster)"
    HLINE "  Alt+Ent / Strg+F: Vollbild (Fullscreen) umschalten"
    HLINE "  Maus / Rad      : Klick=Wahl/Loeschen/C-V, Fusszeile L=+/R=-"
    HLINE "----------------------------------------------------------------"
    HLINE "        Klick, ESC oder Strg+H schliesst diese Hilfe"

; ============================================================================
; Uninitialisierte Daten (nur alignb in .bss)
; ============================================================================
section .bss

alignb 16
hInst       resq 1
hWnd        resq 1
hDC         resq 1
memDC       resq 1
hWaveOut    resq 1
hWaveIn     resq 1
wndClass    resb 72
winRect     resd 4
savedRect   resd 4
tmpRect     resd 4
msg         resb 48
ofn         resb 152
filePath    resb 260
wavPath     resb 260

alignb 4
running       resd 1
dirty         resd 1
lastDraw      resd 1
tmpInt        resd 1
bytesIO       resd 1
mousePt       resd 2
mousePxX      resd 1
showHelp      resd 1
demoMode      resd 1
sampEdMode    resd 1
demoFx        resd 1
tunnelReady   resd 1
demoTick      resd 1
demoStarZ     resd 1
demoScrollX   resd 1
origNos       resd 1
origNop       resd 1
saveNos       resd 1
saveNop       resd 1

; Sample-Editor & ADSR-Synth-Zustand
isRecording   resd 1
recStartTick  resd 1
seLastSlot    resd 1
seLastLen     resd 1
seSelStart    resd 1
seSelEnd      resd 1
seWave        resd 1
sePulse       resd 1
seSynthLfsr   resd 1
seParams      resd 8            ; Attack, Decay, Sustain, Release, Dauer, Sweep, Detune, Filter

; Zwischenablage-Status fuer Spur und Pattern
clipChValid   resd 1
clipPatValid  resd 1
clipPatBreak  resd 1
clipPatTempo  resd 1

; Fullscreen- und Skalierungszustand
isFullscreen  resd 1
scrW          resd 1
scrH          resd 1
dispX         resd 1
dispY         resd 1
dispW         resd 1
dispH         resd 1

; WAV-Parser-Zwischenwerte
wavFmtTag     resd 1
wavChannels   resd 1
wavRate       resd 1
wavBits       resd 1
wavDataOff    resd 1
wavDataSize   resd 1

isPlaying     resd 1
songMode      resd 1
needPatTempo  resd 1
curTempo      resd 1
curOct        resd 1
curInst       resd 1
sampViewTop   resd 1
patViewTop    resd 1
ordViewTop    resd 1
curVol        resd 1
editStep      resd 1

curPat        resd 1
editRow       resd 1
curCh         resd 1
curSubCol     resd 1

playOrder     resd 1
playPat       resd 1
playRow       resd 1
dispPlayOrder resd 1
dispPlayPat   resd 1
dispPlayRow   resd 1
orderLen      resd 1
loopOrder     resd 1
tickSamples   resd 1
fxDivCnt      resd 1
lfsr          resd 1
prevNoise     resd 1
vibPhase      resd 1
previewCell   resb 4

; 64 Sample-Slots (Metadaten)
alignb 16
sampHasPcm       resb MAX_SAMPS
sampNames        resb MAX_SAMPS * 12
alignb 4
sampLenTbl       resd MAX_SAMPS
sampLoopStartTbl resd MAX_SAMPS
sampLoopEndTbl   resd MAX_SAMPS
sampC2Step       resd MAX_SAMPS

; Song-Tabellen (128 Orders, 128 Pattern-Tempos, 128 Pattern-Breaks)
alignb 16
dxTbl          resd 80
sinTbl         resd 256
demoPal        resd 256
demoScrollTxt  resb 128
chMuted        resb NUM_CH
orderList      resb MAX_ORDERS
patTempos      resb MAX_PATS
patBreaks      resb MAX_PATS
voices         resb NUM_CH * VOICE_SIZE
voiceTargetInc resd NUM_CH
clipChBuf      resb PAT_ROWS * CELL_SIZE
clipPatBuf     resb PAT_BYTES
patterns       resb MAX_PATS * PAT_BYTES
lineBuf        resb 32
cellBuf        resb 16

alignb 16
waveHdrs       resb BUF_COUNT * 48
audioBufs      resb BUF_COUNT * BUF_BYTES
recHdr         resb 48
recBuf         resb MAX_SAMP_BYTES
demoFb         resd DEMO_W * DEMO_H
tunnelDist     resb DEMO_W * DEMO_H
tunnelAngle    resb DEMO_W * DEMO_H
tunnelFog      resb DEMO_W * DEMO_H
pcmSlots       resb MAX_SAMPS * MAX_SAMP_BYTES
fileBuf        resb FILE_BUF_SIZE