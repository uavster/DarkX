;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; file: nicedbg.inc
;
;  Just some nice debugging macros. This file must be included inside
;  TXDATA segment!!!
;
;       by Xavier Rubio Jansana, a.k.a. Teknik / #Trouble# Team, July 1998
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

imjere      db      "Imjerenou", 13, 10, "$"    ; DEBUG

DEBUG   macro

        pushad
        pushfd
        push    ds
        mov     ds, [cs:D16_SEL]
        mov     edx, offset imjere
        mov     ah, 9
        int     21h
        xor     ah, ah
        int     16h
        pop     ds
        popfd
        popad

        endm

too_lazy db     "0123456789abcdef"
valof   db      "Value of "
regname db      "   : $"
crlf    db      13, 10, "$"

PRINTREG    macro reg
        local   dh_lp

        pushad
        pushfd
        push    ds
        push    reg
        mov     ds, cs:D16_SEL
        mov     eax, "&reg"             ;; imprimeix el nom del registre
        xchg    ah, al
        mov     word ptr [regname + 1], ax
        shr     eax, 16
        mov     [regname], al
        mov     edx, offset valof
        mov     ah, 9
        int     21h
        pop     eax

        mov     cl, 8                   ;; eight nibbles...
dh_lp:  rol     eax, 4
        mov     ebx, eax
        and     ebx, 0fh
        push    eax
        mov     ah, 2
        mov     dl, [ebx + too_lazy]
        int     21h
        pop     eax
        dec     cl
        jnz     dh_lp

        mov     edx, offset crlf        ;; imprimeix la cucamonada
        mov     ah, 9
        int     21h

        pop     ds
        popfd
        popad

        endm

PAUSE   macro

        pushad
        pushfd
        xor     ah, ah
        int     16h
        popfd
        popad

        endm

