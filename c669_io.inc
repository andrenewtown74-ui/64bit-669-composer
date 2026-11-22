; Datei-I/O: 64-Sample / 128-Pattern / 64-Zeilen .669-I/O, WAV-Importer und Slot-Verwaltung

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

    mov dword [ofn + 0x00], 152
    mov rax, [hWnd]
    mov [ofn + 0x08], rax
    mov [ofn + 0x18], rdx
    mov dword [ofn + 0x2C], 1
    mov [ofn + 0x30], rcx
    mov dword [ofn + 0x38], 260
    mov [ofn + 0x68], r8
    pop rdi
    ret

; Basis-Dateinamen aus Pfad (RCX) in Zielpuffer (RDX) mit Breite (R8D) kopieren
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

; Alle 64 Sample-Slots beim Start auf Standardwerte initialisieren
init_samp_meta:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    xor ebx, ebx
.ism_loop:
    lea rdx, [sampHasPcm]
    mov byte [rdx + rbx], 0
    lea rdx, [sampC2Step]
    mov dword [rdx + rbx*4], 3107       ; (8363 << 16) / (44100 * 4) fuer 669 Note 0

    mov eax, ebx
    imul eax, 12
    lea rdi, [sampNames]
    add rdi, rax

    cmp ebx, 8
    jae .ism_empty

    lea rsi, [defSampNames]
    add rsi, rax
    mov ecx, 12
    rep movsb

    lea rdx, [saveSampLen]
    mov eax, [rdx + rbx*4]
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], eax

    lea rdx, [saveSampStart]
    mov eax, [rdx + rbx*4]
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rbx*4], eax

    lea rdx, [saveSampEnd]
    mov eax, [rdx + rbx*4]
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rbx*4], eax
    jmp .ism_next

.ism_empty:
    mov al, bl
    mov r8, rdi
    call fmt_hex2
    lea rsi, [txtEmptySlot]
    lea rdi, [rdi + 2]
    mov ecx, 10
    rep movsb

    lea rdx, [sampLenTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopEndTbl]
    mov dword [rdx + rbx*4], 0xFFFFF

.ism_next:
    inc ebx
    cmp ebx, MAX_SAMPS
    jb .ism_loop

    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; WAV-Datei aus fileBuf parsen und auf aktiven curInst-Slot (0..63) laden
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

    cmp dword [rbp + 0], 0x46464952
    jne .w_fail
    cmp dword [rbp + 8], 0x45564157
    jne .w_fail

    mov dword [wavFmtTag], 0
    mov dword [wavChannels], 0
    mov dword [wavRate], 0
    mov dword [wavBits], 0
    mov dword [wavDataOff], 0
    mov dword [wavDataSize], 0

    mov r13d, 12
.w_chunk_loop:
    lea eax, [r13 + 8]
    cmp eax, r12d
    ja .w_chunks_done

    mov eax, [rbp + r13 + 0]
    mov ecx, [rbp + r13 + 4]
    add r13d, 8

    mov edx, r12d
    sub edx, r13d
    cmp ecx, edx
    jbe .w_cs_ok
    mov ecx, edx
.w_cs_ok:

    cmp eax, 0x20746D66
    jne .w_chk_data
    cmp ecx, 16
    jb .w_next_chunk

    movzx edx, word [rbp + r13 + 0]
    cmp edx, 0xFFFE
    jne .w_tag_ok
    cmp ecx, 40
    jb .w_tag_ok
    movzx edx, word [rbp + r13 + 24]
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
    cmp eax, 0x61746164
    jne .w_next_chunk
    test ecx, ecx
    jz .w_next_chunk
    mov [wavDataOff], r13d
    mov [wavDataSize], ecx

.w_next_chunk:
    mov edx, ecx
    add edx, 1
    and edx, 0xFFFFFFFE
    add r13d, edx
    cmp r13d, r12d
    jb .w_chunk_loop

.w_chunks_done:
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
    mov ebx, edx
    shr ebx, 3
    imul ebx, [wavChannels]
    test ebx, ebx
    jz .w_fail

    mov eax, [wavDataSize]
    xor edx, edx
    div ebx
    test eax, eax
    jz .w_fail
    cmp eax, MAX_SAMP_BYTES
    jbe .w_cnt_ok
    mov eax, MAX_SAMP_BYTES
.w_cnt_ok:
    mov r14d, eax
    mov r15d, eax

    mov eax, [wavDataOff]
    lea rsi, [rbp + rax]

    mov eax, [curInst]
    and eax, 63
    shl eax, 17
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

    mov ebx, [curInst]
    and ebx, 63
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

; Windows-Dialog: WAV-Sample auf curInst (0..63) laden (Strg+W / Einfg)
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

; Loop fuer aktiven Slot (0..63) umschalten (Strg+L)
toggle_wav_loop:
    sub rsp, 0x28
    mov eax, [curInst]
    and eax, 63
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

; Aktiven Slot (0..63) auf Synth bzw. leer zuruecksetzen (Strg+U)
unload_wav_slot:
    push rbx
    push rsi
    push rdi
    sub rsp, 0x20

    mov ebx, [curInst]
    and ebx, 63
    lea rdx, [sampHasPcm]
    mov byte [rdx + rbx], 0

    mov eax, ebx
    imul eax, 12
    lea rdi, [sampNames]
    add rdi, rax

    cmp ebx, 8
    jae .uw_empty

    lea rdx, [saveSampLen]
    mov ecx, [rdx + rbx*4]
    lea rdx, [sampLenTbl]
    mov [rdx + rbx*4], ecx

    lea rdx, [saveSampStart]
    mov ecx, [rdx + rbx*4]
    lea rdx, [sampLoopStartTbl]
    mov [rdx + rbx*4], ecx

    lea rdx, [saveSampEnd]
    mov ecx, [rdx + rbx*4]
    lea rdx, [sampLoopEndTbl]
    mov [rdx + rbx*4], ecx

    lea rsi, [defSampNames]
    add rsi, rax
    mov ecx, 12
    rep movsb
    jmp .uw_st

.uw_empty:
    lea rdx, [sampLenTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopStartTbl]
    mov dword [rdx + rbx*4], 0
    lea rdx, [sampLoopEndTbl]
    mov dword [rdx + rbx*4], 0xFFFFF

    mov al, bl
    mov r8, rdi
    call fmt_hex2
    lea rsi, [txtEmptySlot]
    lea rdi, [rdi + 2]
    mov ecx, 10
    rep movsb

.uw_st:
    lea rcx, [stSynth]
    call set_status

    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; Prozedurales 8-Bit-Export-Sample (ECX = Slot 0..7) an [RDI] anhaengen
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
; .669-Binaerdatei (bis 64 Samples, 128 Patterns x 64 Zeilen, Breaks & Tempos)
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

    ; Hoechsten belegten Sample-Slot bestimmen (mindestens 8, maximal 64)
    mov ecx, MAX_SAMPS - 1
    lea rdx, [sampHasPcm]
.find_nos:
    cmp ecx, 7
    jbe .nos_min
    cmp byte [rdx + rcx], 0
    jne .nos_found
    dec ecx
    jmp .find_nos
.nos_min:
    mov ecx, 7
.nos_found:
    inc ecx
    mov [saveNos], ecx
    mov [r12 + 110], cl                 ; NOS

    ; Hoechstes genutztes Pattern ermitteln (curPat, orderList, belegte Zellen)
    mov r13d, [curPat]
    xor ecx, ecx
    lea rdx, [orderList]
.fnop_ord:
    cmp ecx, [orderLen]
    jae .fnop_scan
    movzx eax, byte [rdx + rcx]
    and eax, 127
    cmp eax, r13d
    jbe .fnop_onext
    mov r13d, eax
.fnop_onext:
    inc ecx
    jmp .fnop_ord

.fnop_scan:
    mov ebx, MAX_PATS - 1
.fnop_pat:
    cmp ebx, r13d
    jbe .fnop_done
    mov eax, ebx
    shl eax, 11
    lea rsi, [patterns]
    add rsi, rax
    mov ecx, PAT_BYTES / 4
.fnop_dw:
    cmp dword [rsi], 0xFFFFFFFF
    jne .fnop_hit
    add rsi, 4
    dec ecx
    jnz .fnop_dw
    dec ebx
    jmp .fnop_pat
.fnop_hit:
    mov r13d, ebx
.fnop_done:
    inc r13d
    cmp r13d, MAX_PATS
    jbe .fnop_ok
    mov r13d, MAX_PATS
.fnop_ok:
    mov [saveNop], r13d
    mov [r12 + 111], r13b               ; NOP

    mov al, [loopOrder]
    mov [r12 + 112], al                 ; Loop-Order

    ; Order-Liste (128 Byte, ungenutzte Eintraege = 0xFF)
    lea rdi, [r12 + 113]
    mov al, 0xFF
    mov ecx, MAX_ORDERS
    rep stosb

    lea rsi, [orderList]
    lea rdi, [r12 + 113]
    mov ecx, [orderLen]
    cmp ecx, MAX_ORDERS
    jbe .ol_ok
    mov ecx, MAX_ORDERS
.ol_ok:
    rep movsb

    ; Pattern-Tempos (128 Byte) und Pattern-Breaks (128 Byte) uebertragen
    lea rsi, [patTempos]
    lea rdi, [r12 + 241]
    mov ecx, MAX_PATS
    rep movsb

    lea rsi, [patBreaks]
    lea rdi, [r12 + 369]
    mov ecx, MAX_PATS
    rep movsb

    ; Sample-Header (saveNos * 25 Byte) ab Offset 497
    lea rdi, [r12 + 497]
    xor ebx, ebx
.sh_loop:
    cmp ebx, [saveNos]
    jae .sh_done

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
    jne .sh_pcm
    cmp ebx, 8
    jb .sh_def
    mov dword [rdi + 0], 0
    mov dword [rdi + 4], 0
    mov dword [rdi + 8], 0xFFFFF
    jmp .sh_next
.sh_pcm:
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
    jmp .sh_loop
.sh_done:

    ; Alle verwendeten Patterns (je 64 Zeilen * 8 Kanaele * 3 Byte = 1536 Byte) schreiben
    lea rsi, [patterns]
    xor ebx, ebx
.sp_pat:
    cmp ebx, [saveNop]
    jae .sp_done

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

    inc ebx
    jmp .sp_pat
.sp_done:

    ; Sample-Daten (0..saveNos-1) anhaengen
    xor ebx, ebx
.sm_out_loop:
    cmp ebx, [saveNos]
    jae .sm_out_done

    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbx], 0
    jne .sm_copy_pcm
    cmp ebx, 8
    jae .sm_out_next
    mov ecx, ebx
    call gen_synth_export_sample
    jmp .sm_out_next

.sm_copy_pcm:
    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rbx*4]
    mov eax, ebx
    shl eax, 17
    lea rsi, [pcmSlots]
    add rsi, rax
    rep movsb

.sm_out_next:
    inc ebx
    jmp .sm_out_loop
.sm_out_done:

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
; .669-Binaerdatei mit bis zu 128 Patterns x 64 Zeilen, Orders, Breaks & 64 Samples parsen
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

    movzx r13d, byte [rbp + 110]        ; NOS (0..64)
    mov [origNos], r13d
    movzx r14d, byte [rbp + 111]        ; NOP (1..128)
    test r14d, r14d
    jz .fail
    mov [origNop], r14d

    ; Vollstaendige Order-Liste (bis zu 128 Eintraege) einlesen
    lea rdi, [orderList]
    xor eax, eax
    mov ecx, MAX_ORDERS
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
    cmp ebx, MAX_ORDERS
    jb .ol_rd
.ol_end:
    test ebx, ebx
    jnz .ol_has
    mov ebx, 1
.ol_has:
    mov [orderLen], ebx

    ; Loop-Order aus Header-Byte 112 uebernehmen und pruefen
    movzx eax, byte [rbp + 112]
    cmp eax, [orderLen]
    jb .lo_ok
    xor eax, eax
.lo_ok:
    mov [loopOrder], eax

    ; Pattern-Tempos (Offset 241) und Pattern-Breaks (Offset 369) fuer alle 128 Patterns laden
    xor ebx, ebx
.tb_rd:
    movzx eax, byte [rbp + 241 + rbx]
    cmp eax, 1
    jb .tb_tdef
    cmp eax, 15
    jbe .tb_tok
.tb_tdef:
    mov eax, 5
.tb_tok:
    lea rdx, [patTempos]
    mov [rdx + rbx], al

    movzx eax, byte [rbp + 369 + rbx]
    cmp eax, PAT_ROWS - 1
    jbe .tb_bok
    mov eax, PAT_ROWS - 1
.tb_bok:
    lea rdx, [patBreaks]
    mov [rdx + rbx], al

    inc ebx
    cmp ebx, MAX_PATS
    jb .tb_rd

    movzx eax, byte [orderList + 0]
    lea rdx, [patTempos]
    movzx eax, byte [rdx + rax]
    mov [curTempo], eax

    ; Alle 64 Sample-Metadaten leeren
    lea rdi, [sampHasPcm]
    xor eax, eax
    mov ecx, MAX_SAMPS
    rep stosb

    lea rdi, [sampNames]
    mov al, ' '
    mov ecx, MAX_SAMPS * 12
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
    cmp ebx, MAX_SAMPS
    jb .sm_clr

    ; Bis zu 64 Sample-Header einlesen
    mov r15d, 497
    xor ebx, ebx
.sh_rd:
    cmp ebx, [origNos]
    jae .sh_done
    lea eax, [r15 + 25]
    cmp eax, r12d
    ja .fail

    cmp ebx, MAX_SAMPS
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

    ; Alle 128 Patterns (je 64 Zeilen) leeren und bis zu 128 Patterns vollstaendig dekodieren
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
    shl eax, 11                         ; pat * 2048
    lea rdi, [patterns]
    add rdi, rax

    mov ecx, PAT_ROWS * NUM_CH          ; Alle 64 Zeilen * 8 Kanaele = 512 Zellen (1536 Byte)
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
    and r9d, 63
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

    inc ebx
    jmp .pat_rd
.pat_done:

    mov eax, [origNop]
    cmp eax, MAX_PATS
    jbe .pcm_rd_start
    sub eax, MAX_PATS
    imul eax, 1536
    add r15d, eax

.pcm_rd_start:
    ; Alle vorhandenen 8-Bit-PCM-Samples (bis zu 64 Slots) laden
    xor ebx, ebx
.pcm_rd_loop:
    cmp ebx, [origNos]
    jae .pcm_rd_done
    cmp ebx, MAX_SAMPS
    jae .pcm_rd_done
    cmp r15d, r12d
    jae .pcm_rd_done

    lea rdx, [sampLenTbl]
    mov ecx, [rdx + rbx*4]
    test ecx, ecx
    jz .pcm_rd_next

    mov eax, r12d
    sub eax, r15d
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
    mov dword [rax + rbx*4], 3107       ; 8363 Hz auf Oktave 2 (Note 24)

    lea rax, [sampLoopEndTbl]
    mov r8d, [rax + rbx*4]
    cmp r8d, 0xFFFFF
    jae .pcm_rd_adv
    cmp r8d, edx
    jbe .pcm_rd_adv
    mov [rax + rbx*4], edx

.pcm_rd_adv:
    add r15d, ecx
.pcm_rd_next:
    inc ebx
    jmp .pcm_rd_loop
.pcm_rd_done:

    movzx eax, byte [orderList + 0]
    mov [curPat], eax
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov dword [playOrder], 0
    mov dword [dispPlayOrder], 0
    mov dword [editRow], 0
    mov dword [playRow], 0
    mov dword [dispPlayRow], 0
    mov dword [patViewTop], 0
    mov dword [ordViewTop], 0
    mov dword [curCh], 0
    mov dword [curSubCol], 0
    mov dword [curInst], 0
    mov dword [sampViewTop], 0
    mov dword [needPatTempo], 0

    ; Bei Songs mit mehreren Orders direkt in den SONG-ORD-Modus wechseln
    mov dword [songMode], 0
    cmp dword [orderLen], 1
    jbe .sm_done
    mov dword [songMode], 1
.sm_done:
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

; Windows-Dialog: .669 speichern (Strg+S / F12)
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

; Windows-Dialog: .669 laden (Strg+O / F11)
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