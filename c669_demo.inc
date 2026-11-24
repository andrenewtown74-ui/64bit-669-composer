; Integrierter 4K-Demoscene-Visualizer (320x200 32-Bit-Framebuffer -> 960x600)
; Effekte per TAB umschaltbar: 0=Plasma, 1=3D-Tunnel, 2=Rasterbars
; Live gekoppelt an VU-Pegel, Stereo-Audiopuffer und Song-Message der geladenen .669-Datei

; Einzelnes Zeichen pixelgenau in memDC zeichnen: ECX=px, EDX=py, R8=Ptr, R10D=Farbe (inkl. drawOffX/Y)
draw_char_px:
    sub rsp, 0x68
    add ecx, [drawOffX]
    add edx, [drawOffY]
    mov [rsp + 0x40], ecx
    mov [rsp + 0x44], edx
    mov [rsp + 0x48], r8

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
    mov qword [rsp + 0x30], 1
    mov qword [rsp + 0x38], 0
    call ExtTextOutA
    add rsp, 0x68
    ret

; Einmalige Vorausberechnung der 3D-Tunnel-Tabellen (Distanz, Winkel, Tiefennebel)
init_tunnel_lut:
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x28

    lea r14, [tunnelDist]
    lea rbp, [tunnelAngle]
    lea r15, [tunnelFog]
    xor ebx, ebx                        ; Pixelindex 0..63999
    xor r13d, r13d                      ; y = 0..199
.tl_y:
    mov eax, r13d
    sub eax, 100
    mov [rsp + 0x04], eax               ; dy = y - 100

    xor r12d, r12d                      ; x = 0..319
.tl_x:
    mov eax, r12d
    sub eax, 160
    mov [rsp + 0x00], eax               ; dx = x - 160

    imul eax, eax
    mov edx, [rsp + 0x04]
    imul edx, edx
    add eax, edx
    test eax, eax
    jnz .tl_nz
    mov eax, 1
.tl_nz:
    mov [rsp + 0x08], eax               ; dist2 = dx*dx + dy*dy

    ; Radius r = sqrt(dist2), Nebelfaktor = min(2*r, 255)
    fild dword [rsp + 0x08]
    fsqrt
    fld st0
    fadd st0, st0
    fistp dword [rsp + 0x0C]
    mov eax, [rsp + 0x0C]
    cmp eax, 255
    jbe .tl_fog_ok
    mov eax, 255
.tl_fog_ok:
    mov [r15 + rbx], al

    ; Tiefenkoordinate U = int(2560.0 / r) & 255
    fld dword [f_tun_scale]
    fxch st1
    fdivp st1, st0
    fistp dword [rsp + 0x0C]
    mov eax, [rsp + 0x0C]
    mov [r14 + rbx], al

    ; Winkelkoordinate V = int(atan2(dy, dx) * 128 / pi) & 255
    fild dword [rsp + 0x04]
    fild dword [rsp + 0x00]
    fpatan
    fmul dword [f_128_pi]
    fistp dword [rsp + 0x0C]
    mov eax, [rsp + 0x0C]
    mov [rbp + rbx], al

    inc ebx
    inc r12d
    cmp r12d, DEMO_W
    jb .tl_x

    inc r13d
    cmp r13d, DEMO_H
    jb .tl_y

    mov dword [tunnelReady], 1

    add rsp, 0x28
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

; ============================================================================
; Kompletten Demo-Frame in demoFb (320x200) rendern und nach memDC skalieren
; ============================================================================
draw_demo_frame:
    push rbx
    push rbp
    push rsi
    push rdi
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x78

    inc dword [demoTick]
    add dword [demoScrollX], 3

    ; Aktuelle Gesamtenergie aller 8 Kanaele aus den Live-VU-Pegeln summieren (0..240)
    xor ebx, ebx
    xor eax, eax
    lea rdi, [voices]
.df_vu_sum:
    add eax, [rdi + 48]
    add rdi, VOICE_SIZE
    inc ebx
    cmp ebx, NUM_CH
    jb .df_vu_sum
    mov [rsp + 0x68], eax               ; beatAmp sichern

    ; Fluggeschwindigkeit (Sterne / Tunnel) im Takt der Musik erhoehen
    mov ecx, eax
    shr ecx, 4
    add ecx, 2
    add [demoStarZ], ecx

    ; ------------------------------------------------------------------------
    ; 1. 256-Farben-Palette (0x00RRGGBB) mit Beat-Aufhellung erzeugen
    ; ------------------------------------------------------------------------
    mov r12d, [demoTick]
    mov r13d, [rsp + 0x68]
    shr r13d, 2                         ; beatFlash (0..60)
    lea r15, [sinTbl]
    lea r14, [demoPal]
    xor ebx, ebx
.df_pal_loop:
    mov eax, ebx
    add eax, r12d
    and eax, 255
    mov eax, [r15 + rax*4]
    add eax, 128
    shr eax, 1
    add eax, r13d
    cmp eax, 255
    jbe .pal_r_ok
    mov eax, 255
.pal_r_ok:
    shl eax, 16
    mov r8d, eax

    mov eax, ebx
    shl eax, 1
    add eax, 85
    sub eax, r12d
    and eax, 255
    mov eax, [r15 + rax*4]
    add eax, 128
    shr eax, 2
    mov edx, r13d
    shr edx, 1
    add eax, edx
    cmp eax, 255
    jbe .pal_g_ok
    mov eax, 255
.pal_g_ok:
    shl eax, 8
    or r8d, eax

    mov eax, ebx
    add eax, 170
    mov edx, r12d
    shl edx, 1
    add eax, edx
    and eax, 255
    mov eax, [r15 + rax*4]
    add eax, 128
    imul eax, 3
    shr eax, 2
    add eax, 32
    mov edx, r13d
    shr edx, 1
    add eax, edx
    cmp eax, 255
    jbe .pal_b_ok
    mov eax, 255
.pal_b_ok:
    or r8d, eax

    mov [r14 + rbx*4], r8d
    inc ebx
    cmp ebx, 256
    jb .df_pal_loop

    ; ------------------------------------------------------------------------
    ; 2. Aktiven Hintergrund-Effekt waehlen (0=Plasma, 1=3D-Tunnel, 2=Rasterbars)
    ; ------------------------------------------------------------------------
    mov eax, [demoFx]
    cmp eax, 1
    je .df_tunnel
    cmp eax, 2
    je .df_raster

    ; --- Effekt 0: Interferenz-Plasma (320x200) ---
    lea rdi, [demoFb]
    xor r13d, r13d
.df_pl_y:
    mov eax, r13d
    imul eax, 3
    mov edx, r12d
    shl edx, 1
    sub eax, edx
    and eax, 255
    mov ebp, [r15 + rax*4]

    xor ecx, ecx
.df_pl_x:
    mov eax, ecx
    shl eax, 1
    add eax, r12d
    and eax, 255
    mov edx, [r15 + rax*4]
    add edx, ebp

    mov eax, ecx
    add eax, r13d
    add eax, r12d
    and eax, 255
    add edx, [r15 + rax*4]

    mov eax, ecx
    sub eax, r13d
    shl eax, 1
    sub eax, r12d
    and eax, 255
    add edx, [r15 + rax*4]

    sar edx, 1
    and edx, 255
    mov eax, [r14 + rdx*4]
    mov [rdi], eax
    add rdi, 4

    inc ecx
    cmp ecx, DEMO_W
    jb .df_pl_x

    inc r13d
    cmp r13d, DEMO_H
    jb .df_pl_y
    jmp .df_stars

    ; --- Effekt 1: Perspektivischer 3D-Tunnel mit Tiefennebel ---
.df_tunnel:
    cmp dword [tunnelReady], 0
    jne .df_tun_go
    call init_tunnel_lut
.df_tun_go:
    mov r8d, [demoStarZ]
    shl r8d, 1                          ; Flug-Offset U

    mov eax, r12d
    and eax, 255
    mov r9d, [r15 + rax*4]
    sar r9d, 2
    add r9d, r12d                       ; Rotations-Offset V

    lea r10, [tunnelDist]
    lea r11, [tunnelAngle]
    lea rbp, [tunnelFog]
    lea rdi, [demoFb]
    xor ecx, ecx
.df_tun_loop:
    movzx eax, byte [r10 + rcx]
    add eax, r8d
    movzx edx, byte [r11 + rcx]
    add edx, r9d
    xor eax, edx
    add eax, r12d
    and eax, 255
    mov eax, [r14 + rax*4]

    ; Tiefennebel per SWAR-Skalierung anwenden (Zentrum dunkel, Raender hell)
    movzx esi, byte [rbp + rcx]
    mov edx, eax
    and eax, 0x00FF00FF
    imul eax, esi
    shr eax, 8
    and eax, 0x00FF00FF
    and edx, 0x0000FF00
    imul edx, esi
    shr edx, 8
    and edx, 0x0000FF00
    or eax, edx
    mov [rdi + rcx*4], eax

    inc ecx
    cmp ecx, DEMO_W * DEMO_H
    jb .df_tun_loop
    jmp .df_osc                         ; Tunnel ohne ueberlagertes Sternenfeld

    ; --- Effekt 2: 8 Copper-Rasterbars (reagieren pro Kanal auf den Live-VU) ---
.df_raster:
    ; Dunkler CRT-Scanline-Verlauf als Hintergrund
    lea rdi, [demoFb]
    xor r13d, r13d
.df_rbg_y:
    mov eax, r13d
    shr eax, 3
    add eax, 6
    shl eax, 16
    mov edx, r13d
    shr edx, 4
    shl edx, 8
    or eax, edx
    mov edx, r13d
    shr edx, 2
    add edx, 16
    or eax, edx
    test r13d, 1
    jz .df_rbg_fill
    and eax, 0x00FEFEFE
    shr eax, 1
.df_rbg_fill:
    mov ecx, DEMO_W
    rep stosd
    inc r13d
    cmp r13d, DEMO_H
    jb .df_rbg_y

    ; 8 gewebte Copper-Balken zeichnen
    xor ebx, ebx
.df_rb_loop:
    mov eax, ebx
    shl eax, 6
    lea rsi, [voices]
    mov r14d, [rsi + rax + 48]          ; Kanal-VU (0..30)
    cmp r14d, 30
    jbe .rb_vu_ok
    mov r14d, 30
.rb_vu_ok:

    imul eax, r12d, 3
    imul edx, ebx, 22
    add eax, edx
    and eax, 255
    mov eax, [r15 + rax*4]
    imul eax, 62
    sar eax, 7
    add eax, 96

    mov edx, r12d
    shl edx, 1
    imul ecx, ebx, 43
    add edx, ecx
    and edx, 255
    mov edx, [r15 + rdx*4]
    sar edx, 3
    add eax, edx

    cmp eax, 12
    jge .rb_ylo
    mov eax, 12
.rb_ylo:
    cmp eax, 187
    jle .rb_yhi
    mov eax, 187
.rb_yhi:
    sub eax, 8
    mov ebp, eax                        ; Startzeile topY (4..179)

    lea rsi, [rasterCols]
    mov r10d, [rsi + rbx*4]             ; Basisfarbe des Balkens

    xor r8d, r8d                        ; j = 0..16 (17 Scanlines pro Balken)
.df_rb_line:
    lea rsi, [barCurve]
    movzx r9d, byte [rsi + r8]
    lea rsi, [barSpec]
    movzx r11d, byte [rsi + r8]

    ; Auf dem Glanzkern zusaetzliches VU-Aufleuchten addieren
    cmp r8d, 5
    jb .rb_no_vu
    cmp r8d, 11
    ja .rb_no_vu
    lea eax, [r14 + r14]
    add r11d, eax
.rb_no_vu:

    ; Rot
    mov eax, r10d
    shr eax, 16
    and eax, 255
    imul eax, r9d
    shr eax, 8
    add eax, r11d
    cmp eax, 255
    jbe .rb_r_ok
    mov eax, 255
.rb_r_ok:
    shl eax, 16
    mov edx, eax

    ; Gruen
    mov eax, r10d
    shr eax, 8
    and eax, 255
    imul eax, r9d
    shr eax, 8
    add eax, r11d
    cmp eax, 255
    jbe .rb_g_ok
    mov eax, 255
.rb_g_ok:
    shl eax, 8
    or edx, eax

    ; Blau
    mov eax, r10d
    and eax, 255
    imul eax, r9d
    shr eax, 8
    add eax, r11d
    cmp eax, 255
    jbe .rb_b_ok
    mov eax, 255
.rb_b_ok:
    or edx, eax

    lea eax, [rbp + r8]
    imul eax, DEMO_W
    lea rdi, [demoFb]
    lea rdi, [rdi + rax*4]
    mov eax, edx
    mov ecx, DEMO_W
    rep stosd

    inc r8d
    cmp r8d, 17
    jb .df_rb_line

    inc ebx
    cmp ebx, NUM_CH
    jb .df_rb_loop

    ; ------------------------------------------------------------------------
    ; 3. 3D-Sternenfeld (64 Sterne, aktiv bei Plasma und Rasterbars)
    ; ------------------------------------------------------------------------
.df_stars:
    xor ebx, ebx
.df_star_loop:
    mov ecx, ebx
    shl ecx, 2
    sub ecx, [demoStarZ]
    and ecx, 255
    add ecx, 16

    mov eax, ebx
    imul eax, 73
    add eax, 19
    and eax, 255
    sub eax, 128
    imul eax, 160
    cdq
    idiv ecx
    add eax, 160
    mov r8d, eax
    cmp r8d, 1
    jl .df_star_next
    cmp r8d, DEMO_W - 2
    jg .df_star_next

    mov eax, ebx
    imul eax, 151
    add eax, 43
    and eax, 255
    sub eax, 128
    imul eax, 120
    cdq
    idiv ecx
    add eax, 100
    mov r9d, eax
    cmp r9d, 1
    jl .df_star_next
    cmp r9d, DEMO_H - 2
    jg .df_star_next

    imul eax, r9d, DEMO_W
    add eax, r8d
    lea rdi, [demoFb]
    lea rdi, [rdi + rax*4]

    cmp ecx, 96
    jb .df_star_big
    cmp ecx, 180
    jb .df_star_mid
    mov dword [rdi], 0x006098C8
    jmp .df_star_next
.df_star_mid:
    mov dword [rdi], 0x0088FFFF
    jmp .df_star_next
.df_star_big:
    mov dword [rdi], 0x00FFFFFF
    mov dword [rdi + 4], 0x00FFFFFF
    mov dword [rdi + DEMO_W*4], 0x00C0FFFF
    mov dword [rdi + DEMO_W*4 + 4], 0x00C0FFFF
.df_star_next:
    inc ebx
    cmp ebx, 64
    jb .df_star_loop

    ; ------------------------------------------------------------------------
    ; 4. Live-Stereo-Oszilloskop direkt aus dem aktuellen Audio-Puffer
    ; ------------------------------------------------------------------------
.df_osc:
    lea rsi, [audioBufs]
    lea rdi, [demoFb]
    xor ecx, ecx
    mov r10d, 96
.df_osc_loop:
    movsx eax, word [rsi + rcx*4]
    movsx edx, word [rsi + rcx*4 + 2]
    add eax, edx
    sar eax, 10
    add eax, 96
    cmp eax, 24
    jge .osc_lo_ok
    mov eax, 24
.osc_lo_ok:
    cmp eax, 168
    jle .osc_hi_ok
    mov eax, 168
.osc_hi_ok:
    test ecx, ecx
    jnz .osc_has_prev
    mov r10d, eax
.osc_has_prev:
    mov r8d, r10d
    mov r9d, eax
    cmp r8d, r9d
    jle .osc_ord_ok
    xchg r8d, r9d
.osc_ord_ok:
    mov r10d, eax

    mov edx, r8d
.osc_vline:
    imul eax, edx, DEMO_W
    add eax, ecx
    mov dword [rdi + rax*4 - DEMO_W*4], 0x0000B8E8
    mov dword [rdi + rax*4 + DEMO_W*4], 0x0000B8E8
    mov dword [rdi + rax*4], 0x00FFFFFF
    inc edx
    cmp edx, r9d
    jle .osc_vline

    inc ecx
    cmp ecx, DEMO_W
    jb .df_osc_loop

    ; ------------------------------------------------------------------------
    ; 5. 8-Kanal-Spektrum-Equalizer mit Boden-Spiegelung
    ; ------------------------------------------------------------------------
    xor ebx, ebx
.df_bar_loop:
    mov eax, ebx
    shl eax, 6
    lea rsi, [voices]
    mov r12d, [rsi + rax + 48]
    cmp r12d, 30
    jbe .bar_vu_ok
    mov r12d, 30
.bar_vu_ok:
    mov r13d, r12d
    shl r13d, 1
    shr r12d, 1
    add r13d, r12d
    test r13d, r13d
    jz .df_bar_next

    imul r14d, ebx, 36
    add r14d, 18

    xor r8d, r8d
.df_bar_row:
    lea eax, [r13 - 2]
    cmp r8d, eax
    jl .bar_grad
    mov r9d, 0x00FFFFFF
    jmp .bar_fill
.bar_grad:
    imul eax, r8d, 3
    add eax, 70
    cmp eax, 255
    jbe .bg_r
    mov eax, 255
.bg_r:
    shl eax, 16
    imul edx, r8d, 3
    cmp edx, 255
    jbe .bg_g
    mov edx, 255
.bg_g:
    shl edx, 8
    or eax, edx
    or eax, 0x000000E6
    mov r9d, eax
.bar_fill:
    mov eax, 186
    sub eax, r8d
    imul eax, DEMO_W
    add eax, r14d
    lea rdi, [demoFb]
    lea rdi, [rdi + rax*4]
    mov eax, r9d
    mov ecx, 28
    rep stosd

    inc r8d
    cmp r8d, r13d
    jb .df_bar_row

    mov r12d, r13d
    shr r12d, 3
    cmp r12d, 6
    jbe .bar_ref_ok
    mov r12d, 6
.bar_ref_ok:
    xor r8d, r8d
.df_bar_ref:
    cmp r8d, r12d
    jae .df_bar_next
    lea eax, [r8 + 188]
    imul eax, DEMO_W
    add eax, r14d
    lea rdi, [demoFb]
    lea rdi, [rdi + rax*4]
    mov eax, 0x00481868
    mov ecx, 28
    rep stosd
    inc r8d
    jmp .df_bar_ref

.df_bar_next:
    inc ebx
    cmp ebx, NUM_CH
    jb .df_bar_loop

    ; ------------------------------------------------------------------------
    ; 6. 320x200 Framebuffer scharf in aktuelles Fenster (curWinW x curWinH) skalieren
    ; ------------------------------------------------------------------------
    mov dword [drawOffX], 0
    mov dword [drawOffY], 0

    mov rcx, [memDC]
    mov edx, COLORONCOLOR
    call SetStretchBltMode

    mov rcx, [memDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, [curWinW]
    mov eax, [curWinH]
    mov [rsp + 0x20], rax
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], DEMO_W
    mov qword [rsp + 0x40], DEMO_H
    lea rax, [demoFb]
    mov [rsp + 0x48], rax
    lea rax, [demoBmi]
    mov [rsp + 0x50], rax
    mov qword [rsp + 0x58], 0
    mov qword [rsp + 0x60], SRCCOPY
    call StretchDIBits

    ; ------------------------------------------------------------------------
    ; 7. Live-Sinus-Scroller aus der Song-Message (titleBox) des geladenen Songs
    ; ------------------------------------------------------------------------
    lea rsi, [titleBox]
    lea rdi, [demoScrollTxt]
    mov ecx, 108
    rep movsb
    lea rsi, [demoSep]
    mov ecx, 20
    rep movsb

    mov eax, [demoScrollX]
    xor edx, edx
    mov ecx, CHAR_W
    div ecx
    mov r13d, eax
    mov r14d, edx

    xor ebx, ebx
.df_scr_loop:
    mov eax, r13d
    add eax, ebx
    and eax, 127
    lea rsi, [demoScrollTxt]
    add rsi, rax
    cmp byte [rsi], ' '
    je .df_scr_next

    imul r12d, ebx, CHAR_W
    sub r12d, r14d

    imul eax, ebx, 7
    mov edx, [demoTick]
    imul edx, 3
    add eax, edx
    and eax, 255
    mov eax, [r15 + rax*4]
    sar eax, 2
    mov edx, [curWinH]
    shr edx, 3
    add eax, edx
    mov ebp, eax

    lea ecx, [r12 + 2]
    lea edx, [rbp + 2]
    mov r8, rsi
    mov r10d, COL_BLACK
    call draw_char_px

    mov r10d, COL_CYAN
    cmp dword [rsp + 0x68], 90
    jb .df_scr_col
    mov r10d, COL_WHITE
.df_scr_col:
    mov ecx, r12d
    mov edx, ebp
    mov r8, rsi
    call draw_char_px

.df_scr_next:
    inc ebx
    mov eax, [gridCols]
    inc eax
    cmp ebx, eax
    jb .df_scr_loop

    ; ------------------------------------------------------------------------
    ; 8. Kopf- und Fusszeile mit aktivem Effektnamen und Live-Songdaten
    ; ------------------------------------------------------------------------
    xor ecx, ecx
    xor edx, edx
    mov r8d, [curWinW]
    mov r9d, CHAR_H
    mov r10d, COL_BLACK
    call fill_rect

    mov eax, [demoFx]
    imul eax, 6
    lea rsi, [demoFxNames]
    add rsi, rax
    lea rdi, [demoTopBar + 3]
    mov ecx, 6
    rep movsb

    lea rsi, [topBar + 39]
    lea rdi, [demoTopBar + 16]
    mov ecx, 16
    rep movsb

    mov al, [dispPlayOrder]
    lea r8, [demoTopBar + 37]
    call fmt_hex2
    mov al, [dispPlayPat]
    lea r8, [demoTopBar + 44]
    call fmt_hex2
    mov al, [dispPlayRow]
    lea r8, [demoTopBar + 51]
    call fmt_hex2
    mov eax, [curTempo]
    lea rdx, [hexTbl]
    mov al, [rdx + rax]
    mov [demoTopBar + 58], al

    mov ecx, [gridCols]
    sub ecx, 79
    jns .df_tx_ok
    xor ecx, ecx
.df_tx_ok:
    shr ecx, 1
    mov r12d, ecx
    xor edx, edx
    lea r8, [demoTopBar]
    mov r9d, 79
    mov r10d, COL_CYAN
    call draw_text

    mov r13d, [gridRows]
    dec r13d
    mov edx, r13d
    imul edx, CHAR_H
    mov r9d, [curWinH]
    sub r9d, edx
    xor ecx, ecx
    mov r8d, [curWinW]
    mov r10d, COL_BLACK
    call fill_rect

    mov ecx, r12d
    mov edx, r13d
    lea r8, [demoBotBar]
    mov r9d, 79
    mov r10d, COL_TEAL
    call draw_text

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