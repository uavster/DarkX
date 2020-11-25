.386p
.model flat
.stack 100h
.code
start:
        mov     ax,0EE02h
        int     31h
        mov     program_base,ebx

        mov     cx,00100000b ;archive attribute
        mov     ah,3Ch
        mov     edx,offset file_name1
        int     21h
        mov     edx,offset create_error
        jc      exit_error
        movzx   eax,ax
        mov     edx,offset create_msg
        push    eax
        call    show_result
        pop     ebx
        push    ebx

        mov     cx,00100000b ;archive attribute
        mov     ah,3Dh
        mov     edx,offset file_name
        int     21h
        mov     edx,offset open_error
        jc      exit_error
        movzx   eax,ax
        mov     edx,offset open_msg
        push    eax
        call    show_result
        pop     ebx

        mov     ax,4202h
        xor     ecx,ecx
        xor     edx,edx
        int     21h
        shl     edx,16
        mov     dx,ax
        push    edx

        mov     ax,4200h
        xor     ecx,ecx
        xor     edx,edx
        int     21h

        pop     edx
        push    edx ebx
        mov     ax,501h
        mov     cx,dx
        shr     edx,16
        mov     bx,dx
        int     31h
        jc      no_mem_error
        mov     dx,bx
        shl     edx,16
        mov     dx,cx
        pop     ebx ecx

        sub     edx,program_base
        mov     ah,3Fh
        push    edx ecx
        int     21h
        mov     edx,offset read_error_msg
        jc      exit_error
        mov     edx,offset read_msg
        call    show_result

        pop     ecx edx
        pop     ebx
        mov     ah,40h
        int     21h
        push    edx
        mov     edx,offset write_error_msg
        jc      exit_error
        mov     edx,offset write_msg
        call    show_result

        pop     edx
        mov     di,dx
        shr     edx,16
        mov     si,dx
        mov     ax,502h
        int     31h
;        mov     ah,9
;        int     21h

       mov     edx,offset hello_msg
       mov     ah,9
       int     21h

        wait3:
        in      al,60h                
        dec     al
        jnz     wait3

        acaba:
        mov     ah,4Ch
        int     21h

        exit_error:
        mov     ah,9
        int     21h
        jmp     acaba

        no_mem_error:
        mov     edx,offset no_mem_error_msg
        jmp     exit_error

show_result     proc
        push    edx
        mov     edi,offset number_buff
        call    bin_to_dec
        pop     edx
        mov     ah,9
        int     21h
        mov     edx,edi
        int     21h
        ret                
show_result     endp

bin_to_dec      proc
; INPUT  -> EAX = number to convert ; DS:EDI -> buffer where to copy string
; OUTPUT -> DS:EDI -> string number in decimal (skipping initial zeros)
        mov     ebx,10
        mov     ebp,10
        xor     cl,cl
        pass_to_decimal:
                xor     edx,edx
                div     ebx
                or      edx,edx
                jnz     make_ascii
                or      eax,eax
                jz      decide_space_from_now
                make_ascii:
                add     dl,30h
                put_the_char:
                mov     [ebp+edi-1],dl
                dec     ebp
        jnz     pass_to_decimal
        ret

        decide_space_from_now:
        cmp     ebp,10
        jnz     no_first_zero
                mov     byte ptr [edi+9],'0'
                dec     ebp
        no_first_zero:
        push    ebp
        put_spaces:
                mov     byte ptr [ebp+edi-1],' '
        dec     ebp
        jnz     put_spaces
        pop     ebp
        add     edi,ebp
        ret

bin_to_dec      endp

program_base    dd 0

hello_msg       db 13,10,10,"Say hello to protected mode and press ESC...$"

handle_msg      db 13,10,"Created file with handle: $"
create_error    db 13,10,"Error with function CREATE FILE$"
create_msg      db 13,10,"File created with handle: $"
file_name       db "interrup.j",0
file_name1      db "hola.jar",0
read_error_msg  db 13,10,"Error with function READ FILE$"
read_msg        db 13,10,"Bytes read from file: $"
open_error      db 13,10,"Error with function OPEN FILE$"
open_msg        db 13,10,"File opened with handle: $"
write_msg       db 13,10,"Bytes written to file: $"
write_error_msg db 13,10,"Error with function WRITE FILE$"
no_mem_error_msg db 13,10,"Not enough memory to read the whole file$"

number_buff     db 10 dup(?),"$"

end     start
