.386p
.model flat
.stack 400h
.code
start:
  mov ah,9
  mov edx,offset prompt
  int 21h

  mov ebx,0fabecafeh
  mov ecx,31415926h
  mov edx,12345678h
  mov esi,0fedcba98h
  mov edi,56789abch
  mov ebp,13579bdfh
  mov ah,8
  int 21h

  cmp ebx,0fabecafeh
  jnz test_failed
  cmp ecx,31415926h
  jnz test_failed
  cmp edx,12345678h
  jnz test_failed
  cmp esi,0fedcba98h
  jnz test_failed
  cmp edi,56789abch
  jnz test_failed
  cmp ebp,13579bdfh
  jnz test_failed

  mov edx,offset ok_msg
  mov ah,9
  int 21h
  jmp end_program

  test_failed:
  mov edx,offset error_msg
  mov ah,9
  int 21h

  end_program:
  mov ah,4Ch
  int 21h

.data
prompt      db "Testing int 21h, service 8 (get key). Press any key...",13,10,"$"
ok_msg      db "Int 21h preserved all registers.$"
error_msg   db "Int 21h did not preserve registers.$"
end start