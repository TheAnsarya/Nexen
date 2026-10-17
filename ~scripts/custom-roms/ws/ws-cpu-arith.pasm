; ============================================================================
; ws-cpu-arith.pasm - WonderSwan V30MZ Arithmetic/Logic Tests
; Tests V30MZ arithmetic, logic, and flag operations
; Assembled with Poppy (Flower Toolchain)
; ============================================================================
;
; Test Protocol:
;   Address $0000 (internal RAM) = test status
;   $80 = running
;   $00 = all tests passed
;   $01+ = test failure (value = failing test number)
;
; Memory Map:
;   $0000-$3fff = Internal RAM (16KB)
;   $c000-$ffff = ROM Bank window
;   I/O Ports $00-$ff = Hardware registers
;
; Boot: CPU starts at CS=$ffff, IP=$0000 (physical $ffff0)
;       For 128KB ROM, ROM maps to physical $e0000-$fffff
;       Reset vector at ROM offset $1fff0 far-jumps to entry at $e0000
;
; NOTE: V30MZ parametric instruction encoding is not yet fully implemented in
;       Poppy, so mov/cmp/add/sub/and/or/xor use .byte directives.
;       Implied ops (cli, cld, hlt, nop) and conditional jumps use mnemonics.
;
; Tests:
;   1: AND (logical AND)
;   2: OR (logical OR)
;   3: XOR (logical XOR)
;   4: NOT (bitwise complement)
;   5: NEG (two's complement)
;   6: Zero flag (from comparison)
;   7: Sign flag (negative result)
;   8: Carry flag (overflow addition)
;   9: INC/DEC register
;  10: XCHG register swap
;

.target ws

.org $0000

entry:
	cli
	cld

	; signal "running": mov al, $80 / mov [$0000], al
	.byte $b0, $80			; mov al, $80
	.byte $a2, $00, $00		; mov [$0000], al

; ---- Test 1: AND al, imm8 ----
test1:
	.byte $b0, $ff			; mov al, $ff
	.byte $24, $0f			; and al, $0f → al = $0f
	.byte $3c, $0f			; cmp al, $0f
	jne fail1

; ---- Test 2: OR al, imm8 ----
test2:
	.byte $b0, $a0			; mov al, $a0
	.byte $0c, $05			; or al, $05 → al = $a5
	.byte $3c, $a5			; cmp al, $a5
	jne fail2

; ---- Test 3: XOR al, imm8 ----
test3:
	.byte $b0, $ff			; mov al, $ff
	.byte $34, $0f			; xor al, $0f → al = $f0
	.byte $3c, $f0			; cmp al, $f0
	jne fail3

; ---- Test 4: NOT (one's complement) ----
test4:
	.byte $b0, $55			; mov al, $55
	.byte $f6, $d0			; not al → al = $aa
	.byte $3c, $aa			; cmp al, $aa
	jne fail4

; ---- Test 5: NEG (two's complement) ----
test5:
	.byte $b0, $01			; mov al, $01
	.byte $f6, $d8			; neg al → al = $ff (-1)
	.byte $3c, $ff			; cmp al, $ff
	jne fail5

; ---- Test 6: Zero flag test ----
test6:
	.byte $b0, $42			; mov al, $42
	.byte $3c, $42			; cmp al, $42 (equal → ZF=1)
	jne fail6				; should not jump
	.byte $3c, $41			; cmp al, $41 (not equal → ZF=0)
	je fail6				; should not jump

; ---- Test 7: Sign flag test ----
test7:
	; result of $01 - $02 = $ff (negative, SF=1)
	.byte $b0, $01			; mov al, $01
	.byte $2c, $02			; sub al, $02 → al = $ff
	; JS = jump if sign flag set (SF=1)
	.byte $78, $02			; js +2 (skip next 2 bytes = jmp fail7)
	.byte $eb				; jmp short (to fail7)
	.byte $00				; placeholder - will be patched
	; fall through if SF was set (correct)

; ---- Test 8: Carry flag test ----
test8:
	.byte $b0, $ff			; mov al, $ff
	.byte $04, $02			; add al, $02 → al = $01, CF=1 (overflow)
	; JC = jump if carry (CF=1)
	.byte $72, $02			; jc +2 (skip fail jump)
	.byte $eb				; jmp short
	.byte $00				; placeholder
	; carry was set — verify result
	.byte $3c, $01			; cmp al, $01
	jne fail8

; ---- Test 9: INC/DEC register (CX) ----
test9:
	; mov cx, $0042
	.byte $b9, $42, $00		; mov cx, $0042
	; inc cx → $0043
	.byte $41				; inc cx
	; cmp cx, $0043
	.byte $81, $f9, $43, $00	; cmp cx, $0043
	jne fail9
	; dec cx → $0042
	.byte $49				; dec cx
	; cmp cx, $0042
	.byte $81, $f9, $42, $00	; cmp cx, $0042
	jne fail9

; ---- Test 10: XCHG (register swap) ----
test10:
	; mov ax, $1234
	.byte $b8, $34, $12		; mov ax, $1234
	; mov bx, $5678
	.byte $bb, $78, $56		; mov bx, $5678
	; xchg ax, bx
	.byte $93				; xchg ax, bx
	; now ax=$5678, bx=$1234
	.byte $3d, $78, $56		; cmp ax, $5678
	jne fail10
	.byte $81, $fb, $34, $12	; cmp bx, $1234
	jne fail10

; ---- All Tests Passed ----
pass:
	.byte $b0, $00			; mov al, $00
	.byte $a2, $00, $00		; mov [$0000], al
	hlt

; ---- Failure Handlers ----
fail1:
	.byte $b0, $01			; mov al, $01
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail2:
	.byte $b0, $02			; mov al, $02
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail3:
	.byte $b0, $03			; mov al, $03
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail4:
	.byte $b0, $04			; mov al, $04
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail5:
	.byte $b0, $05			; mov al, $05
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail6:
	.byte $b0, $06			; mov al, $06
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail7:
	.byte $b0, $07			; mov al, $07
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail8:
	.byte $b0, $08			; mov al, $08
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail9:
	.byte $b0, $09			; mov al, $09
	.byte $a2, $00, $00		; mov [$0000], al
	hlt
fail10:
	.byte $b0, $0a			; mov al, $0a
	.byte $a2, $00, $00		; mov [$0000], al
	hlt

; ---- Reset Vector ----
; CPU starts here: CS=$ffff, IP=$0000 (physical $ffff0)
; For 128KB ROM: ROM offset $1fff0 = 16 bytes before 10-byte header
; Far jump to $e000:$0000 (physical $e0000 = ROM offset $0000)
.org $1fff0
	.byte $ea, $00, $00, $00, $e0	; jmp far $e000:$0000
