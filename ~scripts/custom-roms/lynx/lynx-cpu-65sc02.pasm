; ============================================================================
; lynx-cpu-65sc02.pasm - Lynx 65SC02-Specific CPU Tests
; Tests 65SC02 extensions beyond base 6502
; Assembled with Poppy (Flower Toolchain)
; ============================================================================
;
; Test Protocol:
;   Address $00 (zero page) = test status
;   $80 = running
;   $00 = all tests passed
;   $01+ = test failure (value = failing test number)
;
; Tests:
;   1: BRA (branch always)
;   2: PHX/PLX (push/pull X)
;   3: PHY/PLY (push/pull Y)
;   4: STZ (store zero) - multiple modes
;   5: TRB (test and reset bits)
;   6: TSB (test and set bits)
;   7: INC/DEC A (accumulator mode)
;   8: BIT immediate mode
;

.target lynx

.org $0200

reset:
	sei
	cld
	ldx #$ff
	txs

	lda #$80
	sta $00			; signal running

; ---- Test 1: BRA (branch always) ----
test1:
	bra test1_ok
	lda #$01
	sta $00
	bra done
test1_ok:

; ---- Test 2: PHX/PLX ----
test2:
	ldx #$a5
	phx				; push X onto stack
	ldx #$00		; clear X
	plx				; pull X back
	cpx #$a5
	bne fail2

; ---- Test 3: PHY/PLY ----
test3:
	ldy #$5a
	phy				; push Y onto stack
	ldy #$00		; clear Y
	ply				; pull Y back
	cpy #$5a
	bne fail3

; ---- Test 4: STZ (multiple addressing modes) ----
test4:
	; STZ zero page
	lda #$ff
	sta $01
	stz $01
	lda $01
	bne fail4
	; STZ absolute
	lda #$ff
	sta $0300
	stz $0300
	lda $0300
	bne fail4

; ---- Test 5: TRB (test and reset bits) ----
test5:
	lda #$cf		; memory = $cf = 11001111
	sta $02
	lda #$0f		; mask = $0f = 00001111
	trb $02			; result: $02 should be $c0
	; Z flag should reflect original AND: $cf & $0f = $0f (non-zero, Z=0)
	lda $02
	cmp #$c0
	bne fail5

; ---- Test 6: TSB (test and set bits) ----
test6:
	lda #$c0		; memory = $c0 = 11000000
	sta $03
	lda #$0f		; mask = $0f = 00001111
	tsb $03			; result: $03 should be $cf
	lda $03
	cmp #$cf
	bne fail6

; ---- Test 7: INC/DEC A (accumulator mode) ----
test7:
	lda #$41
	; INC A not directly supported in all assembler modes
	; Use .byte $1a for INC A opcode
	.byte $1a		; INC A
	cmp #$42
	bne fail7
	; DEC A
	.byte $3a		; DEC A
	cmp #$41
	bne fail7

; ---- Test 8: BIT immediate ----
test8:
	lda #$ff
	; BIT #imm: tests A & imm, sets Z flag
	.byte $89, $ff	; BIT #$ff (65SC02 immediate)
	beq fail8		; $ff & $ff = non-zero, Z should be clear
	.byte $89, $00	; BIT #$00
	bne fail8		; $ff & $00 = 0, Z should be set

; ---- All Tests Passed ----
pass:
	lda #$00
	sta $00
done:
	bra done

; ---- Failure Handlers ----
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
fail6:
	lda #$06
	sta $00
	bra done
fail7:
	lda #$07
	sta $00
	bra done
fail8:
	lda #$08
	sta $00
	bra done
