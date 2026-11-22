; Echtzeit-8-Kanal-Stereo-Mixer (Synth + 64-Slot-PCM-Resampling) und WinMM-Streaming
; Vollstaendige Composer-669-Effekte: a (Up), b (Down), c (Portamento), d (Adjust), e (Vibrato), f (Tempo)

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

; Lineare Note (AL = 0..59) und Slot (ECX = 0..63) in 16.16 PCM-Schrittweite (EAX) wandeln
pcm_note_to_step:
    movzx r8d, cl
    and r8d, 63
    lea r9, [sampC2Step]
    mov r8d, [r9 + r8*4]
    movzx eax, al
    xor edx, edx
    mov ecx, 12
    div ecx
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

; Frequenz-Adjust (Effekt dX) einmalig relativ zu baseInc anwenden
; Eingabe: RDI = VoicePtr, ECX = fxVal (0..15)
apply_freq_adjust:
    mov eax, [rdi + 16]
    test ecx, ecx
    jz .afa_st
    mov edx, eax
    imul rax, rcx
    add rax, 128
    shr rax, 8
    add eax, edx
.afa_st:
    mov [rdi + 12], eax
    cmp dword [rdi + 52], 0
    jne .afa_ret
    mov edx, eax
    shr edx, 1
    add edx, eax
    mov [rdi + 36], edx
.afa_ret:
    ret

; Alle Stimmen, Effekte und VU-Pegel sofort stumm schalten
silence_audio:
    mov dword [isPlaying], 0
    lea rax, [voices]
    lea rdx, [voiceTargetInc]
    xor ecx, ecx
.sa_loop:
    mov dword [rax + 0], 0
    mov dword [rax + 20], 0
    mov dword [rax + 40], -1
    mov dword [rax + 44], 0
    mov dword [rax + 48], 0
    mov dword [rdx + rcx*4], 0
    add rax, VOICE_SIZE
    inc ecx
    cmp ecx, NUM_CH
    jb .sa_loop
    ret

; ============================================================================
; Echtzeit-Kanal-Trigger: ECX=Kanal (0..7), RDX=CellPtr, R8D=forcePreview
; ============================================================================
trigger_channel:
    push rbx
    push rsi
    push rdi
    push r12
    push r13
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

    ; Zellen-Effekt dekodieren: R12D = cmd (-1 oder 0..4), R13D = val (0..15)
    mov r12d, -1
    xor r13d, r13d
    movzx eax, byte [rsi + 3]
    cmp al, 0xFF
    je .fx_decoded
    mov ecx, eax
    shr ecx, 4
    and eax, 0x0F
    cmp ecx, 5                          ; Effekt f: Tempo sofort setzen
    jne .fx_is_pitch
    test eax, eax
    jz .fx_decoded
    mov [curTempo], eax
    jmp .fx_decoded
.fx_is_pitch:
    cmp ecx, 4
    ja .fx_decoded
    mov r12d, ecx
    mov r13d, eax
.fx_decoded:

    ; Lautstaerke uebernehmen falls gesetzt
    movzx eax, byte [rsi + 2]
    cmp al, 0xFF
    je .vol_done
    and eax, 15
    mov [rdi + 24], eax
    cmp byte [rsi + 0], 0xFF
    jne .vol_done
    add eax, eax
    cmp eax, [rdi + 48]
    jle .vol_done
    mov [rdi + 48], eax
.vol_done:

    ; Effekt c (Portamento to Note) auf laufender Stimme: kein Retrigger
    cmp r12d, 2
    jne .chk_note
    cmp dword [rdi + 0], 0
    je .chk_note

    ; Falls vorher Vibrato lief, zuerst Basis-Tonhoehe wiederherstellen
    cmp dword [rdi + 40], 4
    jne .porta_set_fx
    mov eax, [rdi + 16]
    mov [rdi + 12], eax
.porta_set_fx:
    mov dword [rdi + 40], 2
    mov [rdi + 44], r13d

    ; Zieltonhoehe berechnen falls Note angegeben ist
    movzx eax, byte [rsi + 0]
    cmp al, 0xFF
    je .done
    cmp dword [rdi + 52], 0
    jne .porta_pcm_tgt
    call note_to_inc
    jmp .porta_store_tgt
.porta_pcm_tgt:
    mov ecx, [rdi + 4]
    call pcm_note_to_step
.porta_store_tgt:
    lea rdx, [voiceTargetInc]
    mov [rdx + rbx*4], eax
    jmp .done

.chk_note:
    movzx eax, byte [rsi + 0]
    cmp al, 0xFF
    je .no_new_note

    ; Neue Note mit Instrument starten
    movzx ecx, byte [rsi + 1]
    cmp cl, 0xFF
    je .keep_inst
    and ecx, 63
    mov [rdi + 4], ecx
.keep_inst:
    mov ecx, [rdi + 4]

    lea rdx, [sampHasPcm]
    cmp byte [rdx + rcx], 0
    je .trig_synth
    lea rdx, [sampLenTbl]
    cmp dword [rdx + rcx*4], 0
    je .trig_synth

    ; PCM-Stimme starten
    call pcm_note_to_step
    mov [rdi + 12], eax
    mov [rdi + 16], eax
    lea rdx, [voiceTargetInc]
    mov [rdx + rbx*4], eax
    mov dword [rdi + 52], 1
    mov qword [rdi + 56], 0
    mov dword [rdi + 0], 1
    mov dword [rdi + 20], 30000
    jmp .set_vu

.trig_synth:
    mov dword [rdi + 52], 0
    push rax
    call note_to_inc
    mov [rdi + 12], eax
    mov [rdi + 16], eax
    lea rdx, [voiceTargetInc]
    mov [rdx + rbx*4], eax
    mov edx, eax
    shr edx, 1
    add eax, edx
    mov [rdi + 36], eax
    pop rax

    mov dword [rdi + 0], 1
    mov dword [rdi + 8], 0
    mov dword [rdi + 32], 0
    mov dword [rdi + 20], 30000

    mov eax, [rdi + 4]
    and eax, 7
    test eax, eax
    jnz .set_vu
    mov dword [rdi + 12], 16800000
    mov dword [rdi + 16], 16800000
    lea rdx, [voiceTargetInc]
    mov dword [rdx + rbx*4], 16800000
    mov dword [rdi + 20], 32000

.set_vu:
    mov eax, [rdi + 24]
    shl eax, 1
    cmp eax, 8
    jae .m_ok
    mov eax, 8
.m_ok:
    mov [rdi + 48], eax

    ; Effekt fuer die neu getriggerte Note setzen
    cmp r12d, -1
    jne .new_note_fx
    mov dword [rdi + 40], -1
    mov dword [rdi + 44], 0
    jmp .done

.new_note_fx:
    cmp r12d, 3                         ; Effekt d: Frequenz-Adjust sofort anwenden
    jne .new_note_store_fx
    mov ecx, r13d
    call apply_freq_adjust
    mov dword [rdi + 40], -1
    mov dword [rdi + 44], 0
    jmp .done

.new_note_store_fx:
    cmp r12d, 4
    jne .nn_save
    test r13d, r13d
    jnz .nn_save
    mov dword [rdi + 40], -1
    mov dword [rdi + 44], 0
    jmp .done
.nn_save:
    mov [rdi + 40], r12d
    mov [rdi + 44], r13d
    jmp .done

.no_new_note:
    ; Keine neue Note: laufenden Effekt nur bei neuem Befehl ueberschreiben
    cmp r12d, -1
    je .done

    ; Bei Wechsel weg von Vibrato zuerst Basis-Tonhoehe wiederherstellen
    cmp dword [rdi + 40], 4
    jne .nn_not_prev_vib
    cmp r12d, 4
    je .nn_not_prev_vib
    mov eax, [rdi + 16]
    mov [rdi + 12], eax
.nn_not_prev_vib:

    cmp r12d, 3                         ; Effekt d auf laufender Stimme
    jne .nn_chk_vib
    mov ecx, r13d
    call apply_freq_adjust
    mov dword [rdi + 40], -1
    mov dword [rdi + 44], 0
    jmp .done

.nn_chk_vib:
    cmp r12d, 4
    jne .nn_set_cont
    cmp dword [rdi + 40], 4
    je .nn_vib_val
    mov eax, [rdi + 12]
    mov [rdi + 16], eax
.nn_vib_val:
    test r13d, r13d
    jnz .nn_set_cont
    mov eax, [rdi + 16]
    mov [rdi + 12], eax
    mov dword [rdi + 40], -1
    mov dword [rdi + 44], 0
    jmp .done

.nn_set_cont:
    mov [rdi + 40], r12d
    mov [rdi + 44], r13d

.done:
    add rsp, 0x20
    pop r13
    pop r12
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

    ; Bei Pattern-Wechsel im Song-Modus Standard-Tempo des Patterns uebernehmen
    cmp dword [needPatTempo], 0
    je .no_ptmp
    mov dword [needPatTempo], 0
    mov eax, [playPat]
    and eax, 127
    lea rdx, [patTempos]
    movzx eax, byte [rdx + rax]
    cmp eax, 1
    jb .no_ptmp
    cmp eax, 15
    ja .no_ptmp
    mov [curTempo], eax
.no_ptmp:

    ; Aktuell erklingende Position fuer die Anzeige sichern
    mov eax, [playPat]
    mov [dispPlayPat], eax
    mov ecx, [playRow]
    mov [dispPlayRow], ecx
    mov edx, [playOrder]
    mov [dispPlayOrder], edx
    cmp dword [songMode], 0
    je .no_sync_pat
    mov [curPat], eax
.no_sync_pat:

    ; Alle 8 Kanaele der aktuellen Zeile ausloesen
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

    ; Dauer der aktuellen Zeile berechnen
    mov eax, [curTempo]
    imul eax, TICKS_PER_TEMPO
    mov [tickSamples], eax

    ; Naechste Zeile anhand der Break-Tabelle bestimmen
    mov eax, [playPat]
    and eax, 127
    lea rdx, [patBreaks]
    movzx ecx, byte [rdx + rax]
    cmp ecx, PAT_ROWS - 1
    jbe .brk_ok
    mov ecx, PAT_ROWS - 1
.brk_ok:
    mov eax, [playRow]
    cmp eax, ecx
    jae .pat_end
    inc eax
    mov [playRow], eax
    jmp .seq_mark_dirty

.pat_end:
    mov dword [playRow], 0
    cmp dword [songMode], 0
    je .seq_mark_dirty

    mov ecx, [playOrder]
    inc ecx
    cmp ecx, [orderLen]
    jb .ord_ok
    mov ecx, [loopOrder]
    cmp ecx, [orderLen]
    jb .ord_ok
    xor ecx, ecx
.ord_ok:
    mov [playOrder], ecx
    lea rdx, [orderList]
    movzx edx, byte [rdx + rcx]
    and edx, 127
    mov [playPat], edx
    mov dword [needPatTempo], 1

.seq_mark_dirty:
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

    ; Effekt-Tick alle 256 Samples (~172 Hz fuer glatte Slides ohne Stufen)
    inc dword [fxDivCnt]
    mov ebp, [fxDivCnt]
    and ebp, 255

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

    ; Proportionale Pitch-Effekte (einheitlich fuer PCM und Synth)
    test ebp, ebp
    jnz .fx_done

    mov eax, [rdi + 40]
    cmp eax, 0
    je .fx_up
    cmp eax, 1
    je .fx_down
    cmp eax, 2
    je .fx_porta
    cmp eax, 4
    je .fx_vib
    jmp .fx_done

.fx_up:
    mov edx, [rdi + 44]
    test edx, edx
    jz .fx_done
    mov eax, [rdi + 12]
    imul rax, rdx
    add rax, 1024
    shr rax, 11
    test eax, eax
    jnz .fxu_d
    mov eax, 1
.fxu_d:
    add eax, [rdi + 12]
    mov edx, 0x30000000
    cmp dword [rdi + 52], 0
    je .fxu_cap
    mov edx, 0x00400000
.fxu_cap:
    cmp eax, edx
    jbe .fxu_st
    mov eax, edx
.fxu_st:
    mov [rdi + 12], eax
    mov [rdi + 16], eax
    jmp .fx_upd_pad

.fx_down:
    mov edx, [rdi + 44]
    test edx, edx
    jz .fx_done
    mov eax, [rdi + 12]
    imul rax, rdx
    add rax, 1024
    shr rax, 11
    test eax, eax
    jnz .fxd_d
    mov eax, 1
.fxd_d:
    mov edx, [rdi + 12]
    sub edx, eax
    mov eax, 500000
    cmp dword [rdi + 52], 0
    je .fxd_flr
    mov eax, 256
.fxd_flr:
    cmp edx, eax
    jge .fxd_st
    mov edx, eax
.fxd_st:
    mov [rdi + 12], edx
    mov [rdi + 16], edx
    mov eax, edx
    jmp .fx_upd_pad

.fx_porta:
    mov edx, [rdi + 44]
    test edx, edx
    jz .fx_done
    lea rsi, [voiceTargetInc]
    mov r10d, [rsi + rbx*4]
    test r10d, r10d
    jz .fx_done
    mov r11d, [rdi + 12]
    cmp r11d, r10d
    je .fx_done

    mov eax, r11d
    imul rax, rdx
    add rax, 1024
    shr rax, 11
    test eax, eax
    jnz .fxp_d
    mov eax, 1
.fxp_d:
    cmp r11d, r10d
    ja .fxp_down
    add r11d, eax
    cmp r11d, r10d
    jbe .fxp_st
    mov r11d, r10d
    jmp .fxp_st
.fxp_down:
    sub r11d, eax
    cmp r11d, r10d
    jae .fxp_st
    mov r11d, r10d
.fxp_st:
    mov [rdi + 12], r11d
    mov [rdi + 16], r11d
    mov eax, r11d
    jmp .fx_upd_pad

.fx_vib:
    mov edx, [rdi + 44]
    test edx, edx
    jz .fx_done
    mov eax, [vibPhase]
    shr eax, 24
    movsxd r10, dword [r15 + rax*4]
    mov eax, [rdi + 16]
    imul rax, rdx
    imul rax, r10
    sar rax, 14
    add eax, [rdi + 16]
    mov edx, 500000
    cmp dword [rdi + 52], 0
    je .fxv_flr
    mov edx, 256
.fxv_flr:
    cmp eax, edx
    jge .fxv_st
    mov eax, edx
.fxv_st:
    mov [rdi + 12], eax

.fx_upd_pad:
    cmp dword [rdi + 52], 0
    jne .fx_done
    mov edx, eax
    shr edx, 1
    add edx, eax
    mov [rdi + 36], edx

.fx_done:
    cmp dword [rdi + 52], 0
    jne .play_pcm

    mov edx, [rdi + 4]
    and edx, 7
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
    and edx, 63
    lea rsi, [sampLenTbl]
    mov r10d, [rsi + rdx*4]
    test r10d, r10d
    jz .ch_off

    mov r11, [rdi + 56]
    mov rax, r11
    shr rax, 16
    cmp eax, r10d
    jae .ch_off

    mov ecx, edx
    shl ecx, 17
    lea rsi, [pcmSlots]
    add rsi, rcx
    movzx eax, byte [rsi + rax]
    sub eax, 128
    shl eax, 6

    mov ecx, [rdi + 12]
    add r11, rcx

    push rax
    mov rax, r11
    shr rax, 16

    lea rsi, [sampLoopEndTbl]
    mov ecx, [rsi + rdx*4]
    cmp ecx, r10d
    ja .pcm_noloop
    lea rsi, [sampLoopStartTbl]
    mov esi, [rsi + rdx*4]
    cmp ecx, esi
    jbe .pcm_noloop

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

    cmp dword [rdi + 52], 0
    jne .pan_lr
    mov edx, [rdi + 4]
    and edx, 7
    test edx, edx
    jz .pan_center
.pan_lr:
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