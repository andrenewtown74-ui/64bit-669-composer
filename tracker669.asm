; 64-Bit Windows 4K-Demo (NASM, ohne CRT)
; Authentische Composer-669-Oberflaeche (80x50 Raster, 960x600)
; Live gekoppelt an den 8-Kanal-Stereo-Synthesizer (WinMM)
; Beenden ueber ESC-Taste

default rel
bits 64

; Raster- und Fensterkonstanten (80x50 Zeichen a 12x12 Pixel)
CW              equ 12
CHAR_H              equ 12
WIN_W           equ 960
WIN_H           equ 600

; Audio-Konstanten (44100 Hz, 16-Bit Stereo, 4 Patterns a 32 Zeilen = 128 Zeilen)
SAMPLE_RATE     equ 44100
STEP_SAMPLES    equ 5120
TOTAL_ROWS      equ 128
AUDIO_SAMPLES   equ TOTAL_ROWS * STEP_SAMPLES
AUDIO_BYTES     equ AUDIO_SAMPLES * 4

; Windows-Konstanten
WS_POPUP_VIS    equ 0x90000000
SRCCOPY         equ 0x00CC0020
ETO_OPAQUE      equ 2
TRANSPARENT     equ 1
WHDR_LOOP       equ 0x0000000C
TIME_SAMPLES    equ 2
VK_ESCAPE       equ 0x1B

; Composer-669-Farbpalette (GDI COLORREF = 0x00BBGGRR)
COL_BG          equ 0x00624222  ; Stahlblauer Hintergrund
COL_BLACK       equ 0x00000000  ; Schwarze Boxen und Pattern-Spalten
COL_BEAT        equ 0x00463626  ; Markierung jeder 4. Pattern-Zeile
COL_CURROW      equ 0x00785418  ; Aktive Abspielzeile
COL_CYAN        equ 0x00F0F000  ; Helles Cyan (Kopf-/Fusszeile, Titel, Werte)
COL_TEAL        equ 0x00B0B000  ; Dunkleres Cyan (Labels, Zeilennummern)
COL_HDRBG       equ 0x00786400  ; Kanal-Kopfzeilen ("On")
COL_NOTE        equ 0x00F0D8B8  ; Helles Eisblau (Noten, Hex-Tabellen)
COL_WHITE       equ 0x00FFFFFF  ; Aktive Note auf der Abspielzeile
COL_DOT         equ 0x0070583C  ; Punkte in leeren Pattern-Zellen
COL_PURP        equ 0x00900050  ; Violett (VU-Meter, Seitenbalken, Mini-LEDs)
COL_PURPHI      equ 0x00F048B8  ; Helles Violett (Effekt-Codes, Cursorzeile)

extern ExitProcess
extern Sleep
extern CreateWindowExA
extern GetDC
extern PeekMessageA
extern DispatchMessageA
extern GetAsyncKeyState
extern CreateCompatibleDC
extern CreateCompatibleBitmap
extern SelectObject
extern CreateFontA
extern SetBkMode
extern SetBkColor
extern SetTextColor
extern ExtTextOutA
extern BitBlt
extern waveOutOpen
extern waveOutPrepareHeader
extern waveOutWrite
extern waveOutGetPosition

; ============================================================================
; Initialisierte Daten
; ============================================================================
section .data

clsName   db "static", 0
fontName  db "Consolas", 0
hexTbl    db "0123456789ABCDEF"
noteNames db "C C#D D#E F F#G G#A A#B "
txtOn     db "On"

topBar    db " Composer 669: (c) 1992, Tran    Soundcard: WinMM        File: _4KSYNTH        "
botBar    db "  Tempo: 5    Octave: 2    Voice: 0    Pattern: 00    Order: 00    Mem: 0004k  "

; 3x36 Zeichen Song-Message (Original-669-Header-Format)
titleBox:
    db "       * 4K 669 Synth Demo *        "
    db "      x64 NASM Procedural 8-Ch      "
    db "     No CRT - Press ESC to Exit     "

; 12x12 Zeichen Sample-Liste / Begruessung
sampNames:
    db "Kick Drum   "
    db "Snare 669   "
    db "Hi-Hat HP   "
    db "Octave Bass "
    db "Pulse Arp   "
    db "Vibra Lead  "
    db "Tracker Echo"
    db "Sinus Pad   "
    db "------------"
    db "4K x64 ASM  "
    db "Greets:     "
    db "Demoszene!  "

; Sample-Laengen fuer die 5-stellige Hex-Tabelle (Spalten 17..33)
align 4
sampLenTbl:
    dd 0x01D76, 0x01216, 0x00900, 0x052C4
    dd 0x03CCD, 0x083D6, 0x01026, 0x048DA
    dd 0x016BA, 0x025E0, 0x00D22, 0x00CC0

; Zentrale Status-Zeilen (6 Zeilen a 8 Zeichen, Live-Werte werden eingetragen)
statLines:
    db "Vol:   F"
    db "Ord:  00"
    db "Pat:  00"
    db "Row:  00"
    db "Loop: 00"
    db "Inst: 00"

; FPU-Konstanten fuer Sinus-Tabelle
align 4
f_2pi_256 dd 0.024543693
f_127     dd 127.0

; 12-TET Phaseninkremente fuer Oktave 2 (C-2 bis B-2 bei 44100 Hz)
align 4
noteTbl:
    dd  6370030,  6748811,  7150117,  7575285
    dd  8025735,  8502969,  9008583,  9544261
    dd 10111791, 10713070, 11350103, 12025015

; Akkordfolge ueber 8 Takte (0xOS: O=Oktave 0..3, S=Halbton 0..11)
chords:
    db 0x09, 0x19, 0x20, 0x24   ; Am
    db 0x05, 0x15, 0x19, 0x20   ; F
    db 0x00, 0x10, 0x14, 0x17   ; C
    db 0x07, 0x17, 0x1B, 0x22   ; G
    db 0x09, 0x19, 0x20, 0x24   ; Am
    db 0x05, 0x15, 0x19, 0x20   ; F
    db 0x02, 0x12, 0x15, 0x19   ; Dm
    db 0x04, 0x14, 0x18, 0x1B   ; E

arpSeq db 0, 1, 2, 1

; Lead-Melodie (64 Achtelnoten fuer 128 Zeilen, 0xFF = Pause/Halten)
leadPat:
    db 0x29, 0xFF, 0x24, 0xFF, 0x20, 0x24, 0x29, 0xFF
    db 0x25, 0xFF, 0x24, 0xFF, 0x20, 0xFF, 0x19, 0x20
    db 0x24, 0xFF, 0x27, 0xFF, 0x24, 0x20, 0x24, 0x27
    db 0x22, 0xFF, 0x27, 0xFF, 0x2B, 0xFF, 0x27, 0xFF
    db 0x29, 0xFF, 0x30, 0xFF, 0x2B, 0x29, 0x27, 0x24
    db 0x25, 0xFF, 0x29, 0xFF, 0x27, 0x25, 0x24, 0x20
    db 0x22, 0xFF, 0x25, 0xFF, 0x29, 0x25, 0x22, 0xFF
    db 0x24, 0xFF, 0x28, 0xFF, 0x2B, 0x28, 0x24, 0xFF

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
; Uninitialisierte Daten (belegen 0 Byte in der EXE)
; ============================================================================
section .bss

align 16
hWnd      resq 1
hDC       resq 1
memDC     resq 1
hWaveOut  resq 1
waveHdr   resb 48
mmtime    resb 12
msg       resb 48
tmpRect   resd 4

align 4
tmpInt    resd 1
curRow    resd 1
curRowPos resd 1
curPat    resd 1
curPatRow resd 1

; Synth-Zustaende (8 Kanaele)
lfsr       resd 1
prevNoise  resd 1
kickPhase  resd 1
kickInc    resd 1
kickEnv    resd 1
snarePhase resd 1
snareEnv   resd 1
hatEnv     resd 1
bassPhase  resd 1
bassInc    resd 1
bassEnv    resd 1
bassLP     resd 1
arpPhase   resd 1
arpInc     resd 1
arpEnv     resd 1
leadPhase  resd 1
leadInc    resd 1
leadEnv    resd 1
echoPhase  resd 1
echoInc    resd 1
echoEnv    resd 1
padPhase1  resd 1
padInc1    resd 1
padPhase2  resd 1
padInc2    resd 1
vibPhase   resd 1

; Tabellen, Puffer und Text-Zwischenspeicher
align 16
dxTbl     resd 80               ; Feste 12-Pixel-Zeichenabstaende fuer ExtTextOutA
sinTbl    resd 256
rowMeter  resb TOTAL_ROWS * 8   ; Pegelwerte (0..30) pro Zeile und Kanal
patCells  resb TOTAL_ROWS * 64  ; 128 Zeilen * 8 Kanaele * 8 Zeichen
lineBuf   resb 32               ; Textpuffer fuer Hex-Formatierung
audioBuf  resd AUDIO_SAMPLES    ; 16-Bit Stereo-Audio

; ============================================================================
; Programmcode
; ============================================================================
section .text
global start

; Notenbyte (AL = 0xOS) in 32-Bit-Phaseninkrement (EAX) wandeln
note_to_inc:
    movzx ecx, al
    movzx eax, al
    and eax, 0x0F
    shr ecx, 4
    lea rdx, [noteTbl]
    mov eax, [rdx + rax*4]
    shl eax, cl
    ret

; EAX auf 16-Bit-Signed (-32767..+32767) begrenzen
clamp_s16:
    cmp eax, 32767
    jle .chk_lo
    mov eax, 32767
    ret
.chk_lo:
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
.loop5:
    mov r9d, eax
    and r9d, 15
    mov r9b, [rdx + r9]
    mov [r8 + rcx], r9b
    shr eax, 4
    dec ecx
    jns .loop5
    ret

; 8-Zeichen-669-Zelle formatieren: R8=Ziel, AL=Note, R9B=Inst, CL=Vol, DX=Effekt
fmt_note_cell:
    movzx r10d, al
    mov r11d, r10d
    and r10d, 0x0F
    shr r11d, 4
    add r11b, '2'
    push rdx
    lea rdx, [noteNames]
    mov ax, [rdx + r10*2]
    mov [r8], ax
    mov [r8 + 2], r11b
    mov byte [r8 + 3], '0'
    add r9b, '0'
    mov [r8 + 4], r9b
    lea rdx, [hexTbl]
    movzx eax, cl
    and eax, 15
    mov al, [rdx + rax]
    mov [r8 + 5], al
    pop rdx
    mov [r8 + 6], dx
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
    imul ecx, CW
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

start:
    sub rsp, 0x78
    finit

    ; ------------------------------------------------------------------------
    ; 1. Zeichenabstandstabelle (dxTbl), Pattern-Zellen und Sinus-Tabelle
    ; ------------------------------------------------------------------------
    lea rdi, [dxTbl]
    mov eax, CW
    mov ecx, 80
    rep stosd

    lea rdi, [patCells]
    mov al, '.'
    mov ecx, TOTAL_ROWS * 64
    rep stosb

    lea rdi, [sinTbl]
    xor ecx, ecx
.sin_loop:
    mov [tmpInt], ecx
    fild dword [tmpInt]
    fmul dword [f_2pi_256]
    fsin
    fmul dword [f_127]
    fistp dword [rdi + rcx*4]
    inc ecx
    cmp ecx, 256
    jb .sin_loop

    ; ------------------------------------------------------------------------
    ; 2. 8-Kanal-669-Stereo-Audio und Pattern-Anzeigedaten erzeugen
    ; ------------------------------------------------------------------------
    mov dword [lfsr], 0x1337CAFE
    lea r12, [audioBuf]
    lea r15, [sinTbl]
    xor r13d, r13d              ; row = 0..127

.row_loop:
    ; Zeiger auf rowMeter[row * 8] in RSI
    mov eax, r13d
    shl eax, 3
    lea rsi, [rowMeter]
    add rsi, rax

    ; Pegel der vorherigen Zeile weich abklingen lassen
    test r13d, r13d
    jz .no_prev_meter
    xor ecx, ecx
.dec_prev:
    movzx eax, byte [rsi - 8 + rcx]
    sub eax, 6
    jns .dec_ok
    xor eax, eax
.dec_ok:
    mov [rsi + rcx], al
    inc ecx
    cmp ecx, 8
    jb .dec_prev
.no_prev_meter:

    ; Zeiger auf patCells[row * 64] in RDI, Akkordzeiger in RBP
    mov eax, r13d
    shl eax, 6
    lea rdi, [patCells]
    add rdi, rax

    mov ebx, r13d
    shr ebx, 4
    shl ebx, 2
    lea rbp, [chords]
    add rbp, rbx

    ; Kanal 0: Kick
    mov eax, r13d
    and eax, 3
    jz .trig_kick
    test r13d, 16
    jz .no_kick
    mov eax, r13d
    and eax, 15
    cmp eax, 14
    jne .no_kick
.trig_kick:
    mov dword [kickPhase], 0
    mov dword [kickInc], 16800000
    mov dword [kickEnv], 32000
    mov byte [rsi + 0], 30
    lea r8, [rdi + 0]
    mov al, 0x00                ; C-2
    mov r9b, 0
    mov cl, 15
    mov dx, 0x2E2E              ; ".."
    call fmt_note_cell
.no_kick:

    ; Kanal 1: Snare
    mov eax, r13d
    and eax, 7
    cmp eax, 4
    jne .no_snare
    mov dword [snarePhase], 0
    mov dword [snareEnv], 28000
    mov byte [rsi + 1], 28
    lea r8, [rdi + 8]
    mov al, 0x14                ; E-3
    mov r9b, 1
    mov cl, 14
    mov dx, 0x2E2E
    call fmt_note_cell
.no_snare:

    ; Kanal 2: Hi-Hat
    test r13d, 1
    jz .no_hat
    mov eax, r13d
    and eax, 3
    cmp eax, 3
    jne .hat_soft
    mov dword [hatEnv], 19000
    mov byte [rsi + 2], 22
    mov cl, 10
    jmp .hat_cell
.hat_soft:
    mov dword [hatEnv], 10000
    mov byte [rsi + 2], 14
    mov cl, 6
.hat_cell:
    lea r8, [rdi + 16]
    mov al, 0x29                ; A-4
    mov r9b, 2
    mov dx, 0x2E2E
    call fmt_note_cell
.no_hat:

    ; Kanal 3: 16tel-Oktav-Bass (mit Slide-Indikator a1/b1 auf Betonungen)
    mov al, [rbp + 0]
    test r13d, 1
    jz .bass_low
    add al, 0x10
    mov byte [rsi + 3], 25
    mov cl, 14
    mov dx, 0x3161              ; "a1"
    jmp .bass_set
.bass_low:
    mov byte [rsi + 3], 21
    mov cl, 12
    mov dx, 0x2E2E
.bass_set:
    push rax
    lea r8, [rdi + 24]
    mov r9b, 3
    call fmt_note_cell
    pop rax
    call note_to_inc
    mov [bassInc], eax
    mov dword [bassEnv], 27000

    ; Kanal 4: 16tel-Arpeggio
    mov eax, r13d
    and eax, 3
    lea rdx, [arpSeq]
    movzx eax, byte [rdx + rax]
    mov al, [rbp + 1 + rax]
    push rax
    lea r8, [rdi + 32]
    mov r9b, 4
    mov cl, 11
    mov dx, 0x2E2E
    call fmt_note_cell
    pop rax
    call note_to_inc
    mov [arpInc], eax
    mov dword [arpEnv], 20000
    mov byte [rsi + 4], 19

    ; Kanal 5: Lead-Melodie (mit Vibrato-Code c5)
    test r13d, 1
    jnz .no_lead
    mov eax, r13d
    shr eax, 1
    lea rdx, [leadPat]
    mov al, [rdx + rax]
    cmp al, 0xFF
    je .no_lead
    push rax
    lea r8, [rdi + 40]
    mov r9b, 5
    mov cl, 15
    mov dx, 0x3563              ; "c5"
    call fmt_note_cell
    pop rax
    call note_to_inc
    mov [leadInc], eax
    mov dword [leadEnv], 24000
    mov byte [rsi + 5], 27
.no_lead:

    ; Kanal 6: Tracker-Echo (3 Zeilen verzoegert)
    lea eax, [r13 - 3]
    and eax, 127
    test eax, 1
    jnz .no_echo
    shr eax, 1
    lea rdx, [leadPat]
    mov al, [rdx + rax]
    cmp al, 0xFF
    je .no_echo
    push rax
    lea r8, [rdi + 48]
    mov r9b, 6
    mov cl, 7
    mov dx, 0x3162              ; "b1"
    call fmt_note_cell
    pop rax
    call note_to_inc
    mov [echoInc], eax
    mov dword [echoEnv], 12500
    mov byte [rsi + 6], 16
.no_echo:

    ; Kanal 7: Pad-Akkord + absteigende Lautstaerkewerte im Tracker
    mov eax, r13d
    and eax, 15
    jnz .pad_hold
    mov al, [rbp + 1]
    push rax
    lea r8, [rdi + 56]
    mov r9b, 7
    mov cl, 15
    mov dx, 0x2E2E
    call fmt_note_cell
    pop rax
    call note_to_inc
    mov [padInc1], eax
    mov al, [rbp + 3]
    call note_to_inc
    mov [padInc2], eax
    mov byte [rsi + 7], 20
    jmp .pad_done
.pad_hold:
    ; Volume-Kaskade wie im klassischen 669-Pattern anzeigen (E..1)
    mov ecx, 15
    sub ecx, eax
    lea rdx, [hexTbl]
    mov cl, [rdx + rcx]
    mov [rdi + 56 + 5], cl
    mov byte [rsi + 7], 14
.pad_done:

    ; Innere Sample-Schleife (5120 Stereo-Frames)
    xor r14d, r14d
.sample_loop:
    xor r8d, r8d
    xor r9d, r9d

    ; Xorshift32-Rauschen
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
    movsx ebx, al

    ; Kanal 0: Kick
    mov ecx, [kickEnv]
    test ecx, ecx
    jz .s_kick_done
    mov eax, [kickInc]
    add [kickPhase], eax
    cmp eax, 4600000
    jbe .kick_floor
    mov edx, eax
    shr edx, 10
    sub eax, edx
    mov [kickInc], eax
.kick_floor:
    mov eax, [kickPhase]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul eax, ecx
    sar eax, 9
    add r8d, eax
    add r9d, eax
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [kickEnv], ecx
.s_kick_done:

    ; Kanal 1: Snare
    mov ecx, [snareEnv]
    test ecx, ecx
    jz .s_snare_done
    add dword [snarePhase], 19500000
    mov eax, [snarePhase]
    shr eax, 24
    mov eax, [r15 + rax*4]
    sar eax, 1
    add eax, ebx
    imul eax, ecx
    sar eax, 10
    mov edx, eax
    sar edx, 1
    add r8d, edx
    add r9d, eax
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [snareEnv], ecx
.s_snare_done:

    ; Kanal 2: Hi-Hat
    mov ecx, [hatEnv]
    test ecx, ecx
    jz .s_hat_done
    mov eax, ebx
    sub eax, [prevNoise]
    imul eax, ecx
    sar eax, 10
    add r8d, eax
    sar eax, 1
    add r9d, eax
    mov edx, ecx
    shr edx, 9
    sub ecx, edx
    mov [hatEnv], ecx
.s_hat_done:
    mov [prevNoise], ebx

    ; Kanal 3: Tiefpass-Bass
    mov eax, [bassInc]
    add [bassPhase], eax
    mov eax, [bassPhase]
    sar eax, 24
    imul eax, [bassEnv]
    sar eax, 9
    mov edx, [bassLP]
    sub eax, edx
    sar eax, 3
    add edx, eax
    mov [bassLP], edx
    add r8d, edx
    mov eax, edx
    imul eax, 3
    sar eax, 2
    add r9d, eax
    mov ecx, [bassEnv]
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [bassEnv], ecx

    ; Kanal 4: 25%-Puls-Arpeggio
    mov ecx, [arpEnv]
    mov eax, [arpInc]
    add [arpPhase], eax
    cmp dword [arpPhase], 0x40000000
    jb .arp_hi
    mov eax, -36
    jmp .arp_mix
.arp_hi:
    mov eax, 108
.arp_mix:
    imul eax, ecx
    sar eax, 10
    add r8d, eax
    sar eax, 2
    add r9d, eax
    mov edx, ecx
    shr edx, 10
    sub ecx, edx
    mov [arpEnv], ecx

    ; Vibrato-LFO
    add dword [vibPhase], 550000
    mov eax, [vibPhase]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul eax, 750

    ; Kanal 5: Lead
    mov ecx, [leadEnv]
    test ecx, ecx
    jz .s_lead_done
    add eax, [leadInc]
    add [leadPhase], eax
    mov eax, [leadPhase]
    sar eax, 24
    imul eax, ecx
    sar eax, 10
    mov edx, eax
    sar edx, 2
    add r8d, edx
    add r9d, eax
    mov edx, ecx
    shr edx, 12
    sub ecx, edx
    mov [leadEnv], ecx
.s_lead_done:

    ; Kanal 6: Echo
    mov ecx, [echoEnv]
    test ecx, ecx
    jz .s_echo_done
    mov eax, [echoInc]
    add [echoPhase], eax
    mov eax, [echoPhase]
    sar eax, 24
    imul eax, ecx
    sar eax, 10
    add r8d, eax
    sar eax, 2
    add r9d, eax
    mov edx, ecx
    shr edx, 12
    sub ecx, edx
    mov [echoEnv], ecx
.s_echo_done:

    ; Kanal 7: Sinus-Pad
    mov eax, [padInc1]
    add [padPhase1], eax
    mov eax, [padPhase1]
    shr eax, 24
    mov edx, [r15 + rax*4]
    mov eax, [padInc2]
    add [padPhase2], eax
    mov eax, [padPhase2]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul edx, 10
    imul eax, 10
    add r8d, edx
    add r9d, eax
    sar edx, 1
    sar eax, 1
    add r8d, eax
    add r9d, edx

    mov eax, r8d
    call clamp_s16
    mov [r12], ax
    mov eax, r9d
    call clamp_s16
    mov [r12 + 2], ax

    add r12, 4
    inc r14d
    cmp r14d, STEP_SAMPLES
    jb .sample_loop

    inc r13d
    cmp r13d, TOTAL_ROWS
    jb .row_loop

    ; ------------------------------------------------------------------------
    ; 3. Audio-Ausgabe ueber WinMM starten
    ; ------------------------------------------------------------------------
    lea rcx, [hWaveOut]
    mov edx, -1
    lea r8, [wfx]
    xor r9d, r9d
    mov qword [rsp + 0x20], 0
    mov qword [rsp + 0x28], 0
    call waveOutOpen

    lea rax, [audioBuf]
    mov [waveHdr], rax
    mov dword [waveHdr + 8], AUDIO_BYTES

    mov rcx, [hWaveOut]
    lea rdx, [waveHdr]
    mov r8d, 48
    call waveOutPrepareHeader

    or dword [waveHdr + 24], WHDR_LOOP
    mov dword [waveHdr + 28], -1

    mov rcx, [hWaveOut]
    lea rdx, [waveHdr]
    mov r8d, 48
    call waveOutWrite

    ; ------------------------------------------------------------------------
    ; 4. Fenster, flackerfreier Offscreen-DC und Pixel-Font anlegen
    ; ------------------------------------------------------------------------
    xor ecx, ecx
    lea rdx, [clsName]
    xor r8d, r8d
    mov r9d, WS_POPUP_VIS
    mov qword [rsp + 0x20], 100
    mov qword [rsp + 0x28], 70
    mov qword [rsp + 0x30], WIN_W
    mov qword [rsp + 0x38], WIN_H
    mov qword [rsp + 0x40], 0
    mov qword [rsp + 0x48], 0
    mov qword [rsp + 0x50], 0
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

    ; Fette nicht-geglaettete Festbreitenschrift (11px hoch, 9px breit)
    mov ecx, -11
    mov edx, 9
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 700         ; FW_BOLD
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    mov qword [rsp + 0x40], 0           ; ANSI_CHARSET
    mov qword [rsp + 0x48], 0
    mov qword [rsp + 0x50], 0
    mov qword [rsp + 0x58], 3           ; NONANTIALIASED_QUALITY
    mov qword [rsp + 0x60], 49          ; FIXED_PITCH | FF_MODERN
    lea rax, [fontName]
    mov [rsp + 0x68], rax
    call CreateFontA

    mov rcx, [memDC]
    mov rdx, rax
    call SelectObject

    mov rcx, [memDC]
    mov edx, TRANSPARENT
    call SetBkMode

    ; ------------------------------------------------------------------------
    ; 5. Hauptschleife: Audio-Position lesen und Composer-669-Screen zeichnen
    ; ------------------------------------------------------------------------
main_loop:
    mov dword [mmtime], TIME_SAMPLES
    mov rcx, [hWaveOut]
    lea rdx, [mmtime]
    mov r8d, 12
    call waveOutGetPosition
    mov eax, [mmtime + 4]

    xor edx, edx
    mov ecx, AUDIO_SAMPLES
    div ecx
    mov eax, edx
    xor edx, edx
    mov ecx, STEP_SAMPLES
    div ecx
    mov [curRow], eax
    mov [curRowPos], edx
    mov ecx, eax
    shr ecx, 5
    mov [curPat], ecx                   ; Pattern 0..3
    and eax, 31
    mov [curPatRow], eax                ; Zeile 0..31

    ; Gesamten Hintergrund in Composer-669-Stahlblau fuellen
    xor ecx, ecx
    xor edx, edx
    mov r8d, WIN_W
    mov r9d, WIN_H
    mov r10d, COL_BG
    call fill_rect

    ; Kopfzeile (Zeile 0)
    xor ecx, ecx
    xor edx, edx
    lea r8, [topBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    ; Schwarze Sample-Box (Spalten 4..15, Zeilen 2..13)
    mov ecx, 4 * CW
    mov edx, 2 * CHAR_H
    mov r8d, 12 * CW
    mov r9d, 12 * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    ; Schwarze Titel-Box (Spalten 35..70, Zeilen 2..4)
    mov ecx, 35 * CW
    mov edx, 2 * CHAR_H
    mov r8d, 36 * CW
    mov r9d, 3 * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    ; 3 Zeilen Song-Titel ausgeben
    xor ebx, ebx
.title_loop:
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
    jb .title_loop

    ; Sample-Indizes, Sample-Namen, Hex-Adressen und rechte Order-Liste (12 Zeilen)
    xor ebx, ebx
.upper_loop:
    lea r12d, [rbx + 2]                 ; Bildschirmzeile 2..13

    ; Sample-Index (Spalte 1)
    mov al, bl
    lea r8, [lineBuf]
    call fmt_hex2
    mov ecx, 1
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 2
    mov r10d, COL_TEAL
    call draw_text

    ; Sample-Name (Spalte 4)
    mov eax, ebx
    imul eax, 12
    lea r8, [sampNames]
    add r8, rax
    mov ecx, 4
    mov edx, r12d
    mov r9d, 12
    mov r10d, COL_NOTE
    call draw_text

    ; 3 Hex-Spalten (LoopStart, LoopEnd, Laenge) ab Spalte 17
    lea r13, [sampLenTbl]
    mov r14d, [r13 + rbx*4]             ; Sample-Laenge
    xor eax, eax
    cmp ebx, 3                          ; Bass/Pad erhalten Loop-Adressen
    jb .hex_noloop
    cmp ebx, 7
    ja .hex_noloop
    mov eax, r14d
    shr eax, 2                          ; LoopStart = Laenge / 4
.hex_noloop:
    lea r8, [lineBuf]
    call fmt_hex5
    mov byte [lineBuf + 5], ' '

    mov eax, 0xFFFFF
    cmp ebx, 3
    jb .hex_end
    cmp ebx, 7
    ja .hex_end
    mov eax, r14d
.hex_end:
    lea r8, [lineBuf + 6]
    call fmt_hex5
    mov byte [lineBuf + 11], ' '

    mov eax, r14d
    lea r8, [lineBuf + 12]
    call fmt_hex5

    mov ecx, 17
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 17
    mov r10d, COL_NOTE
    call draw_text

    ; Rechte Order-Tabelle (Spalten 72..78: "XX PP 5")
    mov al, bl
    lea r8, [lineBuf]
    call fmt_hex2
    mov byte [lineBuf + 2], ' '
    mov al, bl
    and al, 3
    lea r8, [lineBuf + 3]
    call fmt_hex2
    mov word [lineBuf + 5], 0x3520      ; " 5"

    mov r10d, COL_TEAL
    cmp ebx, [curPat]
    jne .ord_col_ok
    mov r10d, COL_CYAN
.ord_col_ok:
    mov ecx, 72
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 7
    call draw_text

    inc ebx
    cmp ebx, 12
    jb .upper_loop

    ; Zentrale Status-Werte (Ord, Pat, Row, Inst) aktualisieren und ausgeben
    mov al, [curPat]
    lea r8, [statLines + 8 + 6]
    call fmt_hex2
    lea r8, [statLines + 16 + 6]
    call fmt_hex2
    mov al, [curPatRow]
    lea r8, [statLines + 24 + 6]
    call fmt_hex2
    mov al, [curRow]
    and al, 7
    lea r8, [statLines + 40 + 6]
    call fmt_hex2

    xor ebx, ebx
.stat_loop:
    mov eax, ebx
    shl eax, 3
    lea r12, [statLines]
    add r12, rax
    lea r13d, [rbx + 7]                 ; Zeilen 7..12

    ; Label (5 Zeichen in Teal)
    mov ecx, 44
    mov edx, r13d
    mov r8, r12
    mov r9d, 5
    mov r10d, COL_TEAL
    call draw_text

    ; Wert (3 Zeichen in Hellem Cyan)
    mov ecx, 49
    mov edx, r13d
    lea r8, [r12 + 5]
    mov r9d, 3
    mov r10d, COL_CYAN
    call draw_text

    inc ebx
    cmp ebx, 6
    jb .stat_loop

    ; Live-VU-Meter (Spalten 35..42, vertikal) + Mini-LEDs (Spalten 53..68, horizontal)
    mov eax, [curRow]
    shl eax, 3
    lea r12, [rowMeter]
    add r12, rax
    xor ebx, ebx
.vu_loop:
    movzx r13d, byte [r12 + rbx]        ; Startpegel 0..30
    mov eax, [curRowPos]
    shr eax, 9
    sub r13d, eax
    jns .vu_pos
    xor r13d, r13d
.vu_pos:

    ; Vertikaler violetter Balken im Mittelbereich (max 90 Pixel hoch)
    mov r14d, r13d
    imul r14d, 3
    test r14d, r14d
    jz .vu_skip_bar
    lea ecx, [rbx + 35]
    imul ecx, CW
    mov edx, 14 * CHAR_H
    sub edx, r14d
    mov r8d, CW - 2
    mov r9d, r14d
    mov r10d, COL_PURP
    call fill_rect
.vu_skip_bar:

    ; Rechte Mini-Kanalzeile ("00".."07" + violette Punkte)
    mov al, bl
    lea r8, [lineBuf]
    call fmt_hex2
    mov ecx, 53
    lea edx, [rbx + 6]
    lea r8, [lineBuf]
    mov r9d, 2
    mov r10d, COL_NOTE
    call draw_text

    ; Horizontale violette Mini-Segmente (0..10 Stueck)
    mov r14d, r13d
    shr r14d, 2
    xor r15d, r15d
.mini_seg:
    cmp r15d, r14d
    jae .mini_done
    lea ecx, [r15 + 56]
    imul ecx, CW
    lea edx, [rbx + 6]
    imul edx, CHAR_H
    add edx, 3
    mov r8d, 7
    mov r9d, 6
    mov r10d, COL_PURP
    call fill_rect
    inc r15d
    jmp .mini_seg
.mini_done:

    inc ebx
    cmp ebx, 8
    jb .vu_loop

    ; ------------------------------------------------------------------------
    ; 6. Pattern-Bereich: Kanal-Koepfe ("On"), Seitenbalken und 32 Zeilen
    ; ------------------------------------------------------------------------
    ; Linker violetter Vertikalbalken (Spalten 4..6, Zeilen 16..47)
    mov ecx, 4 * CW
    mov edx, 16 * CHAR_H
    mov r8d, 3 * CW
    mov r9d, 32 * CHAR_H
    mov r10d, COL_PURP
    call fill_rect

    ; Heller Positionszeiger auf dem Seitenbalken fuer aktuelle Zeile
    mov ecx, 4 * CW
    mov eax, [curPatRow]
    add eax, 16
    imul eax, CHAR_H
    mov edx, eax
    mov r8d, 3 * CW
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect

    ; 8 Kanal-Kopfzeilen ("On") und schwarze Spaltenhintergruende
    xor ebx, ebx
.ch_hdr_loop:
    mov r12d, ebx
    imul r12d, 9
    add r12d, 8                         ; Startspalte 8, 17, 26, ...

    ; Kopfzeilen-Balken (Zeile 15)
    mov ecx, r12d
    imul ecx, CW
    mov edx, 15 * CHAR_H
    mov r8d, 8 * CW
    mov r9d, CHAR_H
    mov r10d, COL_HDRBG
    call fill_rect

    lea ecx, [r12 + 3]
    mov edx, 15
    lea r8, [txtOn]
    mov r9d, 2
    mov r10d, COL_CYAN
    call draw_text

    ; Schwarze Pattern-Spalte (Zeilen 16..47)
    mov ecx, r12d
    imul ecx, CW
    mov edx, 16 * CHAR_H
    mov r8d, 8 * CW
    mov r9d, 32 * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    inc ebx
    cmp ebx, 8
    jb .ch_hdr_loop

    ; 32 Pattern-Zeilen (00..1F) fuer das aktuelle Pattern zeichnen
    xor r12d, r12d                      ; r = 0..31
.pat_row_loop:
    lea r13d, [r12 + 16]                ; Bildschirmzeile 16..47

    ; Linke Zeilennummer 00..1F
    mov al, r12b
    lea r8, [lineBuf]
    call fmt_hex2
    mov ecx, 1
    mov edx, r13d
    lea r8, [lineBuf]
    mov r9d, 2
    mov r10d, COL_TEAL
    cmp r12d, [curPatRow]
    jne .rnum_col
    mov r10d, COL_CYAN
.rnum_col:
    call draw_text

    ; Zeilen-Basiszeiger in patCells berechnen: ((curPat*32 + r) * 64)
    mov eax, [curPat]
    shl eax, 5
    add eax, r12d
    shl eax, 6
    lea r14, [patCells]
    add r14, rax

    ; 8 Kanaele dieser Zeile zeichnen
    xor ebx, ebx
.pat_ch_loop:
    mov r15d, ebx
    imul r15d, 9
    add r15d, 8                         ; Spalte

    ; Hintergrund-Highlight (aktive Zeile oder jede 4. Beat-Zeile)
    cmp r12d, [curPatRow]
    je .bg_cur
    test r12d, 3
    jnz .bg_done
    mov r10d, COL_BEAT
    jmp .bg_draw
.bg_cur:
    mov r10d, COL_CURROW
.bg_draw:
    mov ecx, r15d
    imul ecx, CW
    mov edx, r13d
    imul edx, CHAR_H
    mov r8d, 8 * CW
    mov r9d, CHAR_H
    call fill_rect
.bg_done:

    ; Zeiger auf 8-Byte-Zelle in RSI
    lea rsi, [r14 + rbx*8]

    ; Fall 1: Komplett leere Zelle ("........")
    cmp byte [rsi], '.'
    jne .cell_note
    cmp byte [rsi + 5], '.'
    jne .cell_vol

    mov ecx, r15d
    mov edx, r13d
    mov r8, rsi
    mov r9d, 8
    mov r10d, COL_DOT
    call draw_text
    jmp .cell_next

.cell_vol:
    ; Fall 2: Reine Lautstaerkeaenderung (".....X..")
    mov ecx, r15d
    mov edx, r13d
    mov r8, rsi
    mov r9d, 5
    mov r10d, COL_DOT
    call draw_text

    lea ecx, [r15 + 5]
    mov edx, r13d
    lea r8, [rsi + 5]
    mov r9d, 1
    mov r10d, COL_NOTE
    call draw_text

    lea ecx, [r15 + 6]
    mov edx, r13d
    lea r8, [rsi + 6]
    mov r9d, 2
    mov r10d, COL_DOT
    call draw_text
    jmp .cell_next

.cell_note:
    ; Fall 3: Note + Inst + Vol (6 Zeichen) + Effekt-Spalte (2 Zeichen)
    mov r10d, COL_NOTE
    cmp r12d, [curPatRow]
    jne .note_col
    mov r10d, COL_WHITE
.note_col:
    mov ecx, r15d
    mov edx, r13d
    mov r8, rsi
    mov r9d, 6
    call draw_text

    mov r10d, COL_DOT
    cmp byte [rsi + 6], '.'
    je .fx_col
    mov r10d, COL_PURPHI
.fx_col:
    lea ecx, [r15 + 6]
    mov edx, r13d
    lea r8, [rsi + 6]
    mov r9d, 2
    call draw_text

.cell_next:
    inc ebx
    cmp ebx, 8
    jb .pat_ch_loop

    inc r12d
    cmp r12d, 32
    jb .pat_row_loop

    ; ------------------------------------------------------------------------
    ; 7. Fusszeile aktualisieren, Frame blitten, Events und ESC pruefen
    ; ------------------------------------------------------------------------
    mov al, [curPat]
    lea r8, [botBar + 48]
    call fmt_hex2
    lea r8, [botBar + 61]
    call fmt_hex2

    xor ecx, ecx
    mov edx, 49
    lea r8, [botBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    ; Offscreen-Puffer 1:1 ins Fenster uebertragen
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

    lea rcx, [msg]
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 1
    call PeekMessageA
    test eax, eax
    jz .no_msg
    lea rcx, [msg]
    call DispatchMessageA
.no_msg:

    mov ecx, 12
    call Sleep

    mov ecx, VK_ESCAPE
    call GetAsyncKeyState
    test ax, 0x8000
    jz main_loop

    xor ecx, ecx
    call ExitProcess