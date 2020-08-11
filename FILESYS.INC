;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; file: filesys.inc
;
;  TX32's file system.
;
;       by Xavier Rubio Jansana, a.k.a. Teknik / #Trouble# Team, July 1998
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

READ_ONLY       equ     0               ; access rights
WRITE_ONLY      equ     1
READ_WRITE      equ     2

SEEK_SET        equ     0               ; start of file
SEEK_CUR        equ     1               ; current file position
SEEK_END        equ     2               ; end of file

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Opens a file with the desired access.
;
;  parameters:
;    edx - pointer to file name to open
;    al - access rights
;
;  returns:
;    cf - set if error
;    eax - handle of file
;
;  modifies:
;    eax
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

openfile        proc

        mov     ah, 3dh
        int     21h
        ret

openfile        endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Reads from a file.
;
;  parameters:
;    edx - pointer to buffer
;    ebx - file handle
;    ecx - number of bytes to read
;
;  returns:
;    cf - set if error
;    eax - number of bytes readed
;
;  modifies:
;    eax
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

read            proc

        mov     ah, 3fh
        int     21h
        jc      r_exit
        cmp     eax, ecx                ; Check if ecx bytes have been readed
        clc                             ; Don't touch zero flag!
        je      r_exit
        stc
r_exit:
        ret

read            endp

;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴
; Sets current file position
;
;  parameters:
;    al - origin of move
;    ebx - file handle
;    edx - file position
;
;  returns:
;    cf - set if error
;    eax - new file position
;
;  modifies:
;    eax
;컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴컴

seek            proc

        mov     ah, 42h
        push    ecx
        push    edx
        shld    ecx, edx, 16            ; cx:dx <- file position
        int     21h
        shld    edx, eax, 16
        mov     eax, edx
        pop     edx
        pop     ecx
        ret

seek            endp

