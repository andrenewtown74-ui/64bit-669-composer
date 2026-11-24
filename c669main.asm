; 64-Bit Windows Composer-669-Tracker (Modularer Aufbau, NASM ohne CRT)
; 64 Sample-Slots (00..3F) + WAV/.669 I/O + Sample-/Synth-Editor + 4K-Demo + Fullscreen

default rel
bits 64

%include "c669_defs.inc"

section .text
global start

%include "c669_audio.inc"
%include "c669_io.inc"
%include "c669_ui.inc"
%include "c669_demo.inc"
%include "c669_samped.inc"

; ============================================================================
; Dynamisches Raster- und Widescreen-/Classic-Layout neu berechnen
; ============================================================================
recalc_layout:
    mov eax, [curWinW]
    cmp eax, WIN_W
    jae .rl_w_ok
    mov eax, WIN_W
.rl_w_ok:
    xor edx, edx
    mov ecx, CHAR_W
    div ecx
    cmp eax, 80
    jae .rl_c_lo
    mov eax, 80
.rl_c_lo:
    cmp eax, 256
    jbe .rl_c_hi
    mov eax, 256
.rl_c_hi:
    mov [gridCols], eax

    mov eax, [curWinH]
    cmp eax, WIN_H
    jae .rl_h_ok
    mov eax, WIN_H
.rl_h_ok:
    xor edx, edx
    mov ecx, CHAR_H
    div ecx
    cmp eax, 50
    jae .rl_r_lo
    mov eax, 50
.rl_r_lo:
    cmp eax, 200
    jbe .rl_r_hi
    mov eax, 200
.rl_r_hi:
    mov [gridRows], eax

    ; Sample-Editor-Zentrierung (960x600 mittig im Fenster)
    mov eax, [curWinW]
    sub eax, WIN_W
    jns .rl_sex
    xor eax, eax
.rl_sex:
    shr eax, 1
    mov [seOffX], eax

    mov eax, [curWinH]
    sub eax, WIN_H
    jns .rl_sey
    xor eax, eax
.rl_sey:
    shr eax, 1
    mov [seOffY], eax

    ; Vertikaler Rest-Offset fuer Tracker-Raster (0..11 px mittig)
    mov eax, [gridRows]
    imul eax, CHAR_H
    mov edx, [curWinH]
    sub edx, eax
    jns .rl_uiy
    xor edx, edx
.rl_uiy:
    shr edx, 1
    mov [uiOffY], edx

    ; Aktions- und Fusszeile immer an den unteren beiden Zeilen
    mov eax, [gridRows]
    lea edx, [rax - 2]
    mov [clrBarRow], edx
    lea edx, [rax - 1]
    mov [botBarRow], edx

    ; Widescreen-Modus ab WIDE_COLS (124 Spalten), sonst klassisch vertikal dynamisch
    cmp dword [gridCols], WIDE_COLS
    jb .rl_classic

    ; --- WIDESCREEN-LAYOUT (Links: Kompakt-Header + bis zu 64 Pattern-Zeilen, Rechts: Samples + Orders) ---
    mov dword [wideMode], 1
    mov dword [layoutCols], WIDE_COLS
    mov eax, [curWinW]
    sub eax, WIDE_COLS * CHAR_W
    jns .rl_w_ox
    xor eax, eax
.rl_w_ox:
    shr eax, 1
    mov [uiOffX], eax

    mov dword [sampListX], 82
    mov dword [sampListY], 2
    mov eax, [gridRows]
    sub eax, 4
    cmp eax, MAX_SAMPS
    jbe .rl_w_sv
    mov eax, MAX_SAMPS
.rl_w_sv:
    mov [sampVisRows], eax

    mov dword [ordListX], 117
    mov dword [ordListY], 2
    mov eax, [gridRows]
    sub eax, 4
    cmp eax, MAX_ORDERS
    jbe .rl_w_ov
    mov eax, MAX_ORDERS
.rl_w_ov:
    mov [ordVisRows], eax

    mov eax, 9
    cmp dword [gridRows], 77
    jb .rl_w_ph
    mov eax, 10
.rl_w_ph:
    mov [patHdrRow], eax
    inc eax
    mov [patStartRow], eax
    mov edx, [clrBarRow]
    sub edx, eax
    cmp edx, PAT_ROWS
    jbe .rl_w_pv
    mov edx, PAT_ROWS
.rl_w_pv:
    mov [patViewRows], edx
    jmp .rl_clamp_scroll

.rl_classic:
    ; --- KLASSISCHES LAYOUT (80 Spalten, dynamisch mehr Zeilen fuer Samples/Orders + Pattern) ---
    mov dword [wideMode], 0
    mov dword [layoutCols], 80
    mov eax, [curWinW]
    sub eax, 80 * CHAR_W
    jns .rl_c_ox
    xor eax, eax
.rl_c_ox:
    shr eax, 1
    mov [uiOffX], eax

    mov eax, [gridRows]
    sub eax, 50
    imul eax, 3
    shr eax, 2
    add eax, 32
    cmp eax, PAT_ROWS
    jbe .rl_c_pv
    mov eax, PAT_ROWS
.rl_c_pv:
    mov [patViewRows], eax

    mov edx, [gridRows]
    sub edx, 6
    sub edx, eax                        ; upperRows (>= 12)
    mov dword [sampListX], 1
    mov dword [sampListY], 2
    mov ecx, edx
    cmp ecx, MAX_SAMPS
    jbe .rl_c_sv
    mov ecx, MAX_SAMPS
.rl_c_sv:
    mov [sampVisRows], ecx

    mov dword [ordListX], 72
    mov dword [ordListY], 2
    mov ecx, edx
    cmp ecx, MAX_ORDERS
    jbe .rl_c_ov
    mov ecx, MAX_ORDERS
.rl_c_ov:
    mov [ordVisRows], ecx

    lea eax, [rdx + 3]                  ; 2 + upperRows + 1
    mov [patHdrRow], eax
    inc eax
    mov [patStartRow], eax

.rl_clamp_scroll:
    mov eax, MAX_SAMPS
    sub eax, [sampVisRows]
    cmp [sampViewTop], eax
    jbe .rl_cs_o
    mov [sampViewTop], eax
.rl_cs_o:
    mov eax, MAX_ORDERS
    sub eax, [ordVisRows]
    cmp [ordViewTop], eax
    jbe .rl_cs_p
    mov [ordViewTop], eax
.rl_cs_p:
    mov eax, PAT_ROWS
    sub eax, [patViewRows]
    cmp [patViewTop], eax
    jbe .rl_done
    mov [patViewTop], eax
.rl_done:
    ; Dynamische Sample-Editor-Geometrie fuer Widescreen vs. Classic berechnen
    cmp dword [wideMode], 0
    je .rl_se_classic

    mov dword [seWfW], 1456             ; 122*12 - 8 px = 728 Spalten a 2 px
    mov eax, [gridRows]
    lea edx, [rax - 3]
    mov [seRecRow], edx

    mov dword [seSlStep], 1
    mov ecx, 21                         ; Panelhoehe bei Schrittweite 1
    cmp eax, 66
    jb .rl_se_calc
    mov dword [seSlStep], 2
    mov ecx, 30                         ; Panelhoehe bei Schrittweite 2 (extra uebersichtlich)
.rl_se_calc:
    ; panelRow = seRecRow - 1 - panelHeight
    mov edx, [seRecRow]
    dec edx
    sub edx, ecx
    mov [sePanelRow], edx
    lea r8d, [rdx - 2]
    mov [seFxRow], r8d
    lea r8d, [rdx - 4]
    mov [seActRow], r8d
    lea r8d, [rdx - 6]
    mov [seInfoRow], r8d
    lea r8d, [rdx - 8]                  ; seWfRows = seInfoRow - 2
    mov [seWfRows], r8d
    ; seBtnRow = sePanelRow + 4 + 9 * seSlStep + 1
    mov eax, [seSlStep]
    imul eax, 9
    add eax, [sePanelRow]
    add eax, 5
    mov [seBtnRow], eax
    jmp .rl_se_end

.rl_se_classic:
    mov dword [seWfW], 928
    mov dword [seWfRows], 18
    mov dword [seInfoRow], 20
    mov dword [seActRow], 22
    mov dword [seFxRow], 24
    mov dword [sePanelRow], 26
    mov dword [seSlStep], 1
    mov dword [seBtnRow], 40
    mov dword [seRecRow], 47

.rl_se_end:
    mov dword [dispX], 0
    mov dword [dispY], 0
    mov eax, [curWinW]
    mov [dispW], eax
    mov eax, [curWinH]
    mov [dispH], eax
    ret

; ============================================================================
; Backbuffer-Bitmap und Layout an neue Client-Groesse (ECX=W, EDX=H) anpassen
; ============================================================================
resize_backbuffer:
    push rbx
    push r12
    push r13
    sub rsp, 0x20

    cmp ecx, 64
    jb .rb_done
    cmp edx, 64
    jb .rb_done

    mov [curWinW], ecx
    mov [curWinH], edx
    call recalc_layout

    cmp qword [memDC], 0
    je .rb_done
    cmp qword [hDC], 0
    je .rb_done

    mov eax, [curWinW]
    cmp eax, [bufW]
    jne .rb_alloc
    mov eax, [curWinH]
    cmp eax, [bufH]
    je .rb_done

.rb_alloc:
    mov rcx, [hDC]
    mov edx, [curWinW]
    mov r8d, [curWinH]
    call CreateCompatibleBitmap
    test rax, rax
    jz .rb_done
    mov r12, rax

    mov rcx, [memDC]
    mov rdx, r12
    call SelectObject
    mov r13, rax

    cmp qword [memBmp], 0
    je .rb_store
    mov rcx, r13
    call DeleteObject

.rb_store:
    mov [memBmp], r12
    mov eax, [curWinW]
    mov [bufW], eax
    mov eax, [curWinH]
    mov [bufH], eax
    mov dword [dirty], 1

.rb_done:
    add rsp, 0x20
    pop r13
    pop r12
    pop rbx
    ret

; ============================================================================
; Zwischen Fenster- und nativem Borderless-Fullscreen-Modus umschalten
; ============================================================================
toggle_fullscreen:
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 0x40

    cmp dword [isFullscreen], 0
    jne .to_windowed

    mov rcx, [hWnd]
    mov edx, GWL_STYLE
    call GetWindowLongA
    xor ecx, ecx
    test eax, WS_MAXIMIZE
    jz .fs_not_max
    mov ecx, 1
.fs_not_max:
    mov [wasMaximized], ecx
    test ecx, ecx
    jnz .fs_skip_rect
    mov rcx, [hWnd]
    lea rdx, [savedRect]
    call GetWindowRect
.fs_skip_rect:

    xor ecx, ecx
    call GetSystemMetrics
    mov [scrW], eax
    mov r12d, eax

    mov ecx, 1
    call GetSystemMetrics
    mov [scrH], eax
    mov r13d, eax

    mov rcx, [hWnd]
    mov edx, GWL_STYLE
    mov r8d, WS_POPUP_VIS
    call SetWindowLongA

    mov rcx, [hWnd]
    xor edx, edx
    xor r8d, r8d
    xor r9d, r9d
    mov [rsp + 0x20], r12
    mov [rsp + 0x28], r13
    mov qword [rsp + 0x30], SWP_FRAMECHANGED
    call SetWindowPos

    mov dword [isFullscreen], 1
    jmp .rebind_dc

.to_windowed:
    mov rcx, [hWnd]
    mov edx, GWL_STYLE
    mov r8d, WS_TRACKER
    call SetWindowLongA

    mov r8d, [savedRect + 0]
    mov r9d, [savedRect + 4]
    mov eax, [savedRect + 8]
    sub eax, r8d
    mov edx, [savedRect + 12]
    sub edx, r9d

    mov rcx, [hWnd]
    push rdx
    xor edx, edx
    pop r10
    mov [rsp + 0x20], rax
    mov [rsp + 0x28], r10
    mov qword [rsp + 0x30], SWP_FRAMECHANGED
    call SetWindowPos

    cmp dword [wasMaximized], 0
    je .fs_clear_flag
    mov rcx, [hWnd]
    mov edx, SW_MAXIMIZE
    call ShowWindow

.fs_clear_flag:
    mov dword [isFullscreen], 0

.rebind_dc:
    mov rcx, [hWnd]
    mov rdx, [hDC]
    call ReleaseDC

    mov rcx, [hWnd]
    call GetDC
    mov [hDC], rax

    mov rcx, rax
    mov edx, COLORONCOLOR
    call SetStretchBltMode

    mov rcx, [hWnd]
    lea rdx, [tmpRect]
    call GetClientRect
    mov ecx, [tmpRect + 8]
    mov edx, [tmpRect + 12]
    call resize_backbuffer

    mov dword [dirty], 1
    add rsp, 0x40
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; ============================================================================
; Fensterprozedur (x64 ABI konform, inkl. Resize, Maus-Drag & Alt+Enter Fullscreen)
; ============================================================================
WndProc:
    cmp edx, WM_SIZE
    je .on_size
    cmp edx, WM_ERASEBKGND
    je .on_erase
    cmp edx, WM_GETMINMAXINFO
    je .on_minmax
    cmp edx, WM_KEYDOWN
    je .on_key
    cmp edx, WM_MOUSEMOVE
    je .on_mousemove
    cmp edx, WM_LBUTTONDOWN
    je .on_lbtn
    cmp edx, WM_LBUTTONUP
    je .on_lbtnup
    cmp edx, WM_RBUTTONDOWN
    je .on_rbtn
    cmp edx, WM_MOUSEWHEEL
    je .on_wheel
    cmp edx, WM_SYSKEYDOWN
    je .on_syskey
    cmp edx, WM_SYSCHAR
    je .on_syschar
    cmp edx, WM_CLOSE
    je .on_close
    cmp edx, WM_DESTROY
    je .on_close
    sub rsp, 0x28
    call DefWindowProcA
    add rsp, 0x28
    ret

.on_erase:
    mov eax, 1
    ret

.on_minmax:
    cmp dword [isFullscreen], 0
    jne .fwd_def
    cmp dword [minWinW], 0
    je .fwd_def
    mov eax, [minWinW]
    mov [r9 + 24], eax
    mov eax, [minWinH]
    mov [r9 + 28], eax
    xor eax, eax
    ret

.on_size:
    cmp r8d, SIZE_MINIMIZED
    je .eat_msg
    sub rsp, 0x28
    movzx ecx, r9w
    mov edx, r9d
    shr edx, 16
    call resize_backbuffer
    mov dword [dirty], 1
    add rsp, 0x28
    xor eax, eax
    ret

.on_close:
    mov dword [running], 0
    xor eax, eax
    ret

.on_syskey:
    cmp r8d, VK_RETURN
    jne .fwd_def
    test r9d, 0x20000000
    jz .fwd_def
    test r9d, 0x40000000
    jnz .eat_msg
    sub rsp, 0x28
    call toggle_fullscreen
    add rsp, 0x28
    xor eax, eax
    ret

.on_syschar:
    cmp r8d, VK_RETURN
    je .eat_msg
.fwd_def:
    sub rsp, 0x28
    call DefWindowProcA
    add rsp, 0x28
    ret

.eat_msg:
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

.on_mousemove:
    cmp dword [sampEdMode], 0
    je .eat_msg
    cmp dword [seDragMode], 0
    je .eat_msg
    sub rsp, 0x28
    mov ecx, r9d
    call samped_mouse_move
    add rsp, 0x28
    xor eax, eax
    ret

.on_lbtn:
    sub rsp, 0x28
    mov ecx, r9d
    xor edx, edx
    call handle_mouse_down
    mov dword [dirty], 1
    add rsp, 0x28
    xor eax, eax
    ret

.on_lbtnup:
    cmp dword [seDragMode], 0
    je .eat_msg
    sub rsp, 0x28
    call samped_mouse_up
    add rsp, 0x28
    xor eax, eax
    ret

.on_rbtn:
    sub rsp, 0x28
    mov ecx, r9d
    mov edx, 1
    call handle_mouse_down
    mov dword [dirty], 1
    add rsp, 0x28
    xor eax, eax
    ret

.on_wheel:
    sub rsp, 0x28
    mov ecx, r8d
    mov edx, r9d
    call handle_mouse_wheel
    mov dword [dirty], 1
    add rsp, 0x28
    xor eax, eax
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
    mov dword [dispW], WIN_W
    mov dword [dispH], WIN_H
    mov dword [curWinW], WIN_W
    mov dword [curWinH], WIN_H
    call recalc_layout

    lea rdi, [dxTbl]
    mov eax, CHAR_W
    mov ecx, 256
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

    call init_samp_meta
    call init_samped_defaults
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
    mov [minWinW], eax
    mov eax, [winRect + 12]
    sub eax, [winRect + 4]
    mov r13d, eax
    mov [minWinH], eax

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

    ; Ausgangsgroesse fuer Wiederherstellung nach Maximieren/Vollbild merken
    mov rcx, [hWnd]
    lea rdx, [savedRect]
    call GetWindowRect

    mov rcx, [hWnd]
    call GetDC
    mov [hDC], rax

    mov rcx, rax
    mov edx, COLORONCOLOR
    call SetStretchBltMode

    mov rcx, [hDC]
    call CreateCompatibleDC
    mov [memDC], rax

    ; Direkt maximiert starten (2x ShowWindow, damit STARTUPINFO SW_HIDE beim Start aus Hintergrund-Shells nicht greift)
    mov rcx, [hWnd]
    mov edx, SW_MAXIMIZE
    call ShowWindow
    mov rcx, [hWnd]
    mov edx, SW_MAXIMIZE
    call ShowWindow

    mov rcx, [hWnd]
    lea rdx, [tmpRect]
    call GetClientRect
    mov ecx, [tmpRect + 8]
    mov edx, [tmpRect + 12]
    call resize_backbuffer

; DOS-Rasterfont "Terminal" (8x12 Pixel, OEM-Zeichensatz, nicht fett)
    mov ecx, 12                         ; Zellenhoehe passend zu CHAR_H = 12
    mov edx, 8                          ; Native Zeichenbreite (0 = automatisch)
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 400         ; FW_NORMAL statt 700 (kein GDI-Fett-Schmieren)
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    mov qword [rsp + 0x40], 255         ; OEM_CHARSET fuer echten Terminal-Font
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
    call poll_recording

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
    call stop_recording
    mov rcx, [hWaveOut]
    call waveOutReset
    mov rcx, [hWaveOut]
    call waveOutClose
    xor ecx, ecx
    call ExitProcess