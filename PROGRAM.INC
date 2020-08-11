       mov     edx,txcode32
       shl     edx,4
       add     edx,offset hello_msg
       mov     ah,9
       int     21h

        wait3:
        in      al,60h                
        dec     al
        jnz     wait3

        acaba:
        mov     ah,4Ch
        int     21h

hello_msg       db 13,10,10,"Say hello to protected mode and press a key...$"
