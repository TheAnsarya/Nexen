; ============================================================================
; lynx-boot-smoke.pasm - Lynx Boot Smoke Test
; Tests basic 65SC02 CPU boot and instruction execution
; Assembled with Poppy (Flower Toolchain)
; ============================================================================
;
; Test Protocol:
;   Address $00 (zero page) = test status
;   $80 = running
;   $00 = all tests passed
;   $01+ = test failure (value = failing test number)
;
; Memory Map:
;   $0000-$00ff = Zero Page RAM
;   $0100-$01ff = Stack
;   $0200+      = ROM (cartridge data loaded here)
;
; Boot: Lynx hardware loads ROM data starting at $0200 and jumps there
;

.target lynx

.org $0200

; ---- System Initialization ----
reset:
	sei				; disable interrupts
	cld				; clear decimal mode
	ldx #$ff
	txs				; set up stack at $01ff

	; signal "running"
	lda #$80
	sta $00

; ---- Test 1: Load/Store ----
test1:
	lda #$42
	sta $01			; store $42 to ZP $01
	lda #$00		; clear A
	lda $01			; reload from ZP
	cmp #$42
	bne fail1

; ---- Test 2: Arithmetic (ADC/SBC) ----
test2:
	clc
	lda #$10
	adc #$20		; $10 + $20 = $30
	cmp #$30
	bne fail2
	sec
	lda #$50
	sbc #$20		; $50 - $20 = $30
	cmp #$30
	bne fail2

; ---- Test 3: Flags (Zero, Negative, Carry) ----
test3:
	lda #$00		; Z flag should be set
	bne fail3
	lda #$80		; N flag should be set
	bpl fail3
	sec				; C flag should be set
	bcc fail3
	clc				; C flag should be clear
	bcs fail3

; ---- Test 4: Index Registers ----
test4:
	ldx #$aa
	ldy #$55
	cpx #$aa
	bne fail4
	cpy #$55
	bne fail4

; ---- Test 5: 65SC02-specific STZ ----
test5:
	lda #$ff
	sta $02			; set ZP $02 to $ff
	stz $02			; store zero (65SC02)
	lda $02			; reload
	bne fail5		; should be zero

; ---- All Tests Passed ----
pass:
	lda #$00
	sta $00			; write pass value
done:
	bra done		; infinite loop (65SC02 BRA)

; ---- Failure Handlers ----
fail1:
	lda #$01
	sta $00
	bra done
fail2:
	lda #$02
	sta $00
	bra done
fail3:
	lda #$03
	sta $00
	bra done
fail4:
	lda #$04
	sta $00
	bra done
fail5:
	lda #$05
	sta $00
	bra done
