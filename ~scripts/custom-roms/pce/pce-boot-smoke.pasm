; ============================================================================
; pce-boot-smoke.pasm - PCE Boot Smoke Test
; Tests basic HuC6280 CPU boot and instruction execution
; Assembled with Poppy (Flower Toolchain)
; ============================================================================
;
; Test Protocol:
;   Address $2000 = test status
;   $80 = running
;   $00 = all tests passed
;   $01+ = test failure (value = failing test number)
;
; Memory Map (after init):
;   MPR0 ($0000-$1FFF) = $ff (I/O)
;   MPR1 ($2000-$3FFF) = $f8 (Work RAM)
;   MPR7 ($e000-$ffff) = ROM (last bank)
;

.target tg16

.org $e000

; ---- System Initialization ----
reset:
	sei				; disable interrupts
	csh				; high-speed clock (7.16 MHz)
	cld				; clear decimal mode
	ldx #$ff
	txs				; set up stack at $21ff

	; configure memory page registers
	lda #$ff
	tam #$01		; MPR0 = $ff (I/O at $0000-$1fff)
	lda #$f8
	tam #$02		; MPR1 = $f8 (RAM at $2000-$3fff)

	; signal "running"
	lda #$80
	sta $2000

; ---- Test 1: Load/Store ----
test1:
	lda #$42
	sta $2010		; store $42
	lda #$00		; clear A
	lda $2010		; reload from RAM
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

; ---- Test 4: Index registers ----
test4:
	ldx #$aa
	ldy #$55
	cpx #$aa
	bne fail4
	cpy #$55
	bne fail4

; ---- Test 5: TAM/TMA (HuC6280-specific) ----
test5:
	lda #$f8
	tam #$02		; MPR1 = $f8 (redundant, but tests instruction)
	tma #$02		; read MPR1 back to A
	cmp #$f8		; should still be $f8
	bne fail5

; ---- All Tests Passed ----
pass:
	lda #$00
	sta $2000		; write pass value
done:
	bra done		; infinite loop

; ---- Failure Handlers ----
fail1:
	lda #$01
	sta $2000
	bra done
fail2:
	lda #$02
	sta $2000
	bra done
fail3:
	lda #$03
	sta $2000
	bra done
fail4:
	lda #$04
	sta $2000
	bra done
fail5:
	lda #$05
	sta $2000
	bra done
