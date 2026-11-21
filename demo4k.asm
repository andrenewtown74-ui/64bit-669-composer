; 64-Bit Windows 4K-Grundgeruest (NASM, ohne CRT)
; Direkter 32-Bit-Pixelbuffer (GDI StretchDIBits) + Prozedurales Audio (WinMM)
; Beenden ueber ESC-Taste

default rel
bits 64

; Konfigurationskonstanten
WIDTH           equ 320
HEIGHT          equ 200
WIN_W           equ 640
WIN_H           equ 400

SAMPLE_RATE     equ 44100
AUDIO_SAMPLES   equ 524288              ; 2^19 Samples (ca. 11,9 s, nahtloser Loop)
AUDIO_BYTES     equ AUDIO_SAMPLES * 2   ; 16-Bit Mono

WS_POPUP_VIS    equ 0x90000000          ; WS_POPUP | WS_VISIBLE
SRCCOPY         equ 0x00CC0020
WHDR_LOOP       equ 0x0000000C          ; WHDR_BEGINLOOP | WHDR_ENDLOOP
TIME_SAMPLES    equ 2
VK_ESCAPE       equ 0x1B

; Externe Windows-API-Funktionen
extern ExitProcess
extern Sleep
extern CreateWindowExA
extern GetDC
extern PeekMessageA
extern DispatchMessageA
extern GetAsyncKeyState
extern StretchDIBits
extern waveOutOpen
extern waveOutPrepareHeader
extern waveOutWrite
extern waveOutGetPosition

; ============================================================================
; Initialisierte Daten
; ============================================================================
section .data

clsName db "static", 0

; 16-Schritt-Pattern (Frequenz-Multiplikatoren in A-Moll fuer 44100 Hz / 8192)
pattern db 20, 20, 24, 20, 30, 27, 24, 20
        db 16, 16, 20, 24, 30, 32, 30, 24

; BITMAPINFOHEADER (40 Byte), negative Hoehe = Top-Down-DIB (Zeile 0 oben)
align 4
bmi:
    dd 40               ; biSize
    dd WIDTH            ; biWidth
    dd -HEIGHT          ; biHeight
    dw 1                ; biPlanes
    dw 32               ; biBitCount (BGRA / 0x00RRGGBB)
    dd 0                ; biCompression (BI_RGB)
    dd 0                ; biSizeImage
    dd 0                ; biXPelsPerMeter
    dd 0                ; biYPelsPerMeter
    dd 0                ; biClrUsed
    dd 0                ; biClrImportant

; WAVEFORMATEX (18 Byte, 1-Byte-Alignment laut mmsystem.h, hier padding-frei)
align 2
wfx:
    dw 1                ; wFormatTag (WAVE_FORMAT_PCM)
    dw 1                ; nChannels (Mono)
    dd SAMPLE_RATE      ; nSamplesPerSec
    dd SAMPLE_RATE * 2  ; nAvgBytesPerSec
    dw 2                ; nBlockAlign
    dw 16               ; wBitsPerSample
    dw 0                ; cbSize

; ============================================================================
; Uninitialisierte Daten (belegen 0 Byte in der EXE-Datei)
; ============================================================================
section .bss

align 16
hWaveOut resq 1
waveHdr  resb 48        ; WAVEHDR-Struktur auf x64 (48 Byte)
mmtime   resb 12        ; MMTIME-Struktur (12 Byte)
msg      resb 48        ; MSG-Struktur auf x64 (48 Byte)

align 16
pixelBuf resd WIDTH * HEIGHT            ; 256 KB Framebuffer
audioBuf resw AUDIO_SAMPLES             ; 1 MB Audio-Puffer

; ============================================================================
; Programmcode
; ============================================================================
section .text
global start

start:
    ; Windows x64 ABI: RSP ist beim Eintritt 8 mod 16.
    ; 0x78 Byte (120 = 7*16 + 8) stellen 16-Byte-Alignment her und bieten
    ; Platz fuer 32 Byte Shadow-Space plus bis zu 10 Stack-Parameter.
    sub rsp, 0x78

    ; ------------------------------------------------------------------------
    ; 1. Prozeduralen Sound in audioBuf berechnen
    ; ------------------------------------------------------------------------
    lea rdi, [audioBuf]
    xor r10d, r10d                      ; Sample-Index t = 0
    lea r11, [pattern]

.synth_loop:
    ; Pattern-Schritt bestimmen (alle 8192 Samples ein neuer Ton)
    mov ecx, r10d
    shr ecx, 13
    and ecx, 15
    movzx r8d, byte [r11 + rcx]

    ; Lineare Abkling-Huellkurve pro Schritt (8191 -> 0)
    mov edx, r10d
    and edx, 8191
    mov r9d, 8191
    sub r9d, edx

    ; Stimme 1: Saegezahn-Bass (Phase = t * Multiplikator)
    mov eax, r10d
    imul eax, r8d
    mov ebx, eax                        ; Phase fuer Stimme 2 sichern
    shl eax, 19                         ; Bit 12 auf Vorzeichenbit schieben
    sar eax, 24                         ; Vorzeichenbehaftet -128..+127
    imul eax, r9d                       ; Mit Huellkurve skalieren
    sar eax, 7                          ; Pegel ca. -8192..+8127

    ; Stimme 2: Oktav-Rechteck (doppelte Frequenz, kurze Gate-Zeit)
    cmp edx, 4096
    jae .no_arp
    mov ecx, 2400
    test ebx, 0x0800                    ; Bit 11 = doppelte Frequenz
    jnz .arp_pos
    neg ecx
.arp_pos:
    add eax, ecx
.no_arp:

    mov [rdi], ax                       ; 16-Bit-Sample speichern
    add rdi, 2
    inc r10d
    cmp r10d, AUDIO_SAMPLES
    jb .synth_loop

    ; ------------------------------------------------------------------------
    ; 2. Audio-Ausgabe ueber WinMM oeffnen und Endlosschleife starten
    ; ------------------------------------------------------------------------
    lea rcx, [hWaveOut]                 ; phwo
    mov edx, -1                         ; uDeviceID (WAVE_MAPPER)
    lea r8, [wfx]                       ; pwfx
    xor r9d, r9d                        ; dwCallback = 0
    mov qword [rsp + 0x20], 0           ; dwInstance = 0
    mov qword [rsp + 0x28], 0           ; fdwOpen = CALLBACK_NULL
    call waveOutOpen

    ; WAVEHDR fuellen (Offset 0: lpData, Offset 8: dwBufferLength)
    lea rax, [audioBuf]
    mov [waveHdr], rax
    mov dword [waveHdr + 8], AUDIO_BYTES

    mov rcx, [hWaveOut]
    lea rdx, [waveHdr]
    mov r8d, 48                         ; sizeof(WAVEHDR) auf x64
    call waveOutPrepareHeader

    ; Loop-Flags nach PrepareHeader setzen (Offset 24: dwFlags, 28: dwLoops)
    or dword [waveHdr + 24], WHDR_LOOP
    mov dword [waveHdr + 28], -1        ; Endlos-Wiederholung

    mov rcx, [hWaveOut]
    lea rdx, [waveHdr]
    mov r8d, 48
    call waveOutWrite

    ; ------------------------------------------------------------------------
    ; 3. Fenster ohne eigene WndProc erzeugen und Device Context holen
    ; ------------------------------------------------------------------------
    xor ecx, ecx                        ; dwExStyle = 0
    lea rdx, [clsName]                  ; lpClassName = "static"
    xor r8d, r8d                        ; lpWindowName = NULL
    mov r9d, WS_POPUP_VIS               ; dwStyle = WS_POPUP | WS_VISIBLE
    mov qword [rsp + 0x20], 100         ; X
    mov qword [rsp + 0x28], 100         ; Y
    mov qword [rsp + 0x30], WIN_W       ; nWidth
    mov qword [rsp + 0x38], WIN_H       ; nHeight
    mov qword [rsp + 0x40], 0           ; hWndParent
    mov qword [rsp + 0x48], 0           ; hMenu
    mov qword [rsp + 0x50], 0           ; hInstance
    mov qword [rsp + 0x58], 0           ; lpParam
    call CreateWindowExA
    mov r12, rax                        ; R12 = HWND (non-volatile)

    mov rcx, r12
    call GetDC
    mov r13, rax                        ; R13 = HDC (non-volatile)

    ; ------------------------------------------------------------------------
    ; 4. Hauptschleife: Audio-Sync, Pixelbuffer fuellen, Blitten, Events
    ; ------------------------------------------------------------------------
main_loop:
    ; Aktuelle Audio-Sampleposition fuer Synchronisation abfragen
    mov dword [mmtime], TIME_SAMPLES
    mov rcx, [hWaveOut]
    lea rdx, [mmtime]
    mov r8d, 12                         ; sizeof(MMTIME)
    call waveOutGetPosition
    mov r14d, [mmtime + 4]              ; R14D = aktueller Sample-Index

    ; Zeitbasis und Beat-Impuls aus der Audio-Position ableiten
    mov r8d, r14d
    shr r8d, 9                          ; Animationszeit t
    mov r9d, r14d
    and r9d, 8191
    shr r9d, 7                          ; 0..63 pro Pattern-Schritt
    mov r10d, 63
    sub r10d, r9d                       ; Beat-Impuls (63 -> 0)

    ; Pixelbuffer (320x200, 32-Bit 0x00RRGGBB) zeilenweise fuellen
    lea rdi, [pixelBuf]
    xor ebx, ebx                        ; y = 0
.loop_y:
    xor ecx, ecx                        ; x = 0
.loop_x:
    lea eax, [rcx + r8]                 ; u = x + t
    mov edx, r8d
    shr edx, 1
    add edx, ebx                        ; v = y + (t >> 1)

    ; Rotkanal: XOR-Textur + Beat-Impuls
    mov r11d, eax
    xor r11d, edx
    add r11d, r10d
    and r11d, 0xFF
    shl r11d, 16

    ; Gruenkanal: phasenverschobenes XOR-Muster
    mov esi, eax
    add esi, r8d
    xor esi, edx
    and esi, 0xFF
    shl esi, 8
    or r11d, esi

    ; Blaukanal: Diagonal-Verlauf
    lea esi, [rcx + rbx]
    add esi, r8d
    and esi, 0xFF
    or r11d, esi

    mov [rdi], r11d
    add rdi, 4
    inc ecx
    cmp ecx, WIDTH
    jb .loop_x

    inc ebx
    cmp ebx, HEIGHT
    jb .loop_y

    ; Pixelbuffer ueber GDI auf 640x400 ins Fenster blitten
    mov rcx, r13                        ; hdc
    xor edx, edx                        ; xDest
    xor r8d, r8d                        ; yDest
    mov r9d, WIN_W                      ; DestWidth
    mov qword [rsp + 0x20], WIN_H       ; DestHeight
    mov qword [rsp + 0x28], 0           ; xSrc
    mov qword [rsp + 0x30], 0           ; ySrc
    mov qword [rsp + 0x38], WIDTH       ; SrcWidth
    mov qword [rsp + 0x40], HEIGHT      ; SrcHeight
    lea rax, [pixelBuf]
    mov [rsp + 0x48], rax               ; lpBits
    lea rax, [bmi]
    mov [rsp + 0x50], rax               ; lpbmi
    mov qword [rsp + 0x58], 0           ; iUsage (DIB_RGB_COLORS)
    mov qword [rsp + 0x60], SRCCOPY     ; rop
    call StretchDIBits

    ; Windows-Nachrichtenwarteschlange leeren (verhindert "Keine Rueckmeldung")
    lea rcx, [msg]
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 1           ; PM_REMOVE
    call PeekMessageA
    test eax, eax
    jz .no_msg
    lea rcx, [msg]
    call DispatchMessageA
.no_msg:

    ; Kurze Pause zur CPU-Entlastung
    mov ecx, 10
    call Sleep

    ; Abbruch pruefen (ESC gedrueckt)
    mov ecx, VK_ESCAPE
    call GetAsyncKeyState
    test ax, 0x8000
    jz main_loop

    ; Prozess beenden (OS gibt HDC, Fenster und Audio-Handle frei)
    xor ecx, ecx
    call ExitProcess