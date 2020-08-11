NUM_DESCS       EQU 12          ; 12 descriptores de DarkX (3 de VCPI)
FREE_DESCS      EQU 10          ; allocatable descriptors for application

PSP_SEL         EQU 8 SHL 3
ENV_SEL         EQU 7 SHL 3
TSS_SEL         EQU 9 SHL 3
VCPI_CODE_SEL   EQU 10 SHL 3

; NOTA : ponemos los bits de acceso a 1 para que el procesador no tenga que
;        ponerlos a 1 cuando se accedan y tener c¢digo m s efectivo 
;        ( v lido para 486 y Pentium ) 

gdt             label

zero_descriptor         db      8 dup(0)       

code_descriptor         dw      0FFFFh          ; 4 Gb
                        dw      ?               ; b15-0 de base de segmento
                        db      ?               ; b23-16 de base de segmento
                        db      10011011b       ; b7 present bit = 1
                                                ; b6-5 priv. level = 0
                                                ; b4 sistema = 1 (code/data)
                                                ; b3 ejecutable = 1 (s¡)
                                                ; b2 conforming (???) = 0
                                                ; b1 bit de lectura = 1 (s¡)
                                                ; b0 bit de acceso = 1 
                        db      11001111b       ; b7 granularidad = 1 (p gs.)    
                                                ; b6 op. size = 1 (32 bits)
                                                ; b5 ???
                                                ; b4 not used
                                                ; b3-0 = b19-16 de l¡mite
                        db      0               ; b31-24 de la base

data_descriptor         dw 0FFFFh               ; 4 Gb
                        dw      0               ; b15-0 de base de segmento
                        db      0               ; b23-16 de base de segmento
                        db      10010011b       ; b7 present bit = 1
                                                ; b6-5 priv. level = 0
                                                ; b4 sistema = 1 (code/data)
                                                ; b3 ejecutable = 0 (no)
                                                ; b2 expandible hacia arriba
                                                ; b1 W = 1 (read/write)
                                                ; b0 bit de acceso = 1
                        db      11001111b       ; b7 granularidad = 1 (p gs.)    
                                                ; b6 op. size = 1 (32 bits)
                                                ; b5 ???
                                                ; b4 not used
                                                ; b3-0 = b19-16 de l¡mite
                        db      0               ; b31-24 de la base

c16_desc                dw      0FFFFh
                        dw      ?
                        db      ?
                        db      9Ah OR 1 
                        db      0       ; segmento de 16 bits de 64Kb
                        db      0

d16_desc                dw      0FFFFh
                        dw      ?
                        db      ?
                        db      92h OR 1 
                        db      0,0

; descriptores para el c¢digo y los datos del programa
code_desc               dw      0FFFFh
                        dw      0
                        db      0
                        db      9Bh,0CFh
                        db      0

data_desc               dw      0FFFFh
                        dw      0
                        db      0
                        db      93h,0CFh
                        db      0

; descriptor para el segmento de variables de entorno
env_desc                dw      0FFFFh  ; l¡mite 64Kb
                        dw      0
                        db      0
                        db      92h OR 1 
                        db      0,0

; a partir de aqu¡ no se pueden mover de sitio los descriptores

; descriptor para el segmento del PSP
psp_desc                dw      0FFh    ; l¡mite 100h bytes
                        dw      0
                        db      0
                        db      93h
                        db      40h,0

; descriptor del Task State Segment (TSS)
tss_desc                dw      0FFFFh
                        dw      0
                        db      0
                        db      89h     ; Present, not Busy
                        db      0
                        db      0

; reservamos 3 descriptores para el VCPI, en caso de que lo haya
vcpi_descs              db      3*8 dup(0)

; espacio para descriptores libres
free_descs_start        db      FREE_DESCS*8 dup(0)
