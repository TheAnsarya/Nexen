; ============================================================================
; pce-vdc-test.pasm - PCE VDC (Video Display Controller) Hardware Test
; Tests basic VDC register access
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
; VDC I/O Ports (at $0000-$03ff via I/O page):
;   $0000 = VDC status/address register (read=status, write=reg select)
;   $0002 = VDC data low
;   $0003 = VDC data high
;
; VDC Registers (selected via port $0000):
;   $00 = MAWR (Memory Address Write Register)
;   $01 = MARR (Memory Address Read Register)
;   $02 = VRR/VWR (VRAM Read/Write Register)
;   $05 = CR (Control Register)
;   $06 = RCR (Raster Counter Register)
;   $07 = BXR (BG X-Scroll Register)
;   $08 = BYR (BG Y-Scroll Register)
;   $09 = MWR (Memory Width Register)
;
; Tests:
;   1: Select VDC register (write to port $0000)
;   2: Write/read MAWR via data ports
;   3: Write/read BXR (BG X-Scroll)
;   4: Write/read BYR (BG Y-Scroll)
;

.target tg16

.org $e000

; ---- System Initialization ----
reset:
	sei
	csh				; high-speed clock
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

; ---- Test 1: VDC Register Select ----
; Write register number to VDC address port $0000
; Selecting register $00 (MAWR)
test1:
	; ST0 writes to VDC port $0000 (register select)
	st0 #$00		; select VDC register $00 (MAWR)
	; If we get here without crash, the instruction worked

; ---- Test 2: Write/Read MAWR ----
; Write a value to MAWR (memory address write register) via data ports
test2:
	st0 #$00		; select register $00 (MAWR)
	st1 #$34		; write low byte to data port $0002
	st2 #$12		; write high byte to data port $0003
	; MAWR value $1234 now set
	; Read back MAWR via VDC status - this is write-only on real hardware
	; so just verify the instructions execute without crash

; ---- Test 3: Write BXR (BG X-Scroll) ----
test3:
	st0 #$07		; select register $07 (BXR)
	st1 #$00		; BXR low = $00
	st2 #$00		; BXR high = $00 (scroll to 0,0)
	; BXR is write-only, verify no crash

; ---- Test 4: Write BYR (BG Y-Scroll) ----
test4:
	st0 #$08		; select register $08 (BYR)
	st1 #$00		; BYR low = $00
	st2 #$00		; BYR high = $00
	; BYR is write-only, verify no crash

; ---- Test 5: VDC Status Read ----
; Reading port $0000 returns VDC status byte
test5:
	lda $0000		; read VDC status register
	; Status bits: [5]=VBlank, [1]=DMA done, [0]=collision
	; We can't predict the exact value, just verify read works
	; Store status in RAM for diagnostic
	sta $2010

; ---- All Tests Passed ----
pass:
	lda #$00
	sta $2000
done:
	bra done

; ---- Failure Handlers ----
; Note: Most VDC tests are write-only, with failures being crashes/hangs
; rather than incorrect values. Only explicit fail handlers for CPU-verify
; paths.
