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
; Zwischen Fenster- und Borderless-Fullscreen-Modus umschalten
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
    lea rdx, [savedRect]
    call GetWindowRect

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

    mov eax, r12d
    imul eax, WIN_H
    mov edx, r13d
    imul edx, WIN_W
    cmp eax, edx
    jb .fs_fit_w

    mov [dispH], r13d
    mov eax, r13d
    imul eax, WIN_W
    xor edx, edx
    mov ecx, WIN_H
    div ecx
    mov [dispW], eax
    mov edx, r12d
    sub edx, eax
    shr edx, 1
    mov [dispX], edx
    mov dword [dispY], 0
    jmp .fs_flag_on

.fs_fit_w:
    mov [dispW], r12d
    mov eax, r12d
    imul eax, WIN_H
    xor edx, edx
    mov ecx, WIN_W
    div ecx
    mov [dispH], eax
    mov edx, r13d
    sub edx, eax
    shr edx, 1
    mov [dispY], edx
    mov dword [dispX], 0

.fs_flag_on:
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

    mov dword [dispX], 0
    mov dword [dispY], 0
    mov dword [dispW], WIN_W
    mov dword [dispH], WIN_H
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

    cmp dword [isFullscreen], 0
    je .tf_done
    mov rcx, [hDC]
    xor edx, edx
    xor r8d, r8d
    mov r9d, [scrW]
    mov eax, [scrH]
    mov [rsp + 0x20], rax
    mov qword [rsp + 0x28], BLACKNESS
    call PatBlt

.tf_done:
    mov dword [dirty], 1
    add rsp, 0x40
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; ============================================================================
; Fensterprozedur (x64 ABI konform, inkl. Maus und Alt+Enter Fullscreen)
; ============================================================================
WndProc:
    cmp edx, WM_KEYDOWN
    je .on_key
    cmp edx, WM_LBUTTONDOWN
    je .on_lbtn
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

.on_lbtn:
    sub rsp, 0x28
    mov ecx, r9d
    xor edx, edx
    call handle_mouse_down
    mov dword [dirty], 1
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

    lea rdi, [dxTbl]
    mov eax, CHAR_W
    mov ecx, 80
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
    mov eax, [winRect + 12]
    sub eax, [winRect + 4]
    mov r13d, eax

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

    mov rcx, rax
    call GetDC
    mov [hDC], rax

    mov rcx, rax
    mov edx, COLORONCOLOR
    call SetStretchBltMode

    mov rcx, [hDC]
    call CreateCompatibleDC
    mov [memDC], rax

    mov rcx, [hDC]
    mov edx, WIN_W
    mov r8d, WIN_H
    call CreateCompatibleBitmap

    mov rcx, [memDC]
    mov rdx, rax
    call SelectObject

    mov ecx, -11
    mov edx, 9
    xor r8d, r8d
    xor r9d, r9d
    mov qword [rsp + 0x20], 700
    mov qword [rsp + 0x28], 0
    mov qword [rsp + 0x30], 0
    mov qword [rsp + 0x38], 0
    mov qword [rsp + 0x40], 0
    mov qword [rsp + 0x48], 0
    mov qword [rsp + 0x50], 0
    mov qword [rsp + 0x58], 3
    mov qword [rsp + 0x60], 49
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