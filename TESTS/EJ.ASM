.model tiny
.386
.code
        org 100h
start:
        jmp short main
        db 10h dup(?)
        db 'VDISK V'
        db 19 dup(?),0,0,15h
void:
        nop
        nop
        nop
        nop
        nop
        nop
        iret

main:        
;        xor ax,ax
;        mov es,ax
;        xor di,di
;        mov cx,8
;        cld
;        mov ax,cs
;        shl eax,16
;        mov ax,offset void
;        rep stosd

;        mov ax,0de0ah
;        int 67h
;        mov ax,0b800h
;        mov ds,ax
;        add bl,30h
;        mov ds:[160*4],bl
;        mov ch,cl
;        shr cl,4
;        add cl,30h
;        mov ds:[160*4+4],cl
;        and ch,0fh
;        add ch,30h
;        mov ds:[160*4+6],ch
;
;        mov ah,4ch
;        int 21h

        mov ax,0FFFFh
        mov es,ax
        mov dword ptr es:[16+3],'SIDV'
        mov byte ptr es:[16+3+4],'K'
        mov word ptr es:[16+1Eh],100h

        xor ax,ax
        mov ds,ax
        mov ax,cs
        add ax,100h
        mov ds:[19h*4+2],ax

        mov ax,cs
        mov es,ax    
        mov ah,4Ah
        mov bx,offset final
        shr bx,4
        inc bx
        int 21h
        mov sp,offset final

        mov dx,offset tx32name
        mov word ptr cs:com_tail,offset cfg_tail
        mov word ptr cs:com_tail+2,cs
        mov ax,cs
        ;mov di,offset com_tail          ;Vamos a acabar de llenar el
        ;mov word ptr cs:[di+2],ax       ;buffer de par metros
        mov word ptr cs:FCB1+2,ax
        mov word ptr cs:FCB2+2,ax
        mov ds,ax
        mov es,ax
        mov bx,offset Exec_Params
        mov ax,4B00h
        int 21h
        jc exec_err
        jmp     acaba
        exec_err:
        mov ax,cs
        mov ds,ax
        mov dx,offset exec_err_msg
        mov ah,9
        int 21h
        acaba:
        mov ah,4ch
        int 21h

tx32name        db 'TX32.EXE',0
exec_err_msg    db "Unable to execute TX32!$"
Exec_Params:
Environment     dw 0 ;copy env. from caller
com_tail        dw ?,?
FCB1            dw 5Ch,?
FCB2            dw 6Ch,?
                dd ?
                dd ?
cfg_tail        db 1,' ',13,0

                db 256 dup(?)
final:
end start

