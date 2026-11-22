; 64-Bit Windows Composer-669-Tracker (NASM, ohne CRT)
; Echtzeit-8-Kanal-Stereo-Mixer (Synth + 8-Bit-PCM) + WAV-Import + .669 Laden/Speichern
; Kompatibel mit lokaler Toolchain (nasm.exe + GoLink.exe inkl. comdlg32.dll)

default rel
bits 64

; Raster- und Fensterkonstanten (80x50 Zeichen a 12x12 Pixel)
CHAR_W          equ 12
CHAR_H          equ 12
WIN_W           equ 960
WIN_H           equ 600

; Tracker-Datenmodell
MAX_PATS        equ 16
PAT_ROWS        equ 32
NUM_CH          equ 8
CELL_SIZE       equ 4                   ; Byte 0=Note, 1=Inst, 2=Vol, 3=FX
ROW_BYTES       equ NUM_CH * CELL_SIZE  ; 32 Byte pro Zeile
PAT_BYTES       equ PAT_ROWS * ROW_BYTES; 1024 Byte pro Pattern
VOICE_SIZE      equ 64
MAX_SAMP_BYTES  equ 131072              ; 128 KB PCM-Speicher pro Sample-Slot (8 Slots = 1 MB)
FILE_BUF_SIZE   equ 2097152             ; 2 MB Datei-Puffer fuer .wav und .669 I/O

; Echtzeit-Audio-Streaming (44100 Hz, 16-Bit Stereo, 4 Puffer a 512 Frames)
SAMPLE_RATE     equ 44100
BUF_COUNT       equ 4
BUF_FRAMES      equ 512
BUF_BYTES       equ BUF_FRAMES * 4      ; 2048 Byte pro Puffer (~11,6 ms)
TICKS_PER_TEMPO equ 1024                ; Tempo 5 = 5120 Samples/Zeile (~129 BPM)

; Windows-Konstanten
WS_TRACKER      equ 0x10CA0000          ; OVERLAPPED|CAPTION|SYSMENU|MINIMIZEBOX|VISIBLE
SRCCOPY         equ 0x00CC0020
ETO_OPAQUE      equ 2
TRANSPARENT     equ 1
WHDR_DONE       equ 0x00000001
IDC_ARROW       equ 32512

GENERIC_READ          equ 0x80000000
GENERIC_WRITE         equ 0x40000000
FILE_SHARE_READ       equ 1
CREATE_ALWAYS         equ 2
OPEN_EXISTING         equ 3
FILE_ATTRIBUTE_NORMAL equ 0x80
OFN_OPEN_FLAGS        equ 0x00001808    ; FILEMUSTEXIST | PATHMUSTEXIST | NOCHANGEDIR
OFN_SAVE_FLAGS        equ 0x0000080A    ; OVERWRITEPROMPT | PATHMUSTEXIST | NOCHANGEDIR

WM_DESTROY      equ 0x0002
WM_CLOSE        equ 0x0010
WM_KEYDOWN      equ 0x0100

VK_BACK         equ 0x08
VK_TAB          equ 0x09
VK_CONTROL      equ 0x11
VK_ESCAPE       equ 0x1B
VK_SPACE        equ 0x20
VK_PRIOR        equ 0x21                ; Bild Auf
VK_NEXT         equ 0x22                ; Bild Ab
VK_LEFT         equ 0x25
VK_UP           equ 0x26
VK_RIGHT        equ 0x27
VK_DOWN         equ 0x28
VK_INSERT       equ 0x2D                ; Einfg (WAV laden)
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
extern PeekMessageA
extern TranslateMessage
extern DispatchMessageA
extern GetKeyState

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

; winmm.dll
extern waveOutOpen
extern waveOutPrepareHeader
extern waveOutWrite
extern waveOutReset
extern waveOutClose

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

; Dateidialog-Strings (.669 und .wav)
flt669       db "Composer 669 (*.669)", 0, "*.669", 0, "Alle Dateien (*.*)", 0, "*.*", 0, 0
defExt       db "669", 0
defSaveName  db "song.669", 0
dlgOpenTitle db "669-Modul laden", 0
dlgSaveTitle db "669-Modul speichern", 0

fltWav       db "WAV-Audio (*.wav)", 0, "*.wav", 0, "Alle Dateien (*.*)", 0, "*.*", 0, 0
defWavExt    db "wav", 0
dlgWavTitle  db "WAV-Sample auf aktiven Slot laden", 0

; 7-Zeichen-Statusmeldungen fuer Kopfzeile rechts
stSaved      db "SAVED  "
stLoaded     db "LOADED "
stError      db "ERROR! "
stBad669     db "BAD669 "
stWavOk      db "WAV OK "
stBadWav     db "BADWAV "
stLoopOn     db "LOOPON "
stLoopOff    db "LOOPOFF"
stSynth      db "SYNTH  "

topBar    db " Composer 669 x64 Tracker        File: _4KSYNTH          Mode: PAT-LOOP        "
botBar    db " Tempo: 5  Oct: 2  Ch: 0  Pat: 00  Row: 00  Inst: 00  Step: 1  [SPACE=Play]    "

; 3x36 Zeichen Song-Message (wird 1:1 im 669-Header gespeichert und geladen)
titleBox:
    db "  Composer 669 Live Tracker (x64)   "
    db "  QWERTZ=Piano  Pfeile/Tab  Del/BS  "
    db "  Ctrl+O:669 Ctrl+S:Save Ctrl+W:WAV "

; 12x12 Zeichen Sample-Namen (00..07 editierbar durch WAV/669-Import)
sampNames:
    db "00 Kick Drum"
    db "01 Snare 669"
    db "02 Hi-Hat HP"
    db "03 Oct Bass "
    db "04 Pulse Arp"
    db "05 Saw Lead "
    db "06 Pluck/Ech"
    db "07 Sinus Pad"
    db "------------"
    db "Ctrl+W: WAV "
    db "Ctrl+L: Loop"
    db "Ctrl+U: Syn "

; Originalnamen fuer Reset einzelner Slots auf Synth (Strg+U)
defSampNames:
    db "00 Kick Drum"
    db "01 Snare 669"
    db "02 Hi-Hat HP"
    db "03 Oct Bass "
    db "04 Pulse Arp"
    db "05 Saw Lead "
    db "06 Pluck/Ech"
    db "07 Sinus Pad"

; Angezeigte Sample-Metadaten (werden bei WAV- und .669-Import aktualisiert)
align 4
sampLenTbl:
    dd 0x00800, 0x00800, 0x00400, 0x00080
    dd 0x00080, 0x00080, 0x00080, 0x00080
    dd 0x00000, 0x00000, 0x00000, 0x00000

sampLoopStartTbl:
    dd 0x00000, 0x00000, 0x00000, 0x00000
    dd 0x00000, 0x00000, 0x00000, 0x00000
    dd 0x00000, 0x00000, 0x00000, 0x00000

sampLoopEndTbl:
    dd 0xFFFFF, 0xFFFFF, 0xFFFFF, 0x00080
    dd 0x00080, 0x00080, 0x00080, 0x00080
    dd 0xFFFFF, 0xFFFFF, 0xFFFFF, 0xFFFFF

; Standard-Groessen/Loops fuer nicht belegte Synth-Slots beim .669-Export
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

; FPU-Konstanten fuer Sinus-Tabelle und 32-Bit-Float-WAV-Konvertierung
align 4
f_2pi_256 dd 0.024543693
f_127     dd 127.0
f_63_5    dd 63.5

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

; Sub-Spalten-Geometrie innerhalb einer 8-Zeichen-Zelle
subColOff db 0, 3, 5, 6, 7
subColWid db 3, 2, 1, 1, 1

; Demo-Akkorde fuer die Startbelegung von Pattern 00 und 01
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

; WAVEFORMATEX (16-Bit Stereo, 44100 Hz)
align 2
wfx:
    dw 1
    dw 2
    dd SAMPLE_RATE
    dd SAMPLE_RATE * 4
    dw 4
    dw 16
    dw 0

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
wndClass    resb 72
winRect     resd 4
tmpRect     resd 4
msg         resb 48
ofn         resb 152            ; OPENFILENAMEA auf x64 (0x98 Byte)
filePath    resb 260            ; Dateipfad fuer .669-Dialog
wavPath     resb 260            ; Separater Dateipfad fuer .wav-Dialog

alignb 4
running     resd 1
dirty       resd 1
lastDraw    resd 1
tmpInt      resd 1
bytesIO     resd 1
origNop     resd 1

; WAV-Parser-Zwischenwerte
wavFmtTag   resd 1
wavChannels resd 1
wavRate     resd 1
wavBits     resd 1
wavDataOff  resd 1
wavDataSize resd 1

isPlaying   resd 1
songMode    resd 1
curTempo    resd 1
curOct      resd 1
curInst     resd 1
curVol      resd 1
editStep    resd 1

curPat      resd 1
editRow     resd 1
curCh       resd 1
curSubCol   resd 1

playOrder   resd 1
playPat     resd 1
playRow     resd 1
orderLen    resd 1
tickSamples resd 1
fxDivCnt    resd 1
lfsr        resd 1
prevNoise   resd 1
vibPhase    resd 1
previewCell resb 4

; Sample-Slot-Zustand (8 Slots: Flag + 16.16 Basis-Schrittweite fuer C-2)
alignb 16
sampHasPcm  resb NUM_CH
alignb 4
sampC2Step  resd NUM_CH

alignb 16
dxTbl       resd 80
sinTbl      resd 256
chMuted     resb NUM_CH
orderList   resb 16
voices      resb NUM_CH * VOICE_SIZE
patterns    resb MAX_PATS * PAT_BYTES
lineBuf     resb 32
cellBuf     resb 16

alignb 16
waveHdrs    resb BUF_COUNT * 48
audioBufs   resb BUF_COUNT * BUF_BYTES
pcmSlots    resb NUM_CH * MAX_SAMP_BYTES
fileBuf     resb FILE_BUF_SIZE

; ============================================================================
; Programmcode
; ============================================================================
section .text
global start

; Lineare Note (AL = 0..59) in 32-Bit-Synth-Phaseninkrement (EAX) wandeln
note_to_inc:
    movzx eax, al
    xor edx, edx
    mov ecx, 12
    div ecx
    mov ecx, eax
    lea r8, [noteTbl]
    mov eax, [r8 + rdx*4]
    shl eax, cl
    ret

; Lineare Note (AL = 0..59) und Slot (ECX = 0..7) in 16.16 PCM-Schrittweite (EAX) wandeln
pcm_note_to_step:
    movzx r8d, cl
    and r8d, 7
    lea r9, [sampC2Step]
    mov r8d, [r9 + r8*4]
    movzx eax, al
    xor edx, edx
    mov ecx, 12
    div ecx                     ; EAX = Oktave (0..4), EDX = Halbton (0..11)
    mov ecx, eax
    lea r9, [semiMulTbl]
    mov eax, [r9 + rdx*4]
    imul rax, r8
    shr rax, 16
    shl eax, cl
    test eax, eax
    jnz .ps_ok
    mov eax, 1
.ps_ok:
    ret

; EAX auf 16-Bit-Signed begrenzen
clamp_s16:
    cmp eax, 32767
    jle .lo
    mov eax, 32767
    ret
.lo:
    cmp eax, -32767
    jge .ok
    mov eax, -32767
.ok:
    ret

; Byte AL als 2 Hex-Zeichen nach [R8] schreiben
fmt_hex2:
    lea rdx, [hexTbl]
    movzx ecx, al
    shr ecx, 4
    mov cl, [rdx + rcx]
    mov [r8], cl
    movzx ecx, al
    and ecx, 15
    mov cl, [rdx + rcx]
    mov [r8 + 1], cl
    ret

; 20-Bit-Wert EAX als 5 Hex-Zeichen nach [R8] schreiben
fmt_hex5:
    lea rdx, [hexTbl]
    mov ecx, 4
.l5:
    mov r9d, eax
    and r9d, 15
    mov r9b, [rdx + r9]
    mov [r8 + rcx], r9b
    shr eax, 4
    dec ecx
    jns .l5
    ret

; Zeiger auf Zelle (ECX=pat, EDX=row, R8D=ch) in RAX berechnen
get_cell_ptr:
    mov eax, ecx
    shl eax, 10
    mov r9d, edx
    shl r9d, 5
    add eax, r9d
    lea eax, [rax + r8*4]
    lea r9, [patterns]
    add rax, r9
    ret

; 7-Zeichen-Statusmeldung (RCX = Ptr) in topBar + 72 uebertragen
set_status:
    push rsi
    push rdi
    mov rsi, rcx
    lea rdi, [topBar + 72]
    mov ecx, 7
    rep movsb
    pop rdi
    pop rsi
    ret

; Alle Stimmen stumm schalten (vor modalen Datei-Dialogen)
silence_audio:
    mov dword [isPlaying], 0
    lea rax, [voices]
    xor ecx, ecx
.sa_loop:
    mov dword [rax + 0], 0
    mov dword [rax + 20], 0
    mov dword [rax + 48], 0
    add rax, VOICE_SIZE
    inc ecx
    cmp ecx, NUM_CH
    jb .sa_loop
    ret

; OPENFILENAMEA-Struktur fuer x64 initialisieren: RCX=pfadBuf, RDX=filter, R8=defExt
init_ofn:
    push rdi
    push rcx
    push rdx
    push r8
    lea rdi, [ofn]
    xor eax, eax
    mov ecx, 152
    rep stosb
    pop r8
    pop rdx
    pop rcx

    mov dword [ofn + 0x00], 152         ; lStructSize
    mov rax, [hWnd]
    mov [ofn + 0x08], rax               ; hwndOwner
    mov [ofn + 0x18], rdx               ; lpstrFilter
    mov dword [ofn + 0x2C], 1           ; nFilterIndex
    mov [ofn + 0x30], rcx               ; lpstrFile
    mov dword [ofn + 0x38], 260         ; nMaxFile
    mov [ofn + 0x68], r8                ; lpstrDefExt
    pop rdi
    ret

; Basis-Dateinamen aus Pfad (RCX) in Zielpuffer (RDX) mit fester Breite (R8D) kopieren
copy_basename:
    push rsi
    push rdi
    mov rsi, rcx
    mov rdi, rsi
.cb_end:
    cmp byte [rdi], 0
    je .cb_back
    inc rdi
    jmp .cb_end
.cb_back:
    cmp rdi, rsi
    je .cb_copy
    dec rdi
    cmp byte [rdi], '\'
    jne .cb_back
    inc rdi
.cb_copy:
    mov rsi, rdi
    mov rdi, rdx
    xor ecx, ecx
.cb_loop:
    cmp ecx, r8d
    jae .cb_done
    mov al, [rsi]
    test al, al
    jz .cb_pad
    cmp al, 32
    jb .cb_sp
    cmp al, 126
    jbe .cb_st
.cb_sp:
    mov al, ' '
.cb_st:
    mov [rdi + rcx], al
    inc rsi
    inc ecx
    jmp .cb_loop
.cb_pad:
    mov byte [rdi + rcx], ' '
    inc ecx
    cmp ecx, r8d
    jb .cb_pad
.cb_done:
    pop rdi
    pop rsi
    ret

; ============================================================================
; WAV-Datei aus fileBuf parsen und auf curInst-Slot (0..7) laden
; Eingabe: ECX = Dateilaenge -> Ausgabe: EAX = 1 OK / 0 Fehler
; ============================================================================
parse_wav_buffer:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x28

    cmp ecx, 44
    jb .w_fail
    mov r12d, ecx
    lea rbp, [fileBuf]

    ; RIFF / WAVE Signatur pruefen
    cmp dword [rbp + 0], 0x46464952     ; "RIFF"
    jne .w_fail
    cmp dword [rbp + 8], 0x45564157     ; "WAVE"
    jne .w_fail

    mov dword [wavFmtTag], 0
    mov dword [wavChannels], 0
    mov dword [wavRate], 0
    mov dword [wavBits], 0
    mov dword [wavDataOff], 0
    mov dword [wavDataSize], 0

    ; Subchunks ab Offset 12 durchlaufen
    mov r13d, 12
.w_chunk_loop:
    lea eax, [r13 + 8]
    cmp eax, r12d
    ja .w_chunks_done

    mov eax, [rbp + r13 + 0]            ; Chunk-ID
    mov ecx, [rbp + r13 + 4]            ; Chunk-Groesse
    add r13d, 8

    ; Groesse auf verbleibende Dateilaenge begrenzen
    mov edx, r12d
    sub edx, r13d
    cmp ecx, edx
    jbe .w_cs_ok
    mov ecx, edx
.w_cs_ok:

    cmp eax, 0x20746D66                 ; "fmt "
    jne .w_chk_data
    cmp ecx, 16
    jb .w_next_chunk

    movzx edx, word [rbp + r13 + 0]     ; wFormatTag
    cmp edx, 0xFFFE                     ; WAVE_FORMAT_EXTENSIBLE
    jne .w_tag_ok
    cmp ecx, 40
    jb .w_tag_ok
    movzx edx, word [rbp + r13 + 24]    ; SubFormat-GUID Prefix (1=PCM, 3=Float)
.w_tag_ok:
    mov [wavFmtTag], edx
    movzx edx, word [rbp + r13 + 2]
    mov [wavChannels], edx
    mov edx, [rbp + r13 + 4]
    mov [wavRate], edx
    movzx edx, word [rbp + r13 + 14]
    mov [wavBits], edx
    jmp .w_next_chunk

.w_chk_data:
    cmp eax, 0x61746164                 ; "data"
    jne .w_next_chunk
    test ecx, ecx
    jz .w_next_chunk
    mov [wavDataOff], r13d
    mov [wavDataSize], ecx

.w_next_chunk:
    ; RIFF-Chunks sind auf gerade Bytegrenzen ausgerichtet
    mov edx, ecx
    add edx, 1
    and edx, 0xFFFFFFFE
    add r13d, edx
    cmp r13d, r12d
    jb .w_chunk_loop

.w_chunks_done:
    ; Formatangaben validieren
    cmp dword [wavDataSize], 0
    je .w_fail
    mov eax, [wavChannels]
    cmp eax, 1
    je .w_ch_ok
    cmp eax, 2
    jne .w_fail
.w_ch_ok:
    mov eax, [wavRate]
    test eax, eax
    jz .w_fail
    cmp eax, 4000
    jae .w_r_lo
    mov eax, 4000
.w_r_lo:
    cmp eax, 192000
    jbe .w_r_hi
    mov eax, 192000
.w_r_hi:
    mov [wavRate], eax

    ; Format-Tag und Bittiefe pruefen (PCM: 8/16/24 Bit, IEEE Float: 32 Bit)
    mov eax, [wavFmtTag]
    mov edx, [wavBits]
    cmp eax, 1
    je .w_chk_pcm_bits
    cmp eax, 3
    jne .w_fail
    cmp edx, 32
    jne .w_fail
    jmp .w_fmt_valid
.w_chk_pcm_bits:
    cmp edx, 8
    je .w_fmt_valid
    cmp edx, 16
    je .w_fmt_valid
    cmp edx, 24
    jne .w_fail

.w_fmt_valid:
    ; Frame-Groesse und Anzahl Samples berechnen
    mov ebx, edx
    shr ebx, 3                          ; Bytes pro Kanal-Sample (1, 2, 3, 4)
    imul ebx, [wavChannels]             ; Bytes pro Frame
    test ebx, ebx
    jz .w_fail

    mov eax, [wavDataSize]
    xor edx, edx
    div ebx                             ; EAX = Anzahl Frames
    test eax, eax
    jz .w_fail
    cmp eax, MAX_SAMP_BYTES
    jbe .w_cnt_ok
    mov eax, MAX_SAMP_BYTES
.w_cnt_ok:
    mov r14d, eax                       ; R14D = Ziel-Sample-Anzahl
    mov r15d, eax                       ; Kopie fuer Metadaten

    ; Quell- und Zielzeiger vorbereiten
    mov eax, [wavDataOff]
    lea rsi, [rbp + rax]

    mov eax, [curInst]
    and eax, 7
    shl eax, 17                         ; curInst * 131072
    lea rdi, [pcmSlots]
    add rdi, rax

    mov r8d, [wavFmtTag]
    mov r9d, [wavBits]
    mov r10d, [wavChannels]

.w_conv_loop:
    cmp r8d, 3
    je .w_c_f32
    cmp r9d, 8
    je .w_c_u8
    cmp r9d, 16
    je .w_c_s16
    jmp .w_c_s24

.w_c_u8:
    movzx eax, byte [rsi]
    cmp r10d, 1
    je .w_store_u8
    movzx edx, byte [rsi + 1]
    add eax, edx
    shr eax, 1
    jmp .w_store_u8

.w_c_s16:
    movsx eax, word [rsi]
    cmp r10d, 1
    je .w_s16_m
    movsx edx, word [rsi + 2]
    add eax, edx
    sar eax, 9
    add eax, 128
    jmp .w_store_u8
.w_s16_m:
    sar eax, 8
    add eax, 128
    jmp .w_store_u8

.w_c_s24:
    movsx eax, byte [rsi + 2]
    cmp r10d, 1
    je .w_s24_m
    movsx edx, byte [rsi + 5]
    add eax, edx
    sar eax, 1
.w_s24_m:
    add eax, 128
    jmp .w_store_u8

.w_c_f32:
    fld dword [rsi]
    cmp r10d, 1
    je .w_f32_m
    fadd dword [rsi + 4]
    fmul dword [f_63_5]
    jmp .w_f32_st
.w_f32_m:
    fmul dword [f_127]
.w_f32_st:
    fistp dword [tmpInt]
    mov eax, [tmpInt]
    cmp eax, 127
    jle .w_f32_lo
    mov eax, 127
.w_f32_lo:
    cmp eax, -128
    jge .w_f32_ok
    mov eax, -128
.w_f32_ok:
    add eax, 128

.w_store_u8:
    mov [rdi], al
    inc rdi
    add rsi, rbx
    dec r14d
    jnz .w_conv_loop

    ; Slot-Metadaten setzen (C-3 spielt in Original-Abtastrate -> C-2 = Rate / 2)
    mov ebx, [curInst]
    and ebx, 7
    lea rdx, [sampHasPcm]
    mov byte [rdx + rbx], 1
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], r15d
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopEndTbl]
    mov dword [rdx + rbx*4], 0xFFFFF

    mov eax, [wavRate]
    shl rax, 15
    xor edx, edx
    mov ecx, SAMPLE_RATE
    div rcx
    test eax, eax
    jnz .w_step_ok
    mov eax, 1
.w_step_ok:
    lea rdx, [sampC2Step]
    mov [rdx + rbx*4], eax

    ; Dateinamen in Sample-Liste (12 Zeichen) uebernehmen
    mov eax, ebx
    imul eax, 12
    lea rdx, [sampNames]
    add rdx, rax
    lea rcx, [wavPath]
    mov r8d, 12
    call copy_basename

    mov eax, 1
    jmp .w_ret

.w_fail:
    xor eax, eax
.w_ret:
    add rsp, 0x28
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; ============================================================================
; Windows-Dialog: WAV-Sample auf aktiven Slot (curInst) laden (Strg+W / Einfg)
; ============================================================================
load_wav_dialog:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x40

    call silence_audio

    mov byte [wavPath], 0
    lea rcx, [wavPath]
    lea rdx, [fltWav]
    lea r8, [defWavExt]
    call init_ofn
    lea rax, [dlgWavTitle]
    mov [ofn + 0x58], rax
    mov dword [ofn + 0x60], OFN_OPEN_FLAGS

    lea rcx, [ofn]
    call GetOpenFileNameA
    test eax, eax
    jz .lw_done

    lea rcx, [wavPath]
    mov edx, GENERIC_READ
    mov r8d, FILE_SHARE_READ
    xor r9d, r9d
    mov qword [rsp + 0x20], OPEN_EXISTING
    mov qword [rsp + 0x28], FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 0x30], 0
    call CreateFileA
    cmp rax, -1
    je .lw_err
    mov rsi, rax

    mov dword [bytesIO], 0
    mov rcx, rsi
    lea rdx, [fileBuf]
    mov r8d, FILE_BUF_SIZE
    lea r9, [bytesIO]
    mov qword [rsp + 0x20], 0
    call ReadFile

    mov rcx, rsi
    call CloseHandle

    mov ecx, [bytesIO]
    call parse_wav_buffer
    test eax, eax
    jz .lw_bad

    lea rcx, [stWavOk]
    call set_status

    ; Geladenes WAV-Sample sofort auf C-3 (Originaltonhoehe) vorhoeren
    mov byte [previewCell + 0], 12
    mov eax, [curInst]
    mov [previewCell + 1], al
    mov byte [previewCell + 2], 15
    mov byte [previewCell + 3], 0xFF
    mov ecx, [curCh]
    lea rdx, [previewCell]
    mov r8d, 1
    call trigger_channel

    mov dword [dirty], 1
    jmp .lw_done

.lw_bad:
    lea rcx, [stBadWav]
    call set_status
    mov dword [dirty], 1
    jmp .lw_done

.lw_err:
    lea rcx, [stError]
    call set_status
    mov dword [dirty], 1

.lw_done:
    add rsp, 0x40
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; Loop fuer aktiven WAV/PCM-Slot umschalten (Strg+L)
; ============================================================================
toggle_wav_loop:
    sub rsp, 0x28
    mov eax, [curInst]
    and eax, 7
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    je .tl_done
    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rax*4]
    test ecx, ecx
    jz .tl_done

    lea rdx, [sampLoopEndTbl]
    cmp dword [rdx + rax*4], 0xFFFFF
    je .tl_on
    mov dword [rdx + rax*4], 0xFFFFF
    lea rcx, [stLoopOff]
    call set_status
    jmp .tl_done
.tl_on:
    mov [rdx + rax*4], ecx
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rax*4], 0
    lea rcx, [stLoopOn]
    call set_status
.tl_done:
    add rsp, 0x28
    ret

; ============================================================================
; Aktiven Sample-Slot auf internen Synthesizer zuruecksetzen (Strg+U)
; ============================================================================
unload_wav_slot:
    push rsi
    push rdi
    sub rsp, 0x28

    mov eax, [curInst]
    and eax, 7
    lea rdx, [sampHasPcm]
    mov byte [rdx + rax], 0

    lea rdx, [saveSampLen]
    mov ecx, [rdx + rax*4]
    lea rdx, [sampLenTbl]
    mov [rdx + rax*4], ecx

    lea rdx, [saveSampStart]
    mov ecx, [rdx + rax*4]
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rax*4], ecx

    lea rdx, [saveSampEnd]
    mov ecx, [rdx + rax*4]
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rax*4], ecx

    imul eax, 12
    lea rsi, [defSampNames]
    add rsi, rax
    lea rdi, [sampNames]
    add rdi, rax
    mov ecx, 12
    rep movsb

    lea rcx, [stSynth]
    call set_status

    add rsp, 0x28
    pop rdi
    pop rsi
    ret

; ============================================================================
; Prozedurales 8-Bit-Export-Sample (ECX = Slot 0..7) an [RDI] anhaengen
; ============================================================================
gen_synth_export_sample:
    push rbx
    push r13
    push r15
    lea r15, [sinTbl]

    cmp ecx, 0
    je .g0
    cmp ecx, 1
    je .g1
    cmp ecx, 2
    je .g2
    cmp ecx, 3
    je .g3
    cmp ecx, 4
    je .g4
    cmp ecx, 5
    je .g5
    cmp ecx, 6
    je .g6
    jmp .g7

.g0:
    xor ebx, ebx
    mov r13d, 0x05800000
    xor ecx, ecx
.g0_l:
    add ebx, r13d
    cmp r13d, 0x01800000
    jbe .g0_f
    mov eax, r13d
    shr eax, 9
    sub r13d, eax
.g0_f:
    mov eax, ebx
    shr eax, 24
    mov eax, [r15 + rax*4]
    mov edx, 2047
    sub edx, ecx
    imul eax, edx
    sar eax, 11
    add eax, 128
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 2048
    jb .g0_l
    jmp .g_done

.g1:
    mov r13d, 0xACE12345
    xor ebx, ebx
    xor ecx, ecx
.g1_l:
    mov eax, r13d
    mov edx, eax
    shl eax, 13
    xor eax, edx
    mov edx, eax
    shr eax, 17
    xor eax, edx
    mov edx, eax
    shl eax, 5
    xor eax, edx
    mov r13d, eax
    movsx r8d, al
    add ebx, 0x06000000
    mov eax, ebx
    shr eax, 24
    mov eax, [r15 + rax*4]
    sar eax, 1
    add eax, r8d
    sar eax, 1
    mov edx, 2047
    sub edx, ecx
    imul eax, edx
    sar eax, 11
    add eax, 128
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 2048
    jb .g1_l
    jmp .g_done

.g2:
    mov r13d, 0xBEEF5678
    xor r9d, r9d
    xor ecx, ecx
.g2_l:
    mov eax, r13d
    mov edx, eax
    shl eax, 13
    xor eax, edx
    mov edx, eax
    shr eax, 17
    xor eax, edx
    mov edx, eax
    shl eax, 5
    xor eax, edx
    mov r13d, eax
    movsx eax, al
    mov edx, eax
    sub eax, r9d
    mov r9d, edx
    sar eax, 1
    mov edx, 1023
    sub edx, ecx
    imul eax, edx
    sar eax, 10
    add eax, 128
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 1024
    jb .g2_l
    jmp .g_done

.g3:
    xor ecx, ecx
.g3_l:
    mov eax, ecx
    shl eax, 1
    xor eax, 128
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 128
    jb .g3_l
    jmp .g_done

.g4:
    xor ecx, ecx
.g4_l:
    mov al, 96
    cmp ecx, 32
    jae .g4_w
    mov al, 224
.g4_w:
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 128
    jb .g4_l
    jmp .g_done

.g5:
    xor ecx, ecx
.g5_l:
    mov eax, ecx
    shl eax, 1
    xor eax, 128
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 128
    jb .g5_l
    jmp .g_done

.g6:
    xor ecx, ecx
.g6_l:
    mov eax, ecx
    cmp eax, 64
    jb .g6_up
    mov edx, 127
    sub edx, eax
    mov eax, edx
.g6_up:
    shl eax, 2
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 128
    jb .g6_l
    jmp .g_done

.g7:
    xor ecx, ecx
.g7_l:
    mov eax, ecx
    shl eax, 1
    mov eax, [r15 + rax*4]
    add eax, 128
    mov [rdi], al
    inc rdi
    inc ecx
    cmp ecx, 128
    jb .g7_l

.g_done:
    pop r15
    pop r13
    pop rbx
    ret

; ============================================================================
; .669-Binaerdatei in fileBuf zusammenbauen -> Rueckgabe: EAX = Gesamtlaenge
; ============================================================================
build_669_buffer:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x28

    lea rdi, [fileBuf]
    xor eax, eax
    mov ecx, 497
    rep stosb

    lea r12, [fileBuf]
    mov word [r12 + 0], 0x6669          ; Signatur 'if'

    lea rsi, [titleBox]
    lea rdi, [r12 + 2]
    mov ecx, 108
    rep movsb

    mov byte [r12 + 110], 8             ; NOS = 8 Samples
    mov byte [r12 + 111], MAX_PATS      ; NOP = 16 Patterns
    mov byte [r12 + 112], 0

    lea rdi, [r12 + 113]
    mov al, 0xFF
    mov ecx, 128
    rep stosb

    lea rsi, [orderList]
    lea rdi, [r12 + 113]
    mov ecx, [orderLen]
    cmp ecx, 16
    jbe .ol_ok
    mov ecx, 16
.ol_ok:
    rep movsb

    lea rdi, [r12 + 241]
    mov al, [curTempo]
    mov ecx, 128
    rep stosb

    lea rdi, [r12 + 369]
    mov al, PAT_ROWS - 1
    mov ecx, 128
    rep stosb

    ; 8 Sample-Header schreiben (echte WAV-Groessen falls geladen, sonst Synth-Standard)
    lea rdi, [r12 + 497]
    xor ebx, ebx
.sh_loop:
    mov eax, ebx
    imul eax, 12
    lea rsi, [sampNames]
    add rsi, rax
    mov ecx, 12
    rep movsb
    mov byte [rdi], 0
    inc rdi

    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .sh_def
    lea rdx, [sampLenTbl]
    mov eax, [rdx + rbx*4]
    mov [rdi + 0], eax
    lea rdx, [sampLoopStartTbl]
    mov eax, [rdx + rbx*4]
    mov [rdi + 4], eax
    lea rdx, [sampLoopEndTbl]
    mov eax, [rdx + rbx*4]
    mov [rdi + 8], eax
    jmp .sh_next
.sh_def:
    lea rdx, [saveSampLen]
    mov eax, [rdx + rbx*4]
    mov [rdi + 0], eax
    lea rdx, [saveSampStart]
    mov eax, [rdx + rbx*4]
    mov [rdi + 4], eax
    lea rdx, [saveSampEnd]
    mov eax, [rdx + rbx*4]
    mov [rdi + 8], eax
.sh_next:
    add rdi, 12
    inc ebx
    cmp ebx, 8
    jb .sh_loop

    ; 16 Patterns schreiben
    lea rsi, [patterns]
    xor ebx, ebx
.sp_pat:
    mov r13d, PAT_ROWS * NUM_CH
.sp_cell:
    movzx eax, byte [rsi + 0]
    movzx ecx, byte [rsi + 1]
    movzx edx, byte [rsi + 2]
    mov r8b, [rsi + 3]

    cmp al, 0xFF
    je .sp_nonote
    cmp al, 59
    jbe .sp_n_ok
    mov al, 59
.sp_n_ok:
    cmp cl, 0xFF
    jne .sp_i_ok
    xor ecx, ecx
.sp_i_ok:
    cmp dl, 0xFF
    jne .sp_v_ok
    mov dl, 15
.sp_v_ok:
    shl al, 2
    mov r9b, cl
    shr r9b, 4
    and r9b, 3
    or al, r9b
    mov [rdi + 0], al

    and cl, 0x0F
    shl cl, 4
    and dl, 0x0F
    or cl, dl
    mov [rdi + 1], cl
    jmp .sp_fx

.sp_nonote:
    cmp dl, 0xFF
    je .sp_empty
    mov byte [rdi + 0], 0xFE
    and dl, 0x0F
    mov [rdi + 1], dl
    jmp .sp_fx

.sp_empty:
    mov byte [rdi + 0], 0xFF
    mov byte [rdi + 1], 0x00

.sp_fx:
    mov [rdi + 2], r8b
    add rsi, 4
    add rdi, 3
    dec r13d
    jnz .sp_cell

    mov r13d, 32 * NUM_CH
.sp_pad:
    mov byte [rdi + 0], 0xFF
    mov byte [rdi + 1], 0x00
    mov byte [rdi + 2], 0xFF
    add rdi, 3
    dec r13d
    jnz .sp_pad

    inc ebx
    cmp ebx, MAX_PATS
    jb .sp_pat

    ; 8 Sample-Datenbloecke anhaengen (geladene WAVs direkt, sonst Synth-Export)
    xor ebx, ebx
.sm_out_loop:
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .sm_gen_def

    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rbx*4]
    mov eax, ebx
    shl eax, 17
    lea rsi, [pcmSlots]
    add rsi, rax
    rep movsb
    jmp .sm_out_next

.sm_gen_def:
    mov ecx, ebx
    call gen_synth_export_sample

.sm_out_next:
    inc ebx
    cmp ebx, 8
    jb .sm_out_loop

    sub rdi, r12
    mov eax, edi

    add rsp, 0x28
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; ============================================================================
; .669-Binaerdatei aus fileBuf parsen (inkl. echter PCM-Samples der Slots 0..7)
; ============================================================================
parse_669_buffer:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x28

    cmp ecx, 497
    jb .fail
    mov r12d, ecx
    lea rbp, [fileBuf]

    mov ax, [rbp + 0]
    cmp ax, 0x6669
    je .magic_ok
    cmp ax, 0x4E4A
    jne .fail
.magic_ok:

    lea rsi, [rbp + 2]
    lea rdi, [titleBox]
    mov ecx, 108
.msg_cp:
    mov al, [rsi]
    cmp al, 32
    jb .msg_sp
    cmp al, 126
    jbe .msg_st
.msg_sp:
    mov al, ' '
.msg_st:
    mov [rdi], al
    inc rsi
    inc rdi
    dec ecx
    jnz .msg_cp

    movzx r13d, byte [rbp + 110]        ; NOS
    movzx r14d, byte [rbp + 111]        ; NOP
    test r14d, r14d
    jz .fail
    mov [origNop], r14d

    lea rdi, [orderList]
    xor eax, eax
    mov ecx, 16
    rep stosb

    xor ebx, ebx
.ol_rd:
    movzx eax, byte [rbp + 113 + rbx]
    cmp al, 0xFF
    je .ol_end
    cmp al, 0xFE
    je .ol_end
    cmp eax, MAX_PATS
    jb .ol_in
    xor eax, eax
.ol_in:
    lea rdx, [orderList]
    mov [rdx + rbx], al
    inc ebx
    cmp ebx, 16
    jb .ol_rd
.ol_end:
    test ebx, ebx
    jnz .ol_has
    mov ebx, 1
.ol_has:
    mov [orderLen], ebx

    movzx eax, byte [orderList + 0]
    movzx eax, byte [rbp + 241 + rax]
    cmp eax, 1
    jb .tmp_def
    cmp eax, 15
    jbe .tmp_ok
.tmp_def:
    mov eax, 5
.tmp_ok:
    mov [curTempo], eax

    ; Sample-Tabelle und PCM-Flags zuruecksetzen
    lea rdi, [sampHasPcm]
    xor eax, eax
    mov ecx, NUM_CH
    rep stosb

    lea rdi, [sampNames]
    mov al, ' '
    mov ecx, 12 * 12
    rep stosb

    xor ebx, ebx
.sm_clr:
    lea rdx, [sampLenTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopEndTbl]
    mov dword [rdx + rbx*4], 0xFFFFF
    inc ebx
    cmp ebx, 12
    jb .sm_clr

    mov r15d, 497
    xor ebx, ebx
.sh_rd:
    cmp ebx, r13d
    jae .sh_done
    lea eax, [r15 + 25]
    cmp eax, r12d
    ja .fail

    cmp ebx, 12
    jae .sh_skip

    mov eax, ebx
    imul eax, 12
    lea rdi, [sampNames]
    add rdi, rax
    lea rsi, [rbp + r15]
    xor ecx, ecx
.sn_cp:
    mov al, [rsi + rcx]
    test al, al
    jz .sn_done
    cmp al, 32
    jb .sn_sp
    cmp al, 126
    jbe .sn_st
.sn_sp:
    mov al, ' '
.sn_st:
    mov [rdi + rcx], al
    inc ecx
    cmp ecx, 12
    jb .sn_cp
.sn_done:

    mov eax, [rbp + r15 + 13]
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], eax
    mov eax, [rbp + r15 + 17]
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rbx*4], eax
    mov eax, [rbp + r15 + 21]
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rbx*4], eax

.sh_skip:
    add r15d, 25
    inc ebx
    jmp .sh_rd
.sh_done:

    lea rdi, [patterns]
    mov al, 0xFF
    mov ecx, MAX_PATS * PAT_BYTES
    rep stosb

    cmp r14d, MAX_PATS
    jbe .np_ok
    mov r14d, MAX_PATS
.np_ok:

    xor ebx, ebx
.pat_rd:
    cmp ebx, r14d
    jae .pat_done
    lea eax, [r15 + 1536]
    cmp eax, r12d
    ja .pat_done

    mov eax, ebx
    shl eax, 10
    lea rdi, [patterns]
    add rdi, rax

    mov ecx, PAT_ROWS * NUM_CH
.cell_rd:
    movzx eax, byte [rbp + r15 + 0]
    movzx edx, byte [rbp + r15 + 1]
    movzx r8d, byte [rbp + r15 + 2]
    add r15d, 3

    cmp al, 0xFE
    je .rd_vol
    cmp al, 0xFF
    je .rd_fx

    mov r9d, eax
    shr r9d, 2
    cmp r9d, 59
    jbe .rd_n_ok
    mov r9d, 59
.rd_n_ok:
    mov [rdi + 0], r9b

    mov r9d, eax
    and r9d, 3
    shl r9d, 4
    mov r10d, edx
    shr r10d, 4
    or r9d, r10d
    and r9d, 7
    mov [rdi + 1], r9b

    mov r9d, edx
    and r9d, 0x0F
    mov [rdi + 2], r9b
    jmp .rd_fx

.rd_vol:
    mov r9d, edx
    and r9d, 0x0F
    mov [rdi + 2], r9b

.rd_fx:
    cmp r8b, 0xFF
    je .rd_next
    mov r9d, r8d
    shr r9d, 4
    cmp r9d, 5
    ja .rd_next
    mov [rdi + 3], r8b

.rd_next:
    add rdi, 4
    dec ecx
    jnz .cell_rd

    add r15d, 768
    inc ebx
    jmp .pat_rd
.pat_done:

    ; Ueberschuessige Patterns > 16 ueberspringen, um die PCM-Daten exakt zu treffen
    mov eax, [origNop]
    cmp eax, MAX_PATS
    jbe .pcm_rd_start
    sub eax, MAX_PATS
    imul eax, 1536
    add r15d, eax

.pcm_rd_start:
    ; Echte 8-Bit-PCM-Samples der ersten 8 Slots einlesen (669-Basisrate 8363 Hz bei C-2)
    xor ebx, ebx
.pcm_rd_loop:
    cmp ebx, r13d
    jae .pcm_rd_done
    cmp ebx, NUM_CH
    jae .pcm_rd_done
    cmp r15d, r12d
    jae .pcm_rd_done

    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rbx*4]              ; Original-Sample-Laenge in der Datei
    test ecx, ecx
    jz .pcm_rd_next

    mov eax, r12d
    sub eax, r15d                       ; Verbleibende Bytes in fileBuf
    mov edx, ecx
    cmp edx, eax
    jbe .pr_av_ok
    mov edx, eax
.pr_av_ok:
    cmp edx, MAX_SAMP_BYTES
    jbe .pr_cap_ok
    mov edx, MAX_SAMP_BYTES
.pr_cap_ok:
    test edx, edx
    jz .pcm_rd_adv

    push rcx
    lea rsi, [rbp + r15]
    mov eax, ebx
    shl eax, 17
    lea rdi, [pcmSlots]
    add rdi, rax
    mov ecx, edx
    rep movsb
    pop rcx

    lea rax, [sampHasPcm]
    mov byte [rax + rbx], 1
    lea rax, [sampLenTbl]
    mov [rax + rbx*4], edx
    lea rax, [sampC2Step]
    mov dword [rax + rbx*4], 12428      ; (8363 << 16) / 44100 fuer 669-Standardstimmung

.pcm_rd_adv:
    add r15d, ecx
.pcm_rd_next:
    inc ebx
    jmp .pcm_rd_loop
.pcm_rd_done:

    movzx eax, byte [orderList + 0]
    mov [curPat], eax
    mov [playPat], eax
    mov dword [playOrder], 0
    mov dword [editRow], 0
    mov dword [playRow], 0
    mov dword [curCh], 0
    mov dword [curSubCol], 0
    mov eax, 1
    jmp .ret_out

.fail:
    xor eax, eax
.ret_out:
    add rsp, 0x28
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; ============================================================================
; Windows-Dialog: .669-Modul speichern (Strg+S / F12)
; ============================================================================
save_669_dialog:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x40

    call silence_audio

    cmp byte [filePath], 0
    jne .s_has_name
    lea rsi, [defSaveName]
    lea rdi, [filePath]
    mov ecx, 9
    rep movsb
.s_has_name:

    lea rcx, [filePath]
    lea rdx, [flt669]
    lea r8, [defExt]
    call init_ofn
    lea rax, [dlgSaveTitle]
    mov [ofn + 0x58], rax
    mov dword [ofn + 0x60], OFN_SAVE_FLAGS

    lea rcx, [ofn]
    call GetSaveFileNameA
    test eax, eax
    jz .s_done

    call build_669_buffer
    mov ebx, eax

    lea rcx, [filePath]
    mov edx, GENERIC_WRITE
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], CREATE_ALWAYS
    mov qword [rsp + 0x28], FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 0x30], 0
    call CreateFileA
    cmp rax, -1
    je .s_err
    mov rsi, rax

    mov rcx, rsi
    lea rdx, [fileBuf]
    mov r8d, ebx
    lea r9, [bytesIO]
    mov qword [rsp + 0x20], 0
    call WriteFile

    mov rcx, rsi
    call CloseHandle

    lea rcx, [filePath]
    lea rdx, [topBar + 39]
    mov r8d, 16
    call copy_basename
    lea rcx, [stSaved]
    call set_status
    mov dword [dirty], 1
    jmp .s_done

.s_err:
    lea rcx, [stError]
    call set_status
    mov dword [dirty], 1

.s_done:
    add rsp, 0x40
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; Windows-Dialog: .669-Modul laden (Strg+O / F11)
; ============================================================================
load_669_dialog:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x40

    call silence_audio

    mov byte [filePath], 0
    lea rcx, [filePath]
    lea rdx, [flt669]
    lea r8, [defExt]
    call init_ofn
    lea rax, [dlgOpenTitle]
    mov [ofn + 0x58], rax
    mov dword [ofn + 0x60], OFN_OPEN_FLAGS

    lea rcx, [ofn]
    call GetOpenFileNameA
    test eax, eax
    jz .l_done

    lea rcx, [filePath]
    mov edx, GENERIC_READ
    mov r8d, FILE_SHARE_READ
    xor r9d, r9d
    mov qword [rsp + 0x20], OPEN_EXISTING
    mov qword [rsp + 0x28], FILE_ATTRIBUTE_NORMAL
    mov qword [rsp + 0x30], 0
    call CreateFileA
    cmp rax, -1
    je .l_err
    mov rsi, rax

    mov dword [bytesIO], 0
    mov rcx, rsi
    lea rdx, [fileBuf]
    mov r8d, FILE_BUF_SIZE
    lea r9, [bytesIO]
    mov qword [rsp + 0x20], 0
    call ReadFile

    mov rcx, rsi
    call CloseHandle

    mov ecx, [bytesIO]
    call parse_669_buffer
    test eax, eax
    jz .l_bad

    lea rcx, [filePath]
    lea rdx, [topBar + 39]
    mov r8d, 16
    call copy_basename
    lea rcx, [stLoaded]
    call set_status
    mov dword [dirty], 1
    jmp .l_done

.l_bad:
    lea rcx, [stBad669]
    call set_status
    mov dword [dirty], 1
    jmp .l_done

.l_err:
    lea rcx, [stError]
    call set_status
    mov dword [dirty], 1

.l_done:
    add rsp, 0x40
    pop rdi
    pop rsi
    pop rbx
    ret

; Rechteck fuellen: ECX=x, EDX=y, R8D=w, R9D=h, R10D=COLORREF
fill_rect:
    sub rsp, 0x48
    mov [tmpRect + 0], ecx
    mov [tmpRect + 4], edx
    add ecx, r8d
    add edx, r9d
    mov [tmpRect + 8], ecx
    mov [tmpRect + 12], edx

    mov rcx, [memDC]
    mov edx, r10d
    call SetBkColor

    mov rcx, [memDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, ETO_OPAQUE
    lea rax, [tmpRect]
    mov [rsp + 0x20], rax
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    call ExtTextOutA
    add rsp, 0x48
    ret

; Text im 12x12-Raster zeichnen: ECX=Spalte, EDX=Zeile, R8=Ptr, R9D=Len, R10D=Farbe
draw_text:
    sub rsp, 0x68
    imul ecx, CHAR_W
    imul edx, CHAR_H
    mov [rsp + 0x40], ecx
    mov [rsp + 0x44], edx
    mov [rsp + 0x48], r8
    mov [rsp + 0x50], r9d

    mov rcx, [memDC]
    mov edx, r10d
    call SetTextColor

    mov rcx, [memDC]
    mov edx, [rsp + 0x40]
    mov r8d, [rsp + 0x44]
    xor r9d, r9d
    mov qword [rsp + 0x20], 0
    mov rax, [rsp + 0x48]
    mov [rsp + 0x28], rax
    mov eax, [rsp + 0x50]
    mov [rsp + 0x30], rax
    lea rax, [dxTbl]
    mov [rsp + 0x38], rax
    call ExtTextOutA
    add rsp, 0x68
    ret

; ============================================================================
; Echtzeit-Kanal-Trigger: ECX=Kanal (0..7), RDX=CellPtr, R8D=forcePreview
; ============================================================================
trigger_channel:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    mov ebx, ecx
    mov rsi, rdx

    test r8d, r8d
    jnz .not_muted
    lea rax, [chMuted]
    cmp byte [rax + rbx], 0
    jne .done
.not_muted:

    mov eax, ebx
    shl eax, 6
    lea rdi, [voices]
    add rdi, rax

    ; Tempo-Effekt (f = 5) ausfuehren
    movzx eax, byte [rsi + 3]
    cmp al, 0xFF
    je .no_fx_load
    mov ecx, eax
    shr ecx, 4
    and eax, 0x0F
    cmp ecx, 5
    jne .store_fx
    test eax, eax
    jz .store_fx
    mov [curTempo], eax
    jmp .no_fx_load
.store_fx:
    mov [rdi + 40], ecx
    mov [rdi + 44], eax
    jmp .chk_vol
.no_fx_load:
    cmp byte [rsi + 0], 0xFF
    je .chk_vol
    mov dword [rdi + 40], -1
    mov dword [rdi + 44], 0

.chk_vol:
    movzx eax, byte [rsi + 2]
    cmp al, 0xFF
    je .chk_note
    and eax, 15
    mov [rdi + 24], eax
    cmp byte [rsi + 0], 0xFF
    jne .chk_note
    add eax, eax
    cmp eax, [rdi + 48]
    jle .chk_note
    mov [rdi + 48], eax

.chk_note:
    movzx eax, byte [rsi + 0]
    cmp al, 0xFF
    je .done

    movzx ecx, byte [rsi + 1]
    cmp cl, 0xFF
    je .keep_inst
    and ecx, 7
    mov [rdi + 4], ecx
.keep_inst:
    mov ecx, [rdi + 4]

    ; Pruefen, ob fuer diesen Slot ein PCM/WAV-Sample geladen ist
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rcx], 0
    je .trig_synth
    lea rdx, [sampLenTbl]
    cmp dword [rdx + rcx*4], 0
    je .trig_synth

    ; PCM-Stimme initialisieren
    call pcm_note_to_step
    mov [rdi + 12], eax
    mov [rdi + 16], eax
    mov dword [rdi + 52], 1             ; isPcm = 1
    mov qword [rdi + 56], 0             ; pcmPos = 0.0
    mov dword [rdi + 0], 1
    mov dword [rdi + 20], 30000
    jmp .set_vu

.trig_synth:
    mov dword [rdi + 52], 0             ; isPcm = 0
    push rax
    call note_to_inc
    mov [rdi + 12], eax
    mov [rdi + 16], eax
    mov edx, eax
    shr edx, 1
    add eax, edx
    mov [rdi + 36], eax
    pop rax

    mov dword [rdi + 0], 1
    mov dword [rdi + 8], 0
    mov dword [rdi + 32], 0
    mov dword [rdi + 20], 30000

    cmp dword [rdi + 4], 0
    jne .set_vu
    mov dword [rdi + 12], 16800000
    mov dword [rdi + 16], 16800000
    mov dword [rdi + 20], 32000

.set_vu:
    mov eax, [rdi + 24]
    shl eax, 1
    cmp eax, 8
    jae .m_ok
    mov eax, 8
.m_ok:
    mov [rdi + 48], eax

.done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; Echtzeit-Audio-Block rendern: RCX=dstPtr, EDX=numFrames
; ============================================================================
render_audio_block:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x28

    mov r12, rcx
    mov r13d, edx
    lea r15, [sinTbl]

.frame_loop:
    cmp dword [isPlaying], 0
    je .seq_done

    dec dword [tickSamples]
    jg .seq_done

    mov eax, [curTempo]
    imul eax, TICKS_PER_TEMPO
    mov [tickSamples], eax

    xor ebx, ebx
.trig_loop:
    mov ecx, [playPat]
    mov edx, [playRow]
    mov r8d, ebx
    call get_cell_ptr
    mov ecx, ebx
    mov rdx, rax
    xor r8d, r8d
    call trigger_channel
    inc ebx
    cmp ebx, NUM_CH
    jb .trig_loop

    mov eax, [playRow]
    inc eax
    cmp eax, PAT_ROWS
    jb .row_ok
    xor eax, eax
    cmp dword [songMode], 0
    je .pat_wrap_done
    mov ecx, [playOrder]
    inc ecx
    cmp ecx, [orderLen]
    jb .ord_ok
    xor ecx, ecx
.ord_ok:
    mov [playOrder], ecx
    lea rdx, [orderList]
    movzx edx, byte [rdx + rcx]
    mov [playPat], edx
    mov [curPat], edx
.pat_wrap_done:
.row_ok:
    mov [playRow], eax
    mov dword [dirty], 1
.seq_done:

    mov eax, [lfsr]
    mov edx, eax
    shl eax, 13
    xor eax, edx
    mov edx, eax
    shr eax, 17
    xor eax, edx
    mov edx, eax
    shl eax, 5
    xor eax, edx
    mov [lfsr], eax
    movsx r14d, al

    add dword [vibPhase], 550000

    inc dword [fxDivCnt]
    mov ebp, [fxDivCnt]
    and ebp, 63

    xor r8d, r8d
    xor r9d, r9d
    lea rdi, [voices]
    xor ebx, ebx

.ch_mix_loop:
    mov eax, [fxDivCnt]
    and eax, 511
    jnz .vu_no_dec
    cmp dword [rdi + 48], 0
    je .vu_no_dec
    dec dword [rdi + 48]
.vu_no_dec:

    cmp dword [rdi + 0], 0
    je .ch_next
    mov ecx, [rdi + 20]
    cmp ecx, 32
    jbe .ch_off

    ; Effekte alle 64 Samples anwenden (getrennt fuer Synth und PCM-Schrittweite)
    test ebp, ebp
    jnz .fx_done
    cmp dword [rdi + 52], 0
    jne .fx_pcm

    mov eax, [rdi + 40]
    cmp eax, 0
    jne .fx_chk_b
    mov edx, [rdi + 44]
    imul edx, 35000
    add [rdi + 12], edx
    jmp .fx_done
.fx_chk_b:
    cmp eax, 1
    jne .fx_chk_e
    mov edx, [rdi + 44]
    imul edx, 35000
    mov eax, [rdi + 12]
    sub eax, edx
    cmp eax, 1000000
    jge .b_ok
    mov eax, 1000000
.b_ok:
    mov [rdi + 12], eax
    jmp .fx_done
.fx_chk_e:
    cmp eax, 4
    jne .fx_done
    mov eax, [vibPhase]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul eax, [rdi + 44]
    imul eax, 400
    add eax, [rdi + 16]
    mov [rdi + 12], eax
    jmp .fx_done

.fx_pcm:
    mov eax, [rdi + 40]
    cmp eax, 0
    jne .fxp_b
    mov edx, [rdi + 44]
    shl edx, 8
    add [rdi + 12], edx
    jmp .fx_done
.fxp_b:
    cmp eax, 1
    jne .fxp_e
    mov edx, [rdi + 44]
    shl edx, 8
    mov eax, [rdi + 12]
    sub eax, edx
    cmp eax, 1024
    jge .fxp_b_ok
    mov eax, 1024
.fxp_b_ok:
    mov [rdi + 12], eax
    jmp .fx_done
.fxp_e:
    cmp eax, 4
    jne .fx_done
    mov eax, [vibPhase]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul eax, [rdi + 44]
    imul eax, 12
    add eax, [rdi + 16]
    cmp eax, 1024
    jge .fxp_e_ok
    mov eax, 1024
.fxp_e_ok:
    mov [rdi + 12], eax

.fx_done:
    ; Verzweigung: PCM/WAV-Wiedergabe oder eingebauter 8-Kanal-Synthesizer
    cmp dword [rdi + 52], 0
    jne .play_pcm

    mov edx, [rdi + 4]
    cmp edx, 0
    je .inst_kick
    cmp edx, 1
    je .inst_snare
    cmp edx, 2
    je .inst_hat
    cmp edx, 3
    je .inst_bass
    cmp edx, 4
    je .inst_arp
    cmp edx, 5
    je .inst_lead
    cmp edx, 6
    je .inst_pluck
    jmp .inst_pad

.play_pcm:
    mov edx, [rdi + 4]
    and edx, 7
    lea rsi, [sampLenTbl]
    mov r10d, [rsi + rdx*4]             ; Sample-Laenge
    test r10d, r10d
    jz .ch_off

    mov r11, [rdi + 56]                 ; 48.16 Position
    mov rax, r11
    shr rax, 16                         ; Ganzzahliger Sample-Index
    cmp eax, r10d
    jae .ch_off

    mov ecx, edx
    shl ecx, 17                         ; Slot * 131072
    lea rsi, [pcmSlots]
    add rsi, rcx
    movzx eax, byte [rsi + rax]         ; 8-Bit Unsigned PCM (0..255)
    sub eax, 128
    shl eax, 6                          ; Pegelanpassung fuer Mixer

    mov ecx, [rdi + 12]                 ; 16.16 Schrittweite
    add r11, rcx

    push rax
    mov rax, r11
    shr rax, 16                         ; Neuer Sample-Index

    lea rsi, [sampLoopEndTbl]
    mov ecx, [rsi + rdx*4]
    cmp ecx, r10d
    ja .pcm_noloop
    lea rsi, [sampLoopStartTbl]
    mov esi, [rsi + rdx*4]
    cmp ecx, esi
    jbe .pcm_noloop

    ; Loop aktiv: bei Erreichen von loopEnd auf loopStart zurueckwickeln
    cmp eax, ecx
    jb .pcm_store_pos
    sub ecx, esi
    shl rcx, 16
    sub r11, rcx
    mov rax, r11
    shr rax, 16
    lea r10, [sampLoopEndTbl]
    cmp eax, [r10 + rdx*4]
    jb .pcm_store_pos
    mov r11d, esi
    shl r11, 16
    jmp .pcm_store_pos

.pcm_noloop:
    cmp eax, r10d
    jb .pcm_store_pos
    mov dword [rdi + 0], 0
.pcm_store_pos:
    mov [rdi + 56], r11
    pop rax
    jmp .mix_vol_pan

.inst_kick:
    mov eax, [rdi + 12]
    add [rdi + 8], eax
    cmp eax, 4600000
    jbe .k_flr
    mov edx, eax
    shr edx, 10
    sub eax, edx
    mov [rdi + 12], eax
.k_flr:
    mov eax, [rdi + 8]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul eax, ecx
    sar eax, 9
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_snare:
    add dword [rdi + 8], 19500000
    mov eax, [rdi + 8]
    shr eax, 24
    mov eax, [r15 + rax*4]
    sar eax, 1
    add eax, r14d
    imul eax, ecx
    sar eax, 10
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_hat:
    mov eax, r14d
    sub eax, [prevNoise]
    imul eax, ecx
    sar eax, 10
    mov edx, ecx
    shr edx, 9
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_bass:
    mov eax, [rdi + 12]
    add [rdi + 8], eax
    mov eax, [rdi + 8]
    sar eax, 24
    imul eax, ecx
    sar eax, 9
    mov edx, [rdi + 28]
    sub eax, edx
    sar eax, 3
    add edx, eax
    mov [rdi + 28], edx
    mov eax, edx
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_arp:
    mov eax, [rdi + 12]
    add [rdi + 8], eax
    cmp dword [rdi + 8], 0x40000000
    jb .a_hi
    mov eax, -36
    jmp .a_go
.a_hi:
    mov eax, 108
.a_go:
    imul eax, ecx
    sar eax, 10
    mov edx, ecx
    shr edx, 10
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_lead:
    mov eax, [rdi + 12]
    add [rdi + 8], eax
    mov eax, [rdi + 8]
    sar eax, 24
    imul eax, ecx
    sar eax, 10
    mov edx, ecx
    shr edx, 12
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_pluck:
    mov eax, [rdi + 12]
    add [rdi + 8], eax
    mov eax, [rdi + 8]
    sar eax, 24
    imul eax, ecx
    sar eax, 10
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [rdi + 20], ecx
    jmp .mix_vol_pan

.inst_pad:
    mov eax, [rdi + 12]
    add [rdi + 8], eax
    mov eax, [rdi + 8]
    shr eax, 24
    mov esi, [r15 + rax*4]
    mov eax, [rdi + 36]
    add [rdi + 32], eax
    mov eax, [rdi + 32]
    shr eax, 24
    add esi, [r15 + rax*4]
    mov eax, esi
    imul eax, ecx
    sar eax, 11
    mov edx, ecx
    shr edx, 14
    sub ecx, edx
    mov [rdi + 20], ecx

.mix_vol_pan:
    imul eax, [rdi + 24]
    sar eax, 4

    cmp dword [rdi + 4], 0
    je .pan_center
    test ebx, 1
    jnz .pan_right
    add r8d, eax
    sar eax, 1
    add r9d, eax
    jmp .ch_next
.pan_right:
    add r9d, eax
    sar eax, 1
    add r8d, eax
    jmp .ch_next
.pan_center:
    add r8d, eax
    add r9d, eax
    jmp .ch_next

.ch_off:
    mov dword [rdi + 0], 0
.ch_next:
    add rdi, VOICE_SIZE
    inc ebx
    cmp ebx, NUM_CH
    jb .ch_mix_loop

    mov [prevNoise], r14d

    mov eax, r8d
    call clamp_s16
    mov [r12], ax
    mov eax, r9d
    call clamp_s16
    mov [r12 + 2], ax

    add r12, 4
    dec r13d
    jnz .frame_loop

    add rsp, 0x28
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; ============================================================================
; Audio-Streaming-Puffer nachfuellen (non-blocking)
; ============================================================================
pump_audio:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x30

    xor ebx, ebx
.p_loop:
    mov eax, ebx
    imul eax, 48
    lea rsi, [waveHdrs]
    add rsi, rax

    test dword [rsi + 24], WHDR_DONE
    jz .p_next

    mov rcx, [rsi + 0]
    mov edx, BUF_FRAMES
    call render_audio_block

    mov rcx, [hWaveOut]
    mov rdx, rsi
    mov r8d, 48
    call waveOutWrite

.p_next:
    inc ebx
    cmp ebx, BUF_COUNT
    jb .p_loop

    add rsp, 0x30
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; Tastatur-Handler: ECX=wParam (VK), EDX=lParam
; ============================================================================
handle_keydown:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    mov ebx, ecx
    mov esi, edx

    ; Strg-Kombinationen: O=669 laden, S=669 speichern, W=WAV laden, L=Loop, U=Synth
    mov ecx, VK_CONTROL
    call GetKeyState
    test ax, 0x8000
    jz .no_ctrl
    cmp ebx, 'O'
    je .do_load
    cmp ebx, 'S'
    je .do_save
    cmp ebx, 'W'
    je .do_wav
    cmp ebx, 'L'
    je .do_loop
    cmp ebx, 'U'
    je .do_unload
    jmp .k_done

.no_ctrl:
    cmp ebx, VK_F11
    je .do_load
    cmp ebx, VK_F12
    je .do_save
    cmp ebx, VK_INSERT
    je .do_wav
    jmp .chk_esc

.do_load:
    test esi, 0x40000000
    jnz .k_done
    call load_669_dialog
    jmp .k_done

.do_save:
    test esi, 0x40000000
    jnz .k_done
    call save_669_dialog
    jmp .k_done

.do_wav:
    test esi, 0x40000000
    jnz .k_done
    call load_wav_dialog
    jmp .k_done

.do_loop:
    test esi, 0x40000000
    jnz .k_done
    call toggle_wav_loop
    jmp .k_done

.do_unload:
    test esi, 0x40000000
    jnz .k_done
    call unload_wav_slot
    jmp .k_done

.chk_esc:
    cmp ebx, VK_ESCAPE
    jne .chk_space
    mov dword [running], 0
    jmp .k_done

.chk_space:
    cmp ebx, VK_SPACE
    jne .chk_f5
    test esi, 0x40000000
    jnz .k_done
    xor dword [isPlaying], 1
    cmp dword [isPlaying], 0
    je .k_done
    mov eax, [curPat]
    mov [playPat], eax
    mov eax, [editRow]
    mov [playRow], eax
    mov dword [tickSamples], 1
    jmp .k_done

.chk_f5:
    cmp ebx, VK_F5
    jne .chk_f6
    mov dword [isPlaying], 1
    mov eax, [curPat]
    mov [playPat], eax
    mov dword [playRow], 0
    mov dword [playOrder], 0
    mov dword [tickSamples], 1
    jmp .k_done

.chk_f6:
    cmp ebx, VK_F6
    jne .chk_f1
    xor dword [songMode], 1
    jmp .k_done

.chk_f1:
    cmp ebx, VK_F1
    jne .chk_f2
    cmp dword [curOct], 0
    je .k_done
    dec dword [curOct]
    jmp .k_done

.chk_f2:
    cmp ebx, VK_F2
    jne .chk_f3
    cmp dword [curOct], 3
    jae .k_done
    inc dword [curOct]
    jmp .k_done

.chk_f3:
    cmp ebx, VK_F3
    jne .chk_f4
    mov eax, [curInst]
    dec eax
    and eax, 7
    mov [curInst], eax
    jmp .k_done

.chk_f4:
    cmp ebx, VK_F4
    jne .chk_f7
    mov eax, [curInst]
    inc eax
    and eax, 7
    mov [curInst], eax
    jmp .k_done

.chk_f7:
    cmp ebx, VK_F7
    jne .chk_f8
    cmp dword [curTempo], 1
    jle .k_done
    dec dword [curTempo]
    jmp .k_done

.chk_f8:
    cmp ebx, VK_F8
    jne .chk_f9
    cmp dword [curTempo], 15
    jae .k_done
    inc dword [curTempo]
    jmp .k_done

.chk_f9:
    cmp ebx, VK_F9
    jne .chk_f10
    mov eax, [editStep]
    cmp eax, 1
    je .st2
    cmp eax, 2
    je .st4
    cmp eax, 4
    je .st0
    mov dword [editStep], 1
    jmp .k_done
.st2:
    mov dword [editStep], 2
    jmp .k_done
.st4:
    mov dword [editStep], 4
    jmp .k_done
.st0:
    mov dword [editStep], 0
    jmp .k_done

.chk_f10:
    cmp ebx, VK_F10
    jne .chk_up
    mov eax, [curCh]
    lea rdx, [chMuted]
    xor byte [rdx + rax], 1
    jmp .k_done

.chk_up:
    cmp ebx, VK_UP
    jne .chk_down
    mov eax, [editRow]
    dec eax
    and eax, 31
    mov [editRow], eax
    jmp .k_done

.chk_down:
    cmp ebx, VK_DOWN
    jne .chk_left
    mov eax, [editRow]
    inc eax
    and eax, 31
    mov [editRow], eax
    jmp .k_done

.chk_left:
    cmp ebx, VK_LEFT
    jne .chk_right
    mov eax, [curSubCol]
    dec eax
    jns .l_ok
    mov eax, 4
    mov ecx, [curCh]
    dec ecx
    and ecx, 7
    mov [curCh], ecx
.l_ok:
    mov [curSubCol], eax
    jmp .k_done

.chk_right:
    cmp ebx, VK_RIGHT
    jne .chk_tab
    mov eax, [curSubCol]
    inc eax
    cmp eax, 5
    jb .r_ok
    xor eax, eax
    mov ecx, [curCh]
    inc ecx
    and ecx, 7
    mov [curCh], ecx
.r_ok:
    mov [curSubCol], eax
    jmp .k_done

.chk_tab:
    cmp ebx, VK_TAB
    jne .chk_pgup
    mov eax, [curCh]
    inc eax
    and eax, 7
    mov [curCh], eax
    mov dword [curSubCol], 0
    jmp .k_done

.chk_pgup:
    cmp ebx, VK_PRIOR
    jne .chk_pgdn
    mov eax, [curPat]
    inc eax
    and eax, 15
    mov [curPat], eax
    jmp .k_done

.chk_pgdn:
    cmp ebx, VK_NEXT
    jne .chk_del
    mov eax, [curPat]
    dec eax
    and eax, 15
    mov [curPat], eax
    jmp .k_done

.chk_del:
    cmp ebx, VK_DELETE
    jne .chk_bs
    mov ecx, [curPat]
    mov edx, [editRow]
    mov r8d, [curCh]
    call get_cell_ptr
    mov ecx, [curSubCol]
    cmp ecx, 0
    je .del_all
    cmp ecx, 2
    je .del_vol
    cmp ecx, 3
    jae .del_fx
    mov byte [rax + 1], 0xFF
    jmp .adv_step
.del_vol:
    mov byte [rax + 2], 0xFF
    jmp .adv_step
.del_fx:
    mov byte [rax + 3], 0xFF
    jmp .adv_step
.del_all:
    mov dword [rax], 0xFFFFFFFF
    jmp .adv_step

.chk_bs:
    cmp ebx, VK_BACK
    jne .chk_edit_input
    mov eax, [editRow]
    dec eax
    and eax, 31
    mov [editRow], eax
    mov ecx, [curPat]
    mov edx, eax
    mov r8d, [curCh]
    call get_cell_ptr
    mov dword [rax], 0xFFFFFFFF
    jmp .k_done

.chk_edit_input:
    mov ecx, [curPat]
    mov edx, [editRow]
    mov r8d, [curCh]
    call get_cell_ptr
    mov rdi, rax

    mov eax, [curSubCol]
    cmp eax, 0
    je .in_note
    cmp eax, 1
    je .in_inst
    cmp eax, 2
    je .in_vol
    cmp eax, 3
    je .in_fxcmd
    jmp .in_fxval

.in_note:
    test esi, 0x40000000
    jnz .k_done
    mov eax, esi
    shr eax, 16
    and eax, 0xFF
    cmp eax, 64
    jae .k_done
    lea rdx, [scanToNote]
    movzx eax, byte [rdx + rax]
    cmp al, 0xFF
    je .k_done

    mov ecx, [curOct]
    imul ecx, 12
    add eax, ecx
    cmp eax, 59
    jle .n_ok
    mov eax, 59
.n_ok:
    mov [rdi + 0], al
    mov eax, [curInst]
    mov [rdi + 1], al
    mov eax, [curVol]
    mov [rdi + 2], al

    mov ecx, [curCh]
    mov rdx, rdi
    mov r8d, 1
    call trigger_channel
    jmp .adv_step

.in_inst:
    call vk_to_hex
    cmp eax, 7
    ja .k_done
    mov [rdi + 1], al
    mov [curInst], eax
    jmp .adv_step

.in_vol:
    call vk_to_hex
    cmp eax, 15
    ja .k_done
    mov [rdi + 2], al
    mov [curVol], eax
    jmp .adv_step

.in_fxcmd:
    cmp ebx, 'A'
    jb .k_done
    cmp ebx, 'F'
    ja .k_done
    mov eax, ebx
    sub eax, 'A'
    shl eax, 4
    movzx ecx, byte [rdi + 3]
    cmp cl, 0xFF
    jne .fx_keep_lo
    or eax, 1
    jmp .fx_store
.fx_keep_lo:
    and ecx, 0x0F
    or eax, ecx
.fx_store:
    mov [rdi + 3], al
    mov dword [curSubCol], 4
    jmp .k_done

.in_fxval:
    call vk_to_hex
    cmp eax, 15
    ja .k_done
    movzx ecx, byte [rdi + 3]
    cmp cl, 0xFF
    je .k_done
    and ecx, 0xF0
    or ecx, eax
    mov [rdi + 3], cl
    jmp .adv_step

.adv_step:
    mov eax, [editRow]
    add eax, [editStep]
    and eax, 31
    mov [editRow], eax

.k_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; Hilfsfunktion: VK in EBX -> Hex-Wert 0..15 in EAX (oder 0xFF)
vk_to_hex:
    cmp ebx, '0'
    jb .vh_no
    cmp ebx, '9'
    jbe .vh_dig
    cmp ebx, 'A'
    jb .vh_num
    cmp ebx, 'F'
    jbe .vh_af
.vh_num:
    cmp ebx, 0x60
    jb .vh_no
    cmp ebx, 0x69
    ja .vh_no
    mov eax, ebx
    sub eax, 0x60
    ret
.vh_dig:
    mov eax, ebx
    sub eax, '0'
    ret
.vh_af:
    mov eax, ebx
    sub eax, 'A' - 10
    ret
.vh_no:
    mov eax, 0xFF
    ret

; ============================================================================
; Fensterprozedur (x64 ABI konform, ohne Tail-Jump auf DLL-Import)
; ============================================================================
WndProc:
    cmp edx, WM_KEYDOWN
    je .on_key
    cmp edx, WM_CLOSE
    je .on_close
    cmp edx, WM_DESTROY
    je .on_close
    sub rsp, 0x28
    call DefWindowProcA
    add rsp, 0x28
    ret

.on_close:
    mov dword [running], 0
    xor eax, eax
    ret

.on_key:
    sub rsp, 0x28
    mov ecx, r8d
    mov edx, r9d
    call handle_keydown
    mov dword [dirty], 1
    add rsp, 0x28
    xor eax, eax
    ret

; ============================================================================
; 4-Byte-Zelle in 8 ASCII-Zeichen nach [cellBuf] formatieren (RSI = CellPtr)
; ============================================================================
format_cell_text:
    mov dword [cellBuf + 0], 0x2E2E2E2E
    mov dword [cellBuf + 4], 0x2E2E2E2E

    movzx eax, byte [rsi + 0]
    cmp al, 0xFF
    je .fc_inst
    xor edx, edx
    mov ecx, 12
    div ecx
    add al, '2'
    lea r8, [noteNames]
    mov cx, [r8 + rdx*2]
    mov [cellBuf + 0], cx
    mov [cellBuf + 2], al

.fc_inst:
    movzx eax, byte [rsi + 1]
    cmp al, 0xFF
    je .fc_vol
    mov byte [cellBuf + 3], '0'
    and al, 7
    add al, '0'
    mov [cellBuf + 4], al

.fc_vol:
    movzx eax, byte [rsi + 2]
    cmp al, 0xFF
    je .fc_fx
    and eax, 15
    lea r8, [hexTbl]
    mov al, [r8 + rax]
    mov [cellBuf + 5], al

.fc_fx:
    movzx eax, byte [rsi + 3]
    cmp al, 0xFF
    je .fc_done
    mov ecx, eax
    shr ecx, 4
    add cl, 'a'
    mov [cellBuf + 6], cl
    and eax, 15
    lea r8, [hexTbl]
    mov al, [r8 + rax]
    mov [cellBuf + 7], al

.fc_done:
    ret

; ============================================================================
; Gesamte Composer-669-Oberflaeche in memDC zeichnen und blitten
; ============================================================================
draw_screen:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x48

    xor ecx, ecx
    xor edx, edx
    mov r8d, WIN_W
    mov r9d, WIN_H
    mov r10d, COL_BG
    call fill_rect

    cmp dword [songMode], 0
    je .m_pat
    mov dword [topBar + 63], 'SONG'
    mov dword [topBar + 67], '-ORD'
    jmp .m_draw
.m_pat:
    mov dword [topBar + 63], 'PAT-'
    mov dword [topBar + 67], 'LOOP'
.m_draw:
    xor ecx, ecx
    xor edx, edx
    lea r8, [topBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    mov ecx, 4 * CHAR_W
    mov edx, 2 * CHAR_H
    mov r8d, 12 * CHAR_W
    mov r9d, 12 * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, 35 * CHAR_W
    mov edx, 2 * CHAR_H
    mov r8d, 36 * CHAR_W
    mov r9d, 3 * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    xor ebx, ebx
.t_loop:
    mov ecx, 35
    lea edx, [rbx + 2]
    mov eax, ebx
    imul eax, 36
    lea r8, [titleBox]
    add r8, rax
    mov r9d, 36
    mov r10d, COL_CYAN
    call draw_text
    inc ebx
    cmp ebx, 3
    jb .t_loop

    xor ebx, ebx
.up_loop:
    lea r12d, [rbx + 2]

    mov al, bl
    lea r8, [lineBuf]
    call fmt_hex2
    mov r10d, COL_TEAL
    cmp ebx, NUM_CH
    jae .si_chk_cur
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .si_chk_cur
    mov r10d, COL_PURPHI
.si_chk_cur:
    cmp ebx, [curInst]
    jne .si_col
    mov r10d, COL_CYAN
.si_col:
    mov ecx, 1
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 2
    call draw_text

    mov eax, ebx
    imul eax, 12
    lea r8, [sampNames]
    add r8, rax
    mov r10d, COL_NOTE
    cmp ebx, NUM_CH
    jae .sn_chk_cur
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .sn_chk_cur
    mov r10d, COL_CYAN
.sn_chk_cur:
    cmp ebx, [curInst]
    jne .sn_col
    mov r10d, COL_WHITE
.sn_col:
    mov ecx, 4
    mov edx, r12d
    mov r9d, 12
    call draw_text

    lea r13, [sampLoopStartTbl]
    mov eax, [r13 + rbx*4]
    lea r8, [lineBuf]
    call fmt_hex5
    mov byte [lineBuf + 5], ' '

    lea r13, [sampLoopEndTbl]
    mov eax, [r13 + rbx*4]
    lea r8, [lineBuf + 6]
    call fmt_hex5
    mov byte [lineBuf + 11], ' '

    lea r13, [sampLenTbl]
    mov eax, [r13 + rbx*4]
    lea r8, [lineBuf + 12]
    call fmt_hex5

    mov ecx, 17
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 17
    mov r10d, COL_NOTE
    call draw_text

    mov al, bl
    lea r8, [lineBuf]
    call fmt_hex2
    mov byte [lineBuf + 2], ' '
    lea r13, [orderList]
    mov al, [r13 + rbx]
    lea r8, [lineBuf + 3]
    call fmt_hex2
    mov byte [lineBuf + 5], ' '
    mov eax, [curTempo]
    lea r13, [hexTbl]
    mov al, [r13 + rax]
    mov [lineBuf + 6], al

    mov r10d, COL_TEAL
    cmp ebx, [playOrder]
    jne .ord_c
    mov r10d, COL_CYAN
.ord_c:
    mov ecx, 72
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 7
    call draw_text

    inc ebx
    cmp ebx, 12
    jb .up_loop

    mov eax, [curVol]
    lea rdx, [hexTbl]
    mov al, [rdx + rax]
    mov [statLines + 0 + 7], al
    mov al, [playOrder]
    lea r8, [statLines + 8 + 6]
    call fmt_hex2
    mov al, [curPat]
    lea r8, [statLines + 16 + 6]
    call fmt_hex2
    mov al, [editRow]
    lea r8, [statLines + 24 + 6]
    call fmt_hex2
    mov al, [editStep]
    lea r8, [statLines + 32 + 6]
    call fmt_hex2
    mov al, [curInst]
    lea r8, [statLines + 40 + 6]
    call fmt_hex2

    xor ebx, ebx
.st_loop:
    mov eax, ebx
    shl eax, 3
    lea r12, [statLines]
    add r12, rax
    lea r13d, [rbx + 7]

    mov ecx, 44
    mov edx, r13d
    mov r8, r12
    mov r9d, 5
    mov r10d, COL_TEAL
    call draw_text

    mov ecx, 49
    mov edx, r13d
    lea r8, [r12 + 5]
    mov r9d, 3
    mov r10d, COL_CYAN
    call draw_text

    inc ebx
    cmp ebx, 6
    jb .st_loop

    xor ebx, ebx
.vu_loop:
    mov eax, ebx
    shl eax, 6
    lea r12, [voices]
    mov r13d, [r12 + rax + 48]

    mov r14d, r13d
    imul r14d, 3
    test r14d, r14d
    jz .vu_no_bar
    lea ecx, [rbx + 35]
    imul ecx, CHAR_W
    mov edx, 14 * CHAR_H
    sub edx, r14d
    mov r8d, CHAR_W - 2
    mov r9d, r14d
    mov r10d, COL_PURP
    call fill_rect
.vu_no_bar:

    mov al, bl
    lea r8, [lineBuf]
    call fmt_hex2
    mov ecx, 53
    lea edx, [rbx + 6]
    lea r8, [lineBuf]
    mov r9d, 2
    mov r10d, COL_NOTE
    call draw_text

    mov r14d, r13d
    shr r14d, 2
    xor r15d, r15d
.m_seg:
    cmp r15d, r14d
    jae .m_done
    lea ecx, [r15 + 56]
    imul ecx, CHAR_W
    lea edx, [rbx + 6]
    imul edx, CHAR_H
    add edx, 3
    mov r8d, 7
    mov r9d, 6
    mov r10d, COL_PURP
    call fill_rect
    inc r15d
    jmp .m_seg
.m_done:

    inc ebx
    cmp ebx, NUM_CH
    jb .vu_loop

    mov ecx, 4 * CHAR_W
    mov edx, 16 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, 32 * CHAR_H
    mov r10d, COL_PURP
    call fill_rect

    cmp dword [isPlaying], 0
    je .no_pbar
    mov ecx, 4 * CHAR_W
    mov eax, [playRow]
    add eax, 16
    imul eax, CHAR_H
    mov edx, eax
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_CYAN
    call fill_rect
.no_pbar:

    mov ecx, 4 * CHAR_W
    mov eax, [editRow]
    add eax, 16
    imul eax, CHAR_H
    mov edx, eax
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect

    xor ebx, ebx
.hdr_loop:
    mov r12d, ebx
    imul r12d, 9
    add r12d, 8

    lea rdx, [chMuted]
    cmp byte [rdx + rbx], 0
    jne .hdr_muted
    mov r10d, COL_HDR_ON
    lea r13, [txtOn]
    mov r14d, COL_CYAN
    jmp .hdr_draw
.hdr_muted:
    mov r10d, COL_HDR_OFF
    lea r13, [txtOff]
    mov r14d, COL_PURPHI
.hdr_draw:
    mov ecx, r12d
    imul ecx, CHAR_W
    mov edx, 15 * CHAR_H
    mov r8d, 8 * CHAR_W
    mov r9d, CHAR_H
    call fill_rect

    lea ecx, [r12 + 3]
    mov edx, 15
    mov r8, r13
    mov r9d, 3
    mov r10d, r14d
    call draw_text

    mov ecx, r12d
    imul ecx, CHAR_W
    mov edx, 16 * CHAR_H
    mov r8d, 8 * CHAR_W
    mov r9d, 32 * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    inc ebx
    cmp ebx, NUM_CH
    jb .hdr_loop

    xor r12d, r12d
.row_draw:
    lea r13d, [r12 + 16]

    mov al, r12b
    lea r8, [lineBuf]
    call fmt_hex2
    mov ecx, 1
    mov edx, r13d
    lea r8, [lineBuf]
    mov r9d, 2
    mov r10d, COL_TEAL
    cmp r12d, [editRow]
    jne .rn_c
    mov r10d, COL_WHITE
.rn_c:
    call draw_text

    xor ebx, ebx
.ch_draw:
    mov r15d, ebx
    imul r15d, 9
    add r15d, 8

    cmp r12d, [editRow]
    je .bg_edit
    cmp dword [isPlaying], 0
    je .bg_chk_beat
    mov eax, [curPat]
    cmp eax, [playPat]
    jne .bg_chk_beat
    cmp r12d, [playRow]
    je .bg_play
.bg_chk_beat:
    test r12d, 3
    jnz .bg_skip
    mov r10d, COL_BEAT
    jmp .bg_paint
.bg_play:
    mov r10d, COL_PLAYROW
    jmp .bg_paint
.bg_edit:
    mov r10d, COL_EDITROW
.bg_paint:
    mov ecx, r15d
    imul ecx, CHAR_W
    mov edx, r13d
    imul edx, CHAR_H
    mov r8d, 8 * CHAR_W
    mov r9d, CHAR_H
    call fill_rect
.bg_skip:

    cmp r12d, [editRow]
    jne .no_cur_box
    cmp ebx, [curCh]
    jne .no_cur_box
    mov eax, [curSubCol]
    lea rdx, [subColOff]
    movzx ecx, byte [rdx + rax]
    add ecx, r15d
    imul ecx, CHAR_W
    mov edx, r13d
    imul edx, CHAR_H
    lea r8, [subColWid]
    movzx r8d, byte [r8 + rax]
    imul r8d, CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_CURSOR
    call fill_rect
.no_cur_box:

    mov ecx, [curPat]
    mov edx, r12d
    mov r8d, ebx
    call get_cell_ptr
    mov rsi, rax
    call format_cell_text

    lea rdx, [chMuted]
    movzx ebp, byte [rdx + rbx]

    mov r10d, COL_DOT
    cmp byte [rsi + 0], 0xFF
    jne .has_note_col
    cmp byte [rsi + 1], 0xFF
    jne .has_note_col
    cmp byte [rsi + 2], 0xFF
    je .draw_left6
.has_note_col:
    mov r10d, COL_NOTE
    test ebp, ebp
    jz .n_not_m
    mov r10d, COL_MUTED
.n_not_m:
.draw_left6:
    mov ecx, r15d
    mov edx, r13d
    lea r8, [cellBuf]
    mov r9d, 6
    call draw_text

    mov r10d, COL_DOT
    cmp byte [rsi + 3], 0xFF
    je .draw_fx2
    mov r10d, COL_PURPHI
    test ebp, ebp
    jz .draw_fx2
    mov r10d, COL_MUTED
.draw_fx2:
    lea ecx, [r15 + 6]
    mov edx, r13d
    lea r8, [cellBuf + 6]
    mov r9d, 2
    call draw_text

    cmp r12d, [editRow]
    jne .ch_d_next
    cmp ebx, [curCh]
    jne .ch_d_next
    mov eax, [curSubCol]
    lea rdx, [subColOff]
    movzx ecx, byte [rdx + rax]
    lea r8, [cellBuf]
    add r8, rcx
    add ecx, r15d
    mov edx, r13d
    lea r9, [subColWid]
    movzx r9d, byte [r9 + rax]
    mov r10d, COL_BLACK
    call draw_text

.ch_d_next:
    inc ebx
    cmp ebx, NUM_CH
    jb .ch_draw

    inc r12d
    cmp r12d, PAT_ROWS
    jb .row_draw

    mov eax, [curTempo]
    lea rdx, [hexTbl]
    mov al, [rdx + rax]
    mov [botBar + 8], al
    mov eax, [curOct]
    add al, '2'
    mov [botBar + 16], al
    mov eax, [curCh]
    add al, '0'
    mov [botBar + 23], al
    mov al, [curPat]
    lea r8, [botBar + 31]
    call fmt_hex2
    mov al, [editRow]
    lea r8, [botBar + 40]
    call fmt_hex2
    mov al, [curInst]
    lea r8, [botBar + 50]
    call fmt_hex2
    mov eax, [editStep]
    add al, '0'
    mov [botBar + 60], al

    xor ecx, ecx
    mov edx, 49
    lea r8, [botBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    mov rcx, [hDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, WIN_W
    mov qword [rsp + 0x20], WIN_H
    mov rax, [memDC]
    mov [rsp + 0x28], rax
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    mov qword [rsp + 0x40], SRCCOPY
    call BitBlt

    add rsp, 0x48
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; ============================================================================
; Start-Demo-Patterns (Pattern 00 und 01) zum direkten Testen vorbelegen
; ============================================================================
init_demo_patterns:
    push rbx
    push rbp
    push r12
    push r13
    sub rsp, 0x28

    lea rdi, [patterns]
    mov al, 0xFF
    mov ecx, MAX_PATS * PAT_BYTES
    rep stosb

    mov byte [orderList + 0], 0
    mov byte [orderList + 1], 1
    mov dword [orderLen], 2

    xor r12d, r12d
.id_loop:
    mov ebx, r12d
    shr ebx, 5
    mov r13d, r12d
    and r13d, 31

    mov eax, r12d
    shr eax, 4
    shl eax, 2
    lea rbp, [demoChords]
    add rbp, rax

    test r13d, 3
    jnz .id_snare
    mov ecx, ebx
    mov edx, r13d
    xor r8d, r8d
    call get_cell_ptr
    mov dword [rax], 0xFF0F0000

.id_snare:
    mov eax, r13d
    and eax, 7
    cmp eax, 4
    jne .id_hat
    mov ecx, ebx
    mov edx, r13d
    mov r8d, 1
    call get_cell_ptr
    mov dword [rax], 0xFF0E0110

.id_hat:
    test r13d, 1
    jz .id_bass
    mov ecx, ebx
    mov edx, r13d
    mov r8d, 2
    call get_cell_ptr
    mov byte [rax + 0], 33
    mov byte [rax + 1], 2
    mov byte [rax + 2], 8
    mov ecx, r13d
    and ecx, 3
    cmp ecx, 3
    jne .id_bass
    mov byte [rax + 2], 12

.id_bass:
    mov ecx, ebx
    mov edx, r13d
    mov r8d, 3
    call get_cell_ptr
    mov cl, [rbp + 0]
    mov byte [rax + 1], 3
    mov byte [rax + 2], 12
    test r13d, 1
    jz .b_lo
    add cl, 12
    mov byte [rax + 2], 14
    mov byte [rax + 3], 0x01
.b_lo:
    mov [rax + 0], cl

    mov ecx, ebx
    mov edx, r13d
    mov r8d, 4
    call get_cell_ptr
    mov edx, r13d
    and edx, 3
    lea r8, [demoArpIdx]
    movzx edx, byte [r8 + rdx]
    mov cl, [rbp + 1 + rdx]
    mov [rax + 0], cl
    mov byte [rax + 1], 4
    mov byte [rax + 2], 10

    test r12d, 1
    jnz .id_echo
    mov eax, r12d
    shr eax, 1
    lea rdx, [demoLead]
    mov cl, [rdx + rax]
    cmp cl, 0xFF
    je .id_echo
    push rcx
    mov ecx, ebx
    mov edx, r13d
    mov r8d, 5
    call get_cell_ptr
    pop rcx
    mov [rax + 0], cl
    mov byte [rax + 1], 5
    mov byte [rax + 2], 14
    mov byte [rax + 3], 0x44

.id_echo:
    lea eax, [r12 - 3]
    and eax, 63
    test eax, 1
    jnz .id_pad
    shr eax, 1
    lea rdx, [demoLead]
    mov cl, [rdx + rax]
    cmp cl, 0xFF
    je .id_pad
    push rcx
    mov ecx, ebx
    mov edx, r13d
    mov r8d, 6
    call get_cell_ptr
    pop rcx
    mov [rax + 0], cl
    mov byte [rax + 1], 6
    mov byte [rax + 2], 7

.id_pad:
    test r13d, 15
    jnz .id_next
    mov ecx, ebx
    mov edx, r13d
    mov r8d, 7
    call get_cell_ptr
    mov cl, [rbp + 1]
    mov [rax + 0], cl
    mov byte [rax + 1], 7
    mov byte [rax + 2], 13

.id_next:
    inc r12d
    cmp r12d, 64
    jb .id_loop

    add rsp, 0x28
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

; ============================================================================
; Programmeinstieg
; ============================================================================
start:
    sub rsp, 0x78
    finit

    mov dword [running], 1
    mov dword [dirty], 1
    mov dword [curTempo], 5
    mov dword [curOct], 1
    mov dword [curInst], 5
    mov dword [curVol], 15
    mov dword [editStep], 1
    mov dword [lfsr], 0x1337CAFE

    lea rdi, [dxTbl]
    mov eax, CHAR_W
    mov ecx, 80
    rep stosd

    lea rdi, [sinTbl]
    xor ecx, ecx
.s_loop:
    mov [tmpInt], ecx
    fild dword [tmpInt]
    fmul dword [f_2pi_256]
    fsin
    fmul dword [f_127]
    fistp dword [rdi + rcx*4]
    inc ecx
    cmp ecx, 256
    jb .s_loop

    call init_demo_patterns
    xor ebx, ebx
.v_init:
    mov eax, ebx
    shl eax, 6
    lea rdi, [voices]
    mov [rdi + rax + 4], ebx
    mov dword [rdi + rax + 24], 15
    mov dword [rdi + rax + 40], -1
    inc ebx
    cmp ebx, NUM_CH
    jb .v_init

    xor ecx, ecx
    call GetModuleHandleA
    mov [hInst], rax

    xor ecx, ecx
    mov edx, IDC_ARROW
    call LoadCursorA
    mov r12, rax

    lea rdi, [wndClass]
    mov dword [rdi + 0], 0
    lea rax, [WndProc]
    mov [rdi + 8], rax
    mov rax, [hInst]
    mov [rdi + 24], rax
    mov [rdi + 40], r12
    lea rax, [clsName]
    mov [rdi + 64], rax

    mov rcx, rdi
    call RegisterClassA

    mov dword [winRect + 0], 0
    mov dword [winRect + 4], 0
    mov dword [winRect + 8], WIN_W
    mov dword [winRect + 12], WIN_H
    lea rcx, [winRect]
    mov edx, WS_TRACKER
    xor r8d, r8d
    call AdjustWindowRect

    mov eax, [winRect + 8]
    sub eax, [winRect + 0]
    mov r12d, eax
    mov eax, [winRect + 12]
    sub eax, [winRect + 4]
    mov r13d, eax

    xor ecx, ecx
    lea rdx, [clsName]
    lea r8, [winTitle]
    mov r9d, WS_TRACKER
    mov qword [rsp + 0x20], 80
    mov qword [rsp + 0x28], 50
    mov [rsp + 0x30], r12
    mov [rsp + 0x38], r13
    mov qword [rsp + 0x40], 0
    mov qword [rsp + 0x48], 0
    mov rax, [hInst]
    mov [rsp + 0x50], rax
    mov qword [rsp + 0x58], 0
    call CreateWindowExA
    mov [hWnd], rax

    mov rcx, rax
    call GetDC
    mov [hDC], rax

    mov rcx, rax
    call CreateCompatibleDC
    mov [memDC], rax

    mov rcx, [hDC]
    mov edx, WIN_W
    mov r8d, WIN_H
    call CreateCompatibleBitmap

    mov rcx, [memDC]
    mov rdx, rax
    call SelectObject

    mov ecx, -11
    mov edx, 9
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 700
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    mov qword [rsp + 0x40], 0
    mov qword [rsp + 0x48], 0
    mov qword [rsp + 0x50], 0
    mov qword [rsp + 0x58], 3
    mov qword [rsp + 0x60], 49
    lea rax, [fontName]
    mov [rsp + 0x68], rax
    call CreateFontA

    mov rcx, [memDC]
    mov rdx, rax
    call SelectObject

    mov rcx, [memDC]
    mov edx, TRANSPARENT
    call SetBkMode

    lea rcx, [hWaveOut]
    mov edx, -1
    lea r8, [wfx]
    xor r9d, r9d
    mov qword [rsp + 0x20], 0
    mov qword [rsp + 0x28], 0
    call waveOutOpen

    xor ebx, ebx
.au_init:
    mov eax, ebx
    imul eax, 48
    lea rsi, [waveHdrs]
    add rsi, rax

    mov eax, ebx
    imul eax, BUF_BYTES
    lea rdi, [audioBufs]
    add rdi, rax

    mov [rsi + 0], rdi
    mov dword [rsi + 8], BUF_BYTES

    mov rcx, [hWaveOut]
    mov rdx, rsi
    mov r8d, 48
    call waveOutPrepareHeader

    mov rcx, [hWaveOut]
    mov rdx, rsi
    mov r8d, 48
    call waveOutWrite

    inc ebx
    cmp ebx, BUF_COUNT
    jb .au_init

main_loop:
.msg_drain:
    lea rcx, [msg]
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 1
    call PeekMessageA
    test eax, eax
    jz .msg_done
    lea rcx, [msg]
    call TranslateMessage
    lea rcx, [msg]
    call DispatchMessageA
    jmp .msg_drain
.msg_done:

    cmp dword [running], 0
    je .exit_prog

    call pump_audio

    call GetTickCount
    mov ecx, eax
    sub eax, [lastDraw]
    cmp eax, 25
    jae .do_draw
    cmp dword [dirty], 0
    je .skip_draw
.do_draw:
    mov [lastDraw], ecx
    mov dword [dirty], 0
    call draw_screen
.skip_draw:

    mov ecx, 4
    call Sleep
    jmp main_loop

.exit_prog:
    mov rcx, [hWaveOut]
    call waveOutReset
    mov rcx, [hWaveOut]
    call waveOutClose
    xor ecx, ecx
    call ExitProcess