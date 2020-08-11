;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Call-Back routines
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

; number of needed call-back slots
CALLBACKS       EQU 32

; call-back address table
callback_address_table:
        push    ax
        mov     ax,0
        jmp     callback_manager
end_of_cb_address:    ; we need it to calculate the length
CBSLOT=size TCallBack
REPT    (CALLBACKS-1)
        push    ax
        mov     ax,CBSLOT
        jmp     word ptr callback_manager
        CBSLOT=CBSLOT+size TCallBack
ENDM

Trm_reg         STRUC
        rm_edi          dd ?
        rm_esi          dd ?
        rm_ebp          dd ?
                        dd ?
        rm_ebx          dd ?
        rm_edx          dd ?
        rm_ecx          dd ?
        rm_eax          dd ?
        rm_flags        dw ?
        rm_es           dw ?
        rm_ds           dw ?
        rm_fs           dw ?
        rm_gs           dw ?
        rm_ip           dw ?
        rm_cs           dw ?
        rm_sp           dw ?
        rm_ss           dw ?
ENDS

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Call-Back Management procedure with IMS technology (Infinite Mode Switch)
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
callback_manager:
        ; guardamos el puntero de pila actual en modo real
        mov     word ptr cs:pila_real,sp        
        mov     word ptr cs:pila_real+2,ss
        ; guardamos el n£mero de call-back para el c¢digo en protegido
        mov     cs:callback_code,ax
        ; guardamos el estado de la CPU
        pushfd
        pushad
        push    ds es fs gs

        ; protegemos el stack switching
        cli

        ; ahora que podemos cambiar los flags, cambiamos 'pila_real'
        add     word ptr cs:pila_real,2         ; POP AX

        ; guardamos las direcciones de cambio de modos
        ASSUME  DS:txdata
        mov     ax,txdata
        mov     ds,ax
        push    dword ptr prot_code_address
        push    word ptr prot_code_address+4
        push    dword ptr cs:saltito+1

        ; ponemos las nuevas
        mov     dword ptr prot_code_address,offset callback_pm

        ; guardamos el stack frame anterior en la pila actual
        push    dword ptr cs:current_stackframe
        ; stack frame actual en la variable de c¢digo
        mov     word ptr cs:current_stackframe,sp
        mov     word ptr cs:current_stackframe+2,ss

        jmp     set_pm_address

        cback16_return:
        cli
        ; recuperamos el stack frame
        lss     sp,dword ptr cs:current_stackframe
        ; ponemos el del posible call back anterior
        pop     dword ptr cs:current_stackframe

        ; recuperamos las direcciones de cambio de modo
        mov     ax,txdata
        mov     ds,ax
        pop     dword ptr cs:saltito+1
        pop     word ptr prot_code_address+4
        pop     dword ptr prot_code_address

        ; recuperamos el estado de la CPU
        pop     gs fs es ds
        popad
        popfd

        pop     ax

        ; restauramos SS,SP,CS,IP y FLAGS de la estructura de modo real        mov
        push    cs:new_flags
        popf
        lss     sp,cs:new_stack
        jmp     dword ptr cs:new_code

current_stackframe      dd ?
new_code                dd ?
new_stack               dd ?
new_flags               dw ?
callback_code           dw ?
pila_real               dd ?
