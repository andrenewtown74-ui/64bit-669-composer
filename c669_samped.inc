; Sample- und Instrument-Editor (Wellenform, Loop-Drag, waveIn-Aufnahme mit Live-VU & Geraetewahl, Sample-FX & Live-ADSR-Synth pro Slot)

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

; ============================================================================
; Pro-Slot-Speicherverwaltung (64 Slots: Modus, Welle, Puls, AutoLoop, 9 Slider, Original-Backup)
; ============================================================================

; Slot ECX (0..63) auf Standard-Synth-Parameter zuruecksetzen
samped_reset_slot_synth:
    push rdi
    and ecx, MAX_SAMPS - 1

    lea rdx, [slotMode]
    mov byte [rdx + rcx], 0
    lea rdx, [slotWave]
    mov byte [rdx + rcx], 1
    lea rdx, [slotPulse]
    mov byte [rdx + rcx], 50
    lea rdx, [slotAutoLoop]
    mov byte [rdx + rcx], 0
    lea rdx, [slotOrigLen]
    mov dword [rdx + rcx*4], 0

    imul eax, ecx, 36
    lea rdi, [slotSynthParams]
    add rdi, rax
    mov dword [rdi + 0*4], 4            ; Attack
    mov dword [rdi + 1*4], 32           ; Decay
    mov dword [rdi + 2*4], 60           ; Sustain
    mov dword [rdi + 3*4], 24           ; Release
    mov dword [rdi + 4*4], 35           ; Dauer
    mov dword [rdi + 5*4], 50           ; Sweep
    mov dword [rdi + 6*4], 18           ; Detune/Chorus
    mov dword [rdi + 7*4], 80           ; Filter LP
    mov dword [rdi + 8*4], 90           ; Lautstaerke

    lea rdi, [slotSmpParams]
    add rdi, rax
    mov dword [rdi + 0*4], 0            ; Einblenden neutral
    mov dword [rdi + 1*4], 0            ; Abfallzeit neutral
    mov dword [rdi + 2*4], 100          ; Haltepegel neutral
    mov dword [rdi + 3*4], 0            ; Ausklingen neutral
    mov dword [rdi + 4*4], 100          ; Laenge/Gate 100%
    mov dword [rdi + 5*4], 50           ; Ton-Sweep neutral
    mov dword [rdi + 6*4], 0            ; Chorus aus
    mov dword [rdi + 7*4], 100          ; Tiefpass offen
    mov dword [rdi + 8*4], 100          ; Lautstaerke 100%

    cmp ecx, [seCurSlot]
    jne .srs_done
    mov dword [seCurSlot], -1
.srs_done:
    pop rdi
    ret

; Aktuelles pcmSlots[ECX] als unbearbeitetes Original nach pcmOrigSlots[ECX] sichern (Sample-FX neutral, Synth bleibt erhalten)
samped_commit_orig_slot:
    push rbx
    push rsi
    push rdi

    mov ebx, ecx
    and ebx, MAX_SAMPS - 1

    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rbx*4]
    cmp ecx, MAX_SAMP_BYTES
    jbe .sco_len_ok
    mov ecx, MAX_SAMP_BYTES
.sco_len_ok:
    lea rdx, [slotOrigLen]
    mov [rdx + rbx*4], ecx

    test ecx, ecx
    jz .sco_params

    mov eax, ebx
    shl eax, 17
    lea rsi, [pcmSlots]
    add rsi, rax
    lea rdi, [pcmOrigSlots]
    add rdi, rax
    rep movsb

.sco_params:
    lea rdx, [slotMode]
    mov byte [rdx + rbx], 1             ; SAMPLE-FX-Modus
    lea rdx, [slotAutoLoop]
    mov byte [rdx + rbx], 0

    imul eax, ebx, 36
    lea rdi, [slotSmpParams]
    add rdi, rax
    mov dword [rdi + 0*4], 0            ; Einblenden neutral
    mov dword [rdi + 1*4], 0            ; Abfallzeit neutral
    mov dword [rdi + 2*4], 100          ; Haltepegel neutral
    mov dword [rdi + 3*4], 0            ; Ausklingen neutral
    mov dword [rdi + 4*4], 100          ; Laenge/Gate 100%
    mov dword [rdi + 5*4], 50           ; Ton-Sweep neutral
    mov dword [rdi + 6*4], 0            ; Chorus aus
    mov dword [rdi + 7*4], 100          ; Tiefpass offen
    mov dword [rdi + 8*4], 100          ; Lautstaerke 100%

    mov dword [seCurSlot], -1
    mov dword [seLastSlot], -1

    pop rdi
    pop rsi
    pop rbx
    ret

; Gespeicherte Einstellungen des aktiven Slots (curInst) in die UI-Variablen laden
samped_load_cur_slot:
    push rsi
    push rdi

    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    cmp eax, [seCurSlot]
    je .slc_done
    mov [seCurSlot], eax

    lea rdx, [slotMode]
    movzx ecx, byte [rdx + rax]
    mov [seMode], ecx
    lea rdx, [slotWave]
    movzx ecx, byte [rdx + rax]
    mov [seWave], ecx
    lea rdx, [slotPulse]
    movzx ecx, byte [rdx + rax]
    mov [sePulse], ecx
    lea rdx, [slotAutoLoop]
    movzx ecx, byte [rdx + rax]
    mov [seAutoLoop], ecx

    imul eax, 36
    push rax
    lea rsi, [slotSynthParams]
    add rsi, rax
    lea rdi, [seSynthParams]
    mov ecx, 9
    rep movsd
    pop rax

    lea rsi, [slotSmpParams]
    add rsi, rax
    lea rdi, [seSmpParams]
    mov ecx, 9
    rep movsd

.slc_done:
    pop rdi
    pop rsi
    ret

; Aktuelle UI-Variablen im Speicher des aktiven Slots (curInst) ablegen
samped_save_cur_slot:
    push rsi
    push rdi

    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    mov [seCurSlot], eax

    mov cl, [seMode]
    lea rdx, [slotMode]
    mov [rdx + rax], cl
    mov cl, [seWave]
    lea rdx, [slotWave]
    mov [rdx + rax], cl
    mov cl, [sePulse]
    lea rdx, [slotPulse]
    mov [rdx + rax], cl
    mov cl, [seAutoLoop]
    lea rdx, [slotAutoLoop]
    mov [rdx + rax], cl

    imul eax, 36
    push rax
    lea rdi, [slotSynthParams]
    add rdi, rax
    lea rsi, [seSynthParams]
    mov ecx, 9
    rep movsd
    pop rax

    lea rdi, [slotSmpParams]
    add rdi, rax
    lea rsi, [seSmpParams]
    mov ecx, 9
    rep movsd

    pop rdi
    pop rsi
    ret

; Standardwerte beim Programmstart setzen und Aufnahmegeraet einlesen
init_samped_defaults:
    sub rsp, 0x28
    mov dword [seCurSlot], -1
    mov dword [seLastSlot], -1
    mov dword [seLastLen], -1
    mov dword [seRecDevId], -1
    mov dword [seLiveSynth], 1
    mov dword [seDragMode], 0
    mov dword [seDragParam], 0
    call samped_load_cur_slot
    call samped_refresh_rec_dev
    add rsp, 0x28
    ret

; Aufnahmegeraete zaehlen und Anzeigenamen (28 Zeichen) in seRecDevName aktualisieren
samped_refresh_rec_dev:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    call waveInGetNumDevs
    mov [seRecDevCount], eax
    mov ebx, eax

    test ebx, ebx
    jnz .srd_has_dev
    mov dword [seRecDevId], -1
    lea rsi, [txtDevNone]
    lea rdi, [seRecDevName]
    mov ecx, 28
    rep movsb
    jmp .srd_done

.srd_has_dev:
    mov eax, [seRecDevId]
    cmp eax, -1
    je .srd_std
    cmp eax, ebx
    jl .srd_query
    mov dword [seRecDevId], -1

.srd_std:
    lea rsi, [txtDevStd]
    lea rdi, [seRecDevName]
    mov ecx, 28
    rep movsb
    jmp .srd_done

.srd_query:
    lea rdi, [recDevCaps]
    xor eax, eax
    mov ecx, 48
    rep stosb

    mov ecx, [seRecDevId]
    lea rdx, [recDevCaps]
    mov r8d, 48
    call waveInGetDevCapsA
    test eax, eax
    jnz .srd_std

    lea rsi, [recDevCaps + 8]
    lea rdi, [seRecDevName]
    xor ecx, ecx
.srd_cp:
    cmp ecx, 28
    jae .srd_done
    mov al, [rsi + rcx]
    test al, al
    jz .srd_pad
    cmp al, 32
    jae .srd_st
    mov al, ' '
.srd_st:
    mov [rdi + rcx], al
    inc ecx
    jmp .srd_cp
.srd_pad:
    mov byte [rdi + rcx], ' '
    inc ecx
    cmp ecx, 28
    jb .srd_pad

.srd_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; Aufnahmegeraet weiterschalten: ECX = +1 (naechstes) oder -1 (vorheriges)
samped_cycle_rec_dev:
    push rbx
    push r12
    sub rsp, 0x28

    cmp dword [isRecording], 0
    jne .scd_done

    mov r12d, ecx
    call waveInGetNumDevs
    mov [seRecDevCount], eax
    mov ebx, eax
    test ebx, ebx
    jz .scd_none

    mov eax, [seRecDevId]
    test r12d, r12d
    js .scd_prev
    inc eax
    cmp eax, ebx
    jl .scd_st
    mov eax, -1
    jmp .scd_st

.scd_prev:
    dec eax
    cmp eax, -1
    jge .scd_st
    lea eax, [rbx - 1]
    jmp .scd_st

.scd_none:
    mov eax, -1

.scd_st:
    mov [seRecDevId], eax
    call samped_refresh_rec_dev
    mov dword [dirty], 1

.scd_done:
    add rsp, 0x28
    pop r12
    pop rbx
    ret

; Client-lParam (ECX) in geklemmte virtuelle X-Koordinate umrechnen (Wide: 0..1487, Classic: 0..959)
se_lparam_to_px:
    movsx eax, cx
    cmp dword [wideMode], 0
    jne .slp_wide
    sub eax, [seOffX]
    mov ecx, WIN_W - 1
    jmp .slp_clamp
.slp_wide:
    sub eax, [uiOffX]
    mov ecx, WIDE_COLS * CHAR_W - 1
.slp_clamp:
    test eax, eax
    jns .slp_hi
    xor eax, eax
    ret
.slp_hi:
    cmp eax, ecx
    jbe .slp_ok
    mov eax, ecx
.slp_ok:
    ret

; Slot-Parameter synchronisieren und Auswahlmarker auf 0..len ausrichten
samped_sync_sel:
    call samped_load_cur_slot

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

; Aktiven Sample-Slot einmal auf Note AL vorhoeren
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
; Audio-Aufnahme ueber WinMM waveIn (4x 512 Bytes Streaming, Live-VU & Geraetewahl)
; ============================================================================

; Fertigen Aufnahme-Chunk (RCX = WAVEHDR-Ptr) auswerten: Live-VU berechnen & an recBuf anhaengen
rec_consume_chunk:
    push rbx
    push rsi
    push rdi

    mov ebx, [rcx + 12]
    test ebx, ebx
    jz .rc_done

    mov rsi, [rcx + 0]
    xor r8d, r8d
    xor r9d, r9d
.rc_pk:
    movzx eax, byte [rsi + r9]
    sub eax, 128
    jns .rc_pos
    neg eax
.rc_pos:
    cmp eax, r8d
    jbe .rc_pn
    mov r8d, eax
.rc_pn:
    inc r9d
    cmp r9d, ebx
    jb .rc_pk

    mov eax, [recVuLevel]
    imul eax, 3
    shr eax, 2
    cmp r8d, eax
    jbe .rc_vu_st
    mov eax, r8d
.rc_vu_st:
    mov [recVuLevel], eax

    cmp r8d, [recVuPeakHold]
    jb .rc_ph_dec
    mov [recVuPeakHold], r8d
    mov dword [recVuHoldCnt], 20
    jmp .rc_copy
.rc_ph_dec:
    cmp dword [recVuHoldCnt], 0
    je .rc_ph_drop
    dec dword [recVuHoldCnt]
    jmp .rc_copy
.rc_ph_drop:
    cmp dword [recVuPeakHold], 3
    jbe .rc_ph_zero
    sub dword [recVuPeakHold], 3
    jmp .rc_copy
.rc_ph_zero:
    mov dword [recVuPeakHold], 0

.rc_copy:
    mov eax, MAX_SAMP_BYTES
    sub eax, [recWritePos]
    jbe .rc_clr
    mov edx, ebx
    cmp edx, eax
    jbe .rc_cp_ok
    mov edx, eax
.rc_cp_ok:
    mov eax, [recWritePos]
    lea rdi, [recBuf]
    add rdi, rax
    push rcx
    mov ecx, edx
    rep movsb
    pop rcx
    add [recWritePos], edx

.rc_clr:
    mov dword [rcx + 12], 0

.rc_done:
    pop rdi
    pop rsi
    pop rbx
    ret

; Aufnahme starten (oder laufende Aufnahme stoppen)
start_recording:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x40

    cmp dword [isRecording], 0
    je .sr_open
    call stop_recording
    jmp .sr_done

.sr_open:
    call silence_audio

    mov dword [recWritePos], 0
    mov dword [recVuLevel], 0
    mov dword [recVuPeakHold], 0
    mov dword [recVuHoldCnt], 0

    lea rcx, [hWaveIn]
    mov edx, [seRecDevId]
    lea r8, [wfxRec]
    xor r9d, r9d
    mov qword [rsp + 0x20], 0
    mov qword [rsp + 0x28], 0
    call waveInOpen
    test eax, eax
    jnz .sr_err

    lea rdi, [recHdrs]
    xor eax, eax
    mov ecx, REC_BUF_COUNT * 48
    rep stosb

    xor ebx, ebx
.sr_buf_loop:
    imul eax, ebx, 48
    lea rsi, [recHdrs]
    add rsi, rax

    imul eax, ebx, REC_CHUNK_BYTES
    lea rdi, [recChunks]
    add rdi, rax

    mov [rsi + 0], rdi
    mov dword [rsi + 8], REC_CHUNK_BYTES

    mov rcx, [hWaveIn]
    mov rdx, rsi
    mov r8d, 48
    call waveInPrepareHeader
    test eax, eax
    jnz .sr_close_err

    mov rcx, [hWaveIn]
    mov rdx, rsi
    mov r8d, 48
    call waveInAddBuffer
    test eax, eax
    jnz .sr_close_err

    inc ebx
    cmp ebx, REC_BUF_COUNT
    jb .sr_buf_loop

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
    call waveInReset
    xor ebx, ebx
.sr_unp_err:
    imul eax, ebx, 48
    lea rdx, [recHdrs]
    add rdx, rax
    mov rcx, [hWaveIn]
    mov r8d, 48
    call waveInUnprepareHeader
    inc ebx
    cmp ebx, REC_BUF_COUNT
    jb .sr_unp_err

    mov rcx, [hWaveIn]
    call waveInClose
    mov qword [hWaveIn], 0
.sr_err:
    lea rcx, [stRecErr]
    call set_status
    mov dword [dirty], 1

.sr_done:
    add rsp, 0x40
    pop rdi
    pop rsi
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
    mov dword [recVuLevel], 0
    mov dword [recVuPeakHold], 0
    mov dword [recVuHoldCnt], 0

    mov rcx, [hWaveIn]
    test rcx, rcx
    jz .st_done

    call waveInStop
    mov rcx, [hWaveIn]
    call waveInReset

    xor ebx, ebx
.st_unp_loop:
    imul eax, ebx, 48
    lea rsi, [recHdrs]
    add rsi, rax
    mov rcx, rsi
    call rec_consume_chunk

    mov rcx, [hWaveIn]
    mov rdx, rsi
    mov r8d, 48
    call waveInUnprepareHeader

    inc ebx
    cmp ebx, REC_BUF_COUNT
    jb .st_unp_loop

    mov rcx, [hWaveIn]
    call waveInClose
    mov qword [hWaveIn], 0

    mov ebx, [recWritePos]
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
    mov dword [rdx + r10*4], 16384

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

    mov ecx, r10d
    call samped_commit_orig_slot
    call samped_sync_sel
    lea rcx, [stRecOk]
    call set_status
    mov eax, [curOct]
    imul eax, 12
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

; In der Hauptschleife fertige 512-Byte-Chunks einsammeln, Live-VU aktualisieren und neu einreihen
poll_recording:
    push rbx
    push rsi
    sub rsp, 0x28

    cmp dword [isRecording], 0
    je .pr_done

    xor ebx, ebx
.pr_loop:
    imul eax, ebx, 48
    lea rsi, [recHdrs]
    add rsi, rax
    test dword [rsi + 24], WHDR_DONE
    jz .pr_next

    mov rcx, rsi
    call rec_consume_chunk
    mov dword [dirty], 1

    cmp dword [recWritePos], MAX_SAMP_BYTES
    jb .pr_requeue
    call stop_recording
    jmp .pr_done

.pr_requeue:
    and dword [rsi + 24], ~WHDR_DONE
    mov rcx, [hWaveIn]
    mov rdx, rsi
    mov r8d, 48
    call waveInAddBuffer

.pr_next:
    inc ebx
    cmp ebx, REC_BUF_COUNT
    jb .pr_loop

.pr_done:
    add rsp, 0x28
    pop rsi
    pop rbx
    ret

; ============================================================================
; Destruktive Sample-Effekte und Schnittfunktionen auf aktiven Slot / Auswahl
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

    xor ebx, ebx
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
    mov ecx, [curInst]
    call samped_commit_orig_slot
    call samped_sync_sel
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

    mov ecx, [curInst]
    call samped_commit_orig_slot
    call samped_sync_sel
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

    mov ecx, [curInst]
    call samped_commit_orig_slot
    call samped_sync_sel
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
    mov ecx, [curInst]
    call samped_commit_orig_slot
    call samped_sync_sel
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

    mov ecx, [curInst]
    call samped_commit_orig_slot
    call samped_sync_sel
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
    mov ecx, ebx
    call samped_commit_orig_slot
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
    mov ebx, edx
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
    mov ecx, eax
    call samped_commit_orig_slot
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
; Zerstoerungsfreier Sample-FX-Prozessor (MOD:SAMPLE): pcmOrigSlots -> pcmSlots
; Eingabe: ECX = loopMode (ignoriert, bestehende Loops bleiben), EDX = doPreview (0/1)
; ============================================================================
sample_fx_render_to_slot:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x78

    mov [rsp + 0x20], ecx
    mov [rsp + 0x24], edx

    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    mov [rsp + 0x64], eax
    mov ebx, eax

    lea rdx, [slotOrigLen]
    mov r14d, [rdx + rbx*4]
    test r14d, r14d
    jnz .sfx_has_orig

    ; Falls noch kein Original-Backup existiert, aktuelles PCM als Basis uebernehmen
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .sfx_done
    lea rdx, [sampLenTbl]
    mov r14d, [rdx + rbx*4]
    test r14d, r14d
    jz .sfx_done
    cmp r14d, MAX_SAMP_BYTES
    jbe .sfx_fb_cp
    mov r14d, MAX_SAMP_BYTES
.sfx_fb_cp:
    lea rdx, [slotOrigLen]
    mov [rdx + rbx*4], r14d
    mov eax, ebx
    shl eax, 17
    lea rsi, [pcmSlots]
    add rsi, rax
    lea rdi, [pcmOrigSlots]
    add rdi, rax
    mov ecx, r14d
    rep movsb

.sfx_has_orig:
    mov [rsp + 0x48], r14d              ; origLen

    ; Ausgabelaenge ueber Dauer-Slider (5..100% der Originallaenge)
    mov eax, r14d
    imul rax, [seSmpParams + 4*4]
    xor edx, edx
    mov ecx, 100
    div rcx
    cmp eax, 32
    jae .sfx_olen_cap
    mov eax, r14d
    cmp eax, 32
    jbe .sfx_olen_cap
    mov eax, 32
.sfx_olen_cap:
    cmp eax, MAX_SAMP_BYTES
    jbe .sfx_olen_ok
    mov eax, MAX_SAMP_BYTES
.sfx_olen_ok:
    mov r12d, eax                       ; r12d = outLen

    ; ADSR-Phasenlaengen (bei 0/0/100/0 komplett neutral)
    mov eax, r12d
    imul eax, [seSmpParams + 0*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x28], eax               ; nA

    mov eax, r12d
    imul eax, [seSmpParams + 1*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x2C], eax               ; nD

    mov eax, r12d
    imul eax, [seSmpParams + 3*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x34], eax               ; nR

    mov eax, [rsp + 0x28]
    add eax, [rsp + 0x2C]
    add eax, [rsp + 0x34]
    cmp eax, r12d
    jbe .sfx_ns_calc
    mov ecx, r12d
    sub ecx, [rsp + 0x28]
    sub ecx, [rsp + 0x2C]
    jns .sfx_nr_fix
    xor ecx, ecx
.sfx_nr_fix:
    mov [rsp + 0x34], ecx
.sfx_ns_calc:
    mov eax, r12d
    sub eax, [rsp + 0x28]
    sub eax, [rsp + 0x2C]
    sub eax, [rsp + 0x34]
    jns .sfx_ns_ok
    xor eax, eax
.sfx_ns_ok:
    add eax, [rsp + 0x28]
    add eax, [rsp + 0x2C]
    mov [rsp + 0x30], eax               ; rStart = nA + nD + nS

    ; Sustain-Pegel 0..32768 (32768 bei 100% = verlustfreie Skalierung)
    mov eax, [seSmpParams + 2*4]
    shl eax, 15
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x38], eax               ; sustLvl

    ; Tiefpass-Alpha (0..256, 256 bei 100% = transparent)
    mov eax, [seSmpParams + 7*4]
    cmp eax, 5
    jae .sfx_lp_ok
    mov eax, 5
.sfx_lp_ok:
    shl eax, 8
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x40], eax               ; lpAlpha

    ; Lautstaerke-Faktor (0..256, 256 bei 100% = Originalpegel)
    mov eax, [seSmpParams + 8*4]
    shl eax, 8
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x44], eax               ; volScale

    ; Pitch-Sweep-Delta (-49000..+49000 bei 0..100, 0 bei 50 = neutral)
    mov eax, [seSmpParams + 5*4]
    sub eax, 50
    imul eax, 980
    mov [rsp + 0x4C], eax               ; sweepDelta

    mov eax, [seSmpParams + 6*4]
    mov [rsp + 0x60], eax               ; detuneVal

    mov dword [rsp + 0x3C], 0           ; lpState
    mov qword [rsp + 0x50], 0           ; readPos1 (48.16)
    mov qword [rsp + 0x58], 0           ; readPos2 (48.16)

    mov eax, [rsp + 0x64]
    shl eax, 17
    lea rsi, [pcmOrigSlots]
    add rsi, rax
    lea rdi, [pcmSlots]
    add rdi, rax

    xor ebx, ebx
.sfx_loop:
    ; 1. Aktuelle Leseschrittweite (16.16) fuer Pitch-Sweep berechnen
    mov r8d, 65536
    movsxd rax, dword [rsp + 0x4C]
    test rax, rax
    jz .sfx_step_ok
    imul rax, rbx
    cqo
    idiv r12
    add r8d, eax
    cmp r8d, 4096
    jge .sfx_step_ok
    mov r8d, 4096
.sfx_step_ok:

    ; 2. Hauptsample lesen
    mov rax, [rsp + 0x50]
    mov rcx, rax
    shr rcx, 16
    mov edx, r8d
    add rax, rdx
    mov [rsp + 0x50], rax
    xor r13d, r13d
    cmp ecx, [rsp + 0x48]
    jae .sfx_osc2
    movzx r13d, byte [rsi + rcx]
    sub r13d, 128

    ; 3. Optionaler Chorus / Detune-Layer
.sfx_osc2:
    mov ecx, [rsp + 0x60]
    test ecx, ecx
    jz .sfx_lp
    mov eax, r8d
    imul rax, rcx
    shr rax, 11
    add eax, r8d
    mov rdx, [rsp + 0x58]
    mov rcx, rdx
    shr rcx, 16
    add rdx, rax
    mov [rsp + 0x58], rdx
    xor eax, eax
    cmp ecx, [rsp + 0x48]
    jae .sfx_mix2
    movzx eax, byte [rsi + rcx]
    sub eax, 128
.sfx_mix2:
    add r13d, eax
    sar r13d, 1

    ; 4. Einpoliger Tiefpassfilter
.sfx_lp:
    shl r13d, 8
    mov eax, r13d
    sub eax, [rsp + 0x3C]
    imul eax, [rsp + 0x40]
    sar eax, 8
    add [rsp + 0x3C], eax
    mov r13d, [rsp + 0x3C]
    sar r13d, 8

    ; 5. ADSR-Huellkurve (r14d = 0..32768)
    mov ecx, [rsp + 0x28]
    cmp ebx, ecx
    jae .sfx_chk_d
    mov eax, ebx
    shl eax, 15
    xor edx, edx
    div ecx
    mov r14d, eax
    jmp .sfx_env_ok

.sfx_chk_d:
    mov r15d, ebx
    sub r15d, ecx
    mov ecx, [rsp + 0x2C]
    cmp r15d, ecx
    jae .sfx_chk_s
    mov eax, 32768
    sub eax, [rsp + 0x38]
    imul eax, r15d
    xor edx, edx
    div ecx
    mov r14d, 32768
    sub r14d, eax
    jmp .sfx_env_ok

.sfx_chk_s:
    mov r15d, [rsp + 0x30]
    cmp ebx, r15d
    jae .sfx_chk_r
    mov r14d, [rsp + 0x38]
    jmp .sfx_env_ok

.sfx_chk_r:
    mov ecx, [rsp + 0x34]
    test ecx, ecx
    jz .sfx_env_zero
    mov r15d, ebx
    sub r15d, [rsp + 0x30]
    mov eax, [rsp + 0x38]
    imul eax, r15d
    xor edx, edx
    div ecx
    mov r14d, [rsp + 0x38]
    sub r14d, eax
    jns .sfx_env_ok
.sfx_env_zero:
    xor r14d, r14d

.sfx_env_ok:
    ; 6. Huellkurve & Lautstaerke anwenden
    imul r13d, r14d
    sar r13d, 15
    imul r13d, [rsp + 0x44]
    sar r13d, 8
    add r13d, 128
    cmp r13d, 0
    jge .sfx_c_hi
    xor r13d, r13d
.sfx_c_hi:
    cmp r13d, 255
    jle .sfx_st
    mov r13d, 255
.sfx_st:
    mov [rdi + rbx], r13b

    inc ebx
    cmp ebx, r12d
    jb .sfx_loop

    ; Slot-Laenge aktualisieren (Name, C2-Stimmung und bestehende Loops bleiben erhalten)
    mov ebx, [rsp + 0x64]
    lea rdx, [sampHasPcm]
    mov byte [rdx + rbx], 1
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], r12d

    lea rdx, [sampLoopEndTbl]
    mov ecx, [rdx + rbx*4]
    cmp ecx, 0xFFFFF
    jae .sfx_fin
    cmp ecx, r12d
    jbe .sfx_lp_s
    mov ecx, r12d
    mov [rdx + rbx*4], ecx
.sfx_lp_s:
    lea rdx, [sampLoopStartTbl]
    mov eax, [rdx + rbx*4]
    lea r8d, [rax + 16]
    cmp r8d, ecx
    jbe .sfx_fin
    mov eax, ecx
    sub eax, 16
    jns .sfx_lp_st
    xor eax, eax
.sfx_lp_st:
    mov [rdx + rbx*4], eax

.sfx_fin:
    mov dword [seLastSlot], -1
    call samped_sync_sel
    lea rcx, [stSmpFx]
    call set_status

    cmp dword [rsp + 0x24], 0
    je .sfx_no_prev
    mov eax, [curOct]
    imul eax, 12
    call samped_preview
.sfx_no_prev:
    mov dword [dirty], 1

.sfx_done:
    add rsp, 0x78
    pop r15
    pop r14
    pop r13
    pop r12
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    ret

; Original-Sample (pcmOrigSlots) mit neutralen Slidern wiederherstellen ([Orig ] Button)
samped_restore_orig:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [slotOrigLen]
    mov ecx, [rdx + rbx*4]
    test ecx, ecx
    jz .sro_synth_def

    cmp ecx, MAX_SAMP_BYTES
    jbe .sro_cp
    mov ecx, MAX_SAMP_BYTES
.sro_cp:
    mov eax, ebx
    shl eax, 17
    lea rsi, [pcmOrigSlots]
    add rsi, rax
    lea rdi, [pcmSlots]
    add rdi, rax
    push rcx
    rep movsb
    pop rcx

    lea rdx, [sampHasPcm]
    mov byte [rdx + rbx], 1
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], ecx

    mov dword [seMode], 1
    mov dword [seSmpParams + 0*4], 0
    mov dword [seSmpParams + 1*4], 0
    mov dword [seSmpParams + 2*4], 100
    mov dword [seSmpParams + 3*4], 0
    mov dword [seSmpParams + 4*4], 100
    mov dword [seSmpParams + 5*4], 50
    mov dword [seSmpParams + 6*4], 0
    mov dword [seSmpParams + 7*4], 100
    mov dword [seSmpParams + 8*4], 100
    call samped_save_cur_slot

    mov dword [seLastSlot], -1
    call samped_sync_sel
    lea rcx, [stOrigOk]
    call set_status
    mov eax, [curOct]
    imul eax, 12
    call samped_preview
    mov dword [dirty], 1
    jmp .sro_done

.sro_synth_def:
    mov ecx, ebx
    call samped_reset_slot_synth
    call samped_load_cur_slot
    cmp dword [seLiveSynth], 0
    je .sro_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot

.sro_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; ADSR-Synthesizer-Generator (MOD:SYNTH) -> rendert 8-Bit-PCM in curInst
; Eingabe: ECX = loopMode (0=aus, 1=Auto-Loop berechnen, 2=bestehenden Loop behalten)
;          EDX = doPreview (0=stumm fuer Drag, 1=sofort vorhoeren)
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

    mov [rsp + 0x20], ecx
    mov [rsp + 0x60], edx
    mov dword [seSynthLfsr], 0x5EED1234

    mov eax, [seSynthParams + 4*4]
    cmp eax, 5
    jae .sr_d_ok
    mov eax, 5
.sr_d_ok:
    imul eax, 441
    cmp eax, MAX_SAMP_BYTES
    jbe .sr_n_ok
    mov eax, MAX_SAMP_BYTES
.sr_n_ok:
    mov [rsp + 0x24], eax
    mov r12d, eax

    mov eax, r12d
    imul eax, [seSynthParams + 0*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x28], eax

    mov eax, r12d
    imul eax, [seSynthParams + 1*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x2C], eax

    mov eax, r12d
    imul eax, [seSynthParams + 3*4]
    xor edx, edx
    mov ecx, 250
    div ecx
    mov [rsp + 0x34], eax

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
    mov [rsp + 0x30], eax
    mov ecx, [rsp + 0x28]
    add ecx, [rsp + 0x2C]
    add ecx, eax
    mov [rsp + 0x5C], ecx

    mov eax, [seSynthParams + 2*4]
    imul eax, 32767
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x38], eax

    mov eax, 25478490
    mov [rsp + 0x40], eax
    mov [rsp + 0x3C], eax

    mov ecx, [seSynthParams + 5*4]
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

    mov eax, [seSynthParams + 7*4]
    cmp eax, 5
    jae .sr_lp_ok
    mov eax, 5
.sr_lp_ok:
    shl eax, 8
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x50], eax

    mov eax, [sePulse]
    imul eax, 255
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x54], eax

    mov eax, [seSynthParams + 8*4]
    shl eax, 8
    xor edx, edx
    mov ecx, 100
    div ecx
    mov [rsp + 0x58], eax

    mov dword [rsp + 0x44], 0
    mov dword [rsp + 0x48], 0
    mov dword [rsp + 0x4C], 0

    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    shl eax, 17
    lea rdi, [pcmSlots]
    add rdi, rax

    mov r8d, [seWave]
    mov r9d, [rsp + 0x54]
    xor ebx, ebx

.sr_loop:
    mov ecx, [seSynthParams + 5*4]
    cmp ecx, 50
    je .sr_sw_done
    jb .sr_sw_down
    sub ecx, 50
    imul eax, ecx, 240
    add [rsp + 0x3C], eax
    cmp dword [rsp + 0x3C], 0x08000000
    jbe .sr_sw_done
    mov dword [rsp + 0x3C], 0x08000000
    jmp .sr_sw_done
.sr_sw_down:
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

    mov eax, [rsp + 0x3C]
    add [rsp + 0x44], eax
    mov eax, [rsp + 0x44]
    call synth_osc_eval
    mov r13d, eax

    mov ecx, [seSynthParams + 6*4]
    test ecx, ecx
    jz .sr_no_det
    cmp r8d, 4
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

    shl r13d, 8
    mov eax, r13d
    sub eax, [rsp + 0x4C]
    imul eax, [rsp + 0x50]
    sar eax, 8
    add [rsp + 0x4C], eax
    mov r13d, [rsp + 0x4C]
    sar r13d, 8

    mov ecx, [rsp + 0x28]
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
    sub r15d, ecx
    mov ecx, [rsp + 0x2C]
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
    mov r15d, [rsp + 0x5C]
    cmp ebx, r15d
    jae .sr_chk_r
    mov r14d, [rsp + 0x38]
    jmp .sr_env_ok

.sr_chk_r:
    mov ecx, [rsp + 0x34]
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
    imul r13d, r14d
    sar r13d, 15
    imul r13d, [rsp + 0x58]
    sar r13d, 8
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

    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    movzx esi, byte [rdx + rbx]
    mov byte [rdx + rbx], 1
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], r12d
    lea rdx, [sampC2Step]
    mov dword [rdx + rbx*4], 16384

    mov eax, [rsp + 0x20]
    cmp eax, 2
    jne .sr_chk_al

    test esi, esi
    jz .sr_use_autoloop
    lea rdx, [sampLoopEndTbl]
    mov ecx, [rdx + rbx*4]
    cmp ecx, 0xFFFFF
    jae .sr_name

    cmp ecx, r12d
    jbe .sr_keep_chk_s
    mov ecx, r12d
    mov [rdx + rbx*4], ecx
.sr_keep_chk_s:
    lea rdx, [sampLoopStartTbl]
    mov eax, [rdx + rbx*4]
    lea r8d, [rax + 16]
    cmp r8d, ecx
    jbe .sr_name
    mov eax, ecx
    sub eax, 16
    jns .sr_keep_st_s
    xor eax, eax
.sr_keep_st_s:
    mov [rdx + rbx*4], eax
    jmp .sr_name

.sr_use_autoloop:
    mov eax, [seAutoLoop]
.sr_chk_al:
    test eax, eax
    jz .sr_noloop
    mov eax, [rsp + 0x28]
    add eax, [rsp + 0x2C]
    mov ecx, [rsp + 0x5C]
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

    cmp dword [rsp + 0x60], 0
    je .sr_no_prev
    mov eax, [curOct]
    imul eax, 12
    call samped_preview
.sr_no_prev:
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

; Dispatcher: je nach seMode entweder Synth oder Sample-FX ausfuehren (ECX=loopMode, EDX=doPreview)
samped_apply_cur_mode:
    cmp dword [seMode], 0
    jne sample_fx_render_to_slot
    jmp synth_render_to_slot

; Preset (ECX = 0..5) laden, Slot auf MOD:SYNTH schalten und sofort rendern
apply_synth_preset:
    push rbx
    sub rsp, 0x20

    imul ebx, ecx, 11
    lea rsi, [sePresets]
    add rsi, rbx

    mov dword [seMode], 0
    movzx eax, byte [rsi + 0]
    mov [seWave], eax
    movzx eax, byte [rsi + 1]
    mov [sePulse], eax

    xor ecx, ecx
.asp_p:
    movzx eax, byte [rsi + 2 + rcx]
    lea rdx, [seSynthParams]
    mov [rdx + rcx*4], eax
    inc ecx
    cmp ecx, 8
    jb .asp_p

    movzx ecx, byte [rsi + 10]
    mov [seAutoLoop], ecx
    call samped_save_cur_slot

    mov ecx, [seAutoLoop]
    mov edx, 1
    call synth_render_to_slot

    add rsp, 0x20
    pop rbx
    ret

; ============================================================================
; Drag-Helfer fuer Loop-Striche (seDragMode 1/2) und Slider (seDragMode 3)
; ============================================================================

; Loop-Start oder Loop-Ende anhand virtueller X-Position (ECX) aktualisieren (Wide & Classic)
se_update_loop_drag:
    push rbx
    sub rsp, 0x20

    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    je .uld_done
    lea rdx, [sampLenTbl]
    mov r9d, [rdx + rbx*4]
    cmp r9d, 32
    jb .uld_done

    mov r8d, [seWfW]
    mov eax, ecx
    sub eax, 16
    jns .uld_lo
    xor eax, eax
.uld_lo:
    cmp eax, r8d
    jbe .uld_hi
    mov eax, r8d
.uld_hi:
    imul rax, r9
    xor edx, edx
    div r8

    cmp dword [seDragMode], 1
    jne .uld_end

    lea rdx, [sampLoopEndTbl]
    mov r10d, [rdx + rbx*4]
    cmp r10d, r9d
    jbe .uld_e_ok
    mov r10d, r9d
.uld_e_ok:
    sub r10d, 16
    jns .uld_ms_ok
    xor r10d, r10d
.uld_ms_ok:
    cmp eax, r10d
    jbe .uld_st_s
    mov eax, r10d
.uld_st_s:
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rbx*4], eax
    mov dword [dirty], 1
    jmp .uld_done

.uld_end:
    lea rdx, [sampLoopStartTbl]
    mov r10d, [rdx + rbx*4]
    add r10d, 16
    cmp r10d, r9d
    jbe .uld_me_ok
    mov r10d, r9d
.uld_me_ok:
    cmp eax, r10d
    jae .uld_st_e
    mov eax, r10d
.uld_st_e:
    cmp eax, r9d
    jbe .uld_st_e2
    mov eax, r9d
.uld_st_e2:
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rbx*4], eax
    mov dword [dirty], 1

.uld_done:
    add rsp, 0x20
    pop rbx
    ret

; Slider-Wert anhand virtueller X-Position (ECX) aktualisieren (Mode 3=Classic, 4=WideSmp, 5=WideSyn)
se_update_slider_drag:
    push rbx
    push r12
    sub rsp, 0x28

    mov ebx, [seDragParam]
    cmp ebx, 8
    jbe .usd_p_ok
    xor ebx, ebx
.usd_p_ok:

    mov eax, ecx
    cmp dword [seDragMode], 4
    je .usd_m4
    cmp dword [seDragMode], 5
    je .usd_m5

    ; Mode 3: Classic Slider (x=168, w=480)
    sub eax, 168
    mov r8d, 480
    lea r12, [seSynthParams]
    cmp dword [seMode], 0
    je .usd_scale
    lea r12, [seSmpParams]
    jmp .usd_scale

.usd_m4:
    ; Mode 4: Widescreen SAMPLE-FX Slider (x=192, w=360)
    sub eax, 192
    mov r8d, 360
    mov dword [seMode], 1
    lea r12, [seSmpParams]
    jmp .usd_scale

.usd_m5:
    ; Mode 5: Widescreen SYNTHESIZER Slider (x=936, w=360)
    sub eax, 936
    mov r8d, 360
    mov dword [seMode], 0
    lea r12, [seSynthParams]

.usd_scale:
    test eax, eax
    jns .usd_lo
    xor eax, eax
.usd_lo:
    cmp eax, r8d
    jbe .usd_hi
    mov eax, r8d
.usd_hi:
    imul eax, 100
    xor edx, edx
    div r8d

    xor r8d, r8d
    cmp ebx, 4
    je .usd_min5
    cmp ebx, 7
    jne .usd_chk_min
.usd_min5:
    mov r8d, 5
.usd_chk_min:
    cmp eax, r8d
    jae .usd_chk_max
    mov eax, r8d
.usd_chk_max:
    cmp eax, 100
    jbe .usd_cmp
    mov eax, 100
.usd_cmp:
    cmp eax, [r12 + rbx*4]
    je .usd_done
    mov [r12 + rbx*4], eax
    call samped_save_cur_slot
    mov dword [dirty], 1

    cmp dword [seLiveSynth], 0
    je .usd_done
    mov ecx, 2
    xor edx, edx
    call samped_apply_cur_mode

.usd_done:
    add rsp, 0x28
    pop r12
    pop rbx
    ret

; WM_MOUSEMOVE im Sample-Editor (ECX = lParam)
samped_mouse_move:
    sub rsp, 0x28
    cmp dword [seDragMode], 0
    je .smm_done
    call se_lparam_to_px
    mov ecx, eax
    cmp dword [seDragMode], 3
    jae .smm_slider
    call se_update_loop_drag
    jmp .smm_done
.smm_slider:
    call se_update_slider_drag
.smm_done:
    add rsp, 0x28
    ret

; WM_LBUTTONUP nach Drag-Vorgang
samped_mouse_up:
    push rbx
    sub rsp, 0x20

    mov ebx, [seDragMode]
    test ebx, ebx
    jz .smu_done
    mov dword [seDragMode], 0
    call ReleaseCapture

    cmp ebx, 3
    jb .smu_redraw
    cmp dword [seLiveSynth], 0
    je .smu_redraw
    mov eax, [curOct]
    imul eax, 12
    call samped_preview

.smu_redraw:
    mov dword [dirty], 1

.smu_done:
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
    call samped_load_cur_slot

    cmp ebx, VK_ESCAPE
    jne .sk_f1
    mov dword [seDragMode], 0
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
    push r14
    push r15
    sub rsp, 0x20

    mov r12d, ecx
    mov r13d, edx
    mov ebx, r8d
    call samped_load_cur_slot
    cmp dword [wideMode], 0
    jne .sm_wide_dispatch

    ; Zeile 0 (Kopfzeile): Slot, Loop, Zurueck
    test r13d, r13d
    jnz .sm_chk_wave

    cmp r12d, 57
    jb .sm_top_slot
    cmp r12d, 70
    ja .sm_done
    mov dword [seDragMode], 0
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
    ; Zeilen 2..19 (Wellenform): Loop-Striche L/E greifen & ziehen oder Auswahl setzen
    cmp r13d, 2
    jb .sm_chk_act
    cmp r13d, 19
    ja .sm_chk_act

    cmp dword [isRecording], 0
    jne .sm_done

    call samped_sync_sel
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    je .sm_done
    lea rdx, [sampLenTbl]
    mov r14d, [rdx + rax*4]
    test r14d, r14d
    jz .sm_done

    mov ecx, VK_SHIFT
    call GetKeyState
    movzx r15d, ax
    and r15d, 0x8000

    test ebx, ebx
    jnz .sm_wave_normal
    test r15d, r15d
    jnz .sm_wave_normal

    mov ecx, [curInst]
    and ecx, MAX_SAMPS - 1
    lea rdx, [sampLoopEndTbl]
    mov r10d, [rdx + rcx*4]
    cmp r10d, r14d
    ja .sm_wave_normal
    lea rdx, [sampLoopStartTbl]
    mov r11d, [rdx + rcx*4]
    cmp r10d, r11d
    jbe .sm_wave_normal

    mov eax, r11d
    imul rax, 928
    xor edx, edx
    div r14
    cmp eax, 926
    jbe .sm_ps_ok
    mov eax, 926
.sm_ps_ok:
    add eax, 16
    mov r11d, [mousePxX]
    sub r11d, eax
    jns .sm_d1_pos
    neg r11d
.sm_d1_pos:

    mov eax, r10d
    imul rax, 928
    xor edx, edx
    div r14
    cmp eax, 926
    jbe .sm_pe_ok
    mov eax, 926
.sm_pe_ok:
    add eax, 16
    mov r10d, [mousePxX]
    sub r10d, eax
    jns .sm_d2_pos
    neg r10d
.sm_d2_pos:

    cmp r11d, 18
    jbe .sm_grab_hit
    cmp r10d, 18
    ja .sm_wave_normal

.sm_grab_hit:
    mov dword [seDragMode], 1
    cmp r11d, r10d
    jb .sm_grab_go
    ja .sm_grab_end
    cmp r13d, 10
    jbe .sm_grab_go
.sm_grab_end:
    mov dword [seDragMode], 2
.sm_grab_go:
    mov rcx, [hWnd]
    call SetCapture
    mov ecx, [mousePxX]
    call se_update_loop_drag
    jmp .sm_done

.sm_wave_normal:
    mov eax, [mousePxX]
    sub eax, 16
    jns .sm_px_lo
    xor eax, eax
.sm_px_lo:
    cmp eax, 927
    jbe .sm_px_hi
    mov eax, 927
.sm_px_hi:
    imul rax, r14
    xor edx, edx
    mov ecx, 928
    div rcx
    mov r10d, eax

    test r15d, r15d
    jnz .sm_wave_loop

    test ebx, ebx
    jnz .sm_sel_end
    mov [seSelStart], r10d
    cmp r10d, [seSelEnd]
    jbe .sm_done
    mov [seSelEnd], r14d
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
    mov [rdx + rcx*4], r14d
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
    ; Zeile 28 (Wellenform & Pulsbreite: nur im MOD:SYNTH aktiv)
    cmp r13d, 28
    jne .sm_chk_sliders
    cmp dword [seMode], 0
    jne .sm_done

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
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
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
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sm_chk_sliders:
    ; Zeilen 30..38 (9 Slider: aktiv in MOD:SYNTH und MOD:SAMPLE)
    cmp r13d, 30
    jb .sm_chk_presets
    cmp r13d, 38
    ja .sm_chk_presets

    mov ecx, r13d
    sub ecx, 30
    lea rdx, [seSynthParams]
    cmp dword [seMode], 0
    je .sm_sl_arr_ok
    lea rdx, [seSmpParams]
.sm_sl_arr_ok:

    cmp r12d, 14
    jb .sm_done
    cmp r12d, 53
    ja .sm_sl_minus
    test ebx, ebx
    jnz .sm_done
    mov dword [seDragMode], 3
    mov [seDragParam], ecx
    mov rcx, [hWnd]
    call SetCapture
    mov ecx, [mousePxX]
    call se_update_slider_drag
    jmp .sm_done

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
    xor r8d, r8d
    cmp ecx, 4
    je .sm_min5
    cmp ecx, 7
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
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call samped_apply_cur_mode
    jmp .sm_done

.sm_chk_presets:
    ; Zeile 40 (6 Synth-Presets)
    cmp r13d, 40
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
    ; Zeile 42: [MOD:SYNTH ] [LIVE:AN ] [AUTOLP:AUS] [-> Slot] [Slot  -] [Slot  +] [Orig ]
    cmp r13d, 42
    jne .sm_chk_recbar
    test ebx, ebx
    jnz .sm_done

    ; [MOD:SYNTH ] / [MOD:SAMPLE] (Spalten 3..14)
    cmp r12d, 3
    jb .sm_done
    cmp r12d, 14
    ja .sm_ren_live
    cmp dword [seMode], 0
    jne .sm_to_synth

    ; Umschalten auf MOD:SAMPLE (falls Original oder PCM im Slot vorhanden)
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [slotOrigLen]
    cmp dword [rdx + rax*4], 0
    jne .sm_to_sample_ok
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    je .sm_done
    mov ecx, eax
    call samped_commit_orig_slot
    call samped_load_cur_slot
    mov dword [dirty], 1
    jmp .sm_done

.sm_to_sample_ok:
    mov dword [seMode], 1
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call sample_fx_render_to_slot
    jmp .sm_done

.sm_to_synth:
    mov dword [seMode], 0
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sm_ren_live:
    ; [LIVE:AN ] / [LIVE:AUS] (Spalten 16..25)
    cmp r12d, 16
    jb .sm_done
    cmp r12d, 25
    ja .sm_ren_al
    xor dword [seLiveSynth], 1
    jmp .sm_done

.sm_ren_al:
    ; [AUTOLP:AUS] / [AUTOLP:AN ] (Spalten 27..38)
    cmp r12d, 27
    jb .sm_done
    cmp r12d, 38
    ja .sm_ren_now
    xor dword [seAutoLoop], 1
    call samped_save_cur_slot
    cmp dword [seMode], 0
    jne .sm_done
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, [seAutoLoop]
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sm_ren_now:
    ; [-> Slot] (Spalten 40..48)
    cmp r12d, 40
    jb .sm_done
    cmp r12d, 48
    ja .sm_ren_sm
    mov ecx, 2
    mov edx, 1
    call samped_apply_cur_mode
    jmp .sm_done

.sm_ren_sm:
    ; [Slot  -] (Spalten 50..58)
    cmp r12d, 50
    jb .sm_done
    cmp r12d, 58
    ja .sm_ren_sp
    mov eax, [curInst]
    dec eax
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sm_ren_sp:
    ; [Slot  +] (Spalten 60..68)
    cmp r12d, 60
    jb .sm_done
    cmp r12d, 68
    ja .sm_ren_orig
    mov eax, [curInst]
    inc eax
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sm_ren_orig:
    ; [Orig ] (Spalten 70..76)
    cmp r12d, 70
    jb .sm_done
    cmp r12d, 76
    ja .sm_done
    call samped_restore_orig
    jmp .sm_done

.sm_chk_recbar:
    ; Zeile 47 (Aufnahmegeraet-Auswahl)
    cmp r13d, 47
    jne .sm_chk_bot

    cmp r12d, 6
    jb .sm_done
    cmp r12d, 8
    ja .sm_rb_name
    test ebx, ebx
    jnz .sm_done
    mov ecx, -1
    call samped_cycle_rec_dev
    jmp .sm_done

.sm_rb_name:
    cmp r12d, 10
    jb .sm_done
    cmp r12d, 37
    ja .sm_rb_next
    mov ecx, 1
    test ebx, ebx
    jz .sm_rb_go
    mov ecx, -1
.sm_rb_go:
    call samped_cycle_rec_dev
    jmp .sm_done

.sm_rb_next:
    cmp r12d, 39
    jb .sm_done
    cmp r12d, 41
    ja .sm_done
    test ebx, ebx
    jnz .sm_done
    mov ecx, 1
    call samped_cycle_rec_dev
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
    mov dword [seDragMode], 0
    call stop_recording
    mov dword [sampEdMode], 0
    jmp .sm_done

; ============================================================================
; Widescreen-Mausklick im Sample-Editor (124 Spalten, getrennte Sample-FX & Synth Panels)
; r12d = Spalte (0..123), r13d = Zeile (0..gridRows-1), ebx = Taste (0=L, 1=R)
; ============================================================================
.sm_wide_dispatch:
    ; --- Zeile 0: Kopfzeile ---
    test r13d, r13d
    jnz .sw_chk_wave

    ; [ESC = Zurueck Tracker] (Spalten 82..104)
    cmp r12d, 82
    jb .sw_top_sp
    cmp r12d, 104
    ja .sm_done
    mov dword [seDragMode], 0
    call stop_recording
    mov dword [sampEdMode], 0
    jmp .sm_done

.sw_top_sp:
    ; [Slot +] (Spalten 71..78)
    cmp r12d, 71
    jb .sw_top_sm
    cmp r12d, 78
    ja .sm_done
    mov eax, [curInst]
    inc eax
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sw_top_sm:
    ; [Slot -] (Spalten 62..69)
    cmp r12d, 62
    jb .sw_top_lp
    cmp r12d, 69
    ja .sm_done
    mov eax, [curInst]
    dec eax
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sw_top_lp:
    ; Loop:EIN/AUS (Spalten 51..59)
    cmp r12d, 51
    jb .sw_top_slot
    cmp r12d, 59
    ja .sm_done
    call toggle_wav_loop
    jmp .sm_done

.sw_top_slot:
    ; Slot:XX (Spalten 16..23)
    cmp r12d, 16
    jb .sm_done
    cmp r12d, 23
    ja .sm_done
    mov eax, [curInst]
    test ebx, ebx
    jnz .sw_ts_dec
    inc eax
    jmp .sw_ts_st
.sw_ts_dec:
    dec eax
.sw_ts_st:
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel
    jmp .sm_done

.sw_chk_wave:
    ; --- Zeilen 2 .. 2+seWfRows-1: Grosse Wellenform (Breite seWfW = 1456 px) ---
    cmp r13d, 2
    jb .sw_chk_act
    mov eax, [seWfRows]
    add eax, 2
    cmp r13d, eax
    jae .sw_chk_act

    cmp dword [isRecording], 0
    jne .sm_done

    call samped_sync_sel
    mov eax, [curInst]
    and eax, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rax], 0
    je .sm_done
    lea rdx, [sampLenTbl]
    mov r14d, [rdx + rax*4]
    test r14d, r14d
    jz .sm_done

    mov ecx, VK_SHIFT
    call GetKeyState
    movzx r15d, ax
    and r15d, 0x8000

    test ebx, ebx
    jnz .sw_wave_norm
    test r15d, r15d
    jnz .sw_wave_norm

    mov ecx, [curInst]
    and ecx, MAX_SAMPS - 1
    lea rdx, [sampLoopEndTbl]
    mov r10d, [rdx + rcx*4]
    cmp r10d, r14d
    ja .sw_wave_norm
    lea rdx, [sampLoopStartTbl]
    mov r11d, [rdx + rcx*4]
    cmp r10d, r11d
    jbe .sw_wave_norm

    mov r8d, [seWfW]
    lea r9d, [r8 - 2]

    mov eax, r11d
    imul rax, r8
    xor edx, edx
    div r14
    cmp eax, r9d
    jbe .sw_ps_ok
    mov eax, r9d
.sw_ps_ok:
    add eax, 16
    mov r11d, [mousePxX]
    sub r11d, eax
    jns .sw_d1_p
    neg r11d
.sw_d1_p:

    mov eax, r10d
    imul rax, r8
    xor edx, edx
    div r14
    cmp eax, r9d
    jbe .sw_pe_ok
    mov eax, r9d
.sw_pe_ok:
    add eax, 16
    mov r10d, [mousePxX]
    sub r10d, eax
    jns .sw_d2_p
    neg r10d
.sw_d2_p:

    cmp r11d, 18
    jbe .sw_grab_hit
    cmp r10d, 18
    ja .sw_wave_norm

.sw_grab_hit:
    mov dword [seDragMode], 1
    cmp r11d, r10d
    jb .sw_grab_go
    ja .sw_grab_end
    mov eax, [seWfRows]
    shr eax, 1
    add eax, 2
    cmp r13d, eax
    jbe .sw_grab_go
.sw_grab_end:
    mov dword [seDragMode], 2
.sw_grab_go:
    mov rcx, [hWnd]
    call SetCapture
    mov ecx, [mousePxX]
    call se_update_loop_drag
    jmp .sm_done

.sw_wave_norm:
    mov r8d, [seWfW]
    lea r9d, [r8 - 1]
    mov eax, [mousePxX]
    sub eax, 16
    jns .sw_px_lo
    xor eax, eax
.sw_px_lo:
    cmp eax, r9d
    jbe .sw_px_hi
    mov eax, r9d
.sw_px_hi:
    imul rax, r14
    xor edx, edx
    div r8
    mov r10d, eax

    test r15d, r15d
    jnz .sw_wave_lp

    test ebx, ebx
    jnz .sw_sel_end
    mov [seSelStart], r10d
    cmp r10d, [seSelEnd]
    jbe .sm_done
    mov [seSelEnd], r14d
    jmp .sm_done
.sw_sel_end:
    inc r10d
    mov [seSelEnd], r10d
    cmp r10d, [seSelStart]
    jae .sm_done
    mov dword [seSelStart], 0
    jmp .sm_done

.sw_wave_lp:
    mov ecx, [curInst]
    and ecx, MAX_SAMPS - 1
    test ebx, ebx
    jnz .sw_lp_e
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rcx*4], r10d
    lea rdx, [sampLoopEndTbl]
    cmp dword [rdx + rcx*4], 0xFFFFF
    jae .sw_lp_ae
    cmp [rdx + rcx*4], r10d
    ja .sm_done
.sw_lp_ae:
    mov [rdx + rcx*4], r14d
    jmp .sm_done
.sw_lp_e:
    inc r10d
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rcx*4], r10d
    lea rdx, [sampLoopStartTbl]
    cmp [rdx + rcx*4], r10d
    jb .sm_done
    mov dword [rdx + rcx*4], 0
    jmp .sm_done

.sw_chk_act:
    ; --- Zeile seActRow: Aktions-Buttons ---
    cmp r13d, [seActRow]
    jne .sw_chk_fx
    test ebx, ebx
    jnz .sm_done

    cmp r12d, 1
    jb .sm_done
    cmp r12d, 11
    ja .sw_act_rec
    call stop_recording
    call load_wav_dialog
    call samped_sync_sel
    jmp .sm_done
.sw_act_rec:
    cmp r12d, 13
    jb .sm_done
    cmp r12d, 23
    ja .sw_act_play
    call start_recording
    jmp .sm_done
.sw_act_play:
    cmp r12d, 25
    jb .sm_done
    cmp r12d, 35
    ja .sw_act_stop
    mov eax, [curOct]
    imul eax, 12
    call samped_preview
    jmp .sm_done
.sw_act_stop:
    cmp r12d, 37
    jb .sm_done
    cmp r12d, 46
    ja .sw_act_lp
    call silence_audio
    jmp .sm_done
.sw_act_lp:
    cmp r12d, 48
    jb .sm_done
    cmp r12d, 58
    ja .sw_act_s2l
    call toggle_wav_loop
    jmp .sm_done
.sw_act_s2l:
    cmp r12d, 60
    jb .sm_done
    cmp r12d, 74
    ja .sw_act_trim
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
.sw_act_trim:
    cmp r12d, 76
    jb .sm_done
    cmp r12d, 88
    ja .sw_act_clr
    call se_fx_trim
    jmp .sm_done
.sw_act_clr:
    cmp r12d, 90
    jb .sm_done
    cmp r12d, 102
    ja .sm_done
    call unload_wav_slot
    call samped_sync_sel
    jmp .sm_done

.sw_chk_fx:
    ; --- Zeile seFxRow: 7 destruktive Auswahl-Effekte ---
    cmp r13d, [seFxRow]
    jne .sw_chk_wave_row
    test ebx, ebx
    jnz .sm_done

    cmp r12d, 14
    jb .sm_done
    cmp r12d, 28
    ja .sw_fx1
    call se_fx_normalize
    jmp .sm_done
.sw_fx1:
    cmp r12d, 30
    jb .sm_done
    cmp r12d, 41
    ja .sw_fx2
    call se_fx_fade_in
    jmp .sm_done
.sw_fx2:
    cmp r12d, 43
    jb .sm_done
    cmp r12d, 54
    ja .sw_fx3
    call se_fx_fade_out
    jmp .sm_done
.sw_fx3:
    cmp r12d, 56
    jb .sm_done
    cmp r12d, 68
    ja .sw_fx4
    call se_fx_reverse
    jmp .sm_done
.sw_fx4:
    cmp r12d, 70
    jb .sm_done
    cmp r12d, 82
    ja .sw_fx5
    call se_fx_boost
    jmp .sm_done
.sw_fx5:
    cmp r12d, 84
    jb .sm_done
    cmp r12d, 94
    ja .sw_fx6
    call se_fx_halve
    jmp .sm_done
.sw_fx6:
    cmp r12d, 96
    jb .sm_done
    cmp r12d, 110
    ja .sm_done
    mov dword [seLastSlot], -1
    call samped_sync_sel
    jmp .sm_done

.sw_chk_wave_row:
    ; --- Zeile sePanelRow + 2 (Rechts: Welle & Pulsbreite im SYNTH-Panel) ---
    mov eax, [sePanelRow]
    add eax, 2
    cmp r13d, eax
    jne .sw_chk_sliders

    ; Pulsbreite Pw: 50% (Spalten 113..120, L:+5 / R:-5)
    cmp r12d, 113
    jb .sw_w_btns
    cmp r12d, 120
    ja .sm_done
    mov eax, [sePulse]
    test ebx, ebx
    jnz .sw_pw_dec
    add eax, 5
    cmp eax, 90
    jbe .sw_pw_st
    mov eax, 90
    jmp .sw_pw_st
.sw_pw_dec:
    sub eax, 5
    cmp eax, 10
    jge .sw_pw_st
    mov eax, 10
.sw_pw_st:
    mov [sePulse], eax
    mov dword [seMode], 0
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sw_w_btns:
    ; 5 Wellenform-Buttons ab Spalte 72, Schritt 8, Breite 7
    test ebx, ebx
    jnz .sm_done
    cmp r12d, 72
    jb .sm_done
    cmp r12d, 110
    ja .sm_done
    mov eax, r12d
    sub eax, 72
    xor edx, edx
    mov ecx, 8
    div ecx
    cmp eax, 5
    jae .sm_done
    cmp edx, 7
    jae .sm_done
    mov [seWave], eax
    mov dword [seMode], 0
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sw_chk_sliders:
    ; --- 9 Slider-Zeilen ab sePanelRow + 4 (Schritt seSlStep) fuer LINKS (Sample-FX) & RECHTS (Synth) ---
    mov eax, [sePanelRow]
    add eax, 4
    mov ecx, r13d
    sub ecx, eax
    js .sw_chk_btnrow
    cmp dword [seSlStep], 2
    jne .sw_sl_idx
    test ecx, 1
    jnz .sw_chk_btnrow
    shr ecx, 1
.sw_sl_idx:
    cmp ecx, 8
    ja .sw_chk_btnrow

    ; Links = SAMPLE-FX Panel (Spalten 16..57)
    cmp r12d, 60
    ja .sw_sl_right

    cmp r12d, 16
    jb .sm_done
    cmp r12d, 45
    ja .sw_sl_l_min
    test ebx, ebx
    jnz .sm_done
    mov dword [seDragMode], 4
    mov [seDragParam], ecx
    mov rcx, [hWnd]
    call SetCapture
    mov ecx, [mousePxX]
    call se_update_slider_drag
    jmp .sm_done

.sw_sl_l_min:
    lea rdx, [seSmpParams]
    cmp r12d, 47
    jb .sm_done
    cmp r12d, 49
    ja .sw_sl_l_pls
    mov eax, [rdx + rcx*4]
    sub eax, 5
    jmp .sw_sl_l_cl
.sw_sl_l_pls:
    cmp r12d, 55
    jb .sm_done
    cmp r12d, 57
    ja .sm_done
    mov eax, [rdx + rcx*4]
    add eax, 5
.sw_sl_l_cl:
    xor r8d, r8d
    cmp ecx, 4
    je .sw_sl_lm5
    cmp ecx, 7
    jne .sw_sl_lmn
.sw_sl_lm5:
    mov r8d, 5
.sw_sl_lmn:
    cmp eax, r8d
    jge .sw_sl_lmx
    mov eax, r8d
.sw_sl_lmx:
    cmp eax, 100
    jle .sw_sl_lst
    mov eax, 100
.sw_sl_lst:
    mov [rdx + rcx*4], eax
    mov dword [seMode], 1
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call sample_fx_render_to_slot
    jmp .sm_done

.sw_sl_right:
    ; Rechts = SYNTHESIZER Panel (Spalten 78..119)
    cmp r12d, 78
    jb .sm_done
    cmp r12d, 107
    ja .sw_sl_r_min
    test ebx, ebx
    jnz .sm_done
    mov dword [seDragMode], 5
    mov [seDragParam], ecx
    mov rcx, [hWnd]
    call SetCapture
    mov ecx, [mousePxX]
    call se_update_slider_drag
    jmp .sm_done

.sw_sl_r_min:
    lea rdx, [seSynthParams]
    cmp r12d, 109
    jb .sm_done
    cmp r12d, 111
    ja .sw_sl_r_pls
    mov eax, [rdx + rcx*4]
    sub eax, 5
    jmp .sw_sl_r_cl
.sw_sl_r_pls:
    cmp r12d, 117
    jb .sm_done
    cmp r12d, 119
    ja .sm_done
    mov eax, [rdx + rcx*4]
    add eax, 5
.sw_sl_r_cl:
    xor r8d, r8d
    cmp ecx, 4
    je .sw_sl_rm5
    cmp ecx, 7
    jne .sw_sl_rmn
.sw_sl_rm5:
    mov r8d, 5
.sw_sl_rmn:
    cmp eax, r8d
    jge .sw_sl_rmx
    mov eax, r8d
.sw_sl_rmx:
    cmp eax, 100
    jle .sw_sl_rst
    mov eax, 100
.sw_sl_rst:
    mov [rdx + rcx*4], eax
    mov dword [seMode], 0
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sw_chk_btnrow:
    ; --- Zeile seBtnRow: Links Sample-FX Buttons, Rechts 6 Synth-Presets ---
    cmp r13d, [seBtnRow]
    jne .sw_chk_btnrow2
    test ebx, ebx
    jnz .sm_done

    ; [Sample-FX anwenden] (Spalten 3..22)
    cmp r12d, 3
    jb .sm_done
    cmp r12d, 22
    ja .sw_br_orig
    mov dword [seMode], 1
    call samped_save_cur_slot
    mov ecx, 2
    mov edx, 1
    call sample_fx_render_to_slot
    jmp .sm_done

.sw_br_orig:
    ; [Original wiederherstellen] (Spalten 25..51)
    cmp r12d, 25
    jb .sm_done
    cmp r12d, 51
    ja .sw_br_pre
    call samped_restore_orig
    jmp .sm_done

.sw_br_pre:
    ; 6 Synth-Presets ueber seWPreBtnPos
    xor ecx, ecx
    lea rdx, [seWPreBtnPos]
.sw_pre_lp:
    movzx eax, byte [rdx + rcx*2 + 0]
    cmp r12d, eax
    jb .sw_pre_nx
    movzx r8d, byte [rdx + rcx*2 + 1]
    add eax, r8d
    cmp r12d, eax
    jae .sw_pre_nx
    call apply_synth_preset
    jmp .sm_done
.sw_pre_nx:
    inc ecx
    cmp ecx, 6
    jb .sw_pre_lp
    jmp .sm_done

.sw_chk_btnrow2:
    ; --- Zeile seBtnRow + 2: Rechts [Synth erzeugen] [Auto-Loop] [Live-Edit] ---
    mov eax, [seBtnRow]
    add eax, 2
    cmp r13d, eax
    jne .sw_chk_rec
    test ebx, ebx
    jnz .sm_done

    ; [Synth erzeugen] (Spalten 65..80)
    cmp r12d, 65
    jb .sm_done
    cmp r12d, 80
    ja .sw_br2_al
    mov dword [seMode], 0
    call samped_save_cur_slot
    mov ecx, 2
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sw_br2_al:
    ; [Auto-Loop: AN/AUS] (Spalten 82..97)
    cmp r12d, 82
    jb .sm_done
    cmp r12d, 97
    ja .sw_br2_lv
    xor dword [seAutoLoop], 1
    mov dword [seMode], 0
    call samped_save_cur_slot
    cmp dword [seLiveSynth], 0
    je .sm_done
    mov ecx, [seAutoLoop]
    mov edx, 1
    call synth_render_to_slot
    jmp .sm_done

.sw_br2_lv:
    ; [Live-Edit: AN/AUS] (Spalten 99..114)
    cmp r12d, 99
    jb .sm_done
    cmp r12d, 114
    ja .sm_done
    xor dword [seLiveSynth], 1
    jmp .sm_done

.sw_chk_rec:
    ; --- Zeile seRecRow: Mikrofon-Auswahl ---
    cmp r13d, [seRecRow]
    jne .sw_chk_bot

    cmp r12d, 6
    jb .sm_done
    cmp r12d, 8
    ja .sw_rec_name
    test ebx, ebx
    jnz .sm_done
    mov ecx, -1
    call samped_cycle_rec_dev
    jmp .sm_done
.sw_rec_name:
    cmp r12d, 10
    jb .sm_done
    cmp r12d, 45
    ja .sw_rec_next
    mov ecx, 1
    test ebx, ebx
    jz .sw_rec_go
    mov ecx, -1
.sw_rec_go:
    call samped_cycle_rec_dev
    jmp .sm_done
.sw_rec_next:
    cmp r12d, 47
    jb .sm_done
    cmp r12d, 49
    ja .sm_done
    test ebx, ebx
    jnz .sm_done
    mov ecx, 1
    call samped_cycle_rec_dev
    jmp .sm_done

.sw_chk_bot:
    ; --- Zeile botBarRow: Fusszeile ---
    cmp r13d, [botBarRow]
    jne .sm_done

    ; [?] Hilfe (Spalten 119..121)
    cmp r12d, 119
    jb .sw_bot_back
    cmp r12d, 121
    ja .sm_done
    xor dword [showHelp], 1
    jmp .sm_done

.sw_bot_back:
    ; [ESC = Tracker] (Spalten 90..104)
    cmp r12d, 90
    jb .sw_bot_play
    cmp r12d, 104
    ja .sm_done
    mov dword [seDragMode], 0
    call stop_recording
    mov dword [sampEdMode], 0
    jmp .sm_done

.sw_bot_play:
    ; [Leertaste = Song Play/Stop] (Spalten 20..47)
    cmp r12d, 20
    jb .sw_bot_oct
    cmp r12d, 47
    ja .sm_done
    test ebx, ebx
    jnz .sm_done
    xor dword [isPlaying], 1
    cmp dword [isPlaying], 0
    jne .sw_bp_start
    call silence_audio
    jmp .sm_done
.sw_bp_start:
    mov eax, [curPat]
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov eax, [editRow]
    mov [playRow], eax
    mov [dispPlayRow], eax
    mov dword [needPatTempo], 0
    mov dword [tickSamples], 1
    jmp .sm_done

.sw_bot_oct:
    ; Oktave:X (Spalten 10..17)
    cmp r12d, 10
    jb .sw_bot_slot
    cmp r12d, 17
    ja .sm_done
    test ebx, ebx
    jnz .sw_bo_dec
    cmp dword [curOct], 3
    jae .sm_done
    inc dword [curOct]
    jmp .sm_done
.sw_bo_dec:
    cmp dword [curOct], 0
    je .sm_done
    dec dword [curOct]
    jmp .sm_done

.sw_bot_slot:
    ; Slot:XX (Spalten 1..7)
    cmp r12d, 1
    jb .sm_done
    cmp r12d, 7
    ja .sm_done
    mov eax, [curInst]
    test ebx, ebx
    jnz .sw_bs_dec
    inc eax
    jmp .sw_bs_st
.sw_bs_dec:
    dec eax
.sw_bs_st:
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    call samped_sync_sel

.sm_done:
    add rsp, 0x20
    pop r15
    pop r14
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

    cmp dword [wideMode], 0
    jne .draw_wide

    xor ecx, ecx
    xor edx, edx
    mov r8d, WIN_W
    mov r9d, WIN_H
    mov r10d, COL_BG
    call fill_rect

    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    movzx r12d, byte [rdx + rbx]
    lea rdx, [sampLenTbl]
    mov r13d, [rdx + rbx*4]

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
    cmp dword [isRecording], 0
    je .ds_top_len
    mov eax, [recWritePos]
.ds_top_len:
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
    cmp dword [isRecording], 0
    je .ds_wf_frame
    mov r10d, COL_PURPHI
.ds_wf_frame:
    call fill_rect

    mov ecx, 16
    mov edx, 28
    mov r8d, 928
    mov r9d, 208
    mov r10d, COL_BLACK
    call fill_rect

    cmp dword [isRecording], 0
    jne .ds_rec_wf

    test r12d, r12d
    jz .ds_no_pcm
    test r13d, r13d
    jz .ds_no_pcm

    ; Auswahlbereich farblich hinterlegen
    mov eax, [seSelStart]
    imul rax, 928
    xor edx, edx
    div r13
    mov r14d, eax

    mov eax, [seSelEnd]
    imul rax, 928
    xor edx, edx
    div r13
    mov r15d, eax
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

    mov ecx, 16
    mov edx, 132
    mov r8d, 928
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    ; Wellenform in 464 Spalten a 2 px zeichnen
    mov eax, ebx
    shl eax, 17
    lea rbp, [pcmSlots]
    add rbp, rax

    xor edi, edi
.ds_wf_loop:
    mov eax, edi
    imul rax, r13
    xor edx, edx
    mov ecx, 464
    div rcx
    mov r8d, eax

    lea eax, [edi + 1]
    imul rax, r13
    xor edx, edx
    mov ecx, 464
    div rcx
    mov r9d, eax
    cmp r9d, r8d
    ja .ds_wf_span
    lea r9d, [r8 + 1]
.ds_wf_span:
    cmp r9d, r13d
    jbe .ds_wf_cap
    mov r9d, r13d
.ds_wf_cap:
    mov ecx, r9d
    sub ecx, r8d
    mov r11d, 1
    cmp ecx, 16
    jbe .ds_wf_step
    mov r11d, ecx
    shr r11d, 4
.ds_wf_step:
    mov r10d, 128
    mov esi, 128
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

    mov eax, esi
    sub eax, 128
    imul eax, 100
    sar eax, 7
    mov ecx, 132
    sub ecx, eax

    mov eax, r10d
    sub eax, 128
    imul eax, 100
    sar eax, 7
    mov edx, 132
    sub edx, eax
    cmp edx, ecx
    jge .ds_wf_h
    xchg ecx, edx
.ds_wf_h:
    sub edx, ecx
    inc edx
    mov r9d, edx
    mov edx, ecx

    lea ecx, [rdi*2 + 16]
    mov r8d, 2
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

    ; Loop-Linien mit greifbaren Anfassern (L oben, E unten)
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
    cmp eax, 926
    jbe .ds_lps_ok
    mov eax, 926
.ds_lps_ok:
    add eax, 16
    mov [rsp + 0x40], eax

    mov ecx, eax
    mov edx, 28
    mov r8d, 2
    mov r9d, 208
    mov r10d, COL_WHITE
    cmp dword [seDragMode], 1
    jne .ds_lps_col
    mov r8d, 3
    mov r10d, COL_CURSOR
.ds_lps_col:
    call fill_rect

    mov eax, [rsp + 0x40]
    sub eax, 6
    cmp eax, 16
    jge .ds_lps_gx1
    mov eax, 16
.ds_lps_gx1:
    cmp eax, 930
    jle .ds_lps_gx2
    mov eax, 930
.ds_lps_gx2:
    mov [rsp + 0x44], eax
    mov ecx, eax
    mov edx, 28
    mov r8d, 14
    mov r9d, 14
    mov r10d, COL_WHITE
    cmp dword [seDragMode], 1
    jne .ds_lps_gcol
    mov r10d, COL_CURSOR
.ds_lps_gcol:
    call fill_rect

    mov ecx, [rsp + 0x44]
    add ecx, 3
    mov edx, 29
    lea r8, [txtLoopL]
    mov r10d, COL_BLACK
    call draw_char_px

    mov eax, r15d
    imul rax, 928
    xor edx, edx
    div r13
    cmp eax, 926
    jbe .ds_lpe_ok
    mov eax, 926
.ds_lpe_ok:
    add eax, 16
    mov [rsp + 0x40], eax

    mov ecx, eax
    mov edx, 28
    mov r8d, 2
    mov r9d, 208
    mov r10d, COL_PURPHI
    cmp dword [seDragMode], 2
    jne .ds_lpe_col
    mov r8d, 3
    mov r10d, COL_CURSOR
.ds_lpe_col:
    call fill_rect

    mov eax, [rsp + 0x40]
    sub eax, 6
    cmp eax, 16
    jge .ds_lpe_gx1
    mov eax, 16
.ds_lpe_gx1:
    cmp eax, 930
    jle .ds_lpe_gx2
    mov eax, 930
.ds_lpe_gx2:
    mov [rsp + 0x44], eax
    mov ecx, eax
    mov edx, 222
    mov r8d, 14
    mov r9d, 14
    mov r10d, COL_PURPHI
    cmp dword [seDragMode], 2
    jne .ds_lpe_gcol
    mov r10d, COL_CURSOR
.ds_lpe_gcol:
    call fill_rect

    mov ecx, [rsp + 0x44]
    add ecx, 3
    mov edx, 223
    lea r8, [txtLoopE]
    mov r10d, COL_WHITE
    cmp dword [seDragMode], 2
    jne .ds_lpe_tcol
    mov r10d, COL_BLACK
.ds_lpe_tcol:
    call draw_char_px
    jmp .ds_wf_done

.ds_rec_wf:
    mov ecx, 16
    mov edx, 132
    mov r8d, 928
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    mov r15d, [recWritePos]
    mov eax, r15d
    imul rax, 464
    shr rax, 17
    cmp eax, 464
    jbe .ds_rwf_cc
    mov eax, 464
.ds_rwf_cc:
    mov r14d, eax
    test r14d, r14d
    jz .ds_rwf_head

    lea rbp, [recBuf]
    xor edi, edi
.ds_rwf_loop:
    mov eax, edi
    shl rax, 17
    xor edx, edx
    mov ecx, 464
    div rcx
    mov r8d, eax

    lea eax, [edi + 1]
    shl rax, 17
    xor edx, edx
    mov ecx, 464
    div rcx
    mov r9d, eax
    cmp r9d, r15d
    jbe .ds_rwf_cap
    mov r9d, r15d
.ds_rwf_cap:
    cmp r9d, r8d
    jbe .ds_rwf_next

    mov r10d, 128
    mov esi, 128
    mov ecx, r8d
.ds_rwf_mm:
    movzx eax, byte [rbp + rcx]
    cmp eax, r10d
    jae .ds_rwf_nmin
    mov r10d, eax
.ds_rwf_nmin:
    cmp eax, esi
    jbe .ds_rwf_nmax
    mov esi, eax
.ds_rwf_nmax:
    add ecx, 16
    cmp ecx, r9d
    jb .ds_rwf_mm

    mov eax, esi
    sub eax, 128
    imul eax, 100
    sar eax, 7
    mov ecx, 132
    sub ecx, eax

    mov eax, r10d
    sub eax, 128
    imul eax, 100
    sar eax, 7
    mov edx, 132
    sub edx, eax
    cmp edx, ecx
    jge .ds_rwf_h
    xchg ecx, edx
.ds_rwf_h:
    sub edx, ecx
    inc edx
    mov r9d, edx
    mov edx, ecx
    lea ecx, [rdi*2 + 16]
    mov r8d, 2
    mov r10d, COL_PURPHI
    call fill_rect

.ds_rwf_next:
    inc edi
    cmp edi, r14d
    jb .ds_rwf_loop

.ds_rwf_head:
    lea ecx, [r14*2 + 16]
    cmp ecx, 942
    jbe .ds_rwf_hd_ok
    mov ecx, 942
.ds_rwf_hd_ok:
    mov edx, 28
    mov r8d, 2
    mov r9d, 208
    mov r10d, COL_WHITE
    call fill_rect
    jmp .ds_wf_done

.ds_no_pcm:
    mov ecx, 16
    mov edx, 132
    mov r8d, 928
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    mov ecx, 2
    mov edx, 10
    lea r8, [seEmptyMsg]
    mov r9d, 76
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

    ; --- Zeilen 26..45: Instrument-Designer (MOD:SYNTH / MOD:SAMPLE) ---
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

    mov ecx, 1 * CHAR_W
    mov edx, 26 * CHAR_H
    mov r8d, 78 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    cmp dword [seMode], 0
    je .ds_shdr_bg
    mov r10d, COL_HDR_ON
.ds_shdr_bg:
    call fill_rect

    mov ecx, 2
    mov edx, 26
    lea r8, [seSynthHdr]
    mov r9d, 76
    mov r10d, COL_WHITE
    call draw_text

    ; Zeile 28: Wellenform-Buttons + Pulsbreite (im MOD:SAMPLE abgeblendet)
    mov ecx, 3
    mov edx, 28
    lea r8, [seWaveBar + 3]
    mov r9d, 6
    mov r10d, COL_TEAL
    cmp dword [seMode], 0
    je .ds_wl_c
    mov r10d, COL_MUTED
.ds_wl_c:
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
    cmp dword [seMode], 0
    jne .ds_wb_bg
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
    mov r10d, COL_MUTED
    cmp dword [seMode], 0
    jne .ds_wb_tx
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
    cmp dword [seMode], 0
    je .ds_pw_c
    mov r10d, COL_MUTED
.ds_pw_c:
    call draw_text

    ; Zeilen 30..38: 9 grafische Parameter-Slider
    xor edi, edi
.ds_sl_loop:
    lea r14d, [rdi + 30]

    imul eax, edi, 10
    lea r8, [seParamNames]
    add r8, rax
    mov ecx, 3
    mov edx, r14d
    mov r9d, 10
    mov r10d, COL_NOTE
    call draw_text

    mov ecx, 14 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 2
    mov r8d, 40 * CHAR_W
    mov r9d, CHAR_H - 4
    mov r10d, COL_BEAT
    call fill_rect

    lea rsi, [seSynthParams]
    cmp dword [seMode], 0
    je .ds_sl_p_ok
    lea rsi, [seSmpParams]
.ds_sl_p_ok:
    mov r15d, [rsi + rdi*4]
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
    mov r10d, COL_CYAN
    cmp edi, 4
    jb .ds_sl_chkdrag
    mov r10d, COL_PURPHI
    cmp edi, 8
    jb .ds_sl_chkdrag
    mov r10d, COL_NOTE
.ds_sl_chkdrag:
    cmp dword [seDragMode], 3
    jne .ds_sl_drawbar
    cmp edi, [seDragParam]
    jne .ds_sl_drawbar
    mov r10d, COL_CURSOR
.ds_sl_drawbar:
    call fill_rect
.ds_sl_nobar:

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

    mov eax, r15d
    lea r8, [lineBuf]
    call fmt_dec3
    mov ecx, 59
    mov edx, r14d
    lea r8, [lineBuf]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

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

    imul eax, edi, 11
    lea r8, [seParamHints]
    add r8, rax
    mov ecx, 67
    mov edx, r14d
    mov r9d, 11
    mov r10d, COL_TEAL
    call draw_text

    inc edi
    cmp edi, 9
    jb .ds_sl_loop

    ; Zeile 40: 6 Presets
    mov ecx, 3
    mov edx, 40
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
    mov edx, 40 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect

    mov ecx, r14d
    mov edx, 40
    lea r8, [sePresetBar]
    add r8, r14
    mov r9d, 9
    mov r10d, COL_WHITE
    call draw_text

    inc edi
    cmp edi, 6
    jb .ds_pr_loop

    ; Zeile 42: [MOD:SYNTH ] [LIVE:AN ] [AUTOLP:AUS] [-> Slot] [Slot  -] [Slot  +] [Orig ]
    mov ecx, 3 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 12 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    lea r14, [txtModSynth]
    cmp dword [seMode], 0
    je .ds_mod_b
    mov r10d, COL_CYAN
    lea r14, [txtModSample]
.ds_mod_b:
    call fill_rect
    mov ecx, 3
    mov edx, 42
    mov r8, r14
    mov r9d, 12
    mov r10d, COL_BLACK
    call draw_text

    mov ecx, 16 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 10 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    lea r14, [txtLiveOff]
    mov r15d, COL_WHITE
    cmp dword [seLiveSynth], 0
    je .ds_live_b
    mov r10d, COL_CURSOR
    lea r14, [txtLiveOn]
    mov r15d, COL_BLACK
.ds_live_b:
    call fill_rect
    mov ecx, 16
    mov edx, 42
    mov r8, r14
    mov r9d, 10
    mov r10d, r15d
    call draw_text

    mov ecx, 27 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 12 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    lea r14, [txtAlOff]
    cmp dword [seAutoLoop], 0
    je .ds_al_b
    mov r10d, COL_HDR_ON
    lea r14, [txtAlOn]
.ds_al_b:
    call fill_rect
    mov ecx, 27
    mov edx, 42
    mov r8, r14
    mov r9d, 12
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 40 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect
    mov ecx, 40
    mov edx, 42
    lea r8, [seRenderBar + 40]
    mov r9d, 9
    mov r10d, COL_BLACK
    call draw_text

    mov ecx, 50 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    call fill_rect
    mov ecx, 50
    mov edx, 42
    lea r8, [seRenderBar + 50]
    mov r9d, 9
    mov r10d, COL_CYAN
    call draw_text

    mov ecx, 60 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    call fill_rect
    mov ecx, 60
    mov edx, 42
    lea r8, [seRenderBar + 60]
    mov r9d, 9
    mov r10d, COL_CYAN
    call draw_text

    ; [Orig ] Button (Spalten 70..76)
    mov ecx, 70 * CHAR_W
    mov edx, 42 * CHAR_H
    mov r8d, 7 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    mov r15d, COL_MUTED
    lea rdx, [slotOrigLen]
    cmp dword [rdx + rbx*4], 0
    je .ds_orig_b
    mov r10d, COL_HDR_ON
    mov r15d, COL_WHITE
.ds_orig_b:
    call fill_rect
    mov ecx, 70
    mov edx, 42
    lea r8, [seRenderBar + 70]
    mov r9d, 7
    mov r10d, r15d
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
    jmp .ds_rec_bar

.ds_hint_norm:
    mov ecx, 2
    mov edx, 44
    lea r8, [seKeyHint]
    mov r9d, 76
    mov r10d, COL_NOTE
    call draw_text

.ds_rec_bar:
    ; --- Zeile 47: Aufnahmegeraet-Auswahl & Live-VU-Pegelbalken ---
    mov ecx, 1
    mov edx, 47
    lea r8, [txtMicLbl]
    mov r9d, 4
    mov r10d, COL_TEAL
    call draw_text

    mov ecx, 6 * CHAR_W
    mov edx, 47 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 6
    mov edx, 47
    lea r8, [txtDevPrev]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 10 * CHAR_W
    mov edx, 47 * CHAR_H
    mov r8d, 28 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BLACK
    call fill_rect
    mov ecx, 10
    mov edx, 47
    lea r8, [seRecDevName]
    mov r9d, 28
    mov r10d, COL_CYAN
    cmp dword [seRecDevCount], 0
    jne .ds_rb_dncol
    mov r10d, COL_PURPHI
.ds_rb_dncol:
    call draw_text

    mov ecx, 39 * CHAR_W
    mov edx, 47 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 39
    mov edx, 47
    lea r8, [txtDevNext]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 43
    mov edx, 47
    lea r8, [txtVuLbl]
    mov r9d, 3
    mov r10d, COL_TEAL
    call draw_text

    mov ecx, 47 * CHAR_W
    mov edx, 47 * CHAR_H + 1
    mov r8d, 24 * CHAR_W
    mov r9d, CHAR_H - 2
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, 47 * CHAR_W
    mov edx, 47 * CHAR_H + 2
    mov r8d, 24 * CHAR_W
    mov r9d, CHAR_H - 4
    mov r10d, COL_BEAT
    call fill_rect

    cmp dword [isRecording], 0
    je .ds_vu_idle

    mov eax, [recVuLevel]
    imul eax, 288
    xor edx, edx
    mov ecx, 127
    div ecx
    cmp eax, 288
    jbe .ds_vu_w_ok
    mov eax, 288
.ds_vu_w_ok:
    mov r14d, eax
    test r14d, r14d
    jz .ds_vu_peak

    mov r8d, r14d
    cmp r8d, 192
    jbe .ds_vu_z1
    mov r8d, 192
.ds_vu_z1:
    mov ecx, 47 * CHAR_W
    mov edx, 47 * CHAR_H + 2
    mov r9d, CHAR_H - 4
    mov r10d, COL_CYAN
    call fill_rect

    cmp r14d, 192
    jbe .ds_vu_peak
    mov r8d, r14d
    cmp r8d, 252
    jbe .ds_vu_z2
    mov r8d, 252
.ds_vu_z2:
    sub r8d, 192
    mov ecx, 47 * CHAR_W + 192
    mov edx, 47 * CHAR_H + 2
    mov r9d, CHAR_H - 4
    mov r10d, COL_CURSOR
    call fill_rect

    cmp r14d, 252
    jbe .ds_vu_peak
    mov r8d, r14d
    sub r8d, 252
    mov ecx, 47 * CHAR_W + 252
    mov edx, 47 * CHAR_H + 2
    mov r9d, CHAR_H - 4
    mov r10d, COL_PURPHI
    call fill_rect

.ds_vu_peak:
    mov eax, [recVuPeakHold]
    cmp eax, 3
    jb .ds_vu_time
    imul eax, 286
    xor edx, edx
    mov ecx, 127
    div ecx
    cmp eax, 286
    jbe .ds_vu_pk_ok
    mov eax, 286
.ds_vu_pk_ok:
    lea ecx, [rax + 47 * CHAR_W]
    mov edx, 47 * CHAR_H + 1
    mov r8d, 2
    mov r9d, CHAR_H - 2
    mov r10d, COL_WHITE
    cmp dword [recVuPeakHold], 122
    jb .ds_vu_pk_draw
    mov r10d, COL_PURPHI
.ds_vu_pk_draw:
    call fill_rect

.ds_vu_time:
    mov eax, [recWritePos]
    imul rax, 10
    xor edx, edx
    mov ecx, 22050
    div rcx
    cmp eax, 59
    jbe .ds_vt_ok
    mov eax, 59
.ds_vt_ok:
    xor edx, edx
    mov ecx, 10
    div ecx
    add al, '0'
    mov [recTimeTxt + 0], al
    mov byte [recTimeTxt + 1], '.'
    add dl, '0'
    mov [recTimeTxt + 2], dl
    mov dword [recTimeTxt + 3], '9.5/'

    mov ecx, 72
    mov edx, 47
    lea r8, [recTimeTxt]
    mov r9d, 7
    mov r10d, COL_CYAN
    cmp dword [recVuPeakHold], 122
    jb .ds_vt_draw
    mov r10d, COL_PURPHI
.ds_vt_draw:
    call draw_text
    jmp .ds_bot_bar

.ds_vu_idle:
    lea r8, [txtRecReady]
    mov r10d, COL_TEAL
    cmp dword [seRecDevCount], 0
    jne .ds_vi_draw
    lea r8, [txtRecNoDev]
    mov r10d, COL_PURPHI
.ds_vi_draw:
    mov ecx, 72
    mov edx, 47
    mov r9d, 7
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
    jmp .ds_epilog

; ============================================================================
; WIDESCREEN SAMPLE- & INSTRUMENT-EDITOR (124 Spalten, getrennte Sample-FX & Synth Panels)
; ============================================================================
.draw_wide:
    mov ebx, [curInst]
    and ebx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
    movzx r12d, byte [rdx + rbx]
    lea rdx, [sampLenTbl]
    mov r13d, [rdx + rbx*4]

    ; --- Zeile 0: Widescreen-Kopfzeile ---
    mov al, bl
    lea r8, [seWTopBar + 21]
    call fmt_hex2

    mov eax, ebx
    imul eax, 12
    lea rsi, [sampNames]
    add rsi, rax
    lea rdi, [seWTopBar + 24]
    mov ecx, 12
    rep movsb

    mov eax, r13d
    cmp dword [isRecording], 0
    je .dw_tlen
    mov eax, [recWritePos]
.dw_tlen:
    lea r8, [seWTopBar + 44]
    call fmt_hex5

    lea rdx, [sampLoopEndTbl]
    mov eax, [rdx + rbx*4]
    mov byte [seWTopBar + 56], 'A'
    mov byte [seWTopBar + 57], 'U'
    mov byte [seWTopBar + 58], 'S'
    test r12d, r12d
    jz .dw_tlp
    cmp eax, r13d
    ja .dw_tlp
    mov byte [seWTopBar + 56], 'E'
    mov byte [seWTopBar + 57], 'I'
    mov byte [seWTopBar + 58], 'N'
.dw_tlp:
    lea rsi, [topBar + 72]
    lea rdi, [seWTopBar + 116]
    mov ecx, 7
    rep movsb

    xor ecx, ecx
    xor edx, edx
    lea r8, [seWTopBar]
    mov r9d, 123
    mov r10d, COL_CYAN
    call draw_text

    ; Hervorgehobene Buttons in Kopfzeile: [Slot -] [Slot +] [ESC = Zurueck Tracker]
    mov ecx, 62 * CHAR_W
    xor edx, edx
    mov r8d, 8 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    call fill_rect
    mov ecx, 62
    xor edx, edx
    lea r8, [seWTopBar + 62]
    mov r9d, 8
    mov r10d, COL_CYAN
    call draw_text

    mov ecx, 71 * CHAR_W
    xor edx, edx
    mov r8d, 8 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    call fill_rect
    mov ecx, 71
    xor edx, edx
    lea r8, [seWTopBar + 71]
    mov r9d, 8
    mov r10d, COL_CYAN
    call draw_text

    mov ecx, 82 * CHAR_W
    xor edx, edx
    mov r8d, 23 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 82
    xor edx, edx
    lea r8, [seWTopBar + 82]
    mov r9d, 23
    mov r10d, COL_WHITE
    call draw_text

    ; --- Zeilen 2 .. 2+seWfRows-1: Grosse Wellenform-Box (x=12..1476, innen x=16, w=1456) ---
    mov eax, [seWfRows]
    imul eax, CHAR_H
    mov r9d, eax                        ; outerH
    sub eax, 8
    mov [rsp + 0x48], eax               ; innerH
    shr eax, 1
    lea edx, [rax + 28]
    mov [rsp + 0x4C], edx               ; midY
    sub eax, 4
    mov [rsp + 0x50], eax               ; amp

    mov ecx, 12
    mov edx, 24
    mov r8d, 1464
    mov r10d, COL_HDR_ON
    cmp dword [isRecording], 0
    je .dw_wfr
    mov r10d, COL_PURPHI
.dw_wfr:
    call fill_rect

    mov ecx, 16
    mov edx, 28
    mov r8d, 1456
    mov r9d, [rsp + 0x48]
    mov r10d, COL_BLACK
    call fill_rect

    cmp dword [isRecording], 0
    jne .dw_rec_wf

    test r12d, r12d
    jz .dw_no_pcm
    test r13d, r13d
    jz .dw_no_pcm

    ; Auswahlbereich hinterlegen
    mov eax, [seSelStart]
    imul rax, 1456
    xor edx, edx
    div r13
    mov r14d, eax

    mov eax, [seSelEnd]
    imul rax, 1456
    xor edx, edx
    div r13
    mov r15d, eax
    cmp r15d, r14d
    jbe .dw_no_sel
    lea ecx, [r14 + 16]
    mov edx, 28
    mov r8d, r15d
    sub r8d, r14d
    mov r9d, [rsp + 0x48]
    mov r10d, COL_BEAT
    call fill_rect
.dw_no_sel:

    mov ecx, 16
    mov edx, [rsp + 0x4C]
    mov r8d, 1456
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    ; Wellenform in 728 Spalten a 2 px zeichnen
    mov eax, ebx
    shl eax, 17
    lea rbp, [pcmSlots]
    add rbp, rax

    xor edi, edi
.dw_wf_loop:
    mov eax, edi
    imul rax, r13
    xor edx, edx
    mov ecx, 728
    div rcx
    mov r8d, eax

    lea eax, [edi + 1]
    imul rax, r13
    xor edx, edx
    mov ecx, 728
    div rcx
    mov r9d, eax
    cmp r9d, r8d
    ja .dw_wf_sp
    lea r9d, [r8 + 1]
.dw_wf_sp:
    cmp r9d, r13d
    jbe .dw_wf_cp
    mov r9d, r13d
.dw_wf_cp:
    mov ecx, r9d
    sub ecx, r8d
    mov r11d, 1
    cmp ecx, 16
    jbe .dw_wf_st
    mov r11d, ecx
    shr r11d, 4
.dw_wf_st:
    mov r10d, 128
    mov esi, 128
    mov ecx, r8d
.dw_wf_mm:
    movzx eax, byte [rbp + rcx]
    cmp eax, r10d
    jae .dw_wf_nm
    mov r10d, eax
.dw_wf_nm:
    cmp eax, esi
    jbe .dw_wf_nx
    mov esi, eax
.dw_wf_nx:
    add ecx, r11d
    cmp ecx, r9d
    jb .dw_wf_mm

    mov eax, esi
    sub eax, 128
    imul eax, [rsp + 0x50]
    sar eax, 7
    mov ecx, [rsp + 0x4C]
    sub ecx, eax

    mov eax, r10d
    sub eax, 128
    imul eax, [rsp + 0x50]
    sar eax, 7
    mov edx, [rsp + 0x4C]
    sub edx, eax
    cmp edx, ecx
    jge .dw_wf_h
    xchg ecx, edx
.dw_wf_h:
    sub edx, ecx
    inc edx
    mov r9d, edx
    mov edx, ecx

    lea ecx, [rdi*2 + 16]
    mov r8d, 2
    mov r10d, COL_TEAL
    lea eax, [rdi*2]
    cmp eax, r14d
    jb .dw_wf_dr
    cmp eax, r15d
    jae .dw_wf_dr
    mov r10d, COL_CYAN
.dw_wf_dr:
    call fill_rect

    inc edi
    cmp edi, 728
    jb .dw_wf_loop

    ; Loop-Linien mit greifbaren Anfassern (L oben, E unten - exakt ausgerichtet!)
    lea rdx, [sampLoopEndTbl]
    mov r15d, [rdx + rbx*4]
    cmp r15d, r13d
    ja .dw_wf_done
    lea rdx, [sampLoopStartTbl]
    mov r14d, [rdx + rbx*4]
    cmp r15d, r14d
    jbe .dw_wf_done

    mov eax, r14d
    imul rax, 1456
    xor edx, edx
    div r13
    cmp eax, 1454
    jbe .dw_lps_ok
    mov eax, 1454
.dw_lps_ok:
    add eax, 16
    mov [rsp + 0x40], eax

    mov ecx, eax
    mov edx, 28
    mov r8d, 2
    mov r9d, [rsp + 0x48]
    mov r10d, COL_WHITE
    cmp dword [seDragMode], 1
    jne .dw_lps_c
    mov r8d, 3
    mov r10d, COL_CURSOR
.dw_lps_c:
    call fill_rect

    mov eax, [rsp + 0x40]
    sub eax, 6
    cmp eax, 16
    jge .dw_lps_x1
    mov eax, 16
.dw_lps_x1:
    cmp eax, 1458
    jle .dw_lps_x2
    mov eax, 1458
.dw_lps_x2:
    mov [rsp + 0x44], eax
    mov ecx, eax
    mov edx, 28
    mov r8d, 14
    mov r9d, 14
    mov r10d, COL_WHITE
    cmp dword [seDragMode], 1
    jne .dw_lps_gc
    mov r10d, COL_CURSOR
.dw_lps_gc:
    call fill_rect

    mov ecx, [rsp + 0x44]
    add ecx, 3
    mov edx, 29
    lea r8, [txtLoopL]
    mov r10d, COL_BLACK
    call draw_char_px

    mov eax, r15d
    imul rax, 1456
    xor edx, edx
    div r13
    cmp eax, 1454
    jbe .dw_lpe_ok
    mov eax, 1454
.dw_lpe_ok:
    add eax, 16
    mov [rsp + 0x40], eax

    mov ecx, eax
    mov edx, 28
    mov r8d, 2
    mov r9d, [rsp + 0x48]
    mov r10d, COL_PURPHI
    cmp dword [seDragMode], 2
    jne .dw_lpe_c
    mov r8d, 3
    mov r10d, COL_CURSOR
.dw_lpe_c:
    call fill_rect

    mov eax, [rsp + 0x40]
    sub eax, 6
    cmp eax, 16
    jge .dw_lpe_x1
    mov eax, 16
.dw_lpe_x1:
    cmp eax, 1458
    jle .dw_lpe_x2
    mov eax, 1458
.dw_lpe_x2:
    mov [rsp + 0x44], eax
    mov ecx, eax
    mov edx, 28
    add edx, [rsp + 0x48]
    sub edx, 14
    mov r14d, edx                       ; y des E-Anfassers merken
    mov r8d, 14
    mov r9d, 14
    mov r10d, COL_PURPHI
    cmp dword [seDragMode], 2
    jne .dw_lpe_gc
    mov r10d, COL_CURSOR
.dw_lpe_gc:
    call fill_rect

    mov ecx, [rsp + 0x44]
    add ecx, 3
    lea edx, [r14 + 1]
    lea r8, [txtLoopE]
    mov r10d, COL_WHITE
    cmp dword [seDragMode], 2
    jne .dw_lpe_tc
    mov r10d, COL_BLACK
.dw_lpe_tc:
    call draw_char_px
    jmp .dw_wf_done

.dw_rec_wf:
    mov ecx, 16
    mov edx, [rsp + 0x4C]
    mov r8d, 1456
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    mov r15d, [recWritePos]
    mov eax, r15d
    imul rax, 728
    shr rax, 17
    cmp eax, 728
    jbe .dw_rw_cc
    mov eax, 728
.dw_rw_cc:
    mov r14d, eax
    test r14d, r14d
    jz .dw_rw_hd

    lea rbp, [recBuf]
    xor edi, edi
.dw_rw_loop:
    mov eax, edi
    shl rax, 17
    xor edx, edx
    mov ecx, 728
    div rcx
    mov r8d, eax

    lea eax, [edi + 1]
    shl rax, 17
    xor edx, edx
    mov ecx, 728
    div rcx
    mov r9d, eax
    cmp r9d, r15d
    jbe .dw_rw_cp
    mov r9d, r15d
.dw_rw_cp:
    cmp r9d, r8d
    jbe .dw_rw_nx

    mov r10d, 128
    mov esi, 128
    mov ecx, r8d
.dw_rw_mm:
    movzx eax, byte [rbp + rcx]
    cmp eax, r10d
    jae .dw_rw_nm
    mov r10d, eax
.dw_rw_nm:
    cmp eax, esi
    jbe .dw_rw_nxm
    mov esi, eax
.dw_rw_nxm:
    add ecx, 16
    cmp ecx, r9d
    jb .dw_rw_mm

    mov eax, esi
    sub eax, 128
    imul eax, [rsp + 0x50]
    sar eax, 7
    mov ecx, [rsp + 0x4C]
    sub ecx, eax

    mov eax, r10d
    sub eax, 128
    imul eax, [rsp + 0x50]
    sar eax, 7
    mov edx, [rsp + 0x4C]
    sub edx, eax
    cmp edx, ecx
    jge .dw_rw_h
    xchg ecx, edx
.dw_rw_h:
    sub edx, ecx
    inc edx
    mov r9d, edx
    mov edx, ecx
    lea ecx, [rdi*2 + 16]
    mov r8d, 2
    mov r10d, COL_PURPHI
    call fill_rect
.dw_rw_nx:
    inc edi
    cmp edi, r14d
    jb .dw_rw_loop

.dw_rw_hd:
    lea ecx, [r14*2 + 16]
    cmp ecx, 1470
    jbe .dw_rw_ho
    mov ecx, 1470
.dw_rw_ho:
    mov edx, 28
    mov r8d, 2
    mov r9d, [rsp + 0x48]
    mov r10d, COL_WHITE
    call fill_rect
    jmp .dw_wf_done

.dw_no_pcm:
    mov ecx, 16
    mov edx, [rsp + 0x4C]
    mov r8d, 1456
    mov r9d, 1
    mov r10d, COL_DOT
    call fill_rect

    mov ecx, 24
    mov edx, [seWfRows]
    shr edx, 1
    add edx, 2
    lea r8, [seEmptyMsg]
    mov r9d, 76
    mov r10d, COL_NOTE
    call draw_text

.dw_wf_done:
    ; --- Zeile seInfoRow: Marker- & Maus-Info ---
    mov eax, [seSelStart]
    lea r8, [seWInfoBar + 10]
    call fmt_hex5
    mov eax, [seSelEnd]
    lea r8, [seWInfoBar + 16]
    call fmt_hex5
    lea rdx, [sampLoopStartTbl]
    mov eax, [rdx + rbx*4]
    lea r8, [seWInfoBar + 30]
    call fmt_hex5
    lea rdx, [sampLoopEndTbl]
    mov eax, [rdx + rbx*4]
    lea r8, [seWInfoBar + 36]
    call fmt_hex5

    xor ecx, ecx
    mov edx, [seInfoRow]
    lea r8, [seWInfoBar]
    mov r9d, 123
    mov r10d, COL_NOTE
    call draw_text

    ; --- Zeile seActRow: Aktions-Buttons ---
    mov r14d, [seActRow]
    mov r15d, r14d
    imul r15d, CHAR_H

    mov ecx, 1 * CHAR_W
    mov edx, r15d
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 1
    mov edx, r14d
    lea r8, [seWActBar + 1]
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 13 * CHAR_W
    mov edx, r15d
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    lea rbp, [txtRecBtnOn]
    cmp dword [isRecording], 0
    je .dw_act_rc
    mov r10d, COL_PURPHI
    lea rbp, [txtRecBtnOff]
.dw_act_rc:
    call fill_rect
    mov ecx, 13
    mov edx, r14d
    mov r8, rbp
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 25 * CHAR_W
    mov edx, r15d
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 25
    mov edx, r14d
    lea r8, [seWActBar + 25]
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 37 * CHAR_W
    mov edx, r15d
    mov r8d, 10 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 37
    mov edx, r14d
    lea r8, [seWActBar + 37]
    mov r9d, 10
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 48 * CHAR_W
    mov edx, r15d
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    lea rbp, [txtWLoopOff]
    test r12d, r12d
    jz .dw_act_lp
    lea rdx, [sampLoopEndTbl]
    cmp [rdx + rbx*4], r13d
    ja .dw_act_lp
    mov r10d, COL_HDR_ON
    lea rbp, [txtWLoopOn]
.dw_act_lp:
    call fill_rect
    mov ecx, 48
    mov edx, r14d
    mov r8, rbp
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 60 * CHAR_W
    mov edx, r15d
    mov r8d, 15 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 60
    mov edx, r14d
    lea r8, [seWActBar + 60]
    mov r9d, 15
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 76 * CHAR_W
    mov edx, r15d
    mov r8d, 13 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    call fill_rect
    mov ecx, 76
    mov edx, r14d
    lea r8, [seWActBar + 76]
    mov r9d, 13
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 90 * CHAR_W
    mov edx, r15d
    mov r8d, 13 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 90
    mov edx, r14d
    lea r8, [seWActBar + 90]
    mov r9d, 13
    mov r10d, COL_WHITE
    call draw_text

    ; --- Zeile seFxRow: 7 destruktive Bearbeitungs-Buttons ---
    mov ecx, 1
    mov edx, [seFxRow]
    lea r8, [seWFxBar + 1]
    mov r9d, 12
    mov r10d, COL_TEAL
    call draw_text

    xor edi, edi
.dw_fx_loop:
    lea rsi, [seWFxBtnPos]
    movzx r14d, byte [rsi + rdi*2 + 0]
    movzx r15d, byte [rsi + rdi*2 + 1]

    mov ecx, r14d
    imul ecx, CHAR_W
    mov edx, [seFxRow]
    imul edx, CHAR_H
    mov r8d, r15d
    imul r8d, CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    test r12d, r12d
    jz .dw_fx_bg
    mov r10d, COL_HDR_ON
.dw_fx_bg:
    call fill_rect

    mov ecx, r14d
    mov edx, [seFxRow]
    lea r8, [seWFxBar]
    add r8, r14
    mov r9d, r15d
    mov r10d, COL_MUTED
    test r12d, r12d
    jz .dw_fx_tx
    mov r10d, COL_WHITE
.dw_fx_tx:
    call draw_text

    inc edi
    cmp edi, 7
    jb .dw_fx_loop

    ; ========================================================================
    ; ZWEI GETRENNTE BOXEN NEBENEINANDER: LINKS = SAMPLE-FX, RECHTS = SYNTH
    ; ========================================================================
    mov ebp, [seRecRow]
    dec ebp
    sub ebp, [sePanelRow]               ; panelRows (21 oder 30)
    imul ebp, CHAR_H                    ; panelH in Pixeln

    ; --- LINKE BOX (Spalten 1..60): SAMPLE-FX ---
    mov ecx, 1 * CHAR_W
    mov edx, [sePanelRow]
    imul edx, CHAR_H
    mov r8d, 60 * CHAR_W
    mov r9d, ebp
    mov r10d, COL_HDR_ON
    cmp dword [seMode], 1
    je .dw_lb_fr
    mov r10d, COL_BEAT
.dw_lb_fr:
    call fill_rect

    mov ecx, 1 * CHAR_W + 3
    mov edx, [sePanelRow]
    imul edx, CHAR_H
    add edx, 3
    mov r8d, 60 * CHAR_W - 6
    lea r9d, [rbp - 6]
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, 1 * CHAR_W
    mov edx, [sePanelRow]
    imul edx, CHAR_H
    mov r8d, 60 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    cmp dword [seMode], 1
    je .dw_lb_hd
    mov r10d, COL_HDR_OFF
.dw_lb_hd:
    call fill_rect

    mov ecx, 2
    mov edx, [sePanelRow]
    lea r8, [seWSmpHdr]
    mov r9d, 58
    mov r10d, COL_WHITE
    call draw_text

    ; Statuszeile Sample-FX (sePanelRow + 2)
    lea r8, [seWSmpNo]
    mov r10d, COL_PURPHI
    lea rdx, [slotOrigLen]
    cmp dword [rdx + rbx*4], 0
    jne .dw_smp_has
    test r12d, r12d
    jz .dw_smp_st
    lea r8, [seWSmpPcm]
    mov r10d, COL_TEAL
    jmp .dw_smp_st
.dw_smp_has:
    lea r8, [seWSmpOk]
    mov r10d, COL_CYAN
.dw_smp_st:
    mov ecx, 3
    mov edx, [sePanelRow]
    add edx, 2
    mov r9d, 57
    call draw_text

    ; --- RECHTE BOX (Spalten 63..122): SYNTHESIZER ---
    mov ecx, 63 * CHAR_W
    mov edx, [sePanelRow]
    imul edx, CHAR_H
    mov r8d, 60 * CHAR_W
    mov r9d, ebp
    mov r10d, COL_PURPHI
    cmp dword [seMode], 0
    je .dw_rb_fr
    mov r10d, COL_BEAT
.dw_rb_fr:
    call fill_rect

    mov ecx, 63 * CHAR_W + 3
    mov edx, [sePanelRow]
    imul edx, CHAR_H
    add edx, 3
    mov r8d, 60 * CHAR_W - 6
    lea r9d, [rbp - 6]
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, 63 * CHAR_W
    mov edx, [sePanelRow]
    imul edx, CHAR_H
    mov r8d, 60 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    cmp dword [seMode], 0
    je .dw_rb_hd
    mov r10d, COL_HDR_OFF
.dw_rb_hd:
    call fill_rect

    mov ecx, 64
    mov edx, [sePanelRow]
    lea r8, [seWSynHdr]
    mov r9d, 58
    mov r10d, COL_WHITE
    call draw_text

    ; Wellenform-Buttons & Pulsbreite (sePanelRow + 2)
    mov r15d, [sePanelRow]
    add r15d, 2

    mov ecx, 65
    mov edx, r15d
    lea r8, [seWSynWave]
    mov r9d, 6
    mov r10d, COL_TEAL
    call draw_text

    xor edi, edi
.dw_wb_loop:
    imul r14d, edi, 8
    add r14d, 72
    mov ecx, r14d
    imul ecx, CHAR_W
    mov edx, r15d
    imul edx, CHAR_H
    mov r8d, 7 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    cmp edi, [seWave]
    jne .dw_wb_bg
    mov r10d, COL_CURSOR
.dw_wb_bg:
    call fill_rect

    mov ecx, r14d
    mov edx, r15d
    lea r8, [seWSynWave + 7]
    imul eax, edi, 8
    add r8, rax
    mov r9d, 7
    mov r10d, COL_NOTE
    cmp edi, [seWave]
    jne .dw_wb_tx
    mov r10d, COL_BLACK
.dw_wb_tx:
    call draw_text

    inc edi
    cmp edi, 5
    jb .dw_wb_loop

    mov eax, [sePulse]
    lea r8, [seWSynWave + 51]
    call fmt_dec3
    mov ecx, 113
    mov edx, r15d
    lea r8, [seWSynWave + 48]
    mov r9d, 8
    mov r10d, COL_CYAN
    call draw_text

    ; --- 9 GETRENNTE SLIDER LINKS (SAMPLE-FX) & RECHTS (SYNTHESIZER) ---
    xor edi, edi
.dw_sl_loop:
    mov r14d, edi
    imul r14d, [seSlStep]
    add r14d, [sePanelRow]
    add r14d, 4                         ; Bildschirmzeile dieses Sliders

    ; === LINKS: SAMPLE-FX SLIDER i ===
    imul eax, edi, 12
    lea r8, [seSmpParamNames]
    add r8, rax
    mov ecx, 3
    mov edx, r14d
    mov r9d, 12
    mov r10d, COL_NOTE
    call draw_text

    mov ecx, 16 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 1
    mov r8d, 30 * CHAR_W
    mov r9d, CHAR_H - 2
    mov r10d, COL_BEAT
    call fill_rect

    lea rsi, [seSmpParams]
    mov r15d, [rsi + rdi*4]
    imul eax, r15d, 360
    xor edx, edx
    mov ecx, 100
    div ecx
    test eax, eax
    jz .dw_sl_lnb
    mov ecx, 16 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 1
    mov r8d, eax
    mov r9d, CHAR_H - 2
    mov r10d, COL_CYAN
    cmp edi, 4
    jb .dw_sl_lcd
    mov r10d, COL_TEAL
    cmp edi, 8
    jb .dw_sl_lcd
    mov r10d, COL_NOTE
.dw_sl_lcd:
    cmp dword [seDragMode], 4
    jne .dw_sl_ldr
    cmp edi, [seDragParam]
    jne .dw_sl_ldr
    mov r10d, COL_CURSOR
.dw_sl_ldr:
    call fill_rect
.dw_sl_lnb:

    mov ecx, 47 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 47
    mov edx, r14d
    lea r8, [txtMinus]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov eax, r15d
    lea r8, [lineBuf]
    call fmt_dec3
    mov ecx, 51
    mov edx, r14d
    lea r8, [lineBuf]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 55 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 55
    mov edx, r14d
    lea r8, [txtPlus]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    ; === RECHTS: SYNTHESIZER SLIDER i ===
    imul eax, edi, 12
    lea r8, [seSynParamNames]
    add r8, rax
    mov ecx, 65
    mov edx, r14d
    mov r9d, 12
    mov r10d, COL_NOTE
    call draw_text

    mov ecx, 78 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 1
    mov r8d, 30 * CHAR_W
    mov r9d, CHAR_H - 2
    mov r10d, COL_BEAT
    call fill_rect

    lea rsi, [seSynthParams]
    mov r15d, [rsi + rdi*4]
    imul eax, r15d, 360
    xor edx, edx
    mov ecx, 100
    div ecx
    test eax, eax
    jz .dw_sl_rnb
    mov ecx, 78 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    add edx, 1
    mov r8d, eax
    mov r9d, CHAR_H - 2
    mov r10d, COL_PURPHI
    cmp edi, 4
    jb .dw_sl_rcd
    mov r10d, COL_CYAN
    cmp edi, 8
    jb .dw_sl_rcd
    mov r10d, COL_NOTE
.dw_sl_rcd:
    cmp dword [seDragMode], 5
    jne .dw_sl_rdr
    cmp edi, [seDragParam]
    jne .dw_sl_rdr
    mov r10d, COL_CURSOR
.dw_sl_rdr:
    call fill_rect
.dw_sl_rnb:

    mov ecx, 109 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 109
    mov edx, r14d
    lea r8, [txtMinus]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov eax, r15d
    lea r8, [lineBuf]
    call fmt_dec3
    mov ecx, 113
    mov edx, r14d
    lea r8, [lineBuf]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 117 * CHAR_W
    mov edx, r14d
    imul edx, CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 117
    mov edx, r14d
    lea r8, [txtPlus]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    inc edi
    cmp edi, 9
    jb .dw_sl_loop

    ; --- Zeile seBtnRow: Links Sample-FX Buttons, Rechts 6 Synth-Presets ---
    mov r14d, [seBtnRow]
    mov r15d, r14d
    imul r15d, CHAR_H

    ; [Sample-FX anwenden] (Spalten 3..22)
    mov ecx, 3 * CHAR_W
    mov edx, r15d
    mov r8d, 20 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_CYAN
    call fill_rect
    mov ecx, 3
    mov edx, r14d
    lea r8, [txtWSmpApp]
    mov r9d, 20
    mov r10d, COL_BLACK
    call draw_text

    ; [Original wiederherstellen] (Spalten 25..51)
    mov ecx, 25 * CHAR_W
    mov edx, r15d
    mov r8d, 27 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    mov ebp, COL_MUTED
    lea rdx, [slotOrigLen]
    cmp dword [rdx + rbx*4], 0
    je .dw_or_bg
    mov r10d, COL_HDR_ON
    mov ebp, COL_WHITE
.dw_or_bg:
    call fill_rect
    mov ecx, 25
    mov edx, r14d
    lea r8, [txtWOrigRes]
    mov r9d, 27
    mov r10d, ebp
    call draw_text

    ; 6 Synth-Presets (Spalten 65..121)
    xor edi, edi
.dw_pre_loop:
    lea rsi, [seWPreBtnPos]
    movzx ecx, byte [rsi + rdi*2 + 0]
    movzx r8d, byte [rsi + rdi*2 + 1]
    push rcx
    push r8
    imul ecx, CHAR_W
    mov edx, r15d
    imul r8d, CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    pop r8
    pop rcx
    mov edx, r14d
    mov r9d, r8d
    lea r8, [seWSynPre]
    lea eax, [rcx - 65]
    add r8, rax
    mov r10d, COL_WHITE
    call draw_text
    inc edi
    cmp edi, 6
    jb .dw_pre_loop

    ; --- Zeile seBtnRow + 2: Links Tipp, Rechts [Synth erzeugen] [Auto-Loop] [Live-Edit] ---
    add r14d, 2
    mov r15d, r14d
    imul r15d, CHAR_H

    mov ecx, 3
    mov edx, r14d
    lea r8, [seWSmpTip]
    mov r9d, 57
    mov r10d, COL_TEAL
    call draw_text

    ; [Synth erzeugen] (Spalten 65..80)
    mov ecx, 65 * CHAR_W
    mov edx, r15d
    mov r8d, 16 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect
    mov ecx, 65
    mov edx, r14d
    lea r8, [txtWSynGen]
    mov r9d, 16
    mov r10d, COL_BLACK
    call draw_text

    ; [Auto-Loop: AN/AUS] (Spalten 82..97)
    mov ecx, 82 * CHAR_W
    mov edx, r15d
    mov r8d, 16 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    lea rbp, [txtWAlOff]
    cmp dword [seAutoLoop], 0
    je .dw_al_b
    mov r10d, COL_HDR_ON
    lea rbp, [txtWAlOn]
.dw_al_b:
    call fill_rect
    mov ecx, 82
    mov edx, r14d
    mov r8, rbp
    mov r9d, 16
    mov r10d, COL_WHITE
    call draw_text

    ; [Live-Edit: AN/AUS] (Spalten 99..114)
    mov ecx, 99 * CHAR_W
    mov edx, r15d
    mov r8d, 16 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    lea rbp, [txtWLiveOff]
    mov esi, COL_WHITE
    cmp dword [seLiveSynth], 0
    je .dw_lv_b
    mov r10d, COL_CURSOR
    lea rbp, [txtWLiveOn]
    mov esi, COL_BLACK
.dw_lv_b:
    call fill_rect
    mov ecx, 99
    mov edx, r14d
    mov r8, rbp
    mov r9d, 16
    mov r10d, esi
    call draw_text

    ; Aufnahme-Banner wenn aktiv (seBtnRow + 4)
    cmp dword [isRecording], 0
    je .dw_rec_bar
    add r14d, 2
    mov r15d, r14d
    imul r15d, CHAR_H
    mov ecx, 2 * CHAR_W
    mov edx, r15d
    mov r8d, 120 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect
    mov ecx, 2
    mov edx, r14d
    lea r8, [seWRecBan]
    mov r9d, 120
    mov r10d, COL_WHITE
    call draw_text

.dw_rec_bar:
    ; --- Zeile seRecRow: Mikrofon-Auswahl & grosser 3-Zonen-Live-VU ---
    mov r14d, [seRecRow]
    mov r15d, r14d
    imul r15d, CHAR_H

    mov ecx, 1
    mov edx, r14d
    lea r8, [txtMicLbl]
    mov r9d, 4
    mov r10d, COL_TEAL
    call draw_text

    mov ecx, 6 * CHAR_W
    mov edx, r15d
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 6
    mov edx, r14d
    lea r8, [txtDevPrev]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 10 * CHAR_W
    mov edx, r15d
    mov r8d, 36 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BLACK
    call fill_rect
    mov ecx, 10
    mov edx, r14d
    lea r8, [seRecDevName]
    mov r9d, 28
    mov r10d, COL_CYAN
    cmp dword [seRecDevCount], 0
    jne .dw_rd_c
    mov r10d, COL_PURPHI
.dw_rd_c:
    call draw_text

    mov ecx, 47 * CHAR_W
    mov edx, r15d
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 47
    mov edx, r14d
    lea r8, [txtDevNext]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 52
    mov edx, r14d
    lea r8, [txtVuLbl]
    mov r9d, 3
    mov r10d, COL_TEAL
    call draw_text

    ; VU-Rahmen (Spalten 56..111 = 56*12 = 672 px breit)
    mov ecx, 56 * CHAR_W
    lea edx, [r15 + 1]
    mov r8d, 56 * CHAR_W
    mov r9d, CHAR_H - 2
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, 56 * CHAR_W
    lea edx, [r15 + 2]
    mov r8d, 56 * CHAR_W
    mov r9d, CHAR_H - 4
    mov r10d, COL_BEAT
    call fill_rect

    cmp dword [isRecording], 0
    je .dw_vu_idle

    mov eax, [recVuLevel]
    imul eax, 672
    xor edx, edx
    mov ecx, 127
    div ecx
    cmp eax, 672
    jbe .dw_vu_w
    mov eax, 672
.dw_vu_w:
    mov ebp, eax
    test ebp, ebp
    jz .dw_vu_pk

    mov r8d, ebp
    cmp r8d, 450
    jbe .dw_vu_z1
    mov r8d, 450
.dw_vu_z1:
    mov ecx, 56 * CHAR_W
    lea edx, [r15 + 2]
    mov r9d, CHAR_H - 4
    mov r10d, COL_CYAN
    call fill_rect

    cmp ebp, 450
    jbe .dw_vu_pk
    mov r8d, ebp
    cmp r8d, 590
    jbe .dw_vu_z2
    mov r8d, 590
.dw_vu_z2:
    sub r8d, 450
    mov ecx, 56 * CHAR_W + 450
    lea edx, [r15 + 2]
    mov r9d, CHAR_H - 4
    mov r10d, COL_CURSOR
    call fill_rect

    cmp ebp, 590
    jbe .dw_vu_pk
    mov r8d, ebp
    sub r8d, 590
    mov ecx, 56 * CHAR_W + 590
    lea edx, [r15 + 2]
    mov r9d, CHAR_H - 4
    mov r10d, COL_PURPHI
    call fill_rect

.dw_vu_pk:
    mov eax, [recVuPeakHold]
    cmp eax, 3
    jb .dw_vu_tm
    imul eax, 670
    xor edx, edx
    mov ecx, 127
    div ecx
    cmp eax, 670
    jbe .dw_vu_po
    mov eax, 670
.dw_vu_po:
    lea ecx, [rax + 56 * CHAR_W]
    lea edx, [r15 + 1]
    mov r8d, 2
    mov r9d, CHAR_H - 2
    mov r10d, COL_WHITE
    cmp dword [recVuPeakHold], 122
    jb .dw_vu_pd
    mov r10d, COL_PURPHI
.dw_vu_pd:
    call fill_rect

.dw_vu_tm:
    mov eax, [recWritePos]
    imul rax, 10
    xor edx, edx
    mov ecx, 22050
    div rcx
    cmp eax, 59
    jbe .dw_vt_ok
    mov eax, 59
.dw_vt_ok:
    xor edx, edx
    mov ecx, 10
    div ecx
    add al, '0'
    mov [recTimeTxt + 0], al
    mov byte [recTimeTxt + 1], '.'
    add dl, '0'
    mov [recTimeTxt + 2], dl
    mov dword [recTimeTxt + 3], '9.5/'

    mov ecx, 114
    mov edx, r14d
    lea r8, [recTimeTxt]
    mov r9d, 7
    mov r10d, COL_CYAN
    cmp dword [recVuPeakHold], 122
    jb .dw_vt_dr
    mov r10d, COL_PURPHI
.dw_vt_dr:
    call draw_text
    jmp .dw_bot

.dw_vu_idle:
    lea r8, [txtRecReady]
    mov r10d, COL_TEAL
    cmp dword [seRecDevCount], 0
    jne .dw_vi_dr
    lea r8, [txtRecNoDev]
    mov r10d, COL_PURPHI
.dw_vi_dr:
    mov ecx, 114
    mov edx, r14d
    mov r9d, 7
    call draw_text

.dw_bot:
    ; --- Zeile botBarRow: Widescreen-Fusszeile ---
    mov r14d, [botBarRow]
    mov r15d, r14d
    imul r15d, CHAR_H

    mov al, bl
    lea r8, [seWBotBar + 6]
    call fmt_hex2
    mov eax, [curOct]
    add al, '2'
    mov [seWBotBar + 17], al

    xor ecx, ecx
    mov edx, r14d
    lea r8, [seWBotBar]
    mov r9d, 123
    mov r10d, COL_CYAN
    call draw_text

    mov ecx, 119 * CHAR_W
    mov edx, r15d
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    cmp dword [showHelp], 0
    je .dw_qbg
    mov r10d, COL_PURPHI
.dw_qbg:
    call fill_rect

    mov ecx, 119
    mov edx, r14d
    lea r8, [seWBotBar + 119]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

.ds_epilog:
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