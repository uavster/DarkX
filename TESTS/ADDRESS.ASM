.386p
pila segment stack 'stack' use16
db 100h dup(?)
pila ends

code32 segment para use32
assume  cs:code32,ds:code32,ss:pila
dd      0
start:  jmp hello
db 'WATCOM'
hello:
        mov     eax,code
        mov     lejos+4,eax
        jmp     fword ptr cs:lejos
lejos   dd offset lejos1,0
code32 ends

code segment para use16
assume  cs:code,ds:code,ss:pila
lejos1:
cli
        xor     ax,ax
        mov     ax,word ptr hola
        int 3
sti
        mov ah,4ch
        int 21h

hola    dd 0CAFE1234h
code ends

end start

