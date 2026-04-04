; ============================================================================
; ws-display-test.pasm - WonderSwan Display Hardware Register Test
; Tests basic display/LCD register access
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
; Key Display I/O Ports:
;   $00 = DISP_CTRL (Display control)
;   $01 = BACK_COLOR (Background color / palette index)
;   $02 = LINE_CUR (Current scanline)
;   $03 = LINE_CMP (Scanline compare for interrupt)
;   $04 = SPR_BASE (Sprite table base)
;   $05 = SPR_FIRST (First sprite index)
;   $06 = SPR_COUNT (Sprite count)
;   $07 = MAP_BASE (Screen map base)
;   $60 = DISP_MODE (Display mode - mono/color)
;
; NOTE: V30MZ uses IN/OUT for I/O port access.
;       IN al, imm8 = $e4 <port>
;       OUT imm8, al = $e6 <port>
;
; Tests:
;   1: Write/read BACK_COLOR register
;   2: Write/read SPR_FIRST register
;   3: Write/read SPR_COUNT register
;   4: Read LINE_CUR (current scanline)
;   5: Write/read LINE_CMP register
;

.target ws

.org $0000

entry:
	cli
	cld

	; signal "running"
	.byte $b0, $80			; mov al, $80
	.byte $a2, $00, $00		; mov [$0000], al

; ---- Test 1: BACK_COLOR register (port $01) ----
test1:
	.byte $b0, $a5			; mov al, $a5
	.byte $e6, $01			; out $01, al (write BACK_COLOR)
	.byte $b0, $00			; mov al, $00
	.byte $e4, $01			; in al, $01 (read BACK_COLOR)
	.byte $3c, $a5			; cmp al, $a5
	jne fail1

; ---- Test 2: SPR_FIRST register (port $05) ----
test2:
	.byte $b0, $00			; mov al, $00
	.byte $e6, $05			; out $05, al (write SPR_FIRST = 0)
	.byte $b0, $ff			; mov al, $ff
	.byte $e4, $05			; in al, $05 (read SPR_FIRST)
	.byte $3c, $00			; cmp al, $00
	jne fail2

; ---- Test 3: SPR_COUNT register (port $06) ----
test3:
	.byte $b0, $80			; mov al, $80 (128 sprites)
	.byte $e6, $06			; out $06, al (write SPR_COUNT)
	.byte $b0, $00			; mov al, $00
	.byte $e4, $06			; in al, $06 (read SPR_COUNT)
	.byte $3c, $80			; cmp al, $80
	jne fail3

; ---- Test 4: LINE_CUR (current scanline, port $02) ----
test4:
	.byte $e4, $02			; in al, $02 (read LINE_CUR)
	; We can't predict the scanline value, just verify the read works
	; Store it in RAM for diagnostic
	.byte $a2, $10, $00		; mov [$0010], al

; ---- Test 5: LINE_CMP register (port $03) ----
test5:
	.byte $b0, $64			; mov al, $64 (scanline 100)
	.byte $e6, $03			; out $03, al (write LINE_CMP)
	.byte $b0, $00			; mov al, $00
	.byte $e4, $03			; in al, $03 (read LINE_CMP)
	.byte $3c, $64			; cmp al, $64
	jne fail5

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
fail5:
	.byte $b0, $05			; mov al, $05
	.byte $a2, $00, $00		; mov [$0000], al
	hlt

; ---- Reset Vector ----
; CPU starts here: CS=$ffff, IP=$0000 (physical $ffff0)
; For 128KB ROM: ROM offset $1fff0 = 16 bytes before 10-byte header
; Far jump to $e000:$0000 (physical $e0000 = ROM offset $0000)
.org $1fff0
	.byte $ea, $00, $00, $00, $e0	; jmp far $e000:$0000
