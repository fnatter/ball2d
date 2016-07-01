[BITS 32]

%define align times ($$ - $) & 3 nop

[GLOBAL _putpixel__FiiUc]
[GLOBAL _getpixel__Fii]
[GLOBAL _hline__FiiiUc]
[GLOBAL _clip_hline__FiiiUc]
[GLOBAL _vline__FiiiUc]
[GLOBAL _getimage__FiiiiPc]
[GLOBAL _putimage__FiiPc]
[GLOBAL _clipputimage__FiiPc]
[GLOBAL _clearoffscreenmem__Fv]
[GLOBAL _flipoffscreenmem__Fv]
[GLOBAL _putbgimage__FPc]
[GLOBAL _getDisplayStart__FPiT0]
[GLOBAL _setDisplayStart__Fii]
[GLOBAL _getLogicalScanlineWidthPixels__Fv]
[GLOBAL _setLogicalScanlineWidthPixels__Fi]
[GLOBAL _waitVR__Fv]
[GLOBAL _setpal__FPc]
[GLOBAL _getpal__FPc]
[GLOBAL _setblackpal__Fv]
[GLOBAL _floatt_hline__FiiiiiiiiPUc] ; eieieieieieieieiei!
[GLOBAL _come_on_push_it__Fv]
[GLOBAL _findnearestcolor__FPcccc]
[GLOBAL _findnearestcolor_8__FPcUcUcUc]
[GLOBAL _drawhll_flat__FiiUc]
[GLOBAL _drawhll_gouraud__FiiUc]
[GLOBAL _drawhll_lineartexture__FiiiPUc]
[GLOBAL _drawhll_lineartexture_gouraud__FiiiPUc]


[EXTERN _offscreen]
[EXTERN _BytesPerScanline]
[EXTERN _screensize_dword]
[EXTERN _LFBselector]
[EXTERN _scanline_list]
[EXTERN _shadinglookie]
[EXTERN _xres]
[EXTERN _yres]

[segment .data = 4]

[segment .text ALIGN = 4]

;void putpixel(int x, int y, unsigned char color);
align
_putpixel__FiiUc:
push ebp
mov ebp,esp
push ebx
push esi
mov esi,[_offscreen]
mov ebx,[_BytesPerScanline]
mov eax,[ebp + 12]
mul ebx
mov ebx,eax
add ebx,[ebp + 8]
add esi,ebx
;ds should be set to our selector (like _my_ds()) so we just care about the offset
mov al,byte [ebp + 16]
mov [esi],al
pop esi
pop ebx
mov esp,ebp
pop ebp
ret

;unsigned char getpixel(int x, int y);
align
_getpixel__Fii:
push ebp
mov ebp,esp
push ebx
push esi
mov esi,[_offscreen]
mov ebx,[_BytesPerScanline]
mov eax,[ebp + 12]
mul ebx
mov ebx,eax
add ebx,[ebp + 8]
add esi,ebx
xor eax,eax
mov al,[esi]
pop esi
pop ebx
mov esp,ebp
pop ebp
ret

;void clearoffscreenmem();
align
_clearoffscreenmem__Fv
push edi
push es
cld
mov edi,[_offscreen]
mov ax,ds
mov es,ax
mov ecx,[_screensize_dword]
xor eax,eax
rep stosd
pop es
pop edi
ret

;void flipoffscreenmem();
align
_flipoffscreenmem__Fv:
push edi
push esi
push es
mov es,[_LFBselector]
xor edi,edi
mov esi,[_offscreen]
mov ecx,[_screensize_dword]
cld
rep movsd
pop es
pop esi
pop edi
ret

;void putbgimage(char* full_screen_img_bits);
align
_putbgimage__FPc:
push ebp
mov ebp, esp
push edi
push esi
push es
mov ax,ds
mov es,ax
mov edi,[_offscreen]
mov esi,[ebp + 8]
mov ecx,[_screensize_dword]
cld
rep movsd
pop es
pop esi
pop edi
mov esp,ebp
pop ebp
ret

;void hline(int x1, int x2, int y, unsigned char color);
; this *excludes* the endpoint
align
_hline__FiiiUc:
push ebp
mov ebp,esp
push edi
push ebx
push es
mov edi,[_offscreen]
mov eax,[ebp + 16]
mov ebx,[_BytesPerScanline]
mul ebx
mov ebx,[ebp + 8]
add edi,eax
add edi,ebx
mov ecx,[ebp + 12]
sub ecx,ebx
mov ax,ds
mov es,ax
xor eax,eax
mov al,byte [ebp + 20]
mov bx,ax
shl bx,8
or ax,bx
mov ebx,eax
shl ebx,16
or eax,ebx		; eax = 0 | color | color << 8 | color << 16 || color << 24
cld
mov ebx,ecx
shr ecx,2
rep stosd
mov ecx,ebx
and ecx,3
rep stosb
pop es
pop ebx
pop edi
mov esp,ebp
pop ebp
ret

;void clip_hline(int x1, int x2, int y, unsigned char color);
; this *excludes* the endpoint and clips (but assumes that x1 <= x2)
align
_clip_hline__FiiiUc:
push ebp
mov ebp,esp
push edi
push ebx
push es
mov edi,[_offscreen]
mov eax,[ebp + 16]
and eax,eax
js .line_is_offscreen ; y < 0
cmp eax,[_yres]
jge .line_is_offscreen ; y >= yres
mov ebx,[_BytesPerScanline]
mul ebx
add edi,eax
mov ebx,[ebp + 8] ; ebx = x1
and ebx,ebx
jns .ok1
xor ebx,ebx ; x1 < 0, so x1 = 0
.ok1:
cmp ebx,[_xres]
jae .line_is_offscreen ; x1 >= xres
add edi,ebx
mov ecx,[ebp + 12]
and ecx,ecx
js .line_is_offscreen ; x2 < 0
cmp ecx,[_xres]
jb .ok2
mov ecx,[_xres]
dec ecx ; x2 >= xres, so x2 = xres - 1
.ok2:
sub ecx,ebx
mov ax,ds
mov es,ax
xor eax,eax
mov al,byte [ebp + 20]
mov bx,ax
shl bx,8
or ax,bx
mov ebx,eax
shl ebx,16
or eax,ebx		; eax = 0 | color | color << 8 | color << 16 || color << 24
cld
mov ebx,ecx
shr ecx,2
rep stosd
mov ecx,ebx
and ecx,3
rep stosb
.line_is_offscreen:
pop es
pop ebx
pop edi
mov esp,ebp
pop ebp
ret


;void vline(int y1, int y2, int x, unsigned char color);
; this *excludes* the endpoint
align
_vline__FiiiUc:
push ebp
mov ebp,esp
push esi
push ebx
mov esi,[_offscreen]
mov eax,[ebp + 8]	; note that I can't keep that value in EDX, cause mul modifies it
mov ebx,[_BytesPerScanline]
mul ebx
mov ecx,[ebp + 16]
add esi,eax
add esi,ecx

mov ecx,[ebp + 12]
sub ecx,[ebp + 8]

mov al,byte [ebp + 20]
; _BytesPerScanline is still in ebx
.yloop:
mov [esi],al ;explicitly [ds:esi]
add esi,ebx
dec ecx
jnz short .yloop
pop ebx
pop esi
mov esp,ebp
pop ebp
ret

;void getimage(int left, int top, int width, int height, char* buffer);
align
_getimage__FiiiiPc:
push ebp
mov ebp,esp
push es
push edi
push esi
push ebx
mov ax,ds
mov es,ax
mov edi,[ebp + 24]
mov esi,[_offscreen]  ; ds:esi points to offscreen, es:edi to buffer
mov eax,[ebp + 16]
mov [es:edi],al ; save width
inc edi
mov [es:edi],ah
inc edi
mov eax,[ebp + 20]
mov [es:edi],al ; save height
inc edi
mov [es:edi],ah
inc edi
;determine starting offset
mov eax,[ebp + 12] ; top -> eax
mov ebx,[_BytesPerScanline]
mul ebx
add eax,[ebp + 8]
add esi,eax
mov edx,[ebp + 20] ; edx = height
mov ecx,[ebp + 16] ; ecx = width
mov ebx,ecx        ; ebx = width
cld
jmp short .yloop_1st
.yloop:
sub esi,ecx                 ; subtract width bytes from offset (go back to left edge)
add esi,[_BytesPerScanline] ; go to next scanline
.yloop_1st
shr ecx,2
rep movsd
mov ecx,ebx
and ecx,3
rep movsb
mov ecx,ebx
dec edx
jnz short .yloop

pop ebx
pop esi
pop edi
pop es
mov esp,ebp
pop ebp
ret

;void putimage(int left, int top, char* buffer);
align
_putimage__FiiPc:
push ebp
mov ebp,esp
push es
push edi
push esi
push ebx
mov ax,ds
mov es,ax
mov esi,[ebp + 16]
mov edi,[_offscreen] ; ds:esi points to buffer and es:edi to offscreen
; determine starting offset
mov eax,[ebp + 12] ; top -> eax
mov ebx,[_BytesPerScanline]
mul ebx
add eax,[ebp + 8]
add edi,eax
;load width & height
cld
xor eax,eax
lodsw
mov ecx,eax ; ecx = width
lodsw
mov edx,eax ; edx = height
mov ebx,ecx ; keep width in ebx
jmp short .yloop_1st

.yloop:
sub edi,ecx                 ; subtract width bytes from offset (go back to left edge)
add edi,[_BytesPerScanline] ; go to next scanline
.yloop_1st
shr ecx,2
rep movsd
mov ecx,ebx
and ecx,3
rep movsb
mov ecx,ebx
dec edx
jnz short .yloop
pop ebx
pop esi
pop edi
pop es
mov esp,ebp
pop ebp
ret


;void clipputimage(int left, int top, char* buffer);
align ; note that this does only clip "right" and "bottom" (meaning left and top must be valid)
_clipputimage__FiiPc:
push ebp
mov ebp,esp
push es
push edi
push esi
push ebx
mov ax,ds
mov es,ax
mov esi,[ebp + 16]
mov edi,[_offscreen] ; ds:esi points to buffer and es:edi to offscreen
; determine starting offset
mov eax,[ebp + 12] ; top -> eax
mov ebx,[_BytesPerScanline]
mul ebx
add eax,[ebp + 8]
add edi,eax
;load width & height
cld
xor eax,eax
lodsw ; eax = width
mov ecx,[_xres]
sub ecx,[ebp + 8]
dec ecx ; ecx = maxwidth = xres - x - 1
cmp ecx,eax
jb .clip1 ; see if we need to clip "right" (jump if maxwidth < width)
mov ecx,eax ; maxwidth > width, so ecx = realwidth = width
.clip1:
mov ebx,ecx ; keep realwidth in ebx
sub eax,ebx ; eax = width - realwidth
mov [ebp + 8],eax
lodsw
mov edx,[_yres]
sub edx,[ebp + 12]
dec edx ; edx = maxheight = yres - y - 1
cmp edx,eax
jb .clip2 ; see if we need to clip "bottom" (jump if maxheight < height)
mov edx,eax ; maxheight > height, so edx = realheight = height
.clip2:
jmp short .yloop_1st

.yloop:
sub edi,ecx ; subtract realwidth bytes from offset (go back to left edge)
add esi,[ebp + 8] ; add width - realwidth to bitmap offset
add edi,[_BytesPerScanline] ; go to next scanline
.yloop_1st
shr ecx,2
rep movsd
mov ecx,ebx
and ecx,3
rep movsb
mov ecx,ebx
dec edx
jnz short .yloop

pop ebx
pop esi
pop edi
pop es
mov esp,ebp
pop ebp
ret

;void waitVR();
align
_waitVR__Fv:
pusha
mov dx,0x03DA
.loop1:
in al,dx
and al,0x08
jnz short .loop1
.loop2:
in al,dx
and al,0x08
jz short .loop2
popa
ret

;void setpal(char* pal);
align
_setpal__FPc:
push ebp
mov ebp,esp
push esi
mov esi,[ebp + 8]
mov ecx,0x0300 ; 0x00C0 = 0x0300 >> 2
mov dx,0x03C8
xor al,al
out dx,al ; send 0 to port 0x03C8
inc dx    ; dx = 0x03C9
cld
rep outsb
pop esi
mov esp,ebp
pop ebp
ret

;void getpal(char* pal);
align
_getpal__FPc:
push ebp
mov ebp,esp
push edi
push es
mov ax,ds
mov es,ax
mov dx,0x03C7
xor al,al
out dx,al
mov dx,0x03C9
mov edi,[ebp + 8]
mov ecx,0x0300 ; 0x00C0 = 0x0300 >> 2
cld
rep insb
pop es
pop edi
mov esp,ebp
pop ebp
ret

;void setblackpal();
align
_setblackpal__Fv:
mov dx,0x03C8
xor al,al
out dx,al
inc dx
mov ecx,0x0300
.loopclrpal:
out dx,al
dec ecx
jnz short .loopclrpal
ret

;int setLogicalScanlineWidthPixels(int width);
align
_setLogicalScanlineWidthPixels__Fi:
push ebp
mov ebp,esp
push ebx
mov ecx,[ebp + 8]
mov eax,0x00004F06
xor ebx,ebx
xor edx,edx
int 0x10
xor eax,eax
cmp ax,0x004F
jne short .Error
mov eax,1
.Error:
pop ebx
mov esp,ebp
pop ebp
ret

;int getLogicalScanlineWidthPixels();
align
_getLogicalScanlineWidthPixels__Fv:
push ebx
mov eax,0x00004F06
xor ecx,ecx
mov ebx,1
int 0x10
xor eax,eax
cmp ax,0x004F
jne short .Error
mov eax,ecx
.Error:
pop ebx
ret

;int setDisplayStart(int x, int y);
align
_setDisplayStart__Fii:
push ebp
mov ebp,esp
push ebx
mov eax,0x00004F07
xor ebx,ebx
mov ecx,[ebp + 8]
mov edx,[ebp + 12]
int 0x10
xor eax,eax
cmp ax,0x004F
jne short .Error
mov eax,1
.Error:
pop ebx
mov esp,ebp
pop ebp
ret

;int getDisplayStart(int* x, int* y);
align
_getDisplayStart__FPiT0:
push ebp
mov ebp,esp
push ebx
mov eax,0x00004F07
mov ebx,1
xor ecx,ecx
xor edx,edx
int 0x10
cmp ax,0x004F
jne short .Error
mov eax,[ebp + 8]
mov dword [eax],edx
mov eax,[ebp + 12]
mov dword [eax],ecx
jmp short .OK
.Error:
xor eax,eax
pop ebx
mov esp,ebp
pop ebp
.OK:
mov eax,1
pop ebx
mov esp,ebp
pop ebp
ret


;struct Scanline
;{
;0     int left PACKED;
;4     int right PACKED;       from here everything is fixed-point: [x.y]
;8     int g_shadeleft PACKED;   12.20, because we have to make the divisor be the same type of fixed.
;12    int g_shaderight PACKED;  12.20, and i.e. (x2-x1) needs 11 bits (CANNOT use 8.24!!)
;16    int texelsU_left PACKED;  12.20
;20    int texelsV_left PACKED;  12.20
;24    int texelsU_right PACKED; 12.20
;28    int texelsV_right PACKED; 12.20
;};
;sizeof(Scanline) = 32

; void drawhll_flat(int minY, int maxY, unsigned char color);
align
_drawhll_flat__FiiUc:
push ebp
mov ebp,esp
sub esp,8 ; 8 bytes local stack space
push ebx
push edi
push esi
push es

mov esi,[_scanline_list]
mov edi,[_offscreen]
mov ax,ds
mov es,ax
; calculate starting offset
mov ebx,[ebp + 8] ; ebx = minY
mov eax,[_BytesPerScanline]
mul ebx
add edi,eax

mov edx,[ebp + 12]
sub edx,ebx ; ...because ebx still holds minY
inc edx ; because nHlines = maxY - minY + 1
mov [ebp - 4],edx ; [ebp - 4] now holds number of hlines left to draw
shl ebx,5
add esi,ebx ; esi now points to the Scanline-structure for the hline at minY
xor eax,eax ; extend color to 32-bit
mov al,byte [ebp + 16]
mov bx,ax
shl bx,8
or ax,bx
mov ebx,eax
shl ebx,16
or eax,ebx		; eax = 0 | color | color << 8 | color << 16 || color << 24
mov [ebp - 8],eax

cld
align
.loop: ;(1): at the beginning of each loop, edi will point to the beginning of each scanline
;draw one hline
mov ecx,[esi + 4] ; ecx = right edge
mov edx,[esi] ; edx = left edge
cmp edx,ecx; see if left edge < right edge
ja short .notvalid ; or jae ??????????????????????
sub ecx,edx ; ecx = count
add edi,edx ; go to correct offset
mov eax,[ebp - 8]
mov ebx,ecx ; in the inner loop, ebx, ecx, eax, ds:edi are used
shr ecx,2
rep stosd
mov ecx,ebx
and ecx,3
rep stosb
sub edi,[esi + 4] ; (if hline was drawn) go back to start of scanline
align
.notvalid:
add edi,[_BytesPerScanline] ; and go to next scanline, so that (1) is true
add esi,32
dec dword [ebp - 4] ; one hline less
jnz short .loop

pop es
pop esi
pop edi
pop ebx
mov esp,ebp
pop ebp
ret

; void drawhll_gouraud(int minY, int maxY, unsigned char basecolor);
align
_drawhll_gouraud__FiiUc
push ebp
mov ebp,esp
sub esp,4 ; 4 bytes local stack space
push ebx
push edi
push esi
push es

mov esi,[_scanline_list]
mov edi,[_offscreen]
mov ax,ds
mov es,ax
; calculate starting offset
mov ebx,[ebp + 8] ; ebx = minY
mov eax,[_BytesPerScanline]
mul ebx
add edi,eax

mov edx,[ebp + 12]
sub edx,ebx ; ...because ebx still holds minY
inc edx ; because nHlines = maxY - minY + 1
mov [ebp - 4],edx ; [ebp - 4] now holds number of hlines left to draw
shl ebx,5
add esi,ebx ; esi now points to the Scanline-structure for the hline at minY

cld ; for stosb
shl dword [ebp + 16],20 ; make basecolor fixed20
align
.loop: ;(1): at the beginning of each loop, edi will point to the beginning of each scanline
; draw one hline
mov ecx,[esi + 4] ; ecx = right edge
mov edx,[esi] ; edx = left edge
cmp edx,ecx ; see if left edge < right edge
jae short .notvalid ; jae, because when left == right, we can't divide by "count" to get dcolor_dx
sub ecx,edx ; ecx = count
add edi,edx ; go to correct offset

mov ebx,ecx
shl ebx,20 ; ebx = count_fixed20, remember that count must be < 2^12 and > 0, which is okay
mov eax,[esi + 12] ; eax = shaderight_fixed20
mov edx,[esi + 8] ; edx = shadeleft_fixed20
sub eax,edx

xor edx,edx
test eax,0x80000000
jz short .nosign
neg eax ; eax = | dshade_fixed20 |; ebx = count_fixed20
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = - (dshade_fixed20 << 20 / count_fixed20)
neg eax
jmp short .after_div
align
.nosign:
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = dshade_fixed20 << 20 / count_fixed20
align
.after_div:

mov edx,eax ; edx = dcolor_dx (fixed20)
mov ebx,[esi + 8] ; ebx = shadeleft_fixed20
add ebx,[ebp + 16] ; ebx = colorleft_fixed20

align
.innerinnerloop:
mov eax,ebx
shr eax,20

stosb ; do mov/inc instead ?
add ebx,edx
dec ecx
jnz short .innerinnerloop

sub edi,[esi + 4] ; (if hline was drawn) go back to start of scanline
align
.notvalid:
add edi,[_BytesPerScanline] ; and go to next scanline, so that (1) is true
add esi,32
dec dword [ebp - 4] ; one hline less
jnz .loop

pop es
pop esi
pop edi
pop ebx
mov esp,ebp
pop ebp
ret

; void drawhll_lineartexture(int minY, int maxY, int width_shift, unsigned char* pixmapbit);
align
_drawhll_lineartexture__FiiiPUc
push ebp
mov ebp,esp
sub esp,16 ; 16 bytes local stack space
push ebx
push edi
push esi
push es

mov esi,[_scanline_list]
mov edi,[_offscreen]
mov ax,ds
mov es,ax
; calculate starting offset
mov ebx,[ebp + 8] ; ebx = minY
mov eax,[_BytesPerScanline]
mul ebx
add edi,eax

mov edx,[ebp + 12]
sub edx,ebx ; ...because ebx still holds minY
inc edx ; because nHlines = maxY - minY + 1
mov [ebp - 4],edx ; [ebp - 4] now holds number of hlines left to draw
shl ebx,5
add esi,ebx ; esi now points to the Scanline-structure for the hline at minY

cld ; for stosb
align
.loop: ;(1): at the beginning of each loop, edi will point to the beginning of each scanline
; draw one hline
mov ecx,[esi + 4] ; ecx = right edge
mov edx,[esi] ; edx = left edge
cmp edx,ecx ; see if left edge < right edge
jae near .notvalid ; jae, because when left == right, we can't divide by "count" to get dcolor_dx
sub ecx,edx ; ecx = count
add edi,edx ; go to correct offset

mov ebx,ecx
shl ebx,20 ; ebx = count_fixed20, remember that count must be < 2^12 and > 0, which is okay

; calculate du_dx
mov eax,[esi + 24] ; eax = texelsU_right_fixed20
mov edx,[esi + 16] ; edx = texelsU_left_fixed20
sub eax,edx
xor edx,edx
test eax,0x80000000
jz short .nosign1
neg eax ; eax = | du_fixed20 |; ebx = count_fixed20
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = - (du_fixed20 << 20 / count_fixed20)
neg eax
jmp short .after_div1
align
.nosign1:
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = du_fixed20 << 20 / count_fixed20
align
.after_div1:
mov [ebp - 8],eax ; [ebp - 8] = du_dx (fixed20)
; calculate dv_dx
; note that ebx has not changed, so count_fixed20 is still in it
mov eax,[esi + 28] ; eax = texelsV_right_fixed20
mov edx,[esi + 20] ; edx = texelsV_left_fixed20
sub eax,edx
xor edx,edx
test eax,0x80000000
jz short .nosign2
neg eax ; eax = | dv_fixed20 |; ebx = count_fixed20
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = - (dv_fixed20 << 20 / count_fixed20)
neg eax
jmp short .after_div2
align
.nosign2:
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = dv_fixed20 << 20 / count_fixed20
align
.after_div2:
mov [ebp - 12],eax ; [ebp - 12] = dv_dx (fixed20)

mov ebx,[esi + 16]
mov edx,[esi + 20]
mov [ebp - 16],esi ; back scanline_index up, and fill esi with ptr to pixmapbits
mov esi,[ebp + 20]

align
.innerinnerloop:
mov eax,edx
push ecx
shr eax,20 ; can't combine those shifts because i have to lose all precision
mov cl,[ebp + 16]
shl eax,cl ; eax = (v >> 20) << width_shift
pop ecx
push edx
mov edx,ebx
shr edx,20
add edx,eax
xor eax,eax
mov al,byte [esi + edx]
pop edx


stosb ; do mov/inc instead ?
add ebx,[ebp - 8]
add edx,[ebp - 12]
dec ecx
jnz short .innerinnerloop

mov esi,[ebp - 16]
sub edi,[esi + 4] ; (if hline was drawn) go back to start of scanline
align
.notvalid:
add edi,[_BytesPerScanline] ; and go to next scanline, so that (1) is true
add esi,32
dec dword [ebp - 4] ; one hline less
jnz near .loop

pop es
pop esi
pop edi
pop ebx
mov esp,ebp
pop ebp
ret

; void drawhll_lineartexture_gouraud(int minY, int maxY, int width_shift, unsigned char* pixmapbit);
align
_drawhll_lineartexture_gouraud__FiiiPUc
push ebp
mov ebp,esp
sub esp,46 ; local stack space
push ebx
push edi
push esi
push es

mov esi,[_scanline_list]
mov edi,[_offscreen]
mov ax,ds
mov es,ax
; calculate starting offset
mov ebx,[ebp + 8] ; ebx = minY
mov eax,[_BytesPerScanline]
mul ebx
add edi,eax

mov edx,[ebp + 12]
sub edx,ebx ; ...because ebx still holds minY
inc edx ; because nHlines = maxY - minY + 1
mov [ebp - 4],edx ; [ebp - 4] now holds number of hlines left to draw
shl ebx,5
add esi,ebx ; esi now points to the Scanline-structure for the hline at minY

cld ; for stosb
align
.loop: ;(1): at the beginning of each loop, edi will point to the beginning of each scanline
; draw one hline
mov ecx,[esi + 4] ; ecx = right edge
mov edx,[esi] ; edx = left edge
cmp edx,ecx ; see if left edge < right edge
jae near .notvalid ; jae, because when left == right, we can't divide by "count" to get dcolor_dx
sub ecx,edx ; ecx = count
add edi,edx ; go to correct offset

mov ebx,ecx
shl ebx,20 ; ebx = count_fixed20, remember that count must be < 2^12 and > 0, which is okay

; calculate du_dx
mov eax,[esi + 24] ; eax = texelsU_right_fixed20
mov edx,[esi + 16] ; edx = texelsU_left_fixed20
sub eax,edx
xor edx,edx
test eax,0x80000000
jz short .nosign1
neg eax ; eax = | du_fixed20 |; ebx = count_fixed20
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = - (du_fixed20 << 20 / count_fixed20)
neg eax
jmp short .after_div1
align
.nosign1:
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = du_fixed20 << 20 / count_fixed20
align
.after_div1:
mov [ebp - 8],eax ; [ebp - 8] = du_dx (fixed20)
; calculate dv_dx
; note that ebx has not changed, so count_fixed20 is still in it
mov eax,[esi + 28] ; eax = texelsV_right_fixed20
mov edx,[esi + 20] ; edx = texelsV_left_fixed20
sub eax,edx
xor edx,edx
test eax,0x80000000
jz short .nosign2
neg eax ; eax = | dv_fixed20 |; ebx = count_fixed20
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = - (dv_fixed20 << 20 / count_fixed20)
neg eax
jmp short .after_div2
align
.nosign2:
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = dv_fixed20 << 20 / count_fixed20
align
.after_div2:
mov [ebp - 12],eax ; [ebp - 12] = dv_dx (fixed20)

; calculate dshade_dx
; note that ebx has not changed, so count_fixed20 is still in it
mov eax,[esi + 12] ; eax = g_shaderight_fixed20
mov edx,[esi + 8] ; edx = g_shadeleft_fixed20
sub eax,edx
xor edx,edx
test eax,0x80000000
jz short .nosign3
neg eax ; eax = | dshade_fixed20 |; ebx = count_fixed20
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = - (dshade_fixed20 << 20 / count_fixed20)
neg eax
jmp short .after_div3
align
.nosign3:
shld edx,eax,20 ; shld dest,src,20 ; eax = src, edx = dest
div ebx ; eax = dshade_fixed20 << 20 / count_fixed20
align
.after_div3:
mov [ebp - 16],eax ; [ebp - 16] = dshade_dx (fixed20)


mov ebx,[esi + 16]
mov [ebp - 24],ebx ; [ebp - 24] = u
mov eax,[esi + 20]
mov [ebp - 28],eax ; [ebp - 28] = v
mov edx,[esi + 8]
mov [ebp - 32],edx ; [ebp - 32] = shade
mov [ebp - 20],esi ; back scanline_index up

align
.innerinnerloop:

mov esi,[ebp + 20]
push ecx
mov cl,[ebp + 16]
shr eax,20 ; can't combine those shifts because i have to lose all precision
shl eax,cl ; eax already holds v, now eax = (v >> 20) << width_shift
pop ecx
shr ebx,20 ; ebx still holds u
add ebx,eax
xor eax,eax
mov al,byte [esi + ebx]
mov esi,[_shadinglookie + eax * 4] ; esi = shadinglookie[pixmapcolor]
mov ebx,[ebp - 32]
shr ebx,20
mov al,byte [esi + ebx]

stosb ; do mov/inc instead ?

mov ebx,[ebp - 24]
add ebx,[ebp - 8]
mov [ebp - 24],ebx
mov eax,[ebp - 28]
add eax,[ebp - 12]
mov [ebp - 28],eax
mov edx,[ebp - 32]
add edx,[ebp - 16]
mov [ebp - 32],edx
dec ecx
jnz short .innerinnerloop

mov esi,[ebp - 20]
sub edi,[esi + 4] ; (if hline was drawn) go back to start of scanline
align
.notvalid:
add edi,[_BytesPerScanline] ; and go to next scanline, so that (1) is true
add esi,32
dec dword [ebp - 4] ; one hline less
jnz near .loop

pop es
pop esi
pop edi
pop ebx
mov esp,ebp
pop ebp
ret

; void floatt_hline(int x1, int x2, int y, int fixed16_tX1, int fixed16_tY1, int fixed20_tX2, int fixed20_tY2, int width, unsigned char* pixmapbits);
align
_floatt_hline__FiiiiiiiiPUc:
push ebp
mov ebp,esp
push edi
push es
push ebx
push esi

mov edi,[_offscreen]
mov ax,ds
mov es,ax
mov eax,[ebp + 16]
mov ebx,[_BytesPerScanline]
mul ebx
add edi,eax
mov ebx,[ebp + 8]
add edi,ebx
mov ecx,[ebp + 12]
sub ecx,ebx ; ecx = count
mov [ebp + 8],ecx ; [ebp + 8] = count

mov eax,[ebp + 20] ; eax = fixed21_leftU
mov edx,[ebp + 28] ; edx = fixed21_rightU
sub edx,eax ; edx =  fixed21_rightU - fixed21_leftU
shr edx,20 ; convert back to integer
mov [ebp + 28],edx
mov eax,[ebp + 24] ; eax = fixed16_leftV
mov edx,[ebp + 32] ; edx = fixed16_rightV
sub edx,eax ; edx = fixed21_rightV - fixed21_leftV
shr edx,20 ; convert back to integer
mov [ebp + 32],edx
finit
fild dword [ebp + 28] ; st(0) = du
fidiv dword [ebp + 8] ; st(0) = du / dx
fild dword [ebp + 32] ; st(0) = dv; st(1) = (du / dx)
fidiv dword [ebp + 8] ; st(0) = dv / dx
fild dword [ebp + 20] ; st(0) = tX1; st(1) = (dv / dx); st(2) = (du / dx)
fild dword [ebp + 24] ; st(0) = tY1; st(1) = tX1; st(2) = (dv / dx); st(3) = (du / dx)

cld
.loop:
mov esi,[ebp + 40] ; parameter is passed as tex.pic + 4, so we don't worry about adding 4
fist dword [ebp + 8]
mov eax,[ebp + 8]
fincstp
fist dword [ebp + 12]
mov ebx,[ebp + 12]
fdecstp
mov edx,[ebp + 36] ; edx = width of pixmap
mul edx ; eax = tY * width
add eax,ebx
add esi,eax
lodsb
stosb
fadd st2 ; st(0) += st(2)
fincstp
fadd st2 ; "st(1)" += "st(3)"
fdecstp
dec ecx
jnz short .loop

pop esi
pop ebx
pop es
pop edi
mov esp,ebp
pop ebp
ret

; subroutines of S3-speedup

align
; dx = port
; al = index
; value -> ah
_write3D4:
mov dx,0x03D4
out dx,al
mov ah,al
inc dx
in al,dx
xchg ah,al
dec dx
ret

align
; dx = port
; al = index
; ah = value
_read3D4:
mov dx,0x03D4
out dx,al
xchg ah,al
inc dx
out dx,al
xchg ah,al
dec dx
ret

align
_S3_EnableExtensions: ; enable extended registers
mov ax,0x4838
call _write3D4 ; write 0x48 to port 0x03D4, index 0x38
mov ax,0xA039
call _write3D4 ; write 0xA0 to port 0x03D4, index 0x39
ret

align
_S3_DisableExtensions:
mov ax,0x0038
call _write3D4 ; write 0x00 to port 0x03D4, index 0x38
mov ax,0x0039
call _write3D4 ; write 0x00 to port 0x03D4, index 0x39
ret

; void come_on_push_it();
align
_come_on_push_it__Fv: ; works only on S3's!!!!!!!! (speeds up write buffer for LFB)
pusha ; pushad
call _S3_EnableExtensions
mov al,0x40        ; bit 0: if set enables 8514/A mode
call _read3D4      ;  *  3: (801, 805, 928): fast write buffer on
or ah,0x08         ;     6: (801, 805, 928): Zero wait state off
call _write3D4     ; enable bit 3
call _S3_DisableExtensions
popa ;popad
ret

; unsigned char findnearestcolor(char* pal, char red, char green, char blue);
align
_findnearestcolor__FPcccc:
push ebp
mov ebp,esp
push esi
push ebx

mov esi,[ebp + 8]
mov ecx,0x100
mov edx,0xFFFFFFFF ; edx holds the best (lowest) error value
.loop:

push edx ; it'll be changed by the "imul reg32" which stores the result in edx:eax
xor eax,eax
mov al,byte [esi]
sub eax,[ebp + 12]
imul eax
mov ebx,eax
inc esi
xor eax,eax
mov al,byte [esi]
sub eax,[ebp + 16]
imul eax
add ebx,eax
inc esi
xor eax,eax
mov al,byte [esi]
sub eax,[ebp + 20]
imul eax
add ebx,eax ; ebx now equals (red_error^2 + green_error^2 + blue_error^2)
inc esi
pop edx

cmp ebx,0
jne short .NoPerfectMatch
mov eax,0x100
sub eax,ecx
jmp short .quit_early
.NoPerfectMatch:
cmp ebx,edx
jae short .I_could_have_a_better_one ; (unsigned! comparison)
mov edx,ebx ; lowest_error = error
mov eax,0x100
sub eax,ecx
mov [ebp + 8],eax ; best_index is stored at ebp + 8
.I_could_have_a_better_one:
dec ecx
jnz short .loop
mov eax,[ebp + 8]
.quit_early:
pop ebx
pop esi
mov esp,ebp
pop ebp
ret

; unsigned char findnearestcolor_8(char* pal, unsigned char red, unsigned char green, unsigned char blue);
; note that this function uses [8bitColor = ((6bitColor + 1) * 4) - 1]
align
_findnearestcolor_8__FPcUcUcUc:
push ebp
mov ebp,esp
push esi
push ebx

mov esi,[ebp + 8]
mov ecx,0x100
mov edx,0xFFFFFFFF ; edx holds the best (lowest) error value
.loop:

push edx ; it'll be changed by the "imul reg32" which stores the result in edx:eax
xor eax,eax
mov al,byte [esi]
inc al
shl al,2
dec al
sub eax,[ebp + 12]
imul eax
mov ebx,eax
inc esi
xor eax,eax
mov al,byte [esi]
inc al
shl al,2
dec al
sub eax,[ebp + 16]
imul eax
add ebx,eax
inc esi
xor eax,eax
mov al,byte [esi]
inc al
shl al,2
dec al
sub eax,[ebp + 20]
imul eax
add ebx,eax ; ebx now equals (red_error^2 + green_error^2 + blue_error^2)
inc esi
pop edx

cmp ebx,0
jne short .NoPerfectMatch
mov eax,0x100
sub eax,ecx
jmp short .quit_early
.NoPerfectMatch:
cmp ebx,edx
jae short .I_could_have_a_better_one ; (unsigned! comparison)
mov edx,ebx ; lowest_error = error
mov eax,0x100
sub eax,ecx
mov [ebp + 8],eax ; best_index is stored at ebp + 8
.I_could_have_a_better_one:
dec ecx
jnz short .loop
mov eax,[ebp + 8]
.quit_early:
pop ebx
pop esi
mov esp,ebp
pop ebp
ret
