.386p
NOWARN PRO
JUMPS

DEBUG_ON	EQU 0

; number of needed call-back slots
NUM_RM_CALLBACKS	EQU 32
; New IRQ base interrupt
NEW_IRQ0_IRQ7_BASE	EQU 50h

; stack sizes in DWORDs
PM_STACKSIZE    EQU 1000h       ; 16 Kb
RM_STACKSIZE    EQU 1000h       ; 16 Kb

DPMI_MAX_REENTRIES EQU 8		; RM_STACKSIZE*4/DPMI_MAX_REENTRIES must be multiple of 4

; TASM 3.1 no ensambla bien los PUSH's de offsets de 32 bits
pushd MACRO address_32
	db 68h
	dw offset address_32,0
ENDM

Tstack_info		struc
	stack_esp	dd ?
	stack_ss	dw ?
ends

Tpushad_regs	struc
	_edi		dd ?
	_esi        dd ?
	_ebp        dd ?
	_esp		dd ?
	_ebx        dd ?
	_edx        dd ?
	_ecx        dd ?
	_eax        dd ?
ends

Tdpmi_rm_regs   STRUC
	rm_edi      dd ?
	rm_esi      dd ?
	rm_ebp      dd ?
				dd ?
	rm_ebx      dd ?
	rm_edx      dd ?
	rm_ecx      dd ?
	rm_eax      dd ?
	rm_flags    dw ?
	rm_es       dw ?
	rm_ds       dw ?
	rm_fs       dw ?
	rm_gs       dw ?
	rm_ip       dw ?
	rm_cs       dw ?
	rm_sp       dw ?
	rm_ss       dw ?
ends



;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; SEGMENTO DE DATOS DE PROGRAMA
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

TXDATA  SEGMENT USE16 'DATA'

        copyright_msg           db "DarkX - the DarkRising DOS Extender [ v0.98.1 ] (c) 1998",13,10
                                db "Extender by B52 , Memory Manager and LE loader by Teknik",13,10
                                db "< http://thedarkrising.home.ml.org >",13,10,"$"

        cpu_type_msg            db " ş CPU Family:  $"
        cpu_strings             dw offset cpu386,offset cpu486,offset pentium_cpu
        cpu386                  db "386$"
        cpu486                  db "486$"
        pentium_cpu             db "Pentium+$"
        fpu_strings             dw offset nofpu,0,offset fpu287,offset fpu387,offset fpu487
        nofpu                   db " no FPU$"
        fpu287                  db " +287$"
        fpu387                  db " +387$"
        fpu487                  db " +487$"
        sys_type_msg            db " ş System Type: $"
        sys_types               dw offset raw_msg,offset vcpi_msg,offset dpmi_msg,offset xms_msg
        raw_msg                 db "RAW$"
        vcpi_msg                db "VCPI$"
        dpmi_msg                db "DPMI$"
        xms_msg                 db "XMS$"
        dos_mem_msg             db " ş DOS memory:  $"
        extended_mem_msg        db " Kb",13,10," ş DPMI memory: $"
        kb_msg                  db " Kb",13,10,10,"$"

        dos_version_error_msg   db "Error: DOS v3.0 or better needed$"
        A20_error_msg           db "Error: Could not enable A20 line$"
        v86_error_msg           db "Error: System is in V86 mode and no VCPI/DPMI manager found$"
        no386_error_msg         db "Error: No 386 or better processor found$"
		cr_lf					db 13,10,"$"

        ; loader error messages
        exe_size_error_msg      db "Error: Not enough memory to load program$"
        load_error_msg          db "Error: Unable to load EXE file from disk (error in disk operations)$"
        unsuported_feature_msg  db "Error: Linear executable contains a feature not suported by the loader$"
        desc_alloc_error_msg    db "Error: Could not allocate descriptors$"
        bad_exec_error_msg      db "Error: Program is NOT a Linear or DOS32 Executable$"
        comp_exe_error_msg      db "Error: DLINK executable is compressed$"

		vcpi_emm_name 			db 'EMMXXXX0',0

        dos_mem_size            dd ?

        ; EXE info (OEM function 3201h)
        exe_info_struc          LABEL
        ;**********************************
        program_offset          dd 0    ; 0 if Linear Executable
        psp_offset              dd ?
        env_offset              dd ?
        program_name_ptr        dd ?
        zero_sel                dw 2 SHL 3      ; =D32_SEL without DPMI
        prog_sel                dw 6 SHL 3      ; =DATA_SEL without DPMI
        psp_selector            dw PSP_SEL      ; sin DPMI ser  ‚ste
        env_selector            dw ENV_SEL      ; para vars. de entorno
        disk_seg                dw ?
        ;**********************************
        exe_size                dd 0    ; 0 with LE's (not implemented)

        ; system info
        cpu_info                dw ?
        system_type             db 0    ; 0=RAW
        fpu_info                db 4    ; 487 if fpuid is not executed (Pentium)

        extended_heap_size      dd ?

        xms_entry_point         dd 0    ; si es 0, es que no hay XMS

	vcpi_entry_point        dd ?
							dw VCPI_CODE_SEL
	vcpi_page_dir			dw 0
	vcpi_ems_test_page		dw 0

        dir_base                dd 0
        page_dir_base           dd 0
        page_dir_entries        dd 0

	vcpi_init_err_msg       db "Error initializing VCPI!$"

	dpmi_32b_error          db "DPMI does not support 32bit programs!$"
	dpmi_init_error         db "Error initializing DPMI!$"

	ALIGN 4		; Align to 4-byte boundary so this registers can be loaded with popad
    dpmi_rm_call_struc      Tdpmi_rm_regs<>

	dpmi_private_buffer     dw ?

	dpmi_save_restore_real  dd ?
	dpmi_save_restore_pm    dw ?,?,?

	dpmi_real_to_pm         dd ?
	dpmi_pm_to_real         dw ?,?,?

	dpmi_rm_call            db 32 dup(0)

	dpmi_sels               dd SELECTORS dup(0)

	dpmi_exc_handlers       db 6*15 dup(0)
	dpmi_old_int21h         dw 0,0,0
	dpmi_old_int31h         dw 0,0,0

	dpmi_exception_flag     db 0

	not_enough_mem          db "Error: Not enough conventional memory$"
	cant_free_mem			db "Error: Unable to free DOS memory$"	

	psp_seg                 dw ?

	; registros en modo real antes del salto a protegido
    rm_regs_struc           LABEL
	real_edi                dd 0
	real_esi                dd 0
	real_ebp                dd 0
                            dd 0    ; reserved
	real_ebx                dd 0
	real_edx                dd 0
	real_ecx                dd 0
	real_eax                dd 0
    real_flags              dw 0
    real_es                 dw 0
    real_ds                 dw 0
    real_fs                 dw 0
    real_gs                 dw 0
    real_ip                 dw 0
	real_cs					dw 0
    real_sp                 dw 0
	real_ss					dw 0

	; Every switch to real mode in the DPMI interface does:
	; * Store PM stack in the index of dpmi_stack_info in dpmi_stack_info_offset
	; * dpmi_stack_info_offset += size Tstack_info
	; Swithing back to protected mode does dpmi_stack_info_offset -= size Tstack_info
	dpmi_stack_info_offset	dd 0
	dpmi_stack_info			Tstack_info DPMI_MAX_REENTRIES dup(<>)

	set_pm_address          dw offset set_pm_raw

        prot_code_address       dd ?
				dw ?            ; selector de c¢digo de 32 bits

	ret_real_address        dw offset ret_real_raw,0

	gdt_reg                 label fword
        gdt_limit               dw (NUM_DESCS + FREE_DESCS) * 8 - 1
	gdt_base                dd ?

	idt_reg                 label fword
	idt_limit               dw 256 * 8 -1   ; 256 descriptores (1 por gate)
	idt_base                dd ?

	idt_real                label fword
				dw 0FFFFh
				dd 0

	; "system registers" para pasarle al VCPI
	vcpi_sys_regs           LABEL
	vcpi_cr3                dd 0                    ; valor del cr3
	vcpi_gdtr               dd offset gdt_reg       ; linear address de la GDT (1er Mb)
	vcpi_idtr               dd offset idt_reg       ;   "       "    de la IDT (1er Mb)
	vcpi_ldtr               dw 0                    ; valor del LDTR
	vcpi_tr                 dw TSS_SEL              ;   "    "  TR
	vcpi_cs_eip             dd 0                    ; entry point (CS:EIP) para el
				dw ?                    ; modo protegido

        INCLUDE gdt.inc
        free_descs_handles      db FREE_DESCS dup(0)

	error_string_table      dw exc_0
				dw exc_1
				REPT    4
				dw exc_default
				ENDM
				dw exc_6
				dw exc_7
				REPT    5
				dw exc_default
				ENDM
				dw exc_13
				dw exc_14
				dw exc_default

	exc_0                   db "DIVIDE OVERFLOW",0
	exc_1                   db "DEBUG EXCEPTION",0
	exc_6                   db "INVALID OPCODE",0
	exc_7                   db "FPU NOT AVAILABLE",0
	exc_13                  db "GENERAL PROTECTION FAILURE",0
	exc_14                  db "PAGE FAULT",0
	exc_default             db "EXCEPTION",0

	invalid_sel_msg         db "INVALID SELECTOR VALUE "

	debug_info_msg          db "ERROR: "
	error_string            db "                          ",13,10
				db "32-bit protected mode exception number: "
	exception_number        db "00h",13,10
				db "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ"
				db "Crash address  CS:EIP = "
	debug_cs                db "0000:"
	debug_eip               db "00000000h  ->  "
	crash_code              db "00 00 00 00 00 00 00 00",13,10,10
				db "Stack contents SS:ESP = "
	debug_ss0               db "0000:"
	debug_esp0              db "00000000h  ->  "
	stack_cont1             db "00000000 "
	stack_cont2             db "00000000 "
	stack_cont3             db "00000000",13,10
				db "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ"
				db "EAX = "
	debug_eax               db "00000000h ; "
				db "EBX = "
	debug_ebx               db "00000000h ; "
				db "ECX = "
	debug_ecx               db "00000000h ; "
				db "EDX = "
	debug_edx               db "00000000h",13,10
				db "ESI = "
	debug_esi               db "00000000h ; "
				db "EDI = "
	debug_edi               db "00000000h ; "
				db "EBP = "
	debug_ebp               db "00000000h ; "
				db "ESP = "
	debug_esp               db "00000000h",13,10
				db "EFLAGS = "
	debug_flags             db "00000000h",13,10
				db "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ"
				db "Selectors:          Descriptors:",13,10
				db "CS = "
	debug_cs1               db "0000h    ->    "
	desc_cs                 db "00 00 00 00 00 00 00 00",13,10
				db "DS = "
	debug_ds                db "0000h    ->    "
	desc_ds                 db "00 00 00 00 00 00 00 00",13,10
				db "ES = "
	debug_es                db "0000h    ->    "
	desc_es                 db "00 00 00 00 00 00 00 00",13,10
				db "FS = "
	debug_fs                db "0000h    ->    "
	desc_fs                 db "00 00 00 00 00 00 00 00",13,10
				db "GS = "
	debug_gs                db "0000h    ->    "
	desc_gs                 db "00 00 00 00 00 00 00 00",13,10
				db "SS = "
	debug_ss                db "0000h    ->    "
	desc_ss                 db "00 00 00 00 00 00 00 00",13,10
				db "ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ"
				db "$"

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

IF DEBUG_ON
        include nicedbg.inc             ; DEBUGGING macros
ENDIF
        include letypes.inc

; ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
; loadle "data segment"
; ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ

handle              dd      ?           ; File handle
le_offset           dd      ?           ; Offset to LE header from the start
                                        ;  of the file
le_hdr              LEHEADER <>         ; LE header
object_tbl          OBJECT 10 dup (?)   ; Object table
object_nfo          OBJNFO 10 dup (?)   ; Objects info
obj_handles_ptr     dd      ?           ; Ptr to current object handle
num_obj_handles     dd      ?           ; Number of memory blocks allocated

obj_count           dd      ?           ; Object counter

curr_page           dd      ?           ; Now loading this page
curr_fixup_pag      dd      ?           ; File ptr to current fixup page table
                                        ;  entry
off_1st_fixup_rec   dd      ?           ; Offset from fixup record table of
                                        ;  1st entry for this page
off_1st_fixup_rec_next_page dd  ?       ; Idem for next page
curr_page_off       dd      ?           ; Offset of currently fixed up page

source_type         db      ?           ; Fixup's source type
target_flags        db      ?           ; Fixup's target flags
source_off          dw      ?           ; Fixup's source offset
object_num          dw      ?           ; Fixup's object number
target_off          dd      ?           ; Fixup's target offset

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

TFword  STRUC
        loffset         dd ?
        selector        dw ?
ENDS

FREE_CB         EQU 0
ALLOCATED_CB    EQU 1
; call-back data table (size must be 16 bytes)
TCallback       STRUC
        callback_state  db ?
        rm_entry_point  dw ?
        pm_proc_to_call TFword ?
        rm_call_struc   TFword ?
ENDS

RM_ENTRY=offset callback_address_table
LABEL   callback_data
REPT    NUM_RM_CALLBACKS
        TCallback <FREE_CB,RM_ENTRY,?,?>
		RM_ENTRY_SIZE=(offset end_of_cb_address-offset callback_address_table)
RM_ENTRY=RM_ENTRY+RM_ENTRY_SIZE
ENDM

debug_row	db 0
debug_col	db 0
debug_tmp	db 16 dup(?)
TXDATA  ENDS

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; SEGMENTO DE CODIGO PARA RUTINAS DE 16 BITS ( ENTRADA EN PM, SALIDA, ... )
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

TXCODE  SEGMENT PARA USE16 'CODE'
ASSUME  CS:TXCODE,DS:txdata,SS:txpila

INCLUDE         raw_xms.inc
INCLUDE         vcpi.inc
INCLUDE         dpmi.inc
INCLUDE         ints16.inc
INCLUDE         cback16.inc
INCLUDE 		debug16.inc

; ////////////////////////////
; PUNTO DE ENTRADA DE DarkX.EXE
; ////////////////////////////
init_pm:
;        inc     dx
;        xor     ax,2E32h
;        dec     dx
;        xor     ax,2e32h

push ebx
mov bh,15
xor bl,bl
call debug16_move_cursor
pop ebx

	; presentamos el Copyright y el t¡tulo
        push    ds
        mov     dx,offset copyright_msg
	call    printf
        pop     ds

        ; comprobamos que el DOS sea 3.0+
        mov     ah,30h
        int     21h
        cmp     al,3
        mov     dx,offset dos_version_error_msg
        jb      error_handle1

        ; buscamos nuestro nombre
        mov     es,ds:[2Ch]
        xor     di,di
        mov     cx,-1
        xor     ax,ax
        cont_searching:
        repnz   scasb
        scasb
        jnz     cont_searching
        add     di,2
        xor     ecx,ecx
        mov     cx,es
        movzx   edi,di
        shl     ecx,4
        add     ecx,edi

        ; rellenamos info varia
        xor     ebx,ebx
	mov     bx,ds
	mov     ax,txdata
	mov     ds,ax
	mov     psp_seg,bx
        shl     ebx,4
        mov     psp_offset,ebx
        xor     ebx,ebx
        mov     bx,es
        shl     ebx,4
        mov     env_offset,ebx
        mov     program_name_ptr,ecx
        mov     ax,disk
        mov     disk_seg,ax
        ; fix-up del descriptor del ENVIRONMENT (necesario bajo todos sistemas)
        mov     fs,psp_seg
        movzx   eax,word ptr fs:[2Ch]
        shl     eax,4
        mov     word ptr env_desc+2,ax
        shr     eax,16
        mov     byte ptr env_desc+4,al
        mov     byte ptr env_desc+7,ah

        call    cpuid
        mov     dx,offset no386_error_msg
        jc      error_handle1
        mov     cpu_info,ax
        cmp     ah,5
        jbe     have_cpu_name
                mov     ah,5
        have_cpu_name:
        push    ax
        xor     bx,bx
        mov     bl,ah
        shl     bx,1
        mov     dx,[bx+cpu_strings-3*2]
        push    dx
        mov     dx,offset cpu_type_msg
        call    printf
        pop     dx
        call    printf
        pop     ax
        cmp     ah,5
        jae     no_fpu_det_needed

        call    fpuid
        mov     fpu_info,al
        xor     bx,bx
        mov     bl,al
        shl     bx,1
        mov     dx,[bx+fpu_strings]
        call    printf

        no_fpu_det_needed:
	mov		dx,offset cr_lf
	call	printf
        ; cambiamos el tama¤o del bloque de programa
	call    init_mem

	; comprobamos si hay DPMI
	mov     ax,1687h
	int     2Fh
	or      ax,ax
	jnz     no_dpmi

		or      system_type,2
		call    save_dpmi_data
                jmp     mem_alloc_done

	no_dpmi:
	; ponemos un par de selectores
	mov     ax,txcode32
	mov     ds,ax
	ASSUME  DS:txcode32
	mov     ax,C32_SEL
	ASSUME  DS:txdata
	mov     bx,txdata
	mov     ds,bx
	mov     word ptr vcpi_cs_eip+4,ax
	mov     word ptr prot_code_address+4,ax

	; Check if there's VCPI.
	; Fragments taken from 
	; http://www.edm2.com/index.php/Virtual_Control_Program_Interface_specification_v1#A.3_Sample_VCPI_Detection_Code
	lea 	dx,vcpi_emm_name			; open EMM device, read-only
	mov 	ax,3D00h
	int 	21h
	jc 		short vcpi_not_present		; branch if error
	mov 	bx,ax            			; get device information
	mov 	ax,4400h
	int 	21h
	jc 		short vcpi_not_present     	; branch if error
	test 	dx,80h          			; branch if file (not device)
	jz 		short vcpi_not_present
	mov 	ax,4407h         			; check output status
	int 	21h
	push 	ax              			; save return code from IOCTL call
	mov 	ah,3Eh           			; close file
	int 	21h
	pop 	ax               			; restore IOCTL return code
	cmp 	al,0FFh          			; branch if status not device ready
	jne 	short vcpi_not_present
	 
	; primero miramos si hay vector o no...
	xor     ax,ax
	mov     es,ax
	cmp     dword ptr es:[67h*4],0
	jz      vcpi_not_present

	; Now allocate an EMS page to make sure the EMS emulator is turned on.
    mov 	bx,1           				; allocate 1 EMS page
    mov 	ah,43h
    int 	67h
    cmp 	ah,0						; branch if error
    jne 	short vcpi_not_present
	; Save handle returned in DX to be freed later
	mov 	vcpi_ems_test_page,dx

	; VCPI detection call
	mov     ax,0DE00h
	mov		di,12h	; This is necessary for DOSBox-X to detect VCPI
	xor		cx,cx
	int     67h
	or      ah,ah
	jnz     vcpi_not_present

		ASSUME  DS:txdata
		inc     system_type
                save_vcpi_data
                jmp     avoid_v86_test
		ASSUME  DS:txdata
	vcpi_not_present:

        ; si no hay VCPI tampoco, comprobamos si estamos en V86
        cmp     system_type,0
        jnz     avoid_v86_test
        smsw    ax
        test    al,1
        ; si lo estamos salimos con error
        mov     dx,offset v86_error_msg
        jnz     error_handle1

        ; si llegamos aki, estamos en RAW / XMS : lo averiguamos
        avoid_v86_test:
        mov     ax,4300h
        int     2Fh
        cmp     al,80h
        jnz     no_xms
                mov     ax,4310h
                int     2Fh
                mov     word ptr xms_entry_point,bx
                mov     word ptr xms_entry_point+2,es
                mov     ah,5
                call    dword ptr xms_entry_point
                or      ax,ax
                jnz     a20_ok
                cmp     ax,94h          ; already enabled
                mov     dx,offset A20_error_msg
                jnz     error_handle1
                a20_ok:
                alloc_xms_mem
                jmp     mem_alloc_done
        no_xms:
        ; activamos la A20 a mano
        call    enable_A20
        ; pillamos extendida con int15h
        mov     ax,txcode32
        mov     es,ax
        mov     byte ptr es:extended_pointers,0
        mov     ah,88h
        int     15h
        movzx   eax,ax
        or      eax,eax
        jz      mem_alloc_done          ; ¨tenemos extendida?
        shl     eax,10  ; *1024 pq lo queremos en bytes
        ASSUME  ES:txcode32
        inc     byte ptr es:extended_pointers
        mov     dword ptr es:extended_pointers+5,eax
        look_for_extended_beginning
        mov     dword ptr es:extended_pointers+1,eax
        sub     dword ptr es:extended_pointers+5,eax    ; si empieza despu‚s,
        add     dword ptr es:extended_pointers+5,100000h; es m s peque¤a
        cmp     dword ptr es:extended_pointers+5,0      ; ¨tenemos extendida? 
        jnz     mem_alloc_done
                dec     byte ptr es:extended_pointers   ; si no, 0 bloques

        mem_alloc_done:
        ; inicializamos los datos comunes
	call    init_data

	; si tenemos DPMI, entramos en protegido aqu¡ (s¢lo inicializaci¢n)
	mov     ax,txdata
	mov     ds,ax
	mov     al,system_type
	test    al,2
	jz      no_dpmi_init
;                call    alloc_dos_mem
                mov     bx,-1
                mov     ah,48h
                int     21h     ; largest DOS memory block for info exit
                movzx   ebx,bx
                shl     ebx,4
                mov     dos_mem_size,ebx
                jmp     dpmi_init

	no_dpmi_init:
	; si tenemos VCPI lo inicializamos
	test    al,1
        jz      get_all_mem     ; Si es RAW/XMS no hace falta inicializar nada

		call    vcpi_init

        get_all_mem :
        ; int's 50h-57h = irq's para el redireccionamiento
        xor     ax,ax
        mov     ds,ax
        mov     es,ax
        mov     di,NEW_IRQ0_IRQ7_BASE*4
        movzx   si,cs:irq_base
        shl     si,2
        mov     cx,8
        cli
        rep     movsd
        ; redireccionamos las irqs
        mov     ah,NEW_IRQ0_IRQ7_BASE
        call    irq_redir
        ; alojamos toda la memoria del DOS
        call    alloc_dos_mem

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; RUTINA DE SALTO A PROTEGIDO PARA TODOS LOS SISTEMAS (MODO REAL)
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
set_pm:
	pushf
	cli
	push    txdata
	pop     ds
	pop     word ptr real_flags
	mov     real_eax,eax
	mov     real_ebx,ebx
	mov     real_ecx,ecx
	mov     real_edx,edx
	mov     real_esi,esi
	mov     real_edi,edi
	mov     real_ebp,ebp
	jmp     set_pm_address

set_pm_dpmi:
	cli
	push    txdata
	pop     ds
	jmp     set_pm_address

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; RUTINAS DE SALIDA AL SISTEMA OPERATIVO PARA TODOS LOS SISTEMAS (MODO REAL)
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Rutina de salida normal
s16:
	cli
	; reajustamos la pila para modo real
	mov     eax,txpila
	mov     ss,ax
	shl     eax,4
	sub     esp,eax

	fin:
	cli
	mov     ax,txdata
	mov     ds,ax
	ASSUME 	DS:txdata
	mov     ah,cs:irq_base
	call    irq_redir
	exit_redirless:
	cmp     dword ptr xms_entry_point,0
	jz      no_xms_allocated
			push	ds
			free_xms_mem
			pop		ds
	no_xms_allocated:
	test    system_type,1
	jz      no_vcpi_end
			call    vcpi_uninit
	no_vcpi_end:
	cmp		vcpi_ems_test_page,0
	jz		no_vcpi_ems_test_page
			mov		ah,45h
			mov		dx,vcpi_ems_test_page
			int		67h
	no_vcpi_ems_test_page:	
	sti             ; activamos irqs por si hubo excepci¢n despu‚s de CLI

	mov     ax,4C00h
	int     21h

irq_base        db 8
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

; Rutina de salida por excepci¢n con informaci¢n de depuraci¢n
debug_info:
	cli

	mov     ax,txdata
	mov     ds,ax

	; reajustamos la pila para modo real
	mov     eax,txpila
	mov     ss,ax
	shl     eax,4
	sub     esp,eax

	mov     ax,3
	int     10h

	mov     dx,offset debug_info_msg
	jmp     error_handle

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; RUTINA DE INICIALIZACION DE DATOS COMUNES A TODOS LOS SISTEMAS (MODO REAL)
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

alloc_dos_mem   proc

        ; alojamos la memoria en real por si tenemos un DPMI paginador de %$#&!
        mov     ax,txcode32
        mov     ds,ax
        ASSUME  DS:txcode32
        mov     byte ptr dos_pointers,0
        mov     di,offset dos_pointers+1
        cont_adding:
        mov     ah,48h
        mov     bx,0FFFFh
        int     21h
        cmp     bx,2
        jb      dos_heap_done
        push    bx
        mov     ah,48h
        int     21h
        pop     cx
        jc      heap_error
        inc     byte ptr dos_pointers
        ASSUME  DS:txdata
        xor     ebx,ebx
        mov     bx,ax
        shl     ebx,4
        movzx   ecx,cx
        shl     ecx,4
        mov     [di],ebx
        mov     [di+4],ecx
        add     di,8
        cmp     di,offset dos_pointers+8*100+1
        je      dos_heap_done
        jmp     cont_adding
        dos_heap_done:
        clc
        ret

        heap_error:
        stc
        ret

alloc_dos_mem   endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

init_data       proc

	cld
	mov     ax,txcode32
	mov     es,ax
	mov     ax,txdata
	mov     ds,ax

	ASSUME  DS:txdata
	test    system_type,2
	jnz     no_gen_idt
	; GENERAMOS LA IDT
	mov     ax,1 SHL 3      ; C32_SEL con RAW/XMS y VCPI
	shl     eax,16
	ASSUME  DS:txcode32
	mov     ax,offset interrup_redir
	mov     di,offset idt
	xor     cl,cl
	cld
	; Interrupt gates clear IF automatically; we use them for system and hardware interrupts.
	; Trap gates leave IF as is; we use them for software services, like DOS and DPMI.
	gen_idt:
		stosd
		cmp		cl,NEW_IRQ0_IRQ7_BASE
		jb		gen_trap_gate
		cmp		cl,NEW_IRQ0_IRQ7_BASE+8
		jb		gen_interrupt_gate
		gen_trap_gate:
				mov     dword ptr es:[di],8F00h
				jmp		gate_generated
		gen_interrupt_gate:
				mov		dword ptr es:[di],8E00h
		gate_generated:
		add     di,4
		add     ax,8
	inc     cl
	jnz     gen_idt

        ASSUME  ES:txcode32
        mov     ax,offset tx_int31h
        mov     es:[idt+8*31h],eax
        mov     ax,offset tx_int21h
        mov     es:[idt+8*21h],eax

        ; fix-up del descriptor del PSP (no necesario bajo DPMI)
        ASSUME  DS:txdata
        mov     eax,psp_offset
        mov     word ptr psp_desc+2,ax
        shr     eax,16
        mov     byte ptr psp_desc+4,al
        mov     byte ptr psp_desc+7,ah

	no_gen_idt:
	; preparamos la tabla de redireccionamiento de interrupciones
        mov     di,offset interrup_redir
	xor     cl,cl
	mov     eax,0E900B050h          ; PUSH EAX , MOV AL,00h , JMP
	mov     ebx,offset real_int-offset interrup_redir-8 ; desplazamiento salto
	gen_ints:
		stosd
		xchg    ebx,eax
		stosd
		xchg    ebx,eax
		add     eax,10000h      ; inc. c¢digo interrupci¢n
		sub     ebx,8           ; dec. desp. salto
	dec     cl
	jnz     short gen_ints
        ; arreglamos las irq's
        mov     ax,offset real_irq-(offset interrup_redir+8*NEW_IRQ0_IRQ7_BASE)-8
        mov     di,offset interrup_redir+8*NEW_IRQ0_IRQ7_BASE+4
        mov     cx,8    ; irqs. bajas
        fix_irqs:
                stosw
                add     edi,2+4
                sub     ax,8
        loop    fix_irqs
	; arreglamos las excepciones
	mov     ax,offset exceptions-offset interrup_redir-8
        mov     di,offset interrup_redir+4
	stosw                                   ; exc.0
	sub     ax,8
	mov     word ptr es:[di-2+8],ax         ; exc.1
	sub     ax,8*2
	add     di,3*8-2                        ; exc.3
	mov     cl,14-3+1                       ; hasta la 0Eh
	repara_excs:
		stosw
		sub     ax,8
		add     di,8-2
	dec     cl
	jnz     repara_excs

	ASSUME  DS:txdata
	; ARREGLAMOS LA GDT
	; ponemos la base del descriptor de datos de 16 bits ( txdata )
	mov     eax,txdata
	shl     eax,4
	mov     word ptr d16_desc+2,ax
	shr     eax,16
	mov     byte ptr d16_desc+4,al

	; base del de c¢digo de 32 bits
	mov     eax,txcode32
	shl     eax,4
	mov     word ptr code_descriptor+2,ax
	shr     eax,16
	mov     byte ptr code_descriptor+4,al

	; base del de c¢digo de 16 bits
	mov     eax,txcode
	shl     eax,4
	mov     word ptr c16_desc+2,ax
	shr     eax,16
	mov     byte ptr c16_desc+4,al

	; direcci¢n de entrada inicial en PM
	mov     prot_code_address,offset c32_init

	; direcciones de 32 bits de la IDT y la GDT
	mov     eax,txdata
	shl     eax,4
        add     eax,offset gdt            ; direcci¢n GDT (32 bits)
	mov     gdt_base,eax

	mov     eax,txcode32
	shl     eax,4
	ASSUME  DS:txcode32
	add     eax,offset idt
	ASSUME  DS:txdata
	mov     idt_base,eax              ; base de IDT

	; habilitamos el punto de entrada en RM a la rutina de interrupci¢n real
	ASSUME  DS:txcode
	mov     word ptr cs:saltito+3,txcode
	ASSUME  DS:txdata

	ret

init_data       endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Rutina de activaci¢n de la l¡nea A20 por hard
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
enable_A20      proc            ; Thanx go to Tran ;D
        xor     ax,ax
        mov     fs,ax
        dec     ax
        mov     gs,ax
        call    test_A20
        jz      A20_enabled
        in      al,92h                       ; para PS/2
        or      al,2
        jmp     $+2
        jmp     $+2
        jmp     $+2
        out     92h,al
        call    test_A20
        jz      A20_enabled
        call    A20_check_state              ; para AT
        jnz     wait_for_A20
        mov     al,0d1h
        out     64h,al
        call    A20_check_state
        jnz     wait_for_A20
        mov     al,0dfh
        out     60h,al
        call    A20_check_state
        jmp     wait_for_A20
        A20_enabled:
        ret
enable_A20      endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
A20_check_state proc
        mov     ecx,20000h
        wait_that_state:
        jmp     $+2
        jmp     $+2
        jmp     $+2
        in      al,64h
        test    al,2
        loopnz  wait_that_state
        ret
A20_check_state endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
test_A20        proc
        mov     al,fs:[0]
        mov     ah,al
        not     al
        push    word ptr gs:[10h]
        mov     gs:[10h],al
        cmp     ah,fs:[0]
        mov     fs:[0],ah
        pop     word ptr gs:[10h]
        ret
test_A20        endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
wait_for_A20    proc
        mov     al,36h
        out     43h,al
        xor     al,al
        out     40h,al
        out     40h,al
        mov     cx,800h
A20_bucle1:
        call    test_A20
        je      A20_enabled
        in      al,40h
        in      al,40h
        mov     ah,al
A20_bucle2:
        in      al,40h
        in      al,40h
        cmp     al,ah
        je      A20_bucle2
        loop    A20_bucle1
        mov     dx,offset A20_error_msg
        jmp     error_handle1
wait_for_A20    endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Procedimiento de identificaci¢n de la CPU
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
cpuid   proc
; OUTPUT -> CF set if no 386 or better detected
;           CF clear if 386 or better detected
;               if CPUID supported
;                       AX = code returned by CPUID if supported
;                            ( AH = 5 if Pentium , ... )
;               if CPUID not supported
;                       AH = cpu family
;                               03h = 386
;                               04h = 486
;                       AL = 0
        pushfd
        cli     ; no queremos ninguna irq por si acaso
        ; comprobamos si tenemos un 386 como m¡nimo
        xor     ah,ah
        push    ax
        popf
        pushf
        pop     ax
        and     ah,0f0h
        cmp     ah,0f0h
        jz      no_386
        mov     ah,0f0h
        push    ax
        popf
        pushf
        pop     ax
        and     ah,0f0h
        jz      no_386
        mov     ax,300h         ; 386 por el momento
        ; comprobamos tenemos 486
        mov     ecx,esp
        and     esp,0FFFFFFFCh  ; as¡ evitamos un AC fault en el 486
        pushfd
        pop     ebx
        mov     edx,ebx
        and     edx,40000h
        xor     ebx,40000h      ; cambiamos el bit de AC
        push    ebx
        popfd
        pushfd
        pop     ebx
        and     ebx,40000h
        mov     esp,ecx
        cmp     ebx,edx
        je      id_done
        inc     ah              ; de momento, tenemos un 486
        pushfd
        pop     ebx
        mov     edx,ebx
        xor     ebx,200000h
        push    ebx
        popfd
        pushfd
        pop     ebx
        and     ebx,200000h
        and     edx,200000h
        cmp     ebx,edx
        je      id_done

        ; tiene CPUID (puede ser 486)
        mov     eax,1
        db      0fh,0a2h
        id_done:
        popfd
        clc
        ret

no_386:
        popfd
        stc
        ret

cpuid   endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Procedimiento de identificaci¢n de la FPU
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
fpuid   proc    ; inspired on CPUID32.ASM by ??? ;)
; OUTPUT -> AL = FPU type ( 00h = no present ; 02h = 287 ; 03h = 387 ; 04h = 487+ )
        xor     al,al
        fninit                          ; reseteamos el estado de la FPU
        mov     cs:fpu_status,5A5Ah
        fnstsw  cs:fpu_status           ; comprobamos el estado
        cmp     byte ptr cs:fpu_status,0
        jne     fpu_found               ; any FPU on system :(
        fnstcw  cs:fpu_status           ; miramos el control word
        mov     bx,cs:fpu_status
        and     bx,103fh
        cmp     bl,3Fh
        jne     fpu_found               ; no FPU <:'(
        ; Si llegamos aqu¡, hay FPU
        ; 486+ -> 487 compatible
        ; 386  -> 287 o 387: lo averiguamos
        mov     ax,txdata
        mov     ds,ax
        mov     ax,cpu_info
        cmp     ah,4
        jb      no_487
                mov     al,4
                jmp     fpu_found
        no_487:
        ; para saber si es 287 o 387 miramos 'infinity'
        fld1
        fldz
        fdiv                    ; tenemos +infinito
        fld     st
        fchs                    ; formamos -infinito
        fcompp                  ; si nos dice que +inf.=-inf., es un 287
        fstsw   cs:fpu_status   ; necesitamos los flags
        mov     ax,cs:fpu_status
        mov     al,2
        sahf
        jz      fpu_found       ; si son iguales, 287
        inc     al              ; si no, 387
        fpu_found:
        ret
fpu_status      dw ?
fpuid   endp
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; RUTINAS GENERALES DE GESTION PARA TODOS LOS SISTEMAS (MODO REAL)
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

error_handle:
	call    printf
	jmp     fin

error_handle1:
        call    printf
        jmp     exit_redirless

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

printf  proc

	mov     ax,txdata
	mov     ds,ax
	mov     ah,9
	int     21h
	ret

printf  endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

init_mem        proc

	; cambiamos el tama¤o del bloque reservado para nuestro programa
        mov     ebx,disk
	sub     ebx,txdata
	shl     ebx,4
        add     ebx,size disk_buffer
	shr     ebx,4
        add     bx,11h  ; redondeamos sumando un p rrafo + 16
	ASSUME  DS:txdata
	mov     ax,psp_seg
	mov     es,ax
	mov     ah,4Ah
	int     21h
	ret

init_mem        endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

get_dos_mem     proc
	mov     ah,48h
	int     21h
	mov     dx,offset not_enough_mem
	jc      error_handle
	ret
get_dos_mem     endp

free_dos_mem	proc
; INPUT: AX = Segment of block to release
; OUTPUT: CF = 0 if success; CF = 1 if error, and AX = error code.
	push	es
	mov		es,ax
	mov		ah,49h
	int		21h
	mov		dx,offset cant_free_mem
	pop		es
	jc		error_handle
	ret
free_dos_mem	endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

irq_redir       proc
; INPUT -> AH = base de IRQ's 0-7

	cli

	; enmascaramos todas las irq's
	mov     al,0FFh
	out     21h,al
	jmp     short $+2       ; delay

	mov     al,10001b       ; level, master, ICW4 on
	out     20h,al          ; ICW1
	jmp     short $+2

	mov     al,ah           ; INT AH = nueva base de irq's
	out     21h,al          ; ICW2
	jmp     short $+2

	mov     al,4            ; pq estamos en un AT
	out     21h,al          ; ICW3
	jmp     short $+2

	mov     al,1            ; modo 8086, EOI normal
	out     21h,al          ; ICW4
	jmp     short $+2

	xor     al,al           ; desenmascaramos las irq's
	out     21h,al

	sti

	ret

irq_redir       endp

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
TXCODE  ENDS

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; SEGMENTO DE CODIGO DE 32 BITS ( 'ENTRY POINT' AL PROGRAMA )
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

TXCODE32  SEGMENT PARA USE32 'CODE32'
ASSUME    CS:txcode32,SS:txpila,DS:txdata

; ponemos los selectores en el segmento de c¢digo para referenciar con CS
C32_SEL                 dw 1 SHL 3,0
D32_SEL                 dw 2 SHL 3,0
C16_SEL                 dw 3 SHL 3,0
D16_SEL                 dw 4 SHL 3,0
CODE_SEL                dw 5 SHL 3,0
DATA_SEL                dw 6 SHL 3,0
ENVRN_SEL               dw 7 SHL 3,0

INCLUDE					debug32.inc
INCLUDE                 dpmisup.inc
INCLUDE                 oemfuncs.inc    ; DarkX OEM functions
INCLUDE                 dos32sup.inc    ; DOS32 basic support
INCLUDE                 ints32.inc
INCLUDE                 excs32.inc
INCLUDE                 cback32.inc     ; call-back's 32 bit code
INCLUDE                 int21oem.inc

INCLUDE                 memmgr.inc      ; Memory Manager by Teknik

INCLUDE                 initmem.inc

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

        include filesys.inc             ; Rutines per carregar LEs
        include loadle.inc

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
bin_to_dec      proc
; INPUT  -> EAX = number to convert ; DS:EDI -> buffer where to copy string
; OUTPUT -> DS:EDI -> number string in decimal (skipping initial zeros)
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
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

c32_init:

	cli

	mov     ds,cs:D16_sel
	ASSUME  DS:txdata
	; ajustamos pila para modo protegido
        mov     esp,txpila
        shl     esp,4
        add     esp,size tx_pila

	; ponemos el punto de retorno a PM para interrupciones reales
	ASSUME  DS:txdata

        mov     ax,cs:D32_SEL
	mov     ds,ax          ; selector del segmento de datos
	mov     fs,ax
	mov     es,ax
	mov     gs,ax
	mov     ss,ax

        mov     ds,cs:D16_SEL
        mov     edx,offset sys_type_msg
        mov     ah,9
        int     21h

        xor     edx,edx
        mov     dl,system_type
        shl     edx,1
        or      dl,dl
        jnz     no_mires_si_xms
        cmp     dword ptr xms_entry_point,0
        jz      no_mires_si_xms
        mov     edx,3*2
        no_mires_si_xms:
        mov     dx,word ptr [edx+sys_types]
        int     21h

		mov		ah,9
		mov		ds,cs:D16_SEL
		mov		edx,offset cr_lf
		int		21h
		
        mov     edx,offset dos_mem_msg
        int     21h

        test    system_type,2
        jz      normal_init_mem
                mov     ecx,dos_mem_size
                mov     ds,cs:D32_SEL
                jmp     print_dos_mem

        normal_init_mem:
        mov     ds,cs:D32_SEL   ; selector del segmento de datos
        ; alojamos memoria DOS
        call    init_dos_heap

        ; alojamos extendida
        call    init_extended_heap

        mov     eax,txcode32
        shl     eax,4
        add     eax,offset dos_heap_info
        call    getmeminfo

        print_dos_mem:
        mov     edi,txcode32
        shl     edi,4
        mov     eax,ecx
        ASSUME  DS:txcode32
        mov     byte ptr [edi+number_string+10],"$"
        ASSUME  DS:txdata
        shr     eax,10
        add     edi,offset number_string
        call    bin_to_dec
        mov     edx,edi
        mov     ah,9
        int     21h
        push    ds
        mov     ds,cs:D16_SEL
        mov     edx,offset extended_mem_msg
        int     21h
        mov     al,system_type
        pop     ds
        test    al,2
        jz      no_dpmi_mem_info
                mov     eax,500h
                push    ds
                pop     es
                mov     edi,txcode32
                shl     edi,4
                push    edi
                add     edi,offset print_buffer
                int     31h
                mov     eax,[edi]
                jmp     have_dpmi_mem
        no_dpmi_mem_info:
        mov     eax,txcode32
        shl     eax,4
        push    eax
        add     eax,offset extended_heap_info
        call    getmeminfo
        mov     eax,ecx
        have_dpmi_mem:
        pop     edi
        shr     eax,10
        add     edi,offset number_string
        call    bin_to_dec
        mov     edx,edi
        mov     ah,9
        int     21h
        push    ds
        mov     ds,cs:D16_SEL
        mov     edx,offset kb_msg
        int     21h
        pop     ds

        sti

        ; ///////////////////////////////
        ; CARGAMOS EL PROGRAMA EN MEMORIA
        ; ///////////////////////////////

; primero, intentamos cargarlo como LE y, si no lo es, como DLINK executable

; loader de LE's
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

        mov     ds, cs:DATA_SEL
        mov     es, cs:D16_SEL
        ASSUME  ES:txdata
        mov     edx, es:program_name_ptr

        call    LoadLE
        jc      LEError

        mov     ds, cs:DATA_SEL
        mov     es, cs:D16_SEL
        ASSUME  ES:txdata
        mov     es, es:psp_selector

        ; Ponemos en el PSP el selector del segmento de vars. de entorno
        mov     bx,cs:ENVRN_SEL
        mov     es:[2Ch],bx

        push    dword ptr cs:CODE_SEL
        push    eax
        retf

LEError:
; E_FILE_ERROR
; E_NOT_LE
; E_NOT_ENOUGH_MEM
; E_UNSUPORTED_FEATURE

        cmp     eax, E_NOT_LE                   ; Si no ‚s un LE carrega el DLINK
        je      load_dlink_file

        mov     edx, offset load_error_msg      ; error_loading_LE
        cmp     eax, E_FILE_ERROR
        je      pm_error

        mov     edx,offset unsuported_feature_msg
        cmp     eax, E_UNSUPORTED_FEATURE
        je      pm_error

        mov     edx, offset exe_size_error_msg
        jmp     pm_error

load_dlink_file:

; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
; Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik Teknik
; ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ


; loader de DLINK executables
        INCLUDE dlnkload.inc

; salida de error en la carga de ejecutable com£n a LE y DLINK loaders
pm_error:
        mov     ds,cs:D16_SEL
        mov     ah,9
        int     21h
        mov     ax,4C01h
        int     21h

	salida:
	ASSUME DS:txdata
	mov     ds,cs:D16_SEL
	test    system_type,2
	jnz     salida_dpmi

	; iniciamos la reentrada
	mov     ds,cs:D32_sel
	mov     ebx,txcode
	shl     ebx,4
	ASSUME  DS:txcode
	add     ebx,offset saltito+1
	mov     word ptr [ebx],offset s16
        mov     ds,cs:D16_SEL   ; hay que cargarlos antes de llamar a RET_REAL_ADDRESS
        mov     es,cs:D32_SEL
	push    dword ptr cs:C16_sel
	ASSUME	DS:txdata
	push	dword ptr ds:ret_real_address
	retf
	ASSUME 	DS:txcode
	
; Punto de salida bajo DPMI
	salida_dpmi:
	ASSUME  DS:txdata
	; recuperamos excepciones
	xor     bl,bl
	mov     edi,offset dpmi_exc_handlers
	restore_ints:
		mov     edx,[edi]
		mov     cx,[edi+4]
		mov     ax,203h
		int     31h
		add     edi,6
                inc     bl
        cmp     bl,15
	jnz restore_ints

	; recuperamos las interrupciones
        ASSUME  ES:txcode32
        mov     es,cs:D32_SEL
        xor     bl,bl
        mov     edi,txcode32
        shl     edi,4
        recupera_ints:
                ASSUME  ES:txcode32
                mov     edx,dword ptr es:[edi+idt]
                mov     cx,word ptr es:[edi+idt+4]
                ASSUME  ES:txdata
                mov     ax,205h
                int     31h
                add     edi,6
        inc bl
        jnz recupera_ints

	; si se produjo una excepci¢n, salimos con info de debugging
	cmp     dpmi_exception_flag,0
	jz      any_exception
	mov     ax,3
	int     10h
	xor     eax,eax
	push    eax
	sub     esp,4*2
	mov     ax,txdata
	push    ax
	sub     esp,2
	cld
        db      66h             ; push de un WORD en segmento de 32 bits
	pushf
	mov     dx,offset debug_info_msg
	mov     ah,9
	pushad
	mov     edi,esp
	push    ss
	pop     es
	mov     bx,21h
	xor     ecx,ecx
	mov     ax,300h
	int     31h

	any_exception:
	; restauramos la base de las irq's y salimos al OS
	push    dword ptr cs:C16_SEL
        pushd   exit_redirless
	retf

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
code_end:
; Interrupt Descriptor Table
idt             dd 256*2 dup(?)

; tabla de redireccionamientos de interrupciones
interrup_redir  dd 256*2 dup(?)

dos_heap_info           dd ?,?
dos_pointers            db 8*100+1 dup(?)

extended_heap_info      dd ?,?
extended_pointers       db 8*100+1 dup(?)

xms_handles             db 8*100 dup(?)

print_buffer            db 80*25+1 dup(?)

number_string           db 11 dup(?)
TXCODE32  ENDS

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ

TXPILA          SEGMENT PARA STACK USE16 'STACK'
tx_pila         dd      PM_STACKSIZE dup(?)
TXPILA          ENDS

; pila auxiliar para procedimientos reales
TXPILA_REAL     SEGMENT PARA USE16 'STACK'
txpr            dd      RM_STACKSIZE dup(?)      ; 30 words es el m¡nimo
TXPILA_REAL     ENDS

DISK            SEGMENT PARA
disk_buffer     db      8000h dup(?)    ; 32Kb would be ok ;)
DISK            ENDS

END init_pm
