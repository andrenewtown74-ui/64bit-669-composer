; GDI-Darstellung, scrollende 64-Zeilen/128-Order/64-Sample-Ansicht, Tastatur-, Maus-, Copy/Paste-, Loesch-, Hilfe- und Demo-UI

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

; Zeiger auf Zelle (ECX=pat 0..127, EDX=row 0..63, R8D=ch 0..7) in RAX berechnen
get_cell_ptr:
    mov eax, ecx
    shl eax, 11
    mov r9d, edx
    shl r9d, 5
    add eax, r9d
    lea eax, [rax + r8*4]
    lea r9, [patterns]
    add rax, r9
    ret

; 7-Zeichen-Statusmeldung (RCX = Ptr) in topBar + 72 setzen
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

; Sample-Scroll-Fenster (12 Zeilen) auf curInst (0..63) ausrichten
sync_samp_scroll:
    mov eax, [curInst]
    mov ecx, [sampViewTop]
    cmp eax, ecx
    jae .chk_bot
    mov [sampViewTop], eax
    ret
.chk_bot:
    add ecx, 11
    cmp eax, ecx
    jbe .ok
    sub eax, 11
    mov [sampViewTop], eax
.ok:
    ret

; Pattern-Scroll-Fenster (32 Zeilen) auf editRow (0..63) ausrichten
sync_pat_scroll:
    mov eax, [editRow]
    mov ecx, [patViewTop]
    cmp eax, ecx
    jae .sps_bot
    mov [patViewTop], eax
    ret
.sps_bot:
    add ecx, VIEW_ROWS - 1
    cmp eax, ecx
    jbe .sps_ok
    sub eax, VIEW_ROWS - 1
    mov [patViewTop], eax
.sps_ok:
    ret

; Order- und Pattern-Fenster waehrend der Wiedergabe mitfuehren
sync_play_scroll:
    mov eax, [dispPlayOrder]
    mov ecx, [ordViewTop]
    cmp eax, ecx
    jae .spo_bot
    mov [ordViewTop], eax
    jmp .spr_chk
.spo_bot:
    add ecx, 11
    cmp eax, ecx
    jbe .spr_chk
    sub eax, 11
    mov [ordViewTop], eax
.spr_chk:
    mov eax, [curPat]
    cmp eax, [dispPlayPat]
    jne .sps_done
    mov eax, [dispPlayRow]
    mov ecx, [patViewTop]
    cmp eax, ecx
    jae .spr_bot
    mov [patViewTop], eax
    ret
.spr_bot:
    add ecx, VIEW_ROWS - 1
    cmp eax, ecx
    jbe .sps_done
    sub eax, VIEW_ROWS - 1
    mov [patViewTop], eax
.sps_done:
    ret

; Rechteck in memDC fuellen: ECX=x, EDX=y, R8D=w, R9D=h, R10D=COLORREF
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

; 4K-Demo-Modus umschalten (startet Wiedergabe automatisch, falls pausiert)
toggle_demo_mode:
    xor dword [demoMode], 1
    cmp dword [demoMode], 0
    je .tdm_done
    mov dword [showHelp], 0
    cmp dword [isPlaying], 0
    jne .tdm_done
    mov dword [isPlaying], 1
    mov eax, [curPat]
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov eax, [editRow]
    mov [playRow], eax
    mov [dispPlayRow], eax
    mov dword [needPatTempo], 0
    mov dword [tickSamples], 1
.tdm_done:
    ret

; ============================================================================
; Copy/Paste fuer einzelne Spuren (64 Zeilen) und ganze Patterns (8x64 Zeilen)
; ============================================================================

; Aktuelle Spur (curCh im curPat, 64 Zeilen) in clipChBuf kopieren
copy_cur_channel:
    push rsi
    push rdi
    sub rsp, 0x28

    mov eax, [curPat]
    and eax, MAX_PATS - 1
    shl eax, 11
    mov ecx, [curCh]
    and ecx, 7
    shl ecx, 2
    add eax, ecx
    lea rsi, [patterns]
    add rsi, rax
    lea rdi, [clipChBuf]

    mov ecx, PAT_ROWS
.ccc_loop:
    mov eax, [rsi]
    mov [rdi], eax
    add rsi, ROW_BYTES
    add rdi, CELL_SIZE
    dec ecx
    jnz .ccc_loop

    mov dword [clipChValid], 1
    lea rcx, [stCpCh]
    call set_status
    mov dword [dirty], 1

    add rsp, 0x28
    pop rdi
    pop rsi
    ret

; Kopierte Spur aus clipChBuf in aktuellen Kanal (curCh im curPat) einfuegen
paste_cur_channel:
    push rsi
    push rdi
    sub rsp, 0x28

    cmp dword [clipChValid], 0
    jne .pcc_do
    lea rcx, [stNoClip]
    call set_status
    mov dword [dirty], 1
    jmp .pcc_done

.pcc_do:
    mov eax, [curPat]
    and eax, MAX_PATS - 1
    shl eax, 11
    mov ecx, [curCh]
    and ecx, 7
    shl ecx, 2
    add eax, ecx
    lea rdi, [patterns]
    add rdi, rax
    lea rsi, [clipChBuf]

    mov ecx, PAT_ROWS
.pcc_loop:
    mov eax, [rsi]
    mov [rdi], eax
    add rsi, CELL_SIZE
    add rdi, ROW_BYTES
    dec ecx
    jnz .pcc_loop

    lea rcx, [stPsCh]
    call set_status
    mov dword [dirty], 1

.pcc_done:
    add rsp, 0x28
    pop rdi
    pop rsi
    ret

; Komplettes Pattern (curPat, 8x64 Zellen + Break + Tempo) in clipPatBuf kopieren
copy_cur_pattern:
    push rsi
    push rdi
    sub rsp, 0x28

    mov eax, [curPat]
    and eax, MAX_PATS - 1
    lea rdx, [patBreaks]
    movzx ecx, byte [rdx + rax]
    mov [clipPatBreak], ecx
    lea rdx, [patTempos]
    movzx ecx, byte [rdx + rax]
    mov [clipPatTempo], ecx

    shl eax, 11
    lea rsi, [patterns]
    add rsi, rax
    lea rdi, [clipPatBuf]
    mov ecx, PAT_BYTES / 4
    rep movsd

    mov dword [clipPatValid], 1
    lea rcx, [stCpPat]
    call set_status
    mov dword [dirty], 1

    add rsp, 0x28
    pop rdi
    pop rsi
    ret

; Kopiertes Pattern aus clipPatBuf inkl. Break und Tempo in curPat einfuegen
paste_cur_pattern:
    push rsi
    push rdi
    sub rsp, 0x28

    cmp dword [clipPatValid], 0
    jne .pcp_do
    lea rcx, [stNoClip]
    call set_status
    mov dword [dirty], 1
    jmp .pcp_done

.pcp_do:
    mov eax, [curPat]
    and eax, MAX_PATS - 1
    mov ecx, [clipPatBreak]
    lea rdx, [patBreaks]
    mov [rdx + rax], cl
    mov ecx, [clipPatTempo]
    lea rdx, [patTempos]
    mov [rdx + rax], cl

    shl eax, 11
    lea rdi, [patterns]
    add rdi, rax
    lea rsi, [clipPatBuf]
    mov ecx, PAT_BYTES / 4
    rep movsd

    lea rcx, [stPsPat]
    call set_status
    mov dword [dirty], 1

.pcp_done:
    add rsp, 0x28
    pop rdi
    pop rsi
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

    ; Bei offenem Hilfe-Overlay nur Schliessen zulassen
    cmp dword [showHelp], 0
    je .kd_norm
    cmp ebx, VK_ESCAPE
    je .kd_close_help
    cmp ebx, VK_RETURN
    je .kd_close_help
    cmp ebx, VK_SPACE
    je .kd_close_help
    mov ecx, VK_CONTROL
    call GetKeyState
    test ax, 0x8000
    jz .k_done
    cmp ebx, 'H'
    jne .k_done
.kd_close_help:
    mov dword [showHelp], 0
    jmp .k_done
.kd_norm:

    ; Strg-Kombinationen inkl. Copy/Paste (C/V, Shift+C/V) und Loeschfunktionen
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
    cmp ebx, 'F'
    je .do_fs
    cmp ebx, 'B'
    je .do_break
    cmp ebx, 'H'
    je .do_help
    cmp ebx, 'D'
    je .do_demo
    cmp ebx, 'N'
    je .do_clr_all
    cmp ebx, 'C'
    je .chk_ctrl_c
    cmp ebx, 'V'
    je .chk_ctrl_v
    cmp ebx, VK_DELETE
    je .do_clr_pat
    cmp ebx, 'P'
    je .chk_ctrl_shift_p
    cmp ebx, 'I'
    je .chk_ctrl_shift_i
    jmp .k_done

.chk_ctrl_c:
    test esi, 0x40000000
    jnz .k_done
    cmp dword [demoMode], 0
    jne .k_done
    mov ecx, VK_SHIFT
    call GetKeyState
    test ax, 0x8000
    jnz .do_cp_pat
    call copy_cur_channel
    jmp .k_done
.do_cp_pat:
    call copy_cur_pattern
    jmp .k_done

.chk_ctrl_v:
    test esi, 0x40000000
    jnz .k_done
    cmp dword [demoMode], 0
    jne .k_done
    mov ecx, VK_SHIFT
    call GetKeyState
    test ax, 0x8000
    jnz .do_ps_pat
    call paste_cur_channel
    jmp .k_done
.do_ps_pat:
    call paste_cur_pattern
    jmp .k_done

.chk_ctrl_shift_p:
    mov ecx, VK_SHIFT
    call GetKeyState
    test ax, 0x8000
    jnz .do_clr_pats
    jmp .k_done

.chk_ctrl_shift_i:
    mov ecx, VK_SHIFT
    call GetKeyState
    test ax, 0x8000
    jnz .do_clr_inst
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

.do_fs:
    test esi, 0x40000000
    jnz .k_done
    call toggle_fullscreen
    jmp .k_done

.do_break:
    mov eax, [curPat]
    and eax, 127
    mov ecx, [editRow]
    and ecx, PAT_ROWS - 1
    lea rdx, [patBreaks]
    mov [rdx + rax], cl
    lea rcx, [stBreak]
    call set_status
    jmp .k_done

.do_help:
    test esi, 0x40000000
    jnz .k_done
    mov dword [demoMode], 0
    xor dword [showHelp], 1
    jmp .k_done

.do_demo:
    test esi, 0x40000000
    jnz .k_done
    call toggle_demo_mode
    jmp .k_done

.do_clr_pat:
    test esi, 0x40000000
    jnz .k_done
    call clear_cur_pattern
    jmp .k_done

.do_clr_pats:
    test esi, 0x40000000
    jnz .k_done
    call clear_all_patterns
    jmp .k_done

.do_clr_inst:
    test esi, 0x40000000
    jnz .k_done
    call clear_all_instruments
    jmp .k_done

.do_clr_all:
    test esi, 0x40000000
    jnz .k_done
    call clear_all_song
    jmp .k_done

.chk_esc:
    cmp ebx, VK_ESCAPE
    jne .chk_space
    cmp dword [demoMode], 0
    je .esc_quit
    mov dword [demoMode], 0
    jmp .k_done
.esc_quit:
    mov dword [running], 0
    jmp .k_done

.chk_space:
    cmp ebx, VK_SPACE
    jne .chk_f5
    test esi, 0x40000000
    jnz .k_done
    xor dword [isPlaying], 1
    cmp dword [isPlaying], 0
    jne .start_play
    call silence_audio
    jmp .k_done
.start_play:
    mov eax, [curPat]
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov eax, [editRow]
    mov [playRow], eax
    mov [dispPlayRow], eax
    mov dword [needPatTempo], 0
    mov dword [tickSamples], 1
    jmp .k_done

.chk_f5:
    cmp ebx, VK_F5
    jne .chk_f6
    mov dword [isPlaying], 1
    mov dword [playRow], 0
    mov dword [dispPlayRow], 0
    cmp dword [songMode], 0
    je .f5_pat
    mov dword [playOrder], 0
    mov dword [dispPlayOrder], 0
    mov dword [ordViewTop], 0
    movzx eax, byte [orderList + 0]
    and eax, 127
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov [curPat], eax
    mov dword [needPatTempo], 1
    jmp .f5_go
.f5_pat:
    mov eax, [curPat]
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov dword [needPatTempo], 0
.f5_go:
    mov dword [patViewTop], 0
    mov dword [tickSamples], 1
    jmp .k_done

.chk_f6:
    cmp ebx, VK_F6
    jne .chk_demo_guard
    xor dword [songMode], 1
    jmp .k_done

.chk_demo_guard:
    ; Im Demo-Modus schaltet TAB zwischen Plasma, 3D-Tunnel und Rasterbars um
    cmp dword [demoMode], 0
    je .chk_f1
    cmp ebx, VK_TAB
    jne .k_done
    test esi, 0x40000000
    jnz .k_done
    mov eax, [demoFx]
    inc eax
    cmp eax, 3
    jb .dfx_ok
    xor eax, eax
.dfx_ok:
    mov [demoFx], eax
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
    and eax, 63
    mov [curInst], eax
    call sync_samp_scroll
    jmp .k_done

.chk_f4:
    cmp ebx, VK_F4
    jne .chk_f7
    mov eax, [curInst]
    inc eax
    and eax, 63
    mov [curInst], eax
    call sync_samp_scroll
    jmp .k_done

.chk_f7:
    cmp ebx, VK_F7
    jne .chk_f8
    cmp dword [curTempo], 1
    jle .k_done
    dec dword [curTempo]
    mov eax, [curPat]
    and eax, 127
    mov cl, [curTempo]
    lea rdx, [patTempos]
    mov [rdx + rax], cl
    jmp .k_done

.chk_f8:
    cmp ebx, VK_F8
    jne .chk_f9
    cmp dword [curTempo], 15
    jae .k_done
    inc dword [curTempo]
    mov eax, [curPat]
    and eax, 127
    mov cl, [curTempo]
    lea rdx, [patTempos]
    mov [rdx + rax], cl
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
    cmp byte [rdx + rax], 0
    je .k_done
    shl eax, 6
    lea rdx, [voices]
    mov dword [rdx + rax + 0], 0
    mov dword [rdx + rax + 20], 0
    mov dword [rdx + rax + 48], 0
    jmp .k_done

.chk_up:
    cmp ebx, VK_UP
    jne .chk_down
    mov eax, [editRow]
    dec eax
    and eax, PAT_ROWS - 1
    mov [editRow], eax
    call sync_pat_scroll
    jmp .k_done

.chk_down:
    cmp ebx, VK_DOWN
    jne .chk_home
    mov eax, [editRow]
    inc eax
    and eax, PAT_ROWS - 1
    mov [editRow], eax
    call sync_pat_scroll
    jmp .k_done

.chk_home:
    cmp ebx, VK_HOME
    jne .chk_end
    mov dword [editRow], 0
    call sync_pat_scroll
    jmp .k_done

.chk_end:
    cmp ebx, VK_END
    jne .chk_left
    mov eax, [curPat]
    and eax, 127
    lea rdx, [patBreaks]
    movzx eax, byte [rdx + rax]
    mov [editRow], eax
    call sync_pat_scroll
    jmp .k_done

.chk_left:
    cmp ebx, VK_LEFT
    jne .chk_right
    mov eax, [curSubCol]
    dec eax
    jns .l_ok
    mov eax, 5
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
    cmp eax, 6
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
    and eax, MAX_PATS - 1
    mov [curPat], eax
    jmp .k_done

.chk_pgdn:
    cmp ebx, VK_NEXT
    jne .chk_del
    mov eax, [curPat]
    dec eax
    and eax, MAX_PATS - 1
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
    jbe .del_inst
    cmp ecx, 3
    je .del_vol
    mov byte [rax + 3], 0xFF
    jmp .adv_step
.del_inst:
    mov byte [rax + 1], 0xFF
    jmp .adv_step
.del_vol:
    mov byte [rax + 2], 0xFF
    jmp .adv_step
.del_all:
    mov dword [rax], 0xFFFFFFFF
    jmp .adv_step

.chk_bs:
    cmp ebx, VK_BACK
    jne .chk_edit_input
    mov eax, [editRow]
    dec eax
    and eax, PAT_ROWS - 1
    mov [editRow], eax
    call sync_pat_scroll
    mov ecx, [curPat]
    mov edx, [editRow]
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
    je .in_inst_hi
    cmp eax, 2
    je .in_inst_lo
    cmp eax, 3
    je .in_vol
    cmp eax, 4
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

.in_inst_hi:
    call vk_to_hex
    cmp eax, 3
    ja .k_done
    shl eax, 4
    movzx ecx, byte [rdi + 1]
    cmp cl, 0xFF
    jne .ih_keep
    xor ecx, ecx
.ih_keep:
    and ecx, 0x0F
    or eax, ecx
    mov [rdi + 1], al
    mov [curInst], eax
    call sync_samp_scroll
    mov dword [curSubCol], 2
    jmp .k_done

.in_inst_lo:
    call vk_to_hex
    cmp eax, 15
    ja .k_done
    movzx ecx, byte [rdi + 1]
    cmp cl, 0xFF
    jne .il_keep
    xor ecx, ecx
.il_keep:
    and ecx, 0x30
    or eax, ecx
    mov [rdi + 1], al
    mov [curInst], eax
    call sync_samp_scroll
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
    mov dword [curSubCol], 5
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
    and eax, PAT_ROWS - 1
    mov [editRow], eax
    call sync_pat_scroll

.k_done:
    add rsp, 0x20
    pop rdi
    pop rsi
    pop rbx
    ret

; ============================================================================
; Client-Koordinaten (ECX=x, EDX=y) in 80x50-Raster umrechnen (Fenster und Fullscreen)
; ============================================================================
mouse_to_grid:
    mov r9d, edx
    cmp dword [isFullscreen], 0
    je .mtg_win

    sub ecx, [dispX]
    js .mtg_out
    cmp ecx, [dispW]
    jae .mtg_out
    sub r9d, [dispY]
    js .mtg_out
    cmp r9d, [dispH]
    jae .mtg_out

    mov eax, ecx
    imul eax, WIN_W
    xor edx, edx
    div dword [dispW]
    mov ecx, eax

    mov eax, r9d
    imul eax, WIN_H
    xor edx, edx
    div dword [dispH]
    mov r9d, eax
    jmp .mtg_div

.mtg_win:
    test ecx, ecx
    js .mtg_out
    cmp ecx, WIN_W
    jae .mtg_out
    test r9d, r9d
    js .mtg_out
    cmp r9d, WIN_H
    jae .mtg_out

.mtg_div:
    mov eax, ecx
    xor edx, edx
    mov r8d, CHAR_W
    div r8d
    mov ecx, eax

    mov eax, r9d
    xor edx, edx
    mov r8d, CHAR_H
    div r8d
    mov edx, eax
    mov eax, ecx
    ret

.mtg_out:
    mov eax, -1
    ret

; ============================================================================
; Mausklick verarbeiten: ECX = lParam (x,y), EDX = Taste (0=Links, 1=Rechts)
; ============================================================================
handle_mouse_down:
    push rbx
    push r12
    push r13
    sub rsp, 0x20

    mov ebx, edx

    ; Im Demo-Modus: Linksklick kehrt zum Tracker zurueck, Rechtsklick = Play/Stop
    cmp dword [demoMode], 0
    je .md_chk_help
    test ebx, ebx
    jnz .md_toggle_play
    mov dword [demoMode], 0
    jmp .md_done

.md_chk_help:
    cmp dword [showHelp], 0
    je .md_norm
    mov dword [showHelp], 0
    jmp .md_done
.md_norm:

    mov eax, ecx
    movsx ecx, ax
    sar eax, 16
    mov edx, eax
    call mouse_to_grid
    cmp eax, -1
    je .md_done

    mov r12d, eax
    mov r13d, edx

    ; Kopfzeile (Zeile 0): Mode PAT-LOOP / SONG-ORD umschalten
    test r13d, r13d
    jnz .md_chk_upper
    test ebx, ebx
    jnz .md_done
    cmp r12d, 57
    jb .md_done
    cmp r12d, 70
    ja .md_done
    xor dword [songMode], 1
    jmp .md_done

.md_chk_upper:
    cmp r13d, 2
    jb .md_chk_hdr
    cmp r13d, 13
    ja .md_chk_hdr

    ; Sample-Liste (Spalten 1..33)
    cmp r12d, 1
    jb .md_chk_ord
    cmp r12d, 33
    ja .md_chk_ord
    mov eax, r13d
    sub eax, 2
    add eax, [sampViewTop]
    cmp eax, MAX_SAMPS
    jae .md_done
    mov [curInst], eax
    test ebx, ebx
    jz .md_done

    mov ecx, [curOct]
    imul ecx, 12
    mov [previewCell + 0], cl
    mov [previewCell + 1], al
    mov ecx, [curVol]
    mov [previewCell + 2], cl
    mov byte [previewCell + 3], 0xFF
    mov ecx, [curCh]
    lea rdx, [previewCell]
    mov r8d, 1
    call trigger_channel
    jmp .md_done

.md_chk_ord:
    ; Rechte Order-Liste (Spalten 72..78)
    cmp r12d, 72
    jb .md_done
    cmp r12d, 78
    ja .md_done
    mov eax, r13d
    sub eax, 2
    add eax, [ordViewTop]
    cmp eax, [orderLen]
    jae .md_done

    lea r8, [orderList]
    movzx ecx, byte [r8 + rax]
    and ecx, 127
    mov [curPat], ecx
    mov [playOrder], eax
    mov [dispPlayOrder], eax
    mov [playPat], ecx
    mov [dispPlayPat], ecx
    mov dword [editRow], 0
    mov dword [patViewTop], 0
    test ebx, ebx
    jz .md_done

    mov dword [songMode], 1
    mov dword [isPlaying], 1
    mov dword [playRow], 0
    mov dword [dispPlayRow], 0
    mov dword [needPatTempo], 1
    mov dword [tickSamples], 1
    jmp .md_done

.md_chk_hdr:
    ; Kanal-Koepfe On/Off (Zeile 15)
    cmp r13d, 15
    jne .md_chk_pat
    cmp r12d, 8
    jb .md_done
    mov eax, r12d
    sub eax, 8
    xor edx, edx
    mov ecx, 9
    div ecx
    cmp eax, NUM_CH
    jae .md_done
    cmp edx, 8
    jae .md_done
    lea r8, [chMuted]
    xor byte [r8 + rax], 1
    cmp byte [r8 + rax], 0
    je .md_done
    shl eax, 6
    lea r8, [voices]
    mov dword [r8 + rax + 0], 0
    mov dword [r8 + rax + 20], 0
    mov dword [r8 + rax + 48], 0
    jmp .md_done

.md_chk_pat:
    ; Pattern-Bereich (Zeilen 16..47)
    cmp r13d, 16
    jb .md_chk_clr
    cmp r13d, 47
    ja .md_chk_clr

    mov eax, r13d
    sub eax, 16
    add eax, [patViewTop]
    cmp eax, PAT_ROWS
    jae .md_done
    mov r13d, eax

    cmp r12d, 1
    jb .md_done
    cmp r12d, 6
    ja .md_pat_cell
    test ebx, ebx
    jnz .md_row_brk
    mov [editRow], r13d
    jmp .md_done
.md_row_brk:
    mov ecx, [curPat]
    and ecx, 127
    lea r8, [patBreaks]
    mov [r8 + rcx], r13b
    lea rcx, [stBreak]
    call set_status
    jmp .md_done

.md_pat_cell:
    cmp r12d, 8
    jb .md_done
    mov eax, r12d
    sub eax, 8
    xor edx, edx
    mov ecx, 9
    div ecx
    cmp eax, NUM_CH
    jae .md_done
    cmp edx, 8
    jae .md_done

    mov [editRow], r13d
    mov [curCh], eax
    sub edx, 2
    jns .md_sub_ok
    xor edx, edx
.md_sub_ok:
    mov [curSubCol], edx

    test ebx, ebx
    jz .md_done
    mov ecx, [curPat]
    mov edx, r13d
    mov r8d, [curCh]
    call get_cell_ptr
    mov ecx, [curCh]
    mov rdx, rax
    mov r8d, 1
    call trigger_channel
    jmp .md_done

.md_chk_clr:
    ; Aktions-Leiste (Zeile 48, nur Linksklick): Loeschen + Copy/Paste
    cmp r13d, 48
    jne .md_chk_bot
    test ebx, ebx
    jnz .md_done

    cmp r12d, 11
    jb .md_done
    cmp r12d, 19
    ja .md_clr_pats
    call clear_cur_pattern
    jmp .md_done

.md_clr_pats:
    cmp r12d, 21
    jb .md_done
    cmp r12d, 30
    ja .md_clr_inst
    call clear_all_patterns
    jmp .md_done

.md_clr_inst:
    cmp r12d, 32
    jb .md_done
    cmp r12d, 42
    ja .md_clr_all
    call clear_all_instruments
    jmp .md_done

.md_clr_all:
    cmp r12d, 44
    jb .md_done
    cmp r12d, 54
    ja .md_btn_spc
    call clear_all_song
    jmp .md_done

.md_btn_spc:
    cmp r12d, 56
    jb .md_done
    cmp r12d, 60
    ja .md_btn_spv
    call copy_cur_channel
    jmp .md_done

.md_btn_spv:
    cmp r12d, 62
    jb .md_done
    cmp r12d, 66
    ja .md_btn_ptc
    call paste_cur_channel
    jmp .md_done

.md_btn_ptc:
    cmp r12d, 68
    jb .md_done
    cmp r12d, 72
    ja .md_btn_ptv
    call copy_cur_pattern
    jmp .md_done

.md_btn_ptv:
    cmp r12d, 74
    jb .md_done
    cmp r12d, 78
    ja .md_done
    call paste_cur_pattern
    jmp .md_done

.md_chk_bot:
    ; Fusszeile (Zeile 49): Werte (L=+/R=-), Play, Break, Demo [D] und Hilfe [?]
    cmp r13d, 49
    jne .md_done

    cmp r12d, 76
    jb .md_bot_demo
    cmp r12d, 78
    ja .md_done
    xor dword [showHelp], 1
    jmp .md_done

.md_bot_demo:
    cmp r12d, 72
    jb .md_bot_tmp
    cmp r12d, 74
    ja .md_done
    call toggle_demo_mode
    jmp .md_done

.md_bot_tmp:
    cmp r12d, 1
    jb .md_done
    cmp r12d, 5
    ja .md_bot_oct
    test ebx, ebx
    jnz .md_tmp_dec
    cmp dword [curTempo], 15
    jae .md_done
    inc dword [curTempo]
    jmp .md_tmp_sync
.md_tmp_dec:
    cmp dword [curTempo], 1
    jle .md_done
    dec dword [curTempo]
.md_tmp_sync:
    mov eax, [curPat]
    and eax, 127
    mov cl, [curTempo]
    lea rdx, [patTempos]
    mov [rdx + rax], cl
    jmp .md_done

.md_bot_oct:
    cmp r12d, 7
    jb .md_done
    cmp r12d, 11
    ja .md_bot_ch
    test ebx, ebx
    jnz .md_oct_dec
    cmp dword [curOct], 3
    jae .md_done
    inc dword [curOct]
    jmp .md_done
.md_oct_dec:
    cmp dword [curOct], 0
    je .md_done
    dec dword [curOct]
    jmp .md_done

.md_bot_ch:
    cmp r12d, 13
    jb .md_done
    cmp r12d, 16
    ja .md_bot_pat
    mov eax, [curCh]
    test ebx, ebx
    jnz .md_ch_dec
    inc eax
    jmp .md_ch_st
.md_ch_dec:
    dec eax
.md_ch_st:
    and eax, 7
    mov [curCh], eax
    jmp .md_done

.md_bot_pat:
    cmp r12d, 18
    jb .md_done
    cmp r12d, 23
    ja .md_bot_ins
    mov eax, [curPat]
    test ebx, ebx
    jnz .md_pat_dec
    inc eax
    jmp .md_pat_st
.md_pat_dec:
    dec eax
.md_pat_st:
    and eax, MAX_PATS - 1
    mov [curPat], eax
    jmp .md_done

.md_bot_ins:
    cmp r12d, 35
    jb .md_done
    cmp r12d, 40
    ja .md_bot_stp
    mov eax, [curInst]
    test ebx, ebx
    jnz .md_ins_dec
    inc eax
    jmp .md_ins_st
.md_ins_dec:
    dec eax
.md_ins_st:
    and eax, MAX_SAMPS - 1
    mov [curInst], eax
    call sync_samp_scroll
    jmp .md_done

.md_bot_stp:
    cmp r12d, 42
    jb .md_done
    cmp r12d, 46
    ja .md_bot_btns
    mov eax, [editStep]
    test ebx, ebx
    jnz .md_stp_dec
    cmp eax, 1
    je .ms_2
    cmp eax, 2
    je .ms_4
    cmp eax, 4
    je .ms_0
    mov dword [editStep], 1
    jmp .md_done
.md_stp_dec:
    cmp eax, 1
    je .ms_0
    cmp eax, 0
    je .ms_4
    cmp eax, 4
    je .ms_2
    mov dword [editStep], 1
    jmp .md_done
.ms_2:
    mov dword [editStep], 2
    jmp .md_done
.ms_4:
    mov dword [editStep], 4
    jmp .md_done
.ms_0:
    mov dword [editStep], 0
    jmp .md_done

.md_bot_btns:
    test ebx, ebx
    jnz .md_done
    cmp r12d, 48
    jb .md_done
    cmp r12d, 58
    jbe .md_toggle_play
    cmp r12d, 60
    jb .md_done
    cmp r12d, 70
    ja .md_done

    mov eax, [curPat]
    and eax, 127
    mov ecx, [editRow]
    and ecx, PAT_ROWS - 1
    lea rdx, [patBreaks]
    mov [rdx + rax], cl
    lea rcx, [stBreak]
    call set_status
    jmp .md_done

.md_toggle_play:
    xor dword [isPlaying], 1
    cmp dword [isPlaying], 0
    jne .md_start_play
    call silence_audio
    jmp .md_done
.md_start_play:
    mov eax, [curPat]
    mov [playPat], eax
    mov [dispPlayPat], eax
    mov eax, [editRow]
    mov [playRow], eax
    mov [dispPlayRow], eax
    mov dword [needPatTempo], 0
    mov dword [tickSamples], 1

.md_done:
    add rsp, 0x20
    pop r13
    pop r12
    pop rbx
    ret

; ============================================================================
; Mausrad verarbeiten: ECX = wParam (Delta), EDX = lParam (Bildschirm-X/Y)
; ============================================================================
handle_mouse_wheel:
    push rbx
    push r12
    push r13
    sub rsp, 0x20

    cmp dword [showHelp], 0
    jne .mw_done

    ; Im Demo-Modus wechselt das Mausrad ebenfalls den Effekt
    cmp dword [demoMode], 0
    je .mw_norm
    mov eax, ecx
    sar eax, 16
    test eax, eax
    jz .mw_done
    mov edx, [demoFx]
    js .mw_dfx_next
    dec edx
    jns .mw_dfx_st
    mov edx, 2
    jmp .mw_dfx_st
.mw_dfx_next:
    inc edx
    cmp edx, 3
    jb .mw_dfx_st
    xor edx, edx
.mw_dfx_st:
    mov [demoFx], edx
    jmp .mw_done
.mw_norm:

    mov eax, ecx
    sar eax, 16
    test eax, eax
    jz .mw_done
    mov ebx, -1
    jns .mw_dir_ok
    mov ebx, 1
.mw_dir_ok:

    mov eax, edx
    movsx ecx, ax
    mov [mousePt + 0], ecx
    sar eax, 16
    mov [mousePt + 4], eax

    mov rcx, [hWnd]
    lea rdx, [mousePt]
    call ScreenToClient

    mov ecx, [mousePt + 0]
    mov edx, [mousePt + 4]
    call mouse_to_grid
    cmp eax, -1
    je .mw_pat

    mov r12d, eax
    mov r13d, edx

    cmp r13d, 2
    jb .mw_pat
    cmp r13d, 13
    ja .mw_pat

    cmp r12d, 34
    ja .mw_chk_ord
    mov eax, [sampViewTop]
    add eax, ebx
    add eax, ebx
    test eax, eax
    jns .mws_hi
    xor eax, eax
.mws_hi:
    cmp eax, MAX_SAMPS - 12
    jbe .mws_st
    mov eax, MAX_SAMPS - 12
.mws_st:
    mov [sampViewTop], eax
    jmp .mw_done

.mw_chk_ord:
    cmp r12d, 71
    jb .mw_pat
    mov eax, [ordViewTop]
    add eax, ebx
    add eax, ebx
    test eax, eax
    jns .mwo_hi
    xor eax, eax
.mwo_hi:
    cmp eax, MAX_ORDERS - 12
    jbe .mwo_st
    mov eax, MAX_ORDERS - 12
.mwo_st:
    mov [ordViewTop], eax
    jmp .mw_done

.mw_pat:
    mov eax, ebx
    shl eax, 2
    add eax, [editRow]
    test eax, eax
    jns .mwp_hi
    xor eax, eax
.mwp_hi:
    cmp eax, PAT_ROWS - 1
    jbe .mwp_st
    mov eax, PAT_ROWS - 1
.mwp_st:
    mov [editRow], eax
    call sync_pat_scroll

.mw_done:
    add rsp, 0x20
    pop r13
    pop r12
    pop rbx
    ret

; 4-Byte-Zelle in 8 ASCII-Zeichen nach [cellBuf] formatieren (RSI = CellPtr)
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
    and al, 63
    lea r8, [cellBuf + 3]
    call fmt_hex2

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
; Hilfe-Overlay mittig in memDC zeichnen (Spalten 7..72, Zeilen 6..41)
; ============================================================================
draw_help_overlay:
    push rbx
    sub rsp, 0x20

    mov ecx, 7 * CHAR_W
    mov edx, 6 * CHAR_H
    mov r8d, 66 * CHAR_W
    mov r9d, 36 * CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect

    mov ecx, 7 * CHAR_W + 3
    mov edx, 6 * CHAR_H + 3
    mov r8d, 66 * CHAR_W - 6
    mov r9d, 36 * CHAR_H - 6
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, 8 * CHAR_W
    mov edx, 7 * CHAR_H
    mov r8d, 64 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    call fill_rect

    xor ebx, ebx
.hl_loop:
    mov r10d, COL_NOTE
    test ebx, ebx
    jnz .hl_c1
    mov r10d, COL_WHITE
    jmp .hl_draw
.hl_c1:
    cmp ebx, 1
    je .hl_teal
    cmp ebx, 32
    je .hl_teal
    cmp ebx, 2
    je .hl_purp
    cmp ebx, 9
    je .hl_purp
    cmp ebx, 21
    je .hl_purp
    cmp ebx, 33
    jne .hl_draw
    mov r10d, COL_CYAN
    jmp .hl_draw
.hl_teal:
    mov r10d, COL_TEAL
    jmp .hl_draw
.hl_purp:
    mov r10d, COL_PURPHI

.hl_draw:
    mov eax, ebx
    shl eax, 6
    lea r8, [helpLines]
    add r8, rax
    mov ecx, 8
    lea edx, [rbx + 7]
    mov r9d, 64
    call draw_text

    inc ebx
    cmp ebx, HELP_LINES
    jb .hl_loop

    add rsp, 0x20
    pop rbx
    ret

; ============================================================================
; Gesamte Composer-669-Oberflaeche oder 4K-Demo in memDC zeichnen und blitten
; Lokale Stack-Slots: [rsp + 0x58] = curDrawRow (0..63), [rsp + 0x5C] = curPatBrk
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
    sub rsp, 0x68

    cmp dword [isPlaying], 0
    je .ds_no_follow
    call sync_play_scroll
.ds_no_follow:

    cmp dword [demoMode], 0
    je .ds_tracker
    call draw_demo_frame
    jmp .ds_output
.ds_tracker:

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
    mov ebp, ebx
    add ebp, [sampViewTop]

    mov al, bpl
    lea r8, [lineBuf]
    call fmt_hex2
    mov r10d, COL_TEAL
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbp], 0
    je .si_chk_cur
    mov r10d, COL_PURPHI
.si_chk_cur:
    cmp ebp, [curInst]
    jne .si_col
    mov r10d, COL_CYAN
.si_col:
    mov ecx, 1
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 2
    call draw_text

    mov eax, ebp
    imul eax, 12
    lea r8, [sampNames]
    add r8, rax
    mov r10d, COL_NOTE
    lea rdx, [sampHasPcm]
    cmp byte [rdx + rbp], 0
    je .sn_chk_cur
    mov r10d, COL_CYAN
.sn_chk_cur:
    cmp ebp, [curInst]
    jne .sn_col
    mov r10d, COL_WHITE
.sn_col:
    mov ecx, 4
    mov edx, r12d
    mov r9d, 12
    call draw_text

    lea r13, [sampLoopStartTbl]
    mov eax, [r13 + rbp*4]
    lea r8, [lineBuf]
    call fmt_hex5
    mov byte [lineBuf + 5], ' '

    lea r13, [sampLoopEndTbl]
    mov eax, [r13 + rbp*4]
    lea r8, [lineBuf + 6]
    call fmt_hex5
    mov byte [lineBuf + 11], ' '

    lea r13, [sampLenTbl]
    mov eax, [r13 + rbp*4]
    lea r8, [lineBuf + 12]
    call fmt_hex5

    mov ecx, 17
    mov edx, r12d
    lea r8, [lineBuf]
    mov r9d, 17
    mov r10d, COL_NOTE
    call draw_text

    mov ebp, ebx
    add ebp, [ordViewTop]
    mov al, bpl
    lea r8, [lineBuf]
    call fmt_hex2
    mov byte [lineBuf + 2], ' '

    cmp ebp, [orderLen]
    jae .ord_empty

    lea r13, [orderList]
    movzx r14d, byte [r13 + rbp]
    and r14d, 127
    mov al, r14b
    lea r8, [lineBuf + 3]
    call fmt_hex2
    mov byte [lineBuf + 5], ' '
    lea r13, [patTempos]
    movzx eax, byte [r13 + r14]
    and eax, 15
    lea r13, [hexTbl]
    mov al, [r13 + rax]
    mov [lineBuf + 6], al

    mov r10d, COL_TEAL
    cmp ebp, [dispPlayOrder]
    jne .ord_c
    mov r10d, COL_CYAN
    jmp .ord_c

.ord_empty:
    mov dword [lineBuf + 3], 0x2D202D2D
    mov r10d, COL_DOT

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
    mov al, [dispPlayOrder]
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

    mov eax, [curPat]
    and eax, 127
    lea rdx, [patBreaks]
    movzx eax, byte [rdx + rax]
    mov [rsp + 0x5C], eax

    mov ecx, 4 * CHAR_W
    mov edx, 16 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, VIEW_ROWS * CHAR_H
    mov r10d, COL_PURP
    call fill_rect

    mov eax, [rsp + 0x5C]
    sub eax, [patViewTop]
    js .no_bbar
    cmp eax, VIEW_ROWS
    jae .no_bbar
    mov ecx, 4 * CHAR_W
    add eax, 16
    imul eax, CHAR_H
    mov edx, eax
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
.no_bbar:

    cmp dword [isPlaying], 0
    je .no_pbar
    mov eax, [curPat]
    cmp eax, [dispPlayPat]
    jne .no_pbar
    mov eax, [dispPlayRow]
    sub eax, [patViewTop]
    js .no_pbar
    cmp eax, VIEW_ROWS
    jae .no_pbar
    mov ecx, 4 * CHAR_W
    add eax, 16
    imul eax, CHAR_H
    mov edx, eax
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_CYAN
    call fill_rect
.no_pbar:

    mov eax, [editRow]
    sub eax, [patViewTop]
    js .no_ebar
    cmp eax, VIEW_ROWS
    jae .no_ebar
    mov ecx, 4 * CHAR_W
    add eax, 16
    imul eax, CHAR_H
    mov edx, eax
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURPHI
    call fill_rect
.no_ebar:

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
    mov r9d, VIEW_ROWS * CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    inc ebx
    cmp ebx, NUM_CH
    jb .hdr_loop

    xor r12d, r12d
.row_draw:
    lea r13d, [r12 + 16]
    mov eax, r12d
    add eax, [patViewTop]
    mov [rsp + 0x58], eax

    lea r8, [lineBuf]
    call fmt_hex2
    mov r10d, COL_TEAL
    mov eax, [rsp + 0x58]
    cmp eax, [rsp + 0x5C]
    ja .rn_dim
    je .rn_brk
    jmp .rn_chk_ed
.rn_dim:
    mov r10d, COL_MUTED
    jmp .rn_chk_ed
.rn_brk:
    mov r10d, COL_PURPHI
.rn_chk_ed:
    cmp eax, [editRow]
    jne .rn_c
    mov r10d, COL_WHITE
.rn_c:
    mov ecx, 1
    mov edx, r13d
    lea r8, [lineBuf]
    mov r9d, 2
    call draw_text

    xor ebx, ebx
.ch_draw:
    mov r15d, ebx
    imul r15d, 9
    add r15d, 8

    mov eax, [rsp + 0x58]
    cmp eax, [editRow]
    je .bg_edit
    cmp dword [isPlaying], 0
    je .bg_chk_beat
    mov ecx, [curPat]
    cmp ecx, [dispPlayPat]
    jne .bg_chk_beat
    cmp eax, [dispPlayRow]
    je .bg_play
.bg_chk_beat:
    cmp eax, [rsp + 0x5C]
    ja .bg_skip
    test eax, 3
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

    mov eax, [rsp + 0x58]
    cmp eax, [editRow]
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
    mov edx, [rsp + 0x58]
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
    jnz .n_muted
    mov eax, [rsp + 0x58]
    cmp eax, [rsp + 0x5C]
    jbe .draw_left6
.n_muted:
    mov r10d, COL_MUTED
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
    jnz .fx_muted
    mov eax, [rsp + 0x58]
    cmp eax, [rsp + 0x5C]
    jbe .draw_fx2
.fx_muted:
    mov r10d, COL_MUTED
.draw_fx2:
    lea ecx, [r15 + 6]
    mov edx, r13d
    lea r8, [cellBuf + 6]
    mov r9d, 2
    call draw_text

    mov eax, [rsp + 0x58]
    cmp eax, [editRow]
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
    cmp r12d, VIEW_ROWS
    jb .row_draw

    ; Loesch-Leiste (Zeile 48): Label + 4 Loesch-Schaltflaechen
    mov ecx, 1
    mov edx, 48
    lea r8, [clrBar + 1]
    mov r9d, 9
    mov r10d, COL_TEAL
    call draw_text

    mov ecx, 11 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 9 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 11
    mov edx, 48
    lea r8, [clrBar + 11]
    mov r9d, 9
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 21 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 10 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 21
    mov edx, 48
    lea r8, [clrBar + 21]
    mov r9d, 10
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 32 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_OFF
    call fill_rect
    mov ecx, 32
    mov edx, 48
    lea r8, [clrBar + 32]
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 44 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 11 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    call fill_rect
    mov ecx, 44
    mov edx, 48
    lea r8, [clrBar + 44]
    mov r9d, 11
    mov r10d, COL_WHITE
    call draw_text

    ; Copy/Paste-Schaltflaechen in Zeile 48: [SpC] [SpV] [PtC] [PtV]
    mov ecx, 56 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 5 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 56
    mov edx, 48
    lea r8, [clrBar + 56]
    mov r9d, 5
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 62 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 5 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    cmp dword [clipChValid], 0
    je .spv_bg
    mov r10d, COL_HDR_ON
.spv_bg:
    call fill_rect
    mov ecx, 62
    mov edx, 48
    lea r8, [clrBar + 62]
    mov r9d, 5
    mov r10d, COL_MUTED
    cmp dword [clipChValid], 0
    je .spv_tx
    mov r10d, COL_WHITE
.spv_tx:
    call draw_text

    mov ecx, 68 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 5 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect
    mov ecx, 68
    mov edx, 48
    lea r8, [clrBar + 68]
    mov r9d, 5
    mov r10d, COL_WHITE
    call draw_text

    mov ecx, 74 * CHAR_W
    mov edx, 48 * CHAR_H
    mov r8d, 5 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_BEAT
    cmp dword [clipPatValid], 0
    je .ptv_bg
    mov r10d, COL_HDR_ON
.ptv_bg:
    call fill_rect
    mov ecx, 74
    mov edx, 48
    lea r8, [clrBar + 74]
    mov r9d, 5
    mov r10d, COL_MUTED
    cmp dword [clipPatValid], 0
    je .ptv_tx
    mov r10d, COL_WHITE
.ptv_tx:
    call draw_text

    ; Fusszeile (Zeile 49)
    mov eax, [curTempo]
    lea rdx, [hexTbl]
    mov al, [rdx + rax]
    mov [botBar + 5], al
    mov eax, [curOct]
    add al, '2'
    mov [botBar + 11], al
    mov eax, [curCh]
    add al, '0'
    mov [botBar + 16], al
    mov al, [curPat]
    lea r8, [botBar + 22]
    call fmt_hex2
    mov al, [editRow]
    lea r8, [botBar + 29]
    call fmt_hex2
    mov al, [rsp + 0x5C]
    lea r8, [botBar + 32]
    call fmt_hex2
    mov al, [curInst]
    lea r8, [botBar + 39]
    call fmt_hex2
    mov eax, [editStep]
    add al, '0'
    mov [botBar + 46], al

    xor ecx, ecx
    mov edx, 49
    lea r8, [botBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    ; Demo-Schaltflaeche [D] (Spalten 72..74) hervorheben
    mov ecx, 72 * CHAR_W
    mov edx, 49 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_HDR_ON
    call fill_rect

    mov ecx, 72
    mov edx, 49
    lea r8, [botBar + 72]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    ; Hilfe-Schaltflaeche [?] (Spalten 76..78) hervorheben
    mov ecx, 76 * CHAR_W
    mov edx, 49 * CHAR_H
    mov r8d, 3 * CHAR_W
    mov r9d, CHAR_H
    mov r10d, COL_PURP
    cmp dword [showHelp], 0
    je .q_bg
    mov r10d, COL_PURPHI
.q_bg:
    call fill_rect

    mov ecx, 76
    mov edx, 49
    lea r8, [botBar + 76]
    mov r9d, 3
    mov r10d, COL_WHITE
    call draw_text

    cmp dword [showHelp], 0
    je .ds_output
    call draw_help_overlay

.ds_output:
    cmp dword [isFullscreen], 0
    je .blit_win

    mov r12d, [dispX]
    test r12d, r12d
    jz .fs_chk_y
    mov rcx, [hDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, r12d
    mov eax, [scrH]
    mov [rsp + 0x20], rax
    mov qword [rsp + 0x28], BLACKNESS
    call PatBlt

    mov rcx, [hDC]
    mov edx, [dispX]
    add edx, [dispW]
    xor r8d, r8d
    mov r9d, [scrW]
    sub r9d, edx
    mov eax, [scrH]
    mov [rsp + 0x20], rax
    mov qword [rsp + 0x28], BLACKNESS
    call PatBlt
    jmp .fs_stretch

.fs_chk_y:
    mov r12d, [dispY]
    test r12d, r12d
    jz .fs_stretch
    mov rcx, [hDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, [scrW]
    mov [rsp + 0x20], r12
    mov qword [rsp + 0x28], BLACKNESS
    call PatBlt

    mov rcx, [hDC]
    xor edx, edx
    mov r8d, [dispY]
    add r8d, [dispH]
    mov r9d, [scrW]
    mov eax, [scrH]
    sub eax, r8d
    mov [rsp + 0x20], rax
    mov qword [rsp + 0x28], BLACKNESS
    call PatBlt

.fs_stretch:
    mov rcx, [hDC]
    mov edx, [dispX]
    mov r8d, [dispY]
    mov r9d, [dispW]
    mov eax, [dispH]
    mov [rsp + 0x20], rax
    mov rax, [memDC]
    mov [rsp + 0x28], rax
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    mov qword [rsp + 0x40], WIN_W
    mov qword [rsp + 0x48], WIN_H
    mov qword [rsp + 0x50], SRCCOPY
    call StretchBlt
    jmp .draw_done

.blit_win:
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

.draw_done:
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

; ============================================================================
; Start-Demo-Patterns (Pattern 00 und 01, Break bei Zeile 31) vorbelegen
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

    lea rdi, [patTempos]
    mov al, 5
    mov ecx, MAX_PATS
    rep stosb

    lea rdi, [patBreaks]
    mov al, PAT_ROWS - 1
    mov ecx, MAX_PATS
    rep stosb

    mov byte [patBreaks + 0], 31
    mov byte [patBreaks + 1], 31

    mov byte [orderList + 0], 0
    mov byte [orderList + 1], 1
    mov dword [orderLen], 2
    mov dword [loopOrder], 0

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