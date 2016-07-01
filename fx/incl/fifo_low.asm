[BITS 32]

%define align times ($$ - $) & 3 nop

[GLOBAL _keyhandler__Fv]
[GLOBAL _keyhandler__Size]

[EXTERN _key_count]

[SECTION .data ALIGN = 4]

[SECTION .text ALIGN = 4]

align
_keyhandler__Fv:
push edx
push eax
in al,0x60
mov dl,al
and edx,0x7F
test al,0x80
setz [_key_count + edx]
; acknowledge keyboard and interrupt controllers
in al,0x61
or al,0x80
out 0x61,al
and al,0x7F
out 0x61,al
mov al,0x20
out 0x20,al
pop eax
pop edx
sti
iret

_keyhandler__Size       dd      $-_keyhandler__Fv


