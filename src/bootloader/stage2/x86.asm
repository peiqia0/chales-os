bits 16

section _TEXT class=Code

; Apparently, according to chat gpt this is necessary to make the code work
; U4D
;
; Operation:      Unsigned 4 byte divide
; Inputs:         DX;AX   Dividend
;                 CX;BX   Divisor
; Outputs:        DX;AX   Quotient
;                 CX;BX   Remainder
; Volatile:       none
; the parameters found above is from line 36 - 44 of
; https://github.com/open-watcom/open-watcom-v2/blob/a9b7369e2bdc42cebdbbf49f0b9955a445ccc690/bld/clib/cgsupp/a/i4d.asm#L54
global __U4D
__U4D:
    shl edx, 16
    mov dx, ax
    mov eax, edx
    xor edx, edx

    shl ecx, 16
    mov cx, bx

    div ecx
    mov ebx, edx
    mov ecx, edx
    shr ecx, 16

    mov edx, eax
    shr edx, 16

    ret
; Some more random functions from that github repo
; U4M
; Operation:      integer four byte multiply
; Inputs:         DX;AX   integer M1
;                 CX;BX   integer M2
; Outputs:        DX;AX   product
; Volatile:       CX, BX destroyed
;
global __U4M
__U4M:
    shl edx, 16         ; dx to upper half of edx
    mov dx, ax          ; m1 in edx
    mov eax, edx        ; m1 in eax

    shl ecx, 16         ; cx to upper half of ecx
    mov cx, bx          ; m2 in ecx

    mul ecx             ; result in edx:eax (we only need eax)
    mov edx, eax        ; move upper half to dx
    shr edx, 16

    ret


global _x86_div64_32
_x86_div64_32:

    ; make new call frame
    push bp             ; save old call frame
    mov bp, sp          ; initialize new call frame

    push bx

    mov eax, [bp + 8]
    mov ecx, [bp + 12]
    xor edx, edx
    div ecx

    mov bx, [bp + 16]
    mov [bx + 4], eax

    mov eax, [bp + 4]
    div ecx

    ; store results
    mov [bx], eax
    mov bx, [bp + 18]
    mov [bx], edx

    pop bx

    mov sp, bp
    pop bp
    ret

global _x86_Video_WriteCharTeletype
_x86_Video_WriteCharTeletype:
    push bp
    mov bp, sp

    push bx

    mov ah, 0Eh
    mov al, [bp + 4]
    mov bh, [bp + 6]

    int 10h

    pop bx

    mov sp, bp
    pop bp
    ret

global _x86_Disk_Read
_x86_Disk_Read:

    push bp
    mov bp, sp

    push bx
    push es

    mov dl, [bp + 4]

    mov ch, [bp + 6]
    mov cl, [bp + 7]
    shl cl, 6
    
    mov al, [bp + 8]
    and al, 3Fh
    or cl, al

    mov dh, [bp + 10]
    mov al, [bp + 12]
    mov bx, [bp + 16]
    mov es, bx
    mov bx, [bp + 14]

    mov ah, 02h
    stc
    int 13h

    mov ax, 1
    sbb ax, 0           ; 1 on success, 0 on fail   

    pop es
    pop bx

    mov sp, bp
    pop bp
    ret

global _x86_Disk_GetDriveParams
_x86_Disk_GetDriveParams:

    push bp
    mov bp, sp

    push es
    push bx
    push si
    push di

    mov dl, [bp + 4]
    mov ah, 08h
    mov di, 0
    mov es, di
    stc
    int 13h

    ; return
    mov ax, 1
    sbb ax, 0

    mov si, [bp + 6]
    mov [si], bl

    mov bl, ch
    mov bh, cl
    shr bh, 6
    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch
    and cl, 3Fh
    mov si, [bp + 10]
    mov [si], cx

    mov cl, dh
    mov si, [bp + 12]
    mov [si], cx

    pop di
    pop si
    pop bx
    pop es

    mov sp, bp
    pop bp
    ret

global _x86_Disk_Reset
_x86_Disk_Reset:

    push bp
    mov bp, sp

    mov ah, 0
    mov dl, [bp + 4]
    stc
    int 13h

    mov ax, 1
    sbb ax, 0 

    mov sp, bp
    pop bp
    ret
