.386p
;pila segment stack 'stack' use16
;db 100h dup(?)
;pila ends
.model flat
.stack 100h
.code
;code segment para use16
;assume  cs:code,ds:code,ss:pila
start:
cli
        mov     eax,11111111h   ; EFLAGS
        push    eax
        mov     eax,1234h       ; CS
        push    eax
        mov     eax,87654321h   ; EIP
        push    eax

        push word ptr hola
        pop word ptr ss:[esp+4*2]
        int 3
sti
        mov ah,4ch
        int 21h
hola    dw 0CAFEh
;code ends

end start
