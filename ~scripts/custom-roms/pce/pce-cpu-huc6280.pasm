; ============================================================================
; pce-cpu-huc6280.pasm - PCE HuC6280-Specific CPU Tests
; Tests HuC6280 extensions beyond base 65SC02
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
; Tests:
;   1: CLA (clear accumulator)
;   2: CLX (clear X register)
;   3: CLY (clear Y register)
;   4: SAX (swap A and X)
;   5: SAY (swap A and Y)
;   6: SXY (swap X and Y)
;   7: STZ (store zero) - zero page and absolute
;   8: TAM/TMA round-trip (multiple MPRs)
;   9: BIT immediate mode
;  10: CSL/CSH (clock speed switching)
;

.target tg16

.org $e000

; ---- System Initialization ----
reset:
	sei
	csh				; high-speed clock (7.16 MHz)
	cld
	ldx #$ff
	txs

	; configure memory page registers
	lda #$ff
	tam #$01		; MPR0 = $ff (I/O at $0000-$1fff)
	lda #$f8
	tam #$02		; MPR1 = $f8 (RAM at $2000-$3fff)

	; signal "running"
	lda #$80
	sta $2000

; ---- Test 1: CLA (clear accumulator) ----
test1:
	lda #$ff		; load non-zero
	cla				; HuC6280: clear A to $00
	bne fail1		; Z flag should be set, A == 0

; ---- Test 2: CLX (clear X register) ----
test2:
	ldx #$ff		; load non-zero
	clx				; HuC6280: clear X to $00
	cpx #$00
	bne fail2

; ---- Test 3: CLY (clear Y register) ----
test3:
	ldy #$ff		; load non-zero
	cly				; HuC6280: clear Y to $00
	cpy #$00
	bne fail3

; ---- Test 4: SAX (swap A and X) ----
test4:
	lda #$aa
	ldx #$55
	sax				; HuC6280: swap A <-> X
	cmp #$55		; A should now be $55
	bne fail4
	cpx #$aa		; X should now be $aa
	bne fail4

; ---- Test 5: SAY (swap A and Y) ----
test5:
	lda #$11
	ldy #$22
	say				; HuC6280: swap A <-> Y
	cmp #$22		; A should now be $22
	bne fail5
	cpy #$11		; Y should now be $11
	bne fail5

; ---- Test 6: SXY (swap X and Y) ----
test6:
	ldx #$33
	ldy #$44
	sxy				; HuC6280: swap X <-> Y
	cpx #$44		; X should now be $44
	bne fail6
	cpy #$33		; Y should now be $33
	bne fail6

; ---- Test 7: STZ (store zero) ----
test7:
	; STZ zero page (via I/O space — use RAM instead)
	lda #$ff
	sta $2010
	stz $2010		; store zero absolute
	lda $2010
	bne fail7
	; STZ another location
	lda #$ff
	sta $2011
	stz $2011
	lda $2011
	bne fail7

; ---- Test 8: TAM/TMA round-trip (multiple MPRs) ----
test8:
	; Write a value to MPR3 and read it back
	lda #$40
	tam #$08		; MPR3 = $40 (bit 3 = MPR3)
	lda #$00		; clear A
	tma #$08		; read MPR3 back
	cmp #$40
	bne fail8
	; Restore MPR3 to something safe
	lda #$00
	tam #$08

; ---- Test 9: BIT immediate mode ----
test9:
	lda #$ff
	; BIT #imm (65SC02/HuC6280): tests A & imm, only affects Z
	.byte $89, $ff	; BIT #$ff
	beq fail9		; $ff & $ff = non-zero, Z should be clear
	.byte $89, $00	; BIT #$00
	bne fail9		; $ff & $00 = 0, Z should be set

; ---- Test 10: CSL/CSH (clock speed switching) ----
test10:
	csl				; low-speed clock (1.79 MHz)
	; if we get here, CSL didn't crash
	csh				; back to high-speed (7.16 MHz)
	; if we get here, CSH didn't crash either
	; no way to directly verify clock speed, just verify no crash/hang

; ---- All Tests Passed ----
pass:
	lda #$00
	sta $2000
done:
	bra done

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
fail6:
	lda #$06
	sta $2000
	bra done
fail7:
	lda #$07
	sta $2000
	bra done
fail8:
	lda #$08
	sta $2000
	bra done
fail9:
	lda #$09
	sta $2000
	bra done
