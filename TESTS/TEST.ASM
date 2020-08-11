.386p
.model flat
.stack  100h
.code
start:
        xor edi,edi
        push edi
        push edi
        mov ax,1234h
        push ax
        inc ax
        push ax
        inc ax
        push ax
        inc ax
        push ax
        pushf
        pushad
        mov bl,16h
        mov ax,300h
        mov edi,esp
        push ss
        pop es
        xor cx,cx
        int 31h
        popad
        popf
        pop bp
        int 3

        mov esi,offset info_struc
        mov ecx,24+2
        mov ax,3201h
        int 31h
        mov eax,[esi]
        mov ebx,[esi+4]
        mov ecx,[esi+8]
        mov edx,[esi+12]
        mov di,[esi+16]
        mov bp,[esi+18]
        int 3
        mov     bx,0FF00h
        xor     ecx,ecx
        mov     si,10h
        xor     edi,edi
        mov     ax,800h
        int     31h
        jc      error
        call    print_result
        mov     bx,0FF00h
        xor     ecx,ecx
        mov     si,1000h
        xor     edi,edi
        mov     ax,800h
        int     31h
        jc      error
        call    print_result
        mov     bx,0FF00h
        xor     ecx,ecx
        mov     si,0h
        mov     di,1
        mov     ax,800h
        int     31h
        jc      error
        call    print_result
        mov     bx,0FF00h
        xor     ecx,ecx
        mov     si,30h
        xor     edi,edi
        mov     ax,800h
        int     31h
        jc      error
        call    print_result
        fin:
        mov     ah,4Ch
        int     21h

error:
        mov     edx,offset error_msg
        mov     ah,9
        int     21h
        jmp     fin

reg_to_hex      proc
; INPUT -> EAX = number to convert ; DS:EDI -> destino 
;          CL = 8/16/32 bit register (2=EAX ; 1=AX ; 0=AL)

	xor     bl,bl
	or      cl,cl
	jz      es_al
	cmp     cl,1
	jz      es_ax
	cmp     cl,2
	jz      es_eax        
	es_al:                
		mov     cl,1
		xor     ebp,ebp
		jmp     convert
	es_ax:
		mov     cl,2
		mov     ebp,2
		jmp     convert
	es_eax:
		mov     cl,4
		mov     ebp,6
	convert:
		push    ecx
		mov     cl,bl
		push    eax
		shr     eax,cl
		call    bin_to_hex
		mov     [edi+ebp],ax
		pop     eax
		pop     ecx
		sub     ebp,2
		add     bl,8
	dec     cl
	jnz     convert
	ret
reg_to_hex      endp

bin_to_hex      proc
; INPUT  -> AL = byte to convert
; OUTPUT -> AX = hex value
	
	mov     ah,al
	and     ah,0fh
	cmp     ah,9 
	jbe     cifra0
		add     ah,'A'-'9'-1
	cifra0:
	add     ah,'0'
	shr     al,4
	cmp     al,9
	jbe     cifra1
		add     al,'A'-'9'-1
	cifra1:
	add     al,'0'
	ret

print_result proc
        push    bx
        push    cx
        pop     eax
        mov     edi,offset string
        mov     cl,2
        call    reg_to_hex
        mov     edx,offset string
        mov     ah,9
        int     21h
        ret
print_result endp

bin_to_hex      endp

error_msg       db "Error mapping memory$"
string          db "00000000h",13,10,"$"
info_struc      db 24 dup(?)
end start
