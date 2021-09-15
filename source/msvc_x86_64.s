.code

__switch_to_msvc_x64 PROC EXPORT FRAME
	.endprolog

    mov [rcx+20*8], rcx
    mov rax, [rsp]
    mov [rcx+21*8], rax
    mov [rcx+22*8], rbx
    mov [rcx+23*8], rbp
    mov [rcx+24*8], rdi
    mov [rcx+25*8], rsi
    mov [rcx+26*8], rsp
    mov [rcx+27*8], r12
    mov [rcx+28*8], r13
    mov [rcx+29*8], r14
    mov [rcx+30*8], r15

    mov r10, gs:[48]
    mov rax, [r10+8]
    mov [rcx+34*8], rax
    mov rax, [r10+16]
    mov [rcx+35*8], rax
    mov rax, [r10+24]
    mov [rcx+36*8], rax
    mov rax, [r10+5240]
    mov [rcx+37*8], rax

    test r9, r9
    je load_regs_msvc_x86_64

    movaps [rcx+0*8], xmm6
    movaps [rcx+2*8], xmm7
    movaps [rcx+4*8], xmm8
    movaps [rcx+6*8], xmm9
    movaps [rcx+8*8], xmm10
    movaps [rcx+10*8], xmm11
    movaps [rcx+12*8], xmm12
    movaps [rcx+14*8], xmm13
    movaps [rcx+16*8], xmm14
    movaps [rcx+18*8], xmm15

    stmxcsr [rcx+32*8]
    fnstcw  [rcx+33*8]

    fldcw  [rdx+33*8]
    ldmxcsr [rdx+32*8]
    
    movaps xmm15, [rdx+18*8]
    movaps xmm14, [rdx+16*8]
    movaps xmm13, [rdx+14*8]
    movaps xmm12, [rdx+12*8]
    movaps xmm11, [rdx+10*8]
    movaps xmm10, [rdx+8*8]
    movaps xmm9, [rdx+6*8]
    movaps xmm8, [rdx+4*8]
    movaps xmm7, [rdx+2*8]
    movaps xmm6, [rdx+0*8]

    load_regs_msvc_x86_64:

    mov rax, [rdx+37*8]
    mov [r10+5240], rax
    mov rax, [rdx+36*8]
    mov [r10+24], rax
    mov rax, [rdx+35*8]
    mov [r10+16], rax
    mov rax, [rdx+34*8]
    mov [r10+8], rax

    mov r15, [rdx+30*8]
    mov r14, [rdx+29*8]
    mov r13, [rdx+28*8]
    mov r12, [rdx+27*8]
    mov rsp, [rdx+26*8]
    mov rsi, [rdx+25*8]
    mov rdi, [rdx+24*8]
    mov rbp, [rdx+23*8]
    mov rbx, [rdx+22*8]
    mov rax, [rdx+21*8]
    mov [rsp], rax
    mov rcx, [rdx+20*8]
    
    ret

__switch_to_msvc_x64 ENDP


__get_mxcsr_msvc_x64 PROC EXPORT FRAME
	.endprolog
    stmxcsr [rcx]
    ret
__get_mxcsr_msvc_x64 ENDP


__get_fcw_msvc_x64 PROC EXPORT FRAME
	.endprolog
    fnstcw [rcx]
    ret
__get_fcw_msvc_x64 ENDP



__get_TEB_8_msvc_x64 PROC EXPORT FRAME
    .endprolog
    mov r10, gs:[48]
    mov rax, [r10+8]
    mov [rcx], rax
    ret
__get_TEB_8_msvc_x64 ENDP

__get_TEB_16_msvc_x64 PROC EXPORT FRAME
    .endprolog
    mov r10, gs:[48]
    mov rax, [r10+16]
    mov [rcx], rax
    ret
__get_TEB_16_msvc_x64 ENDP

__get_TEB_24_msvc_x64 PROC EXPORT FRAME
    .endprolog
    mov r10, gs:[48]
    mov rax, [r10+24]
    mov [rcx], rax
    ret
__get_TEB_24_msvc_x64 ENDP

__get_TEB_5240_msvc_x64 PROC EXPORT FRAME
    .endprolog
    mov r10, gs:[48]
    mov rax, [r10+5240]
    mov [rcx], rax
    ret
__get_TEB_5240_msvc_x64 ENDP


END

