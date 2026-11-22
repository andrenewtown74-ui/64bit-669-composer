; Sample- und Instrument-Editor (Wellenform, waveIn-Aufnahme, Effekte, ADSR-Synth-Designer)

; Wert EAX (0..999) als 3 rechtsbuendige Dezimalzeichen nach [R8] schreiben
fmt_dec3:
    mov byte [r8 + 0], ' '
    mov byte [r8 + 1], ' '
    xor edx, edx
    mov ecx, 100
    div ecx
    test eax, eax
    jz .fd_tens
    add al, '0'
    mov [r8 + 0], al
.fd_tens:
    mov eax, edx
    xor edx, edx
    mov ecx, 10
    div ecx
    cmp byte [r8 + 0], ' '
    jne .fd_t_write
    test eax, eax
    jz .fd_ones
.fd_t_write:
    add al, '0'
    mov [r8 + 1], al
.fd_ones:
    add dl, '0'
    mov [r8 + 2], dl
    ret

; Standardwerte fuer ADSR-Synthesizer beim Programmstart setzen
init_samped_defaults:
    mov dword [seLastSlot], -1
    mov dword [seLastLen], -1
    mov dword [seWave], 1               ; Saegezahn
    mov dword [sePulse], 50             ; 50% Pulsbreite
    mov dword [seParams + 0*4], 4       ; Attack
    mov dword [seParams + 1*4], 32      ; Decay
    mov dword [seParams + 2*4], 60      ; Sustain
    mov dword [seParams + 3*4], 24      ; Release
    mov dword [seParams + 4*4], 35      ; Dauer (~0,7 s)
    mov dword [seParams + 5*4], 50      ; Sweep (50 = neutral)
    mov dword [seParams + 6*4], 18      ; Detune / Chorus
    mov dword [seParams + 7*4], 80      ; Tiefpass-Filter
    ret

; Auswahlmarker bei Slot- oder Laengenaenderung automatisch auf 0..len setzen
samped_sync_sel:
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rax*4]
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    jne .sss_chk
    xor ecx, ecx
.sss_chk:
    cmp eax, [seLastSlot]
    jne .sss_reset
    cmp ecx, [seLastLen]
    jne .sss_reset
    cmp [seSelEnd], ecx
    jbe .sss_ord
.sss_reset:
    mov [seLastSlot], eax
    mov [seLastLen], ecx
    mov dword [seSelStart], 0
    mov [seSelEnd], ecx
    ret
.sss_ord:
    mov edx, [seSelStart]
    cmp edx, [seSelEnd]
    jbe .sss_done
    mov [seSelStart], ecx
    mov [seSelEnd], edx
.sss_done:
    ret

; Aktiven Sample-Slot einmal auf Note AL (z. B. 12 = C-3) vorhoeren
samped_preview:
    sub rsp, 0x28
    mov [previewCell + 0], al
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    mov [previewCell + 1], al
    mov eax, [curVol]
    and eax, 15
    mov [previewCell + 2], al
    mov byte [previewCell + 3], 0xFF
    mov ecx, [curCh]
    lea rdx, [previewCell]
    mov r8d, 1
    call trigger_channel
    add rsp, 0x28
    ret

; ============================================================================
; Audio-Aufnahme ueber WinMM waveIn (22050 Hz, 8-Bit Mono, max. 128 KB)
; ============================================================================
start_recording:
    push rbx
    sub rsp, 0x40

    cmp dword [isRecording], 0
    je .sr_open
    call stop_recording
    jmp .sr_done

.sr_open:
    call silence_audio

    lea rcx, [hWaveIn]
    mov edx, -1                         ; WAVE_MAPPER
    lea r8, [wfxRec]
    xor r9d, r9d
    mov qword [rsp + 0x20], 0
    mov qword [rsp + 0x28], 0
    call waveInOpen
    test eax, eax
    jnz .sr_err

    ; WAVEHDR leeren und Puffer eintragen
    lea rdi, [recHdr]
    xor eax, eax
    mov ecx, 48
    rep stosb

    lea rax, [recBuf]
    mov [recHdr + 0], rax
    mov dword [recHdr + 8], MAX_SAMP_BYTES

    mov rcx, [hWaveIn]
    lea rdx, [recHdr]
    mov r8d, 48
    call waveInPrepareHeader
    test eax, eax
    jnz .sr_close_err

    mov rcx, [hWaveIn]
    lea rdx, [recHdr]
    mov r8d, 48
    call waveInAddBuffer
    test eax, eax
    jnz .sr_close_err

    mov rcx, [hWaveIn]
    call waveInStart
    test eax, eax
    jnz .sr_close_err

    call GetTickCount
    mov [recStartTick], eax
    mov dword [isRecording], 1
    lea rcx, [stRecOn]
    call set_status
    mov dword [dirty], 1
    jmp .sr_done

.sr_close_err:
    mov rcx, [hWaveIn]
    call waveInClose
    mov qword [hWaveIn], 0
.sr_err:
    lea rcx, [stRecErr]
    call set_status
    mov dword [dirty], 1

.sr_done:
    add rsp, 0x40
    pop rbx
    ret

; Laufende Aufnahme stoppen und Ergebnis in aktiven Slot (curInst) uebernehmen
stop_recording:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x30

    cmp dword [isRecording], 0
    je .st_done
    mov dword [isRecording], 0

    mov rcx, [hWaveIn]
    test rcx, rcx
    jz .st_done

    call waveInStop
    mov rcx, [hWaveIn]
    call waveInReset
    mov rcx, [hWaveIn]
    lea rdx, [recHdr]
    mov r8d, 48
    call waveInUnprepareHeader
    mov rcx, [hWaveIn]
    call waveInClose
    mov qword [hWaveIn], 0

    mov ebx, [recHdr + 12]              ; dwBytesRecorded
    test ebx, ebx
    jz .st_empty
    cmp ebx, MAX_SAMP_BYTES
    jbe .st_len_ok
    mov ebx, MAX_SAMP_BYTES
.st_len_ok:

    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    mov r10d, eax
    shl eax, 17
    lea rdi, [pcmSlots]
    add rdi, rax
    lea rsi, [recBuf]
    mov ecx, ebx
    rep movsb

    lea rdx, [sampHasPcm]
    mov byte [rdx + r10], 1
    lea rdx, [sampLenTbl]
    mov [rdx + r10*4], ebx
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + r10*4], 0
    lea rdx, [sampLoopEndTbl]
    mov dword [rdx + r10*4], 0xFFFFF
    lea rdx, [sampC2Step]
    mov dword [rdx + r10*4], 16384      ; (22050 << 15) / 44100 fuer C-3 Originalhoehe

    ; Slot-Namen "REC XX 22kHz" setzen
    mov eax, r10d
    imul eax, 12
    lea rdi, [sampNames]
    add rdi, rax
    lea rsi, [txtRecName]
    mov ecx, 12
    rep movsb
    mov eax, r10d
    imul eax, 12
    lea r8, [sampNames + 4]
    add r8, rax
    mov al, r10b
    call fmt_hex2

    mov dword [seLastSlot], -1
    call samped_sync_sel
    lea rcx, [stRecOk]
    call set_status
    mov al, 12
    call samped_preview
    mov dword [dirty], 1
    jmp .st_done

.st_empty:
    lea rcx, [stRecErr]
    call set_status
    mov dword [dirty], 1

.st_done:
    add rsp, 0x30
    pop rdi
    pop rsi
    pop rbx
    ret

; In der Hauptschleife pruefen, ob der 128-KB-Aufnahmepuffer voll ist
poll_recording:
    sub rsp, 0x28
    cmp dword [isRecording], 0
    je .pr_done
    test dword [recHdr + 24], WHDR_DONE
    jz .pr_redraw
    call stop_recording
    jmp .pr_done
.pr_redraw:
    mov dword [dirty], 1
.pr_done:
    add rsp, 0x28
    ret

; ============================================================================
; Sample-Effekte und Schnittfunktionen auf aktiven Slot / Auswahlbereich
; ============================================================================

; Aktiven Bereich ermitteln: RAX = Slot-Ptr (oder 0), ECX = Start, EDX = Anzahl
se_get_active_range:
    call samped_sync_sel
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea r8, [sampHasPcm]
    cmp byte [r8 + rax], 0
    je .sg_none
    lea r8, [sampLenTbl]
    mov r9d, [r8 + rax*4]
    test r9d, r9d
    jz .sg_none

    mov ecx, [seSelStart]
    mov edx, [seSelEnd]
    cmp edx, r9d
    jbe .sg_end_ok
    mov edx, r9d
.sg_end_ok:
    cmp ecx, edx
    jb .sg_cnt
    xor ecx, ecx
    mov edx, r9d
.sg_cnt:
    sub edx, ecx
    shl eax, 17
    lea r8, [pcmSlots]
    add rax, r8
    ret
.sg_none:
    xor eax, eax
    xor ecx, ecx
    xor edx, edx
    ret

; Auswahl auf volle 8-Bit-Aussteuerung normalisieren
se_fx_normalize:
    push rbx
    push rsi
    push r12
    sub rsp, 0x20

    call se_get_active_range
    test rax, rax
    jz .fn_done
    lea rsi, [rax + rcx]
    mov r12d, edx

    xor ebx, ebx                        ; maxAbs
    xor ecx, ecx
.fn_scan:
    movzx eax, byte [rsi + rcx]
    sub eax, 128
    jns .fn_pos
    neg eax
.fn_pos:
    cmp eax, ebx
    jbe .fn_snext
    mov ebx, eax
.fn_snext:
    inc ecx
    cmp ecx, r12d
    jb .fn_scan

    test ebx, ebx
    jz .fn_done
    cmp ebx, 127
    jae .fn_ok

    xor ecx, ecx
.fn_apply:
    movzx eax, byte [rsi + rcx]
    sub eax, 128
    imul eax, 127
    cdq
    idiv ebx
    add eax, 128
    cmp eax, 0
    jge .fn_lo
    xor eax, eax
.fn_lo:
    cmp eax, 255
    jle .fn_st
    mov eax, 255
.fn_st:
    mov [rsi + rcx], al
    inc ecx
    cmp ecx, r12d
    jb .fn_apply

.fn_ok:
    lea rcx, [stFxNorm]
    call set_status
    mov dword [dirty], 1

.fn_done:
    add rsp, 0x20
    pop r12
    pop rsi
    pop rbx
    ret

; Linearer Fade-In ueber die Auswahl
se_fx_fade_in:
    push rbx
    push rsi
    push r12
    sub rsp, 0x20

    call se_get_active_range
    test rax, rax
    jz .fi_done
    cmp edx, 2
    jb .fi_done
    lea rsi, [rax + rcx]
    mov r12d, edx

    xor ebx, ebx
.fi_loop:
    movzx eax, byte [rsi + rbx]
    sub eax, 128
    imul eax, ebx
    cdq
    idiv r12d
    add eax, 128
    mov [rsi + rbx], al
    inc ebx
    cmp ebx, r12d
    jb .fi_loop

    lea rcx, [stFxFi]
    call set_status
    mov dword [dirty], 1

.fi_done:
    add rsp, 0x20
    pop r12
    pop rsi
    pop rbx
    ret

; Linearer Fade-Out ueber die Auswahl
se_fx_fade_out:
    push rbx
    push rsi
    push r12
    sub rsp, 0x20

    call se_get_active_range
    test rax, rax
    jz .fo_done
    cmp edx, 2
    jb .fo_done
    lea rsi, [rax + rcx]
    mov r12d, edx

    xor ebx, ebx
.fo_loop:
    movzx eax, byte [rsi + rbx]
    sub eax, 128
    mov ecx, r12d
    dec ecx
    sub ecx, ebx
    imul eax, ecx
    cdq
    idiv r12d
    add eax, 128
    mov [rsi + rbx], al
    inc ebx
    cmp ebx, r12d
    jb .fo_loop

    lea rcx, [stFxFo]
    call set_status
    mov dword [dirty], 1

.fo_done:
    add rsp, 0x20
    pop r12
    pop rsi
    pop rbx
    ret

; Auswahl zeitlich umkehren (Reverse)
se_fx_reverse:
    push rsi
    sub rsp, 0x20

    call se_get_active_range
    test rax, rax
    jz .fr_done
    cmp edx, 2
    jb .fr_done
    lea rsi, [rax + rcx]
    xor ecx, ecx
    lea r8d, [rdx - 1]
.fr_loop:
    cmp ecx, r8d
    jae .fr_ok
    mov al, [rsi + rcx]
    mov dl, [rsi + r8]
    mov [rsi + rcx], dl
    mov [rsi + r8], al
    inc ecx
    dec r8d
    jmp .fr_loop

.fr_ok:
    lea rcx, [stFxRev]
    call set_status
    mov dword [dirty], 1

.fr_done:
    add rsp, 0x20
    pop rsi
    ret

; Pegel der Auswahl um +50% anheben (mit Begrenzung)
se_fx_boost:
    push rsi
    sub rsp, 0x20

    call se_get_active_range
    test rax, rax
    jz .fb_done
    lea rsi, [rax + rcx]
    xor ecx, ecx
.fb_loop:
    movzx eax, byte [rsi + rcx]
    sub eax, 128
    mov r8d, eax
    sar r8d, 1
    add eax, r8d
    cmp eax, 127
    jle .fb_lo
    mov eax, 127
.fb_lo:
    cmp eax, -128
    jge .fb_st
    mov eax, -128
.fb_st:
    add eax, 128
    mov [rsi + rcx], al
    inc ecx
    cmp ecx, edx
    jb .fb_loop

    lea rcx, [stFxBst]
    call set_status
    mov dword [dirty], 1

.fb_done:
    add rsp, 0x20
    pop rsi
    ret

; Gesamtes Sample durch Dezimierung (2:1) halbieren (Lo-Fi / +1 Oktave)
se_fx_halve:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .fh_done
    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rbx*4]
    cmp ecx, 64
    jb .fh_done

    shr ecx, 1
    mov eax, ebx
    shl eax, 17
    lea rsi, [pcmSlots]
    add rsi, rax
    mov rdi, rsi

    xor r8d, r8d
.fh_loop:
    mov al, [rsi + r8*2]
    mov [rdi + r8], al
    inc r8d
    cmp r8d, ecx
    jb .fh_loop

    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], ecx
    lea rdx, [sampLoopStartTbl]
    shr dword [rdx + rbx*4], 1
    lea rdx, [sampLoopEndTbl]
    cmp dword [rdx + rbx*4], 0xFFFFF
    jae .fh_fin
    shr dword [rdx + rbx*4], 1
.fh_fin:
    mov dword [seLastSlot], -1
    call samped_sync_sel
    lea rcx, [stFxHlf]
    call set_status
    mov dword [dirty], 1

.fh_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; Sample auf den ausgewaehlten Bereich zuschneiden (Trim)
se_fx_trim:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    call se_get_active_range
    test rax, rax
    jz .ft_done
    cmp edx, 32
    jb .ft_done

    mov rdi, rax
    lea rsi, [rax + rcx]
    mov ebx, edx                        ; neue Laenge
    test ecx, ecx
    jz .ft_upd
    mov ecx, ebx
    rep movsb

.ft_upd:
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [sampLenTbl]
    mov [rdx + rax*4], ebx
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rax*4], 0
    lea rdx, [sampLoopEndTbl]
    cmp dword [rdx + rax*4], 0xFFFFF
    jae .ft_fin
    mov [rdx + rax*4], ebx
.ft_fin:
    mov dword [seLastSlot], -1
    call samped_sync_sel
    lea rcx, [stFxTrm]
    call set_status
    mov dword [dirty], 1

.ft_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; ADSR-Synthesizer-Generator -> rendert 8-Bit-PCM in curInst (ECX = autoLoop)
; ============================================================================

; Oszillator-Sample fuer Phase EAX, Welle R8D (0..4), Puls-Schwelle R9D -> EAX (-128..127)
synth_osc_eval:
    cmp r8d, 0
    je .so_sin
    cmp r8d, 1
    je .so_saw
    cmp r8d, 2
    je .so_pls
    cmp r8d, 3
    je .so_tri
    ; Rauschen ueber lokalen LFSR in [rsp + 0x68] (Aufrufer-Frame)
    mov eax, [seSynthLfsr]
    mov edx, eax
    shl eax, 13
    xor eax, edx
    mov edx, eax
    shr eax, 17
    xor eax, edx
    mov edx, eax
    shl eax, 5
    xor eax, edx
    mov [seSynthLfsr], eax
    movsx eax, al
    ret
.so_sin:
    shr eax, 24
    lea r10, [sinTbl]
    mov eax, [r10 + rax*4]
    ret
.so_saw:
    sar eax, 24
    ret
.so_pls:
    shr eax, 24
    cmp eax, r9d
    jb .so_phi
    mov eax, -110
    ret
.so_phi:
    mov eax, 110
    ret
.so_tri:
    shr eax, 24
    cmp eax, 128
    jb .so_tup
    mov edx, 383
    sub edx, eax
    mov eax, edx
    sub eax, 128
    ret
.so_tup:
    shl eax, 1
    sub eax, 128
    ret

synth_render_to_slot:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x68

    mov [rsp + 0x20], ecx               ; autoLoop (0 oder 1)
    mov dword [seSynthLfsr], 0x5EED1234

    ; Gesamtdauer N = seDur * 441 (bei 22050 Hz: 2205..44100 Samples = 0,1..2,0 s)
    mov eax, [seParams + 4*4]
    cmp eax, 5
    jae .sr_d_ok
    mov eax, 5
.sr_d_ok:
    imul eax, 441
    cmp eax, MAX_SAMP_BYTES
    jbe .sr_n_ok
    mov eax, MAX_SAMP_BYTES
.sr_n_ok:
    mov [rsp + 0x24], eax               ; totalN
    mov r12d, eax

    ; ADSR-Phasenlaengen berechnen (nA, nD, nR jeweils max. 40% von N)
    mov eax, r12d
    imul eax, [seParams + 0*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x28], eax               ; nA

    mov eax, r12d
    imul eax, [seParams + 1*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x2C], eax               ; nD

    mov eax, r12d
    imul eax, [seParams + 3*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x34], eax               ; nR

    ; Sicherstellen, dass nA + nD + nR <= N
    mov eax, [rsp + 0x28]
    add eax, [rsp + 0x2C]
    add eax, [rsp + 0x34]
    cmp eax, r12d
    jbe .sr_ns_calc
    mov ecx, r12d
    sub ecx, [rsp + 0x28]
    sub ecx, [rsp + 0x2C]
    jns .sr_nr_fix
    xor ecx, ecx
.sr_nr_fix:
    mov [rsp + 0x34], ecx
.sr_ns_calc:
    mov eax, r12d
    sub eax, [rsp + 0x28]
    sub eax, [rsp + 0x2C]
    sub eax, [rsp + 0x34]
    jns .sr_ns_ok
    xor eax, eax
.sr_ns_ok:
    mov [rsp + 0x30], eax               ; nS
    mov ecx, [rsp + 0x28]
    add ecx, [rsp + 0x2C]
    add ecx, eax
    mov [rsp + 0x5C], ecx               ; rStart = nA + nD + nS

    ; Sustain-Pegel (0..32767)
    mov eax, [seParams + 2*4]
    imul eax, 32767
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x38], eax               ; sustLvl

    ; Basis-Phaseninkrement fuer C-3 (130,8128 Hz bei 22050 Hz = 25478490)
    mov eax, 25478490
    mov [rsp + 0x40], eax               ; baseInc
    mov [rsp + 0x3C], eax               ; curInc

    ; Start-Offset bei Abwaerts-Sweep (seSweep < 50)
    mov ecx, [seParams + 5*4]
    cmp ecx, 50
    jae .sr_sw_ok
    mov edx, 50
    sub edx, ecx
    mov eax, [rsp + 0x40]
    imul rax, rdx
    xor edx, edx
    mov r8d, 10
    div r8
    add eax, [rsp + 0x40]
    mov [rsp + 0x3C], eax
.sr_sw_ok:

    ; Tiefpass-Alpha (12..256)
    mov eax, [seParams + 7*4]
    cmp eax, 5
    jae .sr_lp_ok
    mov eax, 5
.sr_lp_ok:
    shl eax, 8
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x50], eax               ; lpAlpha

    ; Puls-Schwelle (25..230)
    mov eax, [sePulse]
    imul eax, 255
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x54], eax               ; pulseThr

    mov dword [rsp + 0x44], 0           ; ph1
    mov dword [rsp + 0x48], 0           ; ph2
    mov dword [rsp + 0x4C], 0           ; lpState

    ; Zielzeiger in pcmSlots[curInst]
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    shl eax, 17
    lea rdi, [pcmSlots]
    add rdi, rax

    mov r8d, [seWave]
    mov r9d, [rsp + 0x54]
    xor ebx, ebx                        ; Sample-Zaehler i = 0..N-1

.sr_loop:
    ; 1. Pitch-Sweep aktualisieren
    mov ecx, [seParams + 5*4]
    cmp ecx, 50
    je .sr_sw_done
    jb .sr_sw_down
    ; Aufwaerts-Sweep
    sub ecx, 50
    imul eax, ecx, 240
    add [rsp + 0x3C], eax
    cmp dword [rsp + 0x3C], 0x08000000
    jbe .sr_sw_done
    mov dword [rsp + 0x3C], 0x08000000
    jmp .sr_sw_done
.sr_sw_down:
    ; Exponentieller Abwaerts-Sweep Richtung baseInc
    mov eax, [rsp + 0x3C]
    sub eax, [rsp + 0x40]
    jle .sr_sw_done
    shr eax, 10
    test eax, eax
    jnz .sr_sw_sub
    mov eax, 1
.sr_sw_sub:
    sub [rsp + 0x3C], eax
.sr_sw_done:

    ; 2. Oszillator 1 (+ optional verstimmter Oszillator 2)
    mov eax, [rsp + 0x3C]
    add [rsp + 0x44], eax
    mov eax, [rsp + 0x44]
    call synth_osc_eval
    mov r13d, eax                       ; osc1 (-128..127)

    mov ecx, [seParams + 6*4]           ; Detune
    test ecx, ecx
    jz .sr_no_det
    cmp r8d, 4                          ; Rauschen nicht doppeln
    je .sr_no_det
    mov eax, [rsp + 0x3C]
    imul rax, rcx
    shr rax, 11
    add eax, [rsp + 0x3C]
    add [rsp + 0x48], eax
    mov eax, [rsp + 0x48]
    call synth_osc_eval
    add r13d, eax
    sar r13d, 1
.sr_no_det:

    ; 3. Einpoliger Tiefpassfilter (16-Bit intern fuer feine Aufloesung)
    shl r13d, 8
    mov eax, r13d
    sub eax, [rsp + 0x4C]
    imul eax, [rsp + 0x50]
    sar eax, 8
    add [rsp + 0x4C], eax
    mov r13d, [rsp + 0x4C]
    sar r13d, 8

    ; 4. ADSR-Huellkurve (r14d = 0..32767)
    mov ecx, [rsp + 0x28]               ; nA
    cmp ebx, ecx
    jae .sr_chk_d
    mov eax, ebx
    imul eax, 32767
    xor edx, edx
    div ecx
    mov r14d, eax
    jmp .sr_env_ok

.sr_chk_d:
    mov r15d, ebx
    sub r15d, ecx                       ; i - nA
    mov ecx, [rsp + 0x2C]               ; nD
    cmp r15d, ecx
    jae .sr_chk_s
    mov eax, 32767
    sub eax, [rsp + 0x38]
    imul eax, r15d
    xor edx, edx
    div ecx
    mov r14d, 32767
    sub r14d, eax
    jmp .sr_env_ok

.sr_chk_s:
    mov r15d, [rsp + 0x5C]              ; rStart
    cmp ebx, r15d
    jae .sr_chk_r
    mov r14d, [rsp + 0x38]
    jmp .sr_env_ok

.sr_chk_r:
    mov ecx, [rsp + 0x34]               ; nR
    test ecx, ecx
    jz .sr_env_zero
    mov r15d, ebx
    sub r15d, [rsp + 0x5C]
    mov eax, [rsp + 0x38]
    imul eax, r15d
    xor edx, edx
    div ecx
    mov r14d, [rsp + 0x38]
    sub r14d, eax
    jns .sr_env_ok
.sr_env_zero:
    xor r14d, r14d

.sr_env_ok:
    ; 5. Huellkurve anwenden und als 8-Bit-Unsigned speichern
    imul r13d, r14d
    sar r13d, 15
    add r13d, 128
    cmp r13d, 0
    jge .sr_c_hi
    xor r13d, r13d
.sr_c_hi:
    cmp r13d, 255
    jle .sr_st
    mov r13d, 255
.sr_st:
    mov [rdi + rbx], r13b

    inc ebx
    cmp ebx, r12d
    jb .sr_loop

    ; Slot-Metadaten aktualisieren
    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    mov byte [rdx + rbx], 1
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], r12d
    lea rdx, [sampC2Step]
    mov dword [rdx + rbx*4], 16384      ; 22050 Hz -> C-3 Originalhoehe

    ; Loop-Punkte setzen (One-Shot oder Sustain-Loop)
    cmp dword [rsp + 0x20], 0
    je .sr_noloop
    mov eax, [rsp + 0x28]
    add eax, [rsp + 0x2C]               ; LoopStart = Ende Decay
    mov ecx, [rsp + 0x5C]               ; LoopEnd = Ende Sustain
    lea edx, [rax + 128]
    cmp ecx, edx
    ja .sr_lp_set
    mov eax, r12d
    shr eax, 2
    mov ecx, r12d
.sr_lp_set:
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rbx*4], eax
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rbx*4], ecx
    jmp .sr_name

.sr_noloop:
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopEndTbl]
    mov dword [rdx + rbx*4], 0xFFFFF

.sr_name:
    mov eax, [seWave]
    imul eax, 12
    lea rsi, [seWaveSlotNames]
    add rsi, rax
    mov eax, ebx
    imul eax, 12
    lea rdi, [sampNames]
    add rdi, rax
    mov ecx, 12
    rep movsb

    mov dword [seLastSlot], -1
    call samped_sync_sel
    lea rcx, [stSynOk]
    call set_status
    mov al, 12                          ; Sofort auf C-3 vorhoeren
    call samped_preview
    mov dword [dirty], 1

    add rsp, 0x68
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; Preset (ECX = 0..5) laden und sofort in aktiven Slot rendern
apply_synth_preset:
    push rbx
    sub rsp, 0x20

    imul ebx, ecx, 11
    lea rsi, [sePresets]
    add rsi, rbx

    movzx eax, byte [rsi + 0]
    mov [seWave], eax
    movzx eax, byte [rsi + 1]
    mov [sePulse], eax

    xor ecx, ecx
.asp_p:
    movzx eax, byte [rsi + 2 + rcx]
    lea rdx, [seParams]
    mov [rdx + rcx*4], eax
    inc ecx
    cmp ecx, 8
    jb .asp_p

    movzx ecx, byte [rsi + 10]          ; AutoLoop-Flag des Presets
    call synth_render_to_slot

    add rsp, 0x20
    pop rbx
    ret

; ============================================================================
; Tastatur-Handler fuer den Sample- und Instrument-Editor (ECX=VK, EDX=lParam)
; ============================================================================
samped_keydown:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    mov ebx, ecx
    mov esi, edx

    cmp ebx, VK_ESCAPE
    jne .sk_f1
    call stop_recording
    mov dword [sampEdMode], 0
    jmp .sk_done

.sk_f1:
    cmp ebx, VK_F1
    jne .sk_f2
    cmp dword [curOct], 0
    je .sk_done
    dec dword [curOct]
    jmp .sk_done

.sk_f2:
    cmp ebx, VK_F2
    jne .sk_f3
    cmp dword [curOct], 3
    jae .sk_done
    inc dword [curOct]
    jmp .sk_done

.sk_f3:
    cmp ebx, VK_F3
    jne .sk_f4
    mov eax, [curInst]
    dec eax
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sk_done

.sk_f4:
    cmp ebx, VK_F4
    jne .sk_piano
    mov eax, [curInst]
    inc eax
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sk_done

.sk_piano:
    ; QWERTZ-Klaviatur spielt aktives Instrument direkt live vor
    test esi, 0x40000000
    jnz .sk_done
    mov eax, esi
    shr eax, 16
    and eax, 0xFF
    cmp eax, 64
    jae .sk_done
    lea rdx, [scanToNote]
    movzx eax, byte [rdx + rax]
    cmp al, 0xFF
    je .sk_done
    mov ecx, [curOct]
    imul ecx, 12
    add eax, ecx
    cmp eax, 59
    jle .sk_p_ok
    mov eax, 59
.sk_p_ok:
    call samped_preview

.sk_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; Mausklick im Sample-Editor: ECX=Spalte (0..79), EDX=Zeile (0..49), R8D=Taste (0=L,1=R)
; ============================================================================
samped_mouse_down:
    push rbx
    push r12
    push r13
    sub rsp, 0x20

    mov r12d, ecx                       ; Spalte
    mov r13d, edx                       ; Zeile
    mov ebx, r8d                        ; Taste (0=Links, 1=Rechts)

    ; Zeile 0 (Kopfzeile): Slot, Loop, Zurueck
    test r13d, r13d
    jnz .sm_chk_wave

    cmp r12d, 57
    jb .sm_top_slot
    cmp r12d, 70
    ja .sm_done
    call stop_recording
    mov dword [sampEdMode], 0
    jmp .sm_done

.sm_top_slot:
    cmp r12d, 15
    jb .sm_done
    cmp r12d, 23
    ja .sm_top_loop
    mov eax, [curInst]
    test ebx, ebx
    jnz .sm_ts_dec
    inc eax
    jmp .sm_ts_st
.sm_ts_dec:
    dec eax
.sm_ts_st:
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sm_top_loop:
    cmp r12d, 47
    jb .sm_done
    cmp r12d, 55
    ja .sm_done
    call toggle_wav_loop
    jmp .sm_done

.sm_chk_wave:
    ; Zeilen 2..19 (Wellenform): Auswahl-Marker (L/R) oder Loop-Marker (Shift+L/R)
    cmp r13d, 2
    jb .sm_chk_act
    cmp r13d, 19
    ja .sm_chk_act

    call samped_sync_sel
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    je .sm_done
    lea rdx, [sampLenTbl]
    mov r9d, [rdx + rax*4]              ; Sample-Laenge
    test r9d, r9d
    jz .sm_done

    ; Exakte Sample-Position aus mousePxX (Innenbereich 16..943, Breite 928 px)
    mov eax, [mousePxX]
    sub eax, 16
    jns .sm_px_lo
    xor eax, eax
.sm_px_lo:
    cmp eax, 927
    jbe .sm_px_hi
    mov eax, 927
.sm_px_hi:
    imul rax, r9
    xor edx, edx
    mov ecx, 928
    div rcx
    mov r10d, eax                       ; Sample-Position 0..len-1

    ; Pruefen ob Shift gedrueckt (Loop-Punkte statt Auswahl setzen)
    mov ecx, VK_SHIFT
    call GetKeyState
    test ax, 0x8000
    jnz .sm_wave_loop

    test ebx, ebx
    jnz .sm_sel_end
    mov [seSelStart], r10d
    cmp r10d, [seSelEnd]
    jbe .sm_done
    mov [seSelEnd], r9d
    jmp .sm_done
.sm_sel_end:
    inc r10d
    mov [seSelEnd], r10d
    cmp r10d, [seSelStart]
    jae .sm_done
    mov dword [seSelStart], 0
    jmp .sm_done

.sm_wave_loop:
    mov ecx, [curInst]
    and ecx, MAX_SAMPS - 1
    test ebx, ebx
    jnz .sm_lp_end
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rcx*4], r10d
    lea rdx, [sampLoopEndTbl]
    cmp dword [rdx + rcx*4], 0xFFFFF
    jae .sm_lp_autoe
    cmp [rdx + rcx*4], r10d
    ja .sm_done
.sm_lp_autoe:
    mov [rdx + rcx*4], r9d
    jmp .sm_done
.sm_lp_end:
    inc r10d
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rcx*4], r10d
    lea rdx, [sampLoopStartTbl]
    cmp [rdx + rcx*4], r10d
    jb .sm_done
    mov dword [rdx + rcx*4], 0
    jmp .sm_done

.sm_chk_act:
    ; Zeile 22 (Datei, Aufnahme, Vorschau, Loop, Trim)
    cmp r13d, 22
    jne .sm_chk_fx
    test ebx, ebx
    jnz .sm_done

    cmp r12d, 1
    jb .sm_done
    cmp r12d, 11
    ja .sm_act_rec
    call stop_recording
    call load_wav_dialog
    mov dword [seLastSlot], -1
    call samped_sync_sel
    jmp .sm_done

.sm_act_rec:
    cmp r12d, 13
    jb .sm_done
    cmp r12d, 23
    ja .sm_act_play
    call start_recording
    jmp .sm_done

.sm_act_play:
    cmp r12d, 25
    jb .sm_done
    cmp r12d, 35
    ja .sm_act_stop
    mov eax, [curOct]
    imul eax, 12
    call samped_preview
    jmp .sm_done

.sm_act_stop:
    cmp r12d, 37
    jb .sm_done
    cmp r12d, 45
    ja .sm_act_lp
    call silence_audio
    jmp .sm_done

.sm_act_lp:
    cmp r12d, 47
    jb .sm_done
    cmp r12d, 55
    ja .sm_act_s2l
    call toggle_wav_loop
    jmp .sm_done

.sm_act_s2l:
    cmp r12d, 57
    jb .sm_done
    cmp r12d, 65
    ja .sm_act_trim
    call samped_sync_sel
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    je .sm_done
    mov ecx, [seSelStart]
    mov edx, [seSelEnd]
    cmp edx, ecx
    jbe .sm_done
    lea r8, [sampLoopStartTbl]
    mov [r8 + rax*4], ecx
    lea r8, [sampLoopEndTbl]
    mov [r8 + rax*4], edx
    lea rcx, [stLoopOn]
    call set_status
    jmp .sm_done

.sm_act_trim:
    cmp r12d, 67
    jb .sm_done
    cmp r12d, 72
    ja .sm_act_syn
    call se_fx_trim
    jmp .sm_done

.sm_act_syn:
    cmp r12d, 74
    jb .sm_done
    cmp r12d, 78
    ja .sm_done
    call unload_wav_slot
    mov dword [seLastSlot], -1
    call samped_sync_sel
    jmp .sm_done

.sm_chk_fx:
    ; Zeile 24 (Sample-Effekte)
    cmp r13d, 24
    jne .sm_chk_swave
    test ebx, ebx
    jnz .sm_done

    cmp r12d, 5
    jb .sm_done
    cmp r12d, 14
    ja .sm_fx_fi
    call se_fx_normalize
    jmp .sm_done
.sm_fx_fi:
    cmp r12d, 16
    jb .sm_done
    cmp r12d, 24
    ja .sm_fx_fo
    call se_fx_fade_in
    jmp .sm_done
.sm_fx_fo:
    cmp r12d, 26
    jb .sm_done
    cmp r12d, 34
    ja .sm_fx_rev
    call se_fx_fade_out
    jmp .sm_done
.sm_fx_rev:
    cmp r12d, 36
    jb .sm_done
    cmp r12d, 44
    ja .sm_fx_bst
    call se_fx_reverse
    jmp .sm_done
.sm_fx_bst:
    cmp r12d, 46
    jb .sm_done
    cmp r12d, 54
    ja .sm_fx_hlf
    call se_fx_boost
    jmp .sm_done
.sm_fx_hlf:
    cmp r12d, 56
    jb .sm_done
    cmp r12d, 64
    ja .sm_fx_all
    call se_fx_halve
    jmp .sm_done
.sm_fx_all:
    cmp r12d, 66
    jb .sm_done
    cmp r12d, 73
    ja .sm_done
    mov dword [seLastSlot], -1
    call samped_sync_sel
    jmp .sm_done

.sm_chk_swave:
    ; Zeile 28 (Synth-Wellenform und Pulsbreite)
    cmp r13d, 28
    jne .sm_chk_sliders

    cmp r12d, 61
    jb .sm_sw_btns
    cmp r12d, 76
    ja .sm_done
    mov eax, [sePulse]
    test ebx, ebx
    jnz .sm_pw_dec
    add eax, 5
    cmp eax, 90
    jbe .sm_pw_st
    mov eax, 90
    jmp .sm_pw_st
.sm_pw_dec:
    sub eax, 5
    cmp eax, 10
    jge .sm_pw_st
    mov eax, 10
.sm_pw_st:
    mov [sePulse], eax
    jmp .sm_done

.sm_sw_btns:
    test ebx, ebx
    jnz .sm_done
    cmp r12d, 10
    jb .sm_done
    cmp r12d, 58
    ja .sm_done
    mov eax, r12d
    sub eax, 10
    xor edx, edx
    mov ecx, 10
    div ecx
    cmp eax, 5
    jae .sm_done
    cmp edx, 9
    jae .sm_done
    mov [seWave], eax
    jmp .sm_done

.sm_chk_sliders:
    ; Zeilen 30..37 (8 ADSR- / Synth-Parameter-Slider)
    cmp r13d, 30
    jb .sm_chk_presets
    cmp r13d, 37
    ja .sm_chk_presets

    mov ecx, r13d
    sub ecx, 30                         ; Parameter-Index 0..7
    lea rdx, [seParams]

    ; Klick direkt in den Balken (Spalten 14..53)
    cmp r12d, 14
    jb .sm_done
    cmp r12d, 53
    ja .sm_sl_minus
    mov eax, r12d
    sub eax, 14
    imul eax, 100
    push rdx
    xor edx, edx
    mov r8d, 39
    div r8d
    pop rdx
    jmp .sm_sl_clamp

.sm_sl_minus:
    cmp r12d, 55
    jb .sm_done
    cmp r12d, 57
    ja .sm_sl_plus
    mov eax, [rdx + rcx*4]
    sub eax, 5
    jmp .sm_sl_clamp

.sm_sl_plus:
    cmp r12d, 63
    jb .sm_done
    cmp r12d, 65
    ja .sm_done
    mov eax, [rdx + rcx*4]
    add eax, 5

.sm_sl_clamp:
    mov r8d, 0
    cmp ecx, 4                          ; Dauer min. 5
    je .sm_min5
    cmp ecx, 7                          ; Filter min. 5
    jne .sm_chk_min
.sm_min5:
    mov r8d, 5
.sm_chk_min:
    cmp eax, r8d
    jge .sm_chk_max
    mov eax, r8d
.sm_chk_max:
    cmp eax, 100
    jle .sm_sl_st
    mov eax, 100
.sm_sl_st:
    mov [rdx + rcx*4], eax
    jmp .sm_done

.sm_chk_presets:
    ; Zeile 39 (6 Synth-Presets)
    cmp r13d, 39
    jne .sm_chk_render
    test ebx, ebx
    jnz .sm_done
    cmp r12d, 12
    jb .sm_done
    cmp r12d, 70
    ja .sm_done
    mov eax, r12d
    sub eax, 12
    xor edx, edx
    mov ecx, 10
    div ecx
    cmp eax, 6
    jae .sm_done
    cmp edx, 9
    jae .sm_done
    mov ecx, eax
    call apply_synth_preset
    jmp .sm_done

.sm_chk_render:
    ; Zeile 41 (Rendern + Auto-Loop + Slot -/+)
    cmp r13d, 41
    jne .sm_chk_bot
    test ebx, ebx
    jnz .sm_done

    cmp r12d, 3
    jb .sm_done
    cmp r12d, 28
    ja .sm_ren_loop
    xor ecx, ecx
    call synth_render_to_slot
    jmp .sm_done

.sm_ren_loop:
    cmp r12d, 31
    jb .sm_done
    cmp r12d, 55
    ja .sm_ren_slot
    mov ecx, 1
    call synth_render_to_slot
    jmp .sm_done

.sm_ren_slot:
    cmp r12d, 58
    jb .sm_done
    cmp r12d, 76
    ja .sm_done
    mov eax, [curInst]
    cmp r12d, 67
    jb .sm_rs_dec
    inc eax
    jmp .sm_rs_st
.sm_rs_dec:
    dec eax
.sm_rs_st:
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sm_chk_bot:
    ; Zeile 49 (Fusszeile Sample-Editor)
    cmp r13d, 49
    jne .sm_done

    cmp r12d, 75
    jb .sm_bot_slot
    cmp r12d, 77
    ja .sm_done
    xor dword [showHelp], 1
    jmp .sm_done

.sm_bot_slot:
    cmp r12d, 1
    jb .sm_done
    cmp r12d, 7
    ja .sm_bot_oct
    mov eax, [curInst]
    test ebx, ebx
    jnz .sm_bs_dec
    inc eax
    jmp .sm_bs_st
.sm_bs_dec:
    dec eax
.sm_bs_st:
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sm_bot_oct:
    cmp r12d, 10
    jb .sm_done
    cmp r12d, 14
    ja .sm_bot_play
    test ebx, ebx
    jnz .sm_bo_dec
    cmp dword [curOct], 3
    jae .sm_done
    inc dword [curOct]
    jmp .sm_done
.sm_bo_dec:
    cmp dword [curOct], 0
    je .sm_done
    dec dword [curOct]
    jmp .sm_done

.sm_bot_play:
    test ebx, ebx
    jnz .sm_done
    cmp r12d, 17
    jb .sm_done
    cmp r12d, 33
    ja .sm_bot_back
    xor dword [isPlaying], 1
    cmp dword [isPlaying], 0
    jne .sm_bp_start
    call silence_audio
    jmp .sm_done
.sm_bp_start:
    mov eax, [curPat]
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov eax, [editRow]
    mov [playRow], eax
    mov [dispPlayRow], eax
    mov dword [needPatTempo], 0
    mov dword [tickSamples], 1
    jmp .sm_done

.sm_bot_back:
    cmp r12d, 36
    jb .sm_done
    cmp r12d, 69
    ja .sm_done
    call stop_recording
    mov dword [sampEdMode], 0

.sm_done:
    add rsp, 0x20
    pop r13
    pop r12
    pop rbx
    ret

; ============================================================================
; Sample- und Instrument-Editor komplett in memDC zeichnen
; ============================================================================
draw_samped_screen:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x58

    call samped_sync_sel

    ; Hintergrund
    xor ecx, ecx
    xor edx, edx
    mov r8d, WIN_W
    mov r9d, WIN_H
    mov r10d, COL_BG
    call fill_rect

    ; Aktuellen Slot & Metadaten ermitteln
    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    movzx r12d, byte [rdx + rbx]        ; r12d = hasPcm
    lea rdx, [sampLenTbl]
    mov r13d, [rdx + rbx*4]             ; r13d = len

    ; --- Zeile 0: Kopfzeile ---
    mov al, bl
    lea r8, [seTopBar + 21]
    call fmt_hex2

    mov eax, ebx
    imul eax, 12
    lea rsi, [sampNames]
    add rsi, rax
    lea rdi, [seTopBar + 24]
    mov ecx, 12
    rep movsb

    mov eax, r13d
    lea r8, [seTopBar + 41]
    call fmt_hex5

    lea rdx, [sampLoopEndTbl]
    mov eax, [rdx + rbx*4]
    mov dword [seTopBar + 53], 'OFF '
    test r12d, r12d
    jz .ds_lp_txt_ok
    cmp eax, r13d
    ja .ds_lp_txt_ok
    mov dword [seTopBar + 53], 'ON  '
.ds_lp_txt_ok:

    lea rsi, [topBar + 72]
    lea rdi, [seTopBar + 72]
    mov ecx, 7
    rep movsb

    xor ecx, ecx
    xor edx, edx
    lea r8, [seTopBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    ; --- Zeilen 2..19: Wellenform-Box (x=12..948, y=24..240) ---
    mov ecx, 12
    mov edx, 24
    mov r8d, 936
    mov r9d, 216
    mov r10d, COL_HDR_ON
    call fill_rect

    mov ecx, 16
    mov edx, 28
    mov r8d, 928
    mov r9d, 208
    mov r10d, COL_BLACK
    call fill_rect

    test r12d, r12d
    jz .ds_no_pcm
    test r13d, r13d
    jz .ds_no_pcm

    ; Auswahlbereich farblich hinterlegen
    mov eax, [seSelStart]
    imul rax, 928
    xor edx, edx
    div r13
    mov r14d, eax                       ; selX0 (0..927)

    mov eax, [seSelEnd]
    imul rax, 928
    xor edx, edx
    div r13
    mov r15d, eax                       ; selX1 (0..928)
    cmp r15d, r14d
    jbe .ds_no_sel_bg
    lea ecx, [r14 + 16]
    mov edx, 28
    mov r8d, r15d
    sub r8d, r14d
    mov r9d, 208
    mov r10d, COL_BEAT
    call fill_rect
.ds_no_sel_bg:

    ; Nulllinie bei y = 132
    mov ecx, 16
    mov edx, 132
    mov r8d, 928
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    ; Wellenform in 464 Spalten a 2 px zeichnen (Min/Max pro Spalte)
    mov eax, ebx
    shl eax, 17
    lea rbp, [pcmSlots]
    add rbp, rax                        ; rbp = Sample-Basiszeiger

    xor edi, edi                        ; colPx = 0..463
.ds_wf_loop:
    mov eax, edi
    imul rax, r13
    xor edx, edx
    mov ecx, 464
    div rcx
    mov r8d, eax                        ; startIdx

    lea eax, [edi + 1]
    imul rax, r13
    xor edx, edx
    mov ecx, 464
    div rcx
    mov r9d, eax                        ; endIdx
    cmp r9d, r8d
    ja .ds_wf_span
    lea r9d, [r8 + 1]
.ds_wf_span:
    cmp r9d, r13d
    jbe .ds_wf_cap
    mov r9d, r13d
.ds_wf_cap:
    ; Maximal 16 Stuetzpunkte pro Spalte pruefen (konstante Geschwindigkeit)
    mov ecx, r9d
    sub ecx, r8d
    mov r11d, 1
    cmp ecx, 16
    jbe .ds_wf_step
    mov r11d, ecx
    shr r11d, 4
.ds_wf_step:
    mov r10d, 128                       ; minVal
    mov esi, 128                        ; maxVal
    mov ecx, r8d
.ds_wf_mm:
    movzx eax, byte [rbp + rcx]
    cmp eax, r10d
    jae .ds_wf_nmin
    mov r10d, eax
.ds_wf_nmin:
    cmp eax, esi
    jbe .ds_wf_nmax
    mov esi, eax
.ds_wf_nmax:
    add ecx, r11d
    cmp ecx, r9d
    jb .ds_wf_mm

    ; Y-Koordinaten berechnen (yTop aus maxVal, yBot aus minVal)
    mov eax, esi
    sub eax, 128
    imul eax, 100
    sar eax, 7
    mov ecx, 132
    sub ecx, eax                        ; yTop

    mov eax, r10d
    sub eax, 128
    imul eax, 100
    sar eax, 7
    mov edx, 132
    sub edx, eax                        ; yBot
    cmp edx, ecx
    jge .ds_wf_h
    xchg ecx, edx
.ds_wf_h:
    sub edx, ecx
    inc edx
    mov r9d, edx                        ; Hoehe
    mov edx, ecx                        ; Y

    lea ecx, [rdi*2 + 16]               ; X
    mov r8d, 2                          ; Breite 2 px
    mov r10d, COL_TEAL
    lea eax, [rdi*2]
    cmp eax, r14d
    jb .ds_wf_draw
    cmp eax, r15d
    jae .ds_wf_draw
    mov r10d, COL_CYAN
.ds_wf_draw:
    call fill_rect

    inc edi
    cmp edi, 464
    jb .ds_wf_loop

    ; Loop-Linien ueber der Wellenform einzeichnen, falls Loop aktiv
    lea rdx, [sampLoopEndTbl]
    mov r15d, [rdx + rbx*4]
    cmp r15d, r13d
    ja .ds_wf_done
    lea rdx, [sampLoopStartTbl]
    mov r14d, [rdx + rbx*4]
    cmp r15d, r14d
    jbe .ds_wf_done

    mov eax, r14d
    imul rax, 928
    xor edx, edx
    div r13
    lea ecx, [rax + 16]
    mov edx, 28
    mov r8d, 2
    mov r9d, 208
    mov r10d, COL_WHITE
    call fill_rect

    mov eax, r15d
    imul rax, 928
    xor edx, edx
    div r13
    cmp eax, 926
    jbe .ds_lpe_ok
    mov eax, 926
.ds_lpe_ok:
    lea ecx, [rax + 16]
    mov edx, 28
    mov r8d, 2
    mov r9d, 208
    mov r10d, COL_PURPHI
    call fill_rect
    jmp .ds_wf_done

.ds_no_pcm:
    mov ecx, 16
    mov edx, 132
    mov r8d, 928
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    mov ecx, 6
    mov edx, 10
    lea r8, [seEmptyMsg]
    mov r9d, 68
    mov r10d, COL_NOTE
    call draw_text

.ds_wf_done:
    ; --- Zeile 20: Marker-Info ---
    mov eax, [seSelStart]
    lea r8, [seInfoBar + 5]
    call fmt_hex5
    mov eax, [seSelEnd]
    lea r8, [seInfoBar + 11]
    call fmt_hex5
    lea rdx, [sampLoopStartTbl]
    mov eax, [rdx + rbx*4]
    lea r8, [seInfoBar + 23]
    call fmt_hex5
    lea rdx, [sampLoopEndTbl]
    mov eax, [rdx + rbx*4]
    lea r8, [seInfoBar + 29]
    call fmt_hex5

    xor ecx, ecx
    mov edx, 20
    lea r8, [seInfoBar]
    mov r9d, 79
    mov r10d, COL_NOTE
    call draw_text

    ; --- Zeile 22: Aktions-Buttons ---
    ; [WAV Laden] (1..11)
    mov ecx, 1 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 1
    mov edx, 22
    lea r8, [seActBar + 1]
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    ; [REC Start] / [REC Stop!] (13..23)
    mov ecx, 13 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    lea r14, [txtRecBtnOn]
    cmp dword [isRecording], 0
    je .ds_rec_b
    mov r10d, COL_PURPHI
    lea r14, [txtRecBtnOff]
.ds_rec_b:
    call fill_rect
    mov ecx, 13
    mov edx, 22
    mov r8, r14
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    ; [Vorhoeren] (25..35)
    mov ecx, 25 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 25
    mov edx, 22
    lea r8, [seActBar + 25]
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    ; [StopTon] (37..45)
    mov ecx, 37 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 37
    mov edx, 22
    lea r8, [seActBar + 37]
    mov r9d, 9
    mov r10d, COL_WHITE
    call draw_text

    ; [Loop:ON] / [LoopOFF] (47..55)
    mov ecx, 47 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    lea r14, [txtLpBtnOff]
    test r12d, r12d
    jz .ds_lpb
    lea rdx, [sampLoopEndTbl]
    cmp [rdx + rbx*4], r13d
    ja .ds_lpb
    mov r10d, COL_HDR_ON
    lea r14, [txtLpBtnOn]
.ds_lpb:
    call fill_rect
    mov ecx, 47
    mov edx, 22
    mov r8, r14
    mov r9d, 9
    mov r10d, COL_WHITE
    call draw_text

    ; [Sel->Lp] (57..65)
    mov ecx, 57 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 57
    mov edx, 22
    lea r8, [seActBar + 57]
    mov r9d, 9
    mov r10d, COL_WHITE
    call draw_text

    ; [Trim] (67..72)
    mov ecx, 67 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 6 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    call fill_rect
    mov ecx, 67
    mov edx, 22
    lea r8, [seActBar + 67]
    mov r9d, 6
    mov r10d, COL_WHITE
    call draw_text

    ; [Syn] (74..78)
    mov ecx, 74 * CHAR_W
    mov edx, 22 * CHAR_H
    mov r8d, 5 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 74
    mov edx, 22
    lea r8, [seActBar + 74]
    mov r9d, 5
    mov r10d, COL_WHITE
    call draw_text

    ; --- Zeile 24: Sample-Effekte ---
    mov ecx, 1
    mov edx, 24
    lea r8, [seFxBar + 1]
    mov r9d, 3
    mov r10d, COL_TEAL
    call draw_text

    ; 7 Effekt-Buttons anhand Tabelle seFxBtnPos (Startspalte, Breite)
    xor edi, edi
.ds_fxb_loop:
    lea rsi, [seFxBtnPos]
    movzx r14d, byte [rsi + rdi*2 + 0]
    movzx r15d, byte [rsi + rdi*2 + 1]

    mov ecx, r14d
    imul ecx, CHAR_W
    mov edx, 24 * CHAR_H
    mov r8d, r15d
    imul r8d, CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    test r12d, r12d
    jz .ds_fxb_bg
    mov r10d, COL_HDR_ON
.ds_fxb_bg:
    call fill_rect

    mov ecx, r14d
    mov edx, 24
    lea r8, [seFxBar]
    add r8, r14
    mov r9d, r15d
    mov r10d, COL_MUTED
    test r12d, r12d
    jz .ds_fxb_tx
    mov r10d, COL_WHITE
.ds_fxb_tx:
    call draw_text

    inc edi
    cmp edi, 7
    jb .ds_fxb_loop

    ; --- Zeilen 26..45: Synth- / Instrument-Designer ---
    mov ecx, 1 * CHAR_W
    mov edx, 26 * CHAR_H
    mov r8d, 78 * CHAR_W
    mov r9d, 20 * CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect

    mov ecx, 1 * CHAR_W + 3
    mov edx, 26 * CHAR_H + 3
    mov r8d, 78 * CHAR_W - 6
    mov r9d, 20 * CHAR_H - 6
    mov r10d, COL_BLACK
    call fill_rect

    ; Titelbalken Zeile 26
    mov ecx, 1 * CHAR_W
    mov edx, 26 * CHAR_H
    mov r8d, 78 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    call fill_rect

    mov ecx, 2
    mov edx, 26
    lea r8, [seSynthHdr]
    mov r9d, 76
    mov r10d, COL_WHITE
    call draw_text

    ; Zeile 28: Wellenform-Buttons + Pulsbreite
    mov ecx, 3
    mov edx, 28
    lea r8, [seWaveBar + 3]
    mov r9d, 6
    mov r10d, COL_TEAL
    call draw_text

    xor edi, edi
.ds_wb_loop:
    imul r14d, edi, 10
    add r14d, 10
    mov ecx, r14d
    imul ecx, CHAR_W
    mov edx, 28 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    cmp edi, [seWave]
    jne .ds_wb_bg
    mov r10d, COL_CURSOR
.ds_wb_bg:
    call fill_rect

    mov ecx, r14d
    mov edx, 28
    lea r8, [seWaveBar]
    add r8, r14
    mov r9d, 9
    mov r10d, COL_NOTE
    cmp edi, [seWave]
    jne .ds_wb_tx
    mov r10d, COL_BLACK
.ds_wb_tx:
    call draw_text

    inc edi
    cmp edi, 5
    jb .ds_wb_loop

    mov eax, [sePulse]
    lea r8, [sePulseTxt + 11]
    call fmt_dec3
    mov ecx, 61
    mov edx, 28
    lea r8, [sePulseTxt]
    mov r9d, 16
    mov r10d, COL_CYAN
    call draw_text

    ; Zeilen 30..37: 8 grafische Parameter-Slider
    xor edi, edi
.ds_sl_loop:
    lea r14d, [rdi + 30]                ; Bildschirmzeile

    ; Parametername (Spalten 3..12)
    imul eax, edi, 10
    lea r8, [seParamNames]
    add r8, rax
    mov ecx, 3
    mov edx, r14d
    mov r9d, 10
    mov r10d, COL_NOTE
    call draw_text

    ; Balken-Hintergrund (Spalten 14..53 = 480 px)
    mov ecx, 14 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 2
    mov r8d, 40 * CHAR_W
    mov r9d, CHAR_H - 4
    mov r10d, COL_BEAT
    call fill_rect

    ; Gefuellter Balkenanteil
    lea rsi, [seParams]
    mov r15d, [rsi + rdi*4]             ; Wert 0..100
    imul eax, r15d, 480
    xor edx, edx
    mov ecx, 100
    div ecx
    test eax, eax
    jz .ds_sl_nobar
    mov ecx, 14 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 2
    mov r8d, eax
    mov r9d, CHAR_H - 4
    mov r10d, COL_PURPHI
    cmp edi, 4
    jae .ds_sl_col2
    mov r10d, COL_CYAN
.ds_sl_col2:
    call fill_rect
.ds_sl_nobar:

    ; [-] Button (55..57)
    mov ecx, 55 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 55
    mov edx, r14d
    lea r8, [txtMinus]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    ; Zahlenwert (59..61)
    mov eax, r15d
    lea r8, [lineBuf]
    call fmt_dec3
    mov ecx, 59
    mov edx, r14d
    lea r8, [lineBuf]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    ; [+] Button (63..65)
    mov ecx, 63 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 63
    mov edx, r14d
    lea r8, [txtPlus]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    ; Kurzer Beschreibungstext (67..77)
    imul eax, edi, 11
    lea r8, [seParamHints]
    add r8, rax
    mov ecx, 67
    mov edx, r14d
    mov r9d, 11
    mov r10d, COL_TEAL
    call draw_text

    inc edi
    cmp edi, 8
    jb .ds_sl_loop

    ; Zeile 39: 6 Presets
    mov ecx, 3
    mov edx, 39
    lea r8, [sePresetBar + 3]
    mov r9d, 8
    mov r10d, COL_TEAL
    call draw_text

    xor edi, edi
.ds_pr_loop:
    imul r14d, edi, 10
    add r14d, 12
    mov ecx, r14d
    imul ecx, CHAR_W
    mov edx, 39 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect

    mov ecx, r14d
    mov edx, 39
    lea r8, [sePresetBar]
    add r8, r14
    mov r9d, 9
    mov r10d, COL_WHITE
    call draw_text

    inc edi
    cmp edi, 6
    jb .ds_pr_loop

    ; Zeile 41: Rendern-Buttons + Slot -/+
    mov ecx, 3 * CHAR_W
    mov edx, 41 * CHAR_H
    mov r8d, 26 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect
    mov ecx, 3
    mov edx, 41
    lea r8, [seRenderBar + 3]
    mov r9d, 26
    mov r10d, COL_BLACK
    call draw_text

    mov ecx, 31 * CHAR_W
    mov edx, 41 * CHAR_H
    mov r8d, 25 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 31
    mov edx, 41
    lea r8, [seRenderBar + 31]
    mov r9d, 25
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 58 * CHAR_W
    mov edx, 41 * CHAR_H
    mov r8d, 19 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    call fill_rect
    mov ecx, 58
    mov edx, 41
    lea r8, [seRenderBar + 58]
    mov r9d, 19
    mov r10d, COL_CYAN
    call draw_text

    ; Zeile 44: Aufnahme-Banner oder Bedienungshinweis
    cmp dword [isRecording], 0
    je .ds_hint_norm
    mov ecx, 3 * CHAR_W
    mov edx, 44 * CHAR_H
    mov r8d, 74 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect
    mov ecx, 3
    mov edx, 44
    lea r8, [seRecBanner]
    mov r9d, 74
    mov r10d, COL_WHITE
    call draw_text
    jmp .ds_bot_bar

.ds_hint_norm:
    mov ecx, 3
    mov edx, 44
    lea r8, [seKeyHint]
    mov r9d, 74
    mov r10d, COL_NOTE
    call draw_text

.ds_bot_bar:
    ; Zeile 49: Fusszeile im Sample-Editor
    mov al, bl
    lea r8, [seBotBar + 6]
    call fmt_hex2
    mov eax, [curOct]
    add al, '2'
    mov [seBotBar + 14], al

    xor ecx, ecx
    mov edx, 49
    lea r8, [seBotBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    mov ecx, 75 * CHAR_W
    mov edx, 49 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    cmp dword [showHelp], 0
    je .ds_qbg
    mov r10d, COL_PURPHI
.ds_qbg:
    call fill_rect

    mov ecx, 75
    mov edx, 49
    lea r8, [seBotBar + 75]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    add rsp, 0x58
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret