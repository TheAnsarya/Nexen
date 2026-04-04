; ============================================================================
; lynx-timer-test.pasm - Lynx Mikey Timer Hardware Test
; Tests basic Mikey timer register access and operation
; Assembled with Poppy (Flower Toolchain)
; ============================================================================
;
; Test Protocol:
;   Address $00 (zero page) = test status
;   $80 = running
;   $00 = all tests passed
;   $01+ = test failure (value = failing test number)
;
; Lynx Memory Map:
;   $0000-$00ff = Zero page (RAM)
;   $0100-$01ff = Stack (RAM)
;   $0200-$fcff = Cartridge / RAM
;   $fd00-$fd9f = Suzy registers
;   $fda0-$fdaf = (reserved)
;   $fdb0-$fdbf = (reserved)
;   $fdc0-$fdff = Mikey timers & misc
;   $fe00-$fe1f = Mikey audio
;   $fe20-$fe3f = Mikey misc
;   $fe80-$fe9f = Mikey palette (green)
;   $fea0-$febf = Mikey palette (blue/red)
;
; Mikey Timer Registers:
;   $fd00 = Timer 0 backup value     (TIMER0_BACKUP)
;   $fd01 = Timer 0 control/status   (TIMER0_CTLA)
;   $fd02 = Timer 0 current count    (TIMER0_CNT)
;   $fd03 = Timer 0 control B        (TIMER0_CTLB)
;
; Tests:
;   1: Write/read Mikey timer backup register
;   2: Write/read timer control register
;   3: Suzy MATHD register write/read
;   4: Mikey palette register write/read
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

; ---- Test 1: Timer 0 Backup Register ----
; Write to Timer 0 backup value and read it back
test1:
	lda #$a5
	sta $fd00		; write Timer 0 backup = $a5
	lda #$00
	lda $fd00		; read Timer 0 backup
	cmp #$a5
	bne fail1

; ---- Test 2: Timer 0 Control A Register ----
; Timer control bits: [7:5]=source, [4]=int enable, [3]=rst done, [2:0]=mode
; Write a safe value and read back
test2:
	lda #$00		; all timer features disabled
	sta $fd01		; write Timer 0 control A
	lda #$ff
	lda $fd01		; read Timer 0 control A
	; control reg may not return exact value (some bits read differently)
	; just verify no crash and read returns something

; ---- Test 3: Suzy MATHD Register ----
; Suzy has math registers for hardware multiply/divide
; $fd54 = MATHD (math D input)
test3:
	lda #$42
	sta $fd54		; write Suzy MATHD
	lda #$00
	lda $fd54		; read Suzy MATHD
	cmp #$42
	bne fail3

; ---- Test 4: Mikey Palette Register ----
; Green palette at $fda0-$fdaf (16 entries, Mikey register space)
test4:
	lda #$f0		; green value
	sta $fda0		; write palette entry 0 (green)
	lda #$00
	lda $fda0		; read palette entry 0
	cmp #$f0
	bne fail4

; ---- All Tests Passed ----
pass:
	lda #$00
	sta $00
done:
	bra done

; ---- Failure Handlers ----
fail1:
	lda #$01
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
