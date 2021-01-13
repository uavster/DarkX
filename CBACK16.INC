;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Real Mode Call-Back routines
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

; call-back address table
callback_address_table:
        push    ax
        mov     ax,0
        jmp     callback_manager
end_of_cb_address:    ; we need it to calculate the length
CBSLOT=size TCallBack
REPT    (NUM_RM_CALLBACKS-1)
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
		push	bp
		mov		bp,sp
		mov		ax,ss:[bp+2*4]	; BP, AX, IP, CS, ->FLAGS<-
		pop		bp
		mov		cs:flags_real,ax
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

push eax ebx
mov al,'|'
call debug16_char
mov al,'J'
call debug16_char
movzx eax,cs:flags_real
call debug16_reg
mov al,':'
call debug16_char
xor eax,eax
mov ax,word ptr cs:pila_real+2
shl eax,4
movzx ebx,word ptr cs:pila_real
add eax,ebx
call debug16_reg
pop ebx eax

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

;mov ax,0b800h
;mov ds,ax
;mov	byte ptr ds:[6*160],'Z'
;mov byte ptr ds:[6*160+1],3eh
;mov ax,cs:new_flags
;and ah,0fh
;add ah,'0'
;mov	byte ptr ds:[4*160+2],ah
;mov	byte ptr ds:[4*160+3],3eh

        ; recuperamos el estado de la CPU
        pop     gs fs es ds
        popad
        popfd

        pop     ax

push	ax
mov al,'|'
call debug16_char
mov al,'K'
call debug16_char
movzx eax,cs:new_flags
call debug16_reg
mov al,':'
call debug16_char
xor eax,eax
mov ax,ss
shl eax,4
movzx esp,sp
add eax,esp
add eax,2
call debug16_reg
pop ax

        ; restauramos SS,SP,CS,IP y FLAGS de la estructura de modo real        mov
        lss     sp,cs:new_stack


push	ax
mov al,':'
call debug16_char
xor eax,eax
mov ax,ss
shl eax,4
movzx esp,sp
add eax,esp
add eax,2
call debug16_reg
pop ax

        push    cs:new_flags
		push	dword ptr cs:new_code
		iret

current_stackframe      dd ?
new_code                dd ?
new_stack               dd ?
new_flags               dw ?
callback_code           dw ?
pila_real               dd ?
flags_real				dw ?
