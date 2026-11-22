; 64-Bit Windows 4K-Demo (NASM, ohne CRT)
; 3D-Plasma-Tunnel mit Tiefennebel + 8-Kanal-669-Stereo-Synth + LED-VU-Meter
; Beenden ueber ESC-Taste

default rel
bits 64

; Grafik- und Fensterkonstanten
WIDTH           equ 320
HEIGHT          equ 200
W2              equ 640
H2              equ 400
LUT_SIZE        equ W2 * H2
WIN_W           equ 960
WIN_H           equ 600

; Audio-Konstanten (44100 Hz, 16-Bit Stereo, 128 Zeilen a 5120 Samples)
SAMPLE_RATE     equ 44100
STEP_SAMPLES    equ 5120
TOTAL_ROWS      equ 128
AUDIO_SAMPLES   equ TOTAL_ROWS * STEP_SAMPLES   ; 655360 Frames (ca. 14,86 s)
AUDIO_BYTES     equ AUDIO_SAMPLES * 4           ; 2 Kanaele * 2 Byte

WS_POPUP_VIS    equ 0x90000000
SRCCOPY         equ 0x00CC0020
WHDR_LOOP       equ 0x0000000C
TIME_SAMPLES    equ 2
VK_ESCAPE       equ 0x1B

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

; FPU-Konstanten fuer Sinus- und Tunnel-Lookup-Tabellen
align 4
f_2pi_256 dd 0.024543693        ; 2*pi / 256
f_127     dd 127.0
f_10      dd 10.0
f_2800    dd 2800.0
f_shade   dd 0.115              ; Skalierung fuer 16 Nebelstufen
f_128_pi  dd 40.743665          ; 128 / pi

; 12-TET Phaseninkremente fuer Oktave 2 (C-2 bis B-2 bei 44100 Hz, 32-Bit-Phase)
align 4
noteTbl:
    dd  6370030,  6748811,  7150117,  7575285   ; C-2, C#2, D-2, D#2
    dd  8025735,  8502969,  9008583,  9544261   ; E-2, F-2, F#2, G-2
    dd 10111791, 10713070, 11350103, 12025015   ; G#2, A-2, A#2, B-2

; Akkordfolge ueber 8 Takte (Format 0xOS: O=Oktave 0..3, S=Halbton 0..11)
; Je Takt: Bass-Grundton, Arp-Ton 1, Arp-Ton 2, Arp-Ton 3
chords:
    db 0x09, 0x19, 0x20, 0x24   ; Takt 0: Am
    db 0x05, 0x15, 0x19, 0x20   ; Takt 1: F
    db 0x00, 0x10, 0x14, 0x17   ; Takt 2: C
    db 0x07, 0x17, 0x1B, 0x22   ; Takt 3: G
    db 0x09, 0x19, 0x20, 0x24   ; Takt 4: Am
    db 0x05, 0x15, 0x19, 0x20   ; Takt 5: F
    db 0x02, 0x12, 0x15, 0x19   ; Takt 6: Dm
    db 0x04, 0x14, 0x18, 0x1B   ; Takt 7: E

arpSeq db 0, 1, 2, 1

; Lead-Melodie (64 Achtelnoten fuer 128 Zeilen, 0xFF = Halten/Pause)
leadPat:
    db 0x29, 0xFF, 0x24, 0xFF, 0x20, 0x24, 0x29, 0xFF
    db 0x25, 0xFF, 0x24, 0xFF, 0x20, 0xFF, 0x19, 0x20
    db 0x24, 0xFF, 0x27, 0xFF, 0x24, 0x20, 0x24, 0x27
    db 0x22, 0xFF, 0x27, 0xFF, 0x2B, 0xFF, 0x27, 0xFF
    db 0x29, 0xFF, 0x30, 0xFF, 0x2B, 0x29, 0x27, 0x24
    db 0x25, 0xFF, 0x29, 0xFF, 0x27, 0x25, 0x24, 0x20
    db 0x22, 0xFF, 0x25, 0xFF, 0x29, 0x25, 0x22, 0xFF
    db 0x24, 0xFF, 0x28, 0xFF, 0x2B, 0x28, 0x24, 0xFF

; BITMAPINFOHEADER (Top-Down 32-Bit DIB)
align 4
bmi:
    dd 40
    dd WIDTH
    dd -HEIGHT
    dw 1
    dw 32
    dd 0, 0, 0, 0, 0, 0

; WAVEFORMATEX (16-Bit Stereo, 44100 Hz)
align 2
wfx:
    dw 1                        ; WAVE_FORMAT_PCM
    dw 2                        ; Stereo
    dd SAMPLE_RATE
    dd SAMPLE_RATE * 4
    dw 4                        ; BlockAlign
    dw 16                       ; BitsPerSample
    dw 0

; ============================================================================
; Uninitialisierte Daten (0 Byte in der EXE)
; ============================================================================
section .bss

align 16
hWnd      resq 1
hDC       resq 1
hWaveOut  resq 1
waveHdr   resb 48
mmtime    resb 12
msg       resb 48

; Hilfsvariablen fuer FPU und Hauptschleife
align 4
tmpInt    resd 1
tmpX      resd 1
tmpY      resd 1
tmpF      resd 1
curRow    resd 1
curRowPos resd 1
kickPulse resd 1

; Synth-Zustandsvariablen (8 Kanaele)
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

; Lookup-Tabellen, Palette, Tracker-Pegel und Puffer
align 16
sinTbl    resd 256
shadePal  resd 16 * 256         ; 16 Helligkeitsstufen * 256 Farben
rowMeter  resb TOTAL_ROWS * 8   ; Pegelwerte pro Zeile und Kanal (0..30)
distTbl   resb LUT_SIZE
angTbl    resb LUT_SIZE
shadeTbl  resb LUT_SIZE
pixelBuf  resd WIDTH * HEIGHT
audioBuf  resd AUDIO_SAMPLES    ; 16-Bit L + 16-Bit R = 32 Bit pro Frame

; ============================================================================
; Programmcode
; ============================================================================
section .text
global start

; Hilfsfunktion: Notenbyte (AL = 0xOS) in 32-Bit-Phaseninkrement (EAX) wandeln
note_to_inc:
    movzx ecx, al
    movzx eax, al
    and eax, 0x0F
    shr ecx, 4
    lea rdx, [noteTbl]
    mov eax, [rdx + rax*4]
    shl eax, cl
    ret

; Hilfsfunktion: EAX auf 16-Bit-Signed (-32767..+32767) begrenzen
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

start:
    sub rsp, 0x78
    finit

    ; ------------------------------------------------------------------------
    ; 1. Sinus-Tabelle (256 Werte, -127..+127) vorberechnen
    ; ------------------------------------------------------------------------
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
    ; 2. Tunnel-Lookup-Tabellen (640x400: Distanz, Winkel, Nebelstufe)
    ; ------------------------------------------------------------------------
    lea r12, [distTbl]
    lea r13, [angTbl]
    lea r14, [shadeTbl]
    xor rdi, rdi
    xor ebx, ebx                ; y = 0..399
.lut_y:
    xor ecx, ecx                ; x = 0..639
.lut_x:
    lea eax, [rcx - 320]
    mov [tmpX], eax
    lea eax, [rbx - 200]
    mov [tmpY], eax

    fild dword [tmpX]
    fmul st0, st0
    fild dword [tmpY]
    fmul st0, st0
    faddp st1, st0
    fsqrt                       ; ST0 = r

    fld st0
    fadd dword [f_10]
    fstp dword [tmpF]
    fld dword [f_2800]
    fdiv dword [tmpF]
    fistp dword [tmpInt]
    mov eax, [tmpInt]
    mov [r12 + rdi], al         ; distTbl

    fmul dword [f_shade]
    fistp dword [tmpInt]
    mov eax, [tmpInt]
    cmp eax, 15
    jle .sh_max
    mov eax, 15
.sh_max:
    test eax, eax
    jns .sh_min
    xor eax, eax
.sh_min:
    mov [r14 + rdi], al         ; shadeTbl (0..15)

    fild dword [tmpY]
    fild dword [tmpX]
    fpatan                      ; atan2(dy, dx)
    fmul dword [f_128_pi]
    fistp dword [tmpInt]
    mov eax, [tmpInt]
    mov [r13 + rdi], al         ; angTbl

    inc rdi
    inc ecx
    cmp ecx, W2
    jb .lut_x
    inc ebx
    cmp ebx, H2
    jb .lut_y

    ; ------------------------------------------------------------------------
    ; 3. 8-Kanal-669-Stereo-Audio in audioBuf rendern
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

    ; Aktueller Takt (bar = row >> 4) und Akkordzeiger in RBP
    mov ebx, r13d
    shr ebx, 4
    shl ebx, 2
    lea rbp, [chords]
    add rbp, rbx

    ; Kanal 0: Kick (Zeilen 0,4,8,12 sowie Synkope auf Zeile 14 in ungeraden Takten)
    mov eax, r13d
    and eax, 3
    jz .trig_kick
    test r13d, 16               ; ungerader Takt?
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
.no_kick:

    ; Kanal 1: Snare (Zeilen 4 und 12)
    mov eax, r13d
    and eax, 7
    cmp eax, 4
    jne .no_snare
    mov dword [snarePhase], 0
    mov dword [snareEnv], 28000
    mov byte [rsi + 1], 28
.no_snare:

    ; Kanal 2: Hi-Hat (ungerade Zeilen, Betonung auf 3,7,11,15)
    test r13d, 1
    jz .no_hat
    mov eax, r13d
    and eax, 3
    cmp eax, 3
    jne .hat_soft
    mov dword [hatEnv], 19000
    mov byte [rsi + 2], 22
    jmp .no_hat
.hat_soft:
    mov dword [hatEnv], 10000
    mov byte [rsi + 2], 14
.no_hat:

    ; Kanal 3: 16tel-Oktav-Bass
    mov al, [rbp + 0]
    test r13d, 1
    jz .bass_low
    add al, 0x10                ; eine Oktave hoeher
    mov byte [rsi + 3], 25
    jmp .bass_set
.bass_low:
    mov byte [rsi + 3], 21
.bass_set:
    call note_to_inc
    mov [bassInc], eax
    mov dword [bassEnv], 27000

    ; Kanal 4: 16tel-Arpeggio
    mov eax, r13d
    and eax, 3
    lea rdx, [arpSeq]
    movzx eax, byte [rdx + rax]
    mov al, [rbp + 1 + rax]
    call note_to_inc
    mov [arpInc], eax
    mov dword [arpEnv], 20000
    mov byte [rsi + 4], 19

    ; Kanal 5: Lead-Melodie (auf geraden Zeilen)
    test r13d, 1
    jnz .no_lead
    mov eax, r13d
    shr eax, 1
    lea rdx, [leadPat]
    mov al, [rdx + rax]
    cmp al, 0xFF
    je .no_lead
    call note_to_inc
    mov [leadInc], eax
    mov dword [leadEnv], 24000
    mov byte [rsi + 5], 27
.no_lead:

    ; Kanal 6: Tracker-Echo der Lead-Melodie (3 Zeilen verzoegert)
    lea eax, [r13 - 3]
    and eax, 127
    test eax, 1
    jnz .no_echo
    shr eax, 1
    lea rdx, [leadPat]
    mov al, [rdx + rax]
    cmp al, 0xFF
    je .no_echo
    call note_to_inc
    mov [echoInc], eax
    mov dword [echoEnv], 12500
    mov byte [rsi + 6], 16
.no_echo:

    ; Kanal 7: Pad-Akkord (Grundton + Quinte pro Takt)
    mov eax, r13d
    and eax, 15
    jnz .pad_hold
    mov al, [rbp + 1]
    call note_to_inc
    mov [padInc1], eax
    mov al, [rbp + 3]
    call note_to_inc
    mov [padInc2], eax
    mov byte [rsi + 7], 18
    jmp .pad_done
.pad_hold:
    mov byte [rsi + 7], 13
.pad_done:

    ; Innere Sample-Schleife fuer aktuelle Zeile (5120 Stereo-Frames)
    xor r14d, r14d
.sample_loop:
    xor r8d, r8d                ; L-Akkumulator
    xor r9d, r9d                ; R-Akkumulator

    ; Xorshift32-Rauschen aktualisieren (EBX = -128..+127)
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

    ; Kanal 0: Kick (Mitte)
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

    ; Kanal 1: Snare (Rechts betont)
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
    add r8d, edx                ; 50% L
    add r9d, eax                ; 100% R
    mov edx, ecx
    shr edx, 11
    sub ecx, edx
    mov [snareEnv], ecx
.s_snare_done:

    ; Kanal 2: Hi-Hat (Links betont, 1. Ordnung Highpass)
    mov ecx, [hatEnv]
    test ecx, ecx
    jz .s_hat_done
    mov eax, ebx
    sub eax, [prevNoise]
    imul eax, ecx
    sar eax, 10
    add r8d, eax                ; 100% L
    sar eax, 1
    add r9d, eax                ; 50% R
    mov edx, ecx
    shr edx, 9
    sub ecx, edx
    mov [hatEnv], ecx
.s_hat_done:
    mov [prevNoise], ebx

    ; Kanal 3: Tiefpassgefilterter Saegezahn-Bass (Mitte/Leicht L)
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

    ; Kanal 4: 25%-Puls-Arpeggio (Links betont)
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

    ; Globales Vibrato fuer Lead fortschalten
    add dword [vibPhase], 550000
    mov eax, [vibPhase]
    shr eax, 24
    mov eax, [r15 + rax*4]
    imul eax, 750

    ; Kanal 5: Lead mit Vibrato (Rechts betont)
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

    ; Kanal 6: Tracker-Echo (Links betont)
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

    ; Kanal 7: Zweistimmiges Sinus-Pad (Stereo-Breite)
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

    ; Stereo-Sample auf 16 Bit begrenzen und speichern
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
    ; 4. WinMM Stereo-Ausgabe starten
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
    ; 5. Fenster erzeugen und DC holen
    ; ------------------------------------------------------------------------
    xor ecx, ecx
    lea rdx, [clsName]
    xor r8d, r8d
    mov r9d, WS_POPUP_VIS
    mov qword [rsp + 0x20], 120
    mov qword [rsp + 0x28], 80
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

    ; ------------------------------------------------------------------------
    ; 6. Hauptschleife: Audio-Sync, Palette, 3D-Tunnel, 8-Kanal-HUD
    ; ------------------------------------------------------------------------
main_loop:
    ; Sample-Position abfragen und in aktuelle Zeile + Zeilenposition umrechnen
    mov dword [mmtime], TIME_SAMPLES
    mov rcx, [hWaveOut]
    lea rdx, [mmtime]
    mov r8d, 12
    call waveOutGetPosition
    mov eax, [mmtime + 4]
    mov r14d, eax               ; R14D = fortlaufende Sample-Zeit

    xor edx, edx
    mov ecx, AUDIO_SAMPLES
    div ecx                     ; EDX = Position innerhalb des Loops
    mov eax, edx
    xor edx, edx
    mov ecx, STEP_SAMPLES
    div ecx                     ; EAX = curRow (0..127), EDX = curRowPos (0..5119)
    mov [curRow], eax
    mov [curRowPos], edx

    ; Kick-Impuls aus Kanal 0 ableiten (0..48)
    shl eax, 3
    lea rcx, [rowMeter]
    cmp byte [rcx + rax], 0
    je .no_k_flash
    mov eax, edx
    shr eax, 6
    mov ecx, 48
    sub ecx, eax
    jns .k_store
.no_k_flash:
    xor ecx, ecx
.k_store:
    mov [kickPulse], ecx

    ; Animationszeit t in R8D
    mov r8d, r14d
    shr r8d, 9
    lea r15, [sinTbl]

    ; 256-Farben-Palette fuer alle 16 Tiefennebel-Stufen aktualisieren
    lea rdi, [shadePal]
    mov r10d, [kickPulse]
    xor ecx, ecx                ; Farbindex i = 0..255
.pal_loop:
    ; Rotkanal
    lea eax, [rcx*2 + r8]
    and eax, 255
    mov r9d, [r15 + rax*4]
    add r9d, 127
    add r9d, r10d
    cmp r9d, 255
    jle .r_ok
    mov r9d, 255
.r_ok:

    ; Gruenkanal
    lea eax, [rcx + r8*2 + 85]
    and eax, 255
    mov r11d, [r15 + rax*4]
    add r11d, 127
    mov eax, r10d
    shr eax, 1
    add r11d, eax
    cmp r11d, 255
    jle .g_ok
    mov r11d, 255
.g_ok:

    ; Blaukanal
    lea eax, [rcx + r8 + 170]
    and eax, 255
    mov esi, [r15 + rax*4]
    add esi, 165
    cmp esi, 255
    jle .b_ok
    mov esi, 255
.b_ok:

    ; 16 Helligkeitsstufen (0 = Schwarz im Zentrum, 15 = voll)
    xor ebx, ebx
.shade_loop:
    mov eax, r9d
    imul eax, ebx
    shr eax, 4
    shl eax, 16

    mov edx, r11d
    imul edx, ebx
    shr edx, 4
    shl edx, 8
    or eax, edx

    mov edx, esi
    imul edx, ebx
    shr edx, 4
    or eax, edx

    mov edx, ebx
    shl edx, 8
    or edx, ecx
    mov [rdi + rdx*4], eax

    inc ebx
    cmp ebx, 16
    jb .shade_loop

    inc ecx
    cmp ecx, 256
    jb .pal_loop

    ; Tunnel-Kamerabewegung (ox, oy) innerhalb der 640x400-LUT
    mov eax, r8d
    shr eax, 1
    and eax, 255
    mov eax, [r15 + rax*4]
    sar eax, 1
    add eax, 160                ; ox (96..223)
    mov r9d, eax

    mov eax, r8d
    imul eax, 3
    shr eax, 2
    add eax, 64
    and eax, 255
    mov eax, [r15 + rax*4]
    sar eax, 2
    add eax, 100                ; oy (68..131)

    ; Startoffset in 640x400-LUT: rsi = oy * 640 + ox
    imul eax, W2
    add eax, r9d
    movsxd rsi, eax

    ; Tunnel-Rotation (r9d) und Tiefe mit Beat-Zoom (r10d)
    mov eax, r8d
    shr eax, 1
    and eax, 255
    mov r9d, [r15 + rax*4]
    sar r9d, 2
    add r9d, r8d                ; rot

    mov r10d, r8d
    imul r10d, 3
    mov eax, [kickPulse]
    shr eax, 1
    add r10d, eax               ; zOff

    ; 3D-Tunnel in pixelBuf rendern
    lea rdi, [pixelBuf]
    lea r12, [angTbl]
    lea r13, [distTbl]
    lea r14, [shadeTbl]
    lea rbp, [shadePal]

    mov r11d, HEIGHT
.tun_y:
    mov ebx, WIDTH
.tun_x:
    movzx eax, byte [r12 + rsi]
    add eax, r9d
    and eax, 255                ; u

    movzx edx, byte [r13 + rsi]
    add edx, r10d
    and edx, 255                ; v

    mov ecx, eax
    add ecx, edx
    add ecx, r8d
    and ecx, 255
    mov ecx, [r15 + rcx*4]
    sar ecx, 2                  ; Plasma-Modulation

    xor eax, edx
    add eax, ecx
    and eax, 255                ; Farbindex 0..255

    movzx edx, byte [r14 + rsi] ; Nebelstufe 0..15
    shl edx, 8
    or eax, edx

    mov eax, [rbp + rax*4]
    mov [rdi], eax

    add rdi, 4
    inc rsi
    dec ebx
    jnz .tun_x

    add rsi, W2 - WIDTH
    dec r11d
    jnz .tun_y

    ; ------------------------------------------------------------------------
    ; 7. Widescreen-Rasterbalken und halbtransparentes 8-Kanal-669-HUD
    ; ------------------------------------------------------------------------
    ; Oberer Streifen (Zeilen 0..7 abdunkeln)
    lea rdi, [pixelBuf]
    mov ecx, WIDTH * 8
.top_dim:
    mov eax, [rdi]
    shr eax, 2
    and eax, 0x003F3F3F
    mov [rdi], eax
    add rdi, 4
    dec ecx
    jnz .top_dim

    ; Obere Leuchtlinie (Zeilen 8..9)
    mov eax, 0x0066DDFF
    mov ecx, WIDTH
    rep stosd
    mov eax, 0x00184466
    mov ecx, WIDTH
    rep stosd

    ; Unterer HUD-Bereich (Zeilen 160..199)
    lea rdi, [pixelBuf + 160 * WIDTH * 4]
    mov eax, 0x00184466
    mov ecx, WIDTH
    rep stosd
    mov eax, 0x0066DDFF
    mov ecx, WIDTH
    rep stosd

    mov ecx, WIDTH * 38
.bot_dim:
    mov eax, [rdi]
    shr eax, 2
    and eax, 0x003F3F3F
    mov [rdi], eax
    add rdi, 4
    dec ecx
    jnz .bot_dim

    ; 8 Kanal-Balken zeichnen (C0..C7, L=Cyan, R=Magenta)
    mov eax, [curRow]
    shl eax, 3
    lea r12, [rowMeter]
    add r12, rax
    xor ebx, ebx                ; Kanal c = 0..7
.bar_ch:
    movzx r9d, byte [r12 + rbx] ; Startpegel (0..30)
    mov eax, [curRowPos]
    cmp ebx, 7
    je .pad_dec
    shr eax, 8                  ; Schneller Abfall fuer Trigger-Kanaele
    jmp .apply_dec
.pad_dec:
    shr eax, 9                  ; Langsamer Abfall fuer Pad
.apply_dec:
    sub r9d, eax
    cmp r9d, 2
    jge .h_ok
    mov r9d, 2
.h_ok:

    ; Balken von unten (y = 196) nach oben mit LED-Scanlines zeichnen
    xor r10d, r10d              ; Segment k = 0..h-1
.bar_seg:
    mov eax, 196
    sub eax, r10d               ; y
    test eax, 1
    jnz .seg_skip               ; jede 2. Zeile dunkel (LED-Optik)

    ; Zeilenstartadresse in pixelBuf berechnen
    imul eax, WIDTH
    mov ecx, ebx
    imul ecx, 38
    add ecx, 11
    add eax, ecx
    lea rdi, [pixelBuf]
    lea rdi, [rdi + rax*4]

    ; Farbverlauf je nach Kanal-Panning (gerade = Cyan, ungerade = Magenta)
    test ebx, 1
    jnz .col_right
    mov eax, r10d
    shl eax, 3                  ; R = k * 8
    shl eax, 16
    mov edx, r10d
    shl edx, 1
    add edx, 180                ; G = 180 + k * 2
    shl edx, 8
    or eax, edx
    or eax, 0x000000FF          ; B = 255
    jmp .draw_seg
.col_right:
    mov eax, 0x00FF0000         ; R = 255
    mov edx, r10d
    imul edx, 7
    add edx, 40                 ; G = 40 + k * 7
    shl edx, 8
    or eax, edx
    mov edx, r10d
    shl edx, 2
    mov ecx, 180
    sub ecx, edx                ; B = 180 - k * 4
    or eax, ecx

.draw_seg:
    mov ecx, 32                 ; Balkenbreite 32 Pixel
    rep stosd

.seg_skip:
    inc r10d
    cmp r10d, r9d
    jb .bar_seg

    inc ebx
    cmp ebx, 8
    jb .bar_ch

    ; ------------------------------------------------------------------------
    ; 8. Frame blitten, Events abarbeiten, ESC pruefen
    ; ------------------------------------------------------------------------
    mov rcx, [hDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, WIN_W
    mov qword [rsp + 0x20], WIN_H
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], WIDTH
    mov qword [rsp + 0x40], HEIGHT
    lea rax, [pixelBuf]
    mov [rsp + 0x48], rax
    lea rax, [bmi]
    mov [rsp + 0x50], rax
    mov qword [rsp + 0x58], 0
    mov qword [rsp + 0x60], SRCCOPY
    call StretchDIBits

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

    mov ecx, 10
    call Sleep

    mov ecx, VK_ESCAPE
    call GetAsyncKeyState
    test ax, 0x8000
    jz main_loop

    xor ecx, ecx
    call ExitProcess