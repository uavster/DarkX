.386p
.model flat
.stack 100h
.code
        INCLUDE utils.inc
main:
        InitDPMI
start:
        mov ah,1
        int 16h
        jz start
        xor ah,ah
        int 16h
        Exit
end main

