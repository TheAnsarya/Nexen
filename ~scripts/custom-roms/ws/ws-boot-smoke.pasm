; ============================================================================
; ws-boot-smoke.pasm - WonderSwan Boot Smoke Test
; Tests basic V30MZ CPU boot and instruction execution
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
; NOTE: V30MZ parametric instruction encoding is not yet implemented in
;       Poppy, so mov/cmp/add/sub/jmp-far use .byte directives.
;       Implied ops (cli, cld, hlt, nop) and conditional jumps (jne, je)
;       use native mnemonics.
;

.target ws

; ---- Main Code ----
; ROM offset $0000 = physical $e0000 (segment $e000, offset $0000)
.org $0000

entry:
	cli				; disable interrupts
	cld				; clear direction flag

	; DS defaults to $0000 after reset, so direct addresses hit internal RAM

	; signal "running": mov al, $80 / mov [$0000], al
	.byte $b0, $80			; mov al, $80
	.byte $a2, $00, $00		; mov [$0000], al

; ---- Test 1: Load/Compare Immediate ----
test1:
	.byte $b0, $42			; mov al, $42
	.byte $3c, $42			; cmp al, $42
	jne fail1				; should NOT jump (ZF=1)

; ---- Test 2: Addition ----
test2:
	.byte $b0, $10			; mov al, $10
	.byte $04, $20			; add al, $20 (= $30)
	.byte $3c, $30			; cmp al, $30
	jne fail2

; ---- Test 3: Subtraction ----
test3:
	.byte $b0, $50			; mov al, $50
	.byte $2c, $20			; sub al, $20 (= $30)
	.byte $3c, $30			; cmp al, $30
	jne fail3

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

; ---- Reset Vector ----
; CPU starts here: CS=$ffff, IP=$0000 (physical $ffff0)
; For 128KB ROM: ROM offset $1fff0 = 16 bytes before 10-byte header
; Far jump to $e000:$0000 (physical $e0000 = ROM offset $0000)
.org $1fff0
	.byte $ea, $00, $00, $00, $e0	; jmp far $e000:$0000
