; ============================================================================
; ws-timing-order-scaffold.pasm - WonderSwan timing trace scaffold
; First scaffold ROM for #1285: emits a compact trace buffer in WS RAM.
; Assembled with Poppy (Flower Toolchain)
; ============================================================================
;
; RAM protocol (internal RAM):
;   $0000 = status ($80 running, $00 pass, $01+ fail)
;   $0001 = trace entry count
;   $0100.. = trace entries (6 bytes each)
;       +0 event id
;       +1 scanline (LINE_CUR sample)
;       +2 irq active (port $b4)
;       +3 irq enabled (port $b2)
;       +4 htimer low (port $a8)
;       +5 vtimer low (port $aa)
;
; Event ids (scaffold v1):
;   $01 = boot-start
;   $02 = line-cmp programmed
;   $03 = pre-wait sample
;   $10 = reached line-cmp target
;   $20 = reached vblank candidate line
;
; Notes:
; - This is intentionally a scaffold ROM: deterministic skeleton for future
;   IRQ flag/timer trace expansion.
; - Current version uses LINE_CUR reads and deterministic wait loops.
;

.target ws

.org $0000

entry:
	cli
	cld

	; status = running
	.byte $b0, $80			; mov al, $80
	.byte $a2, $00, $00		; mov [$0000], al

	; trace count = 0
	.byte $b0, $00			; mov al, $00
	.byte $a2, $01, $00		; mov [$0001], al

	; configure IRQ and timers for trace visibility
	.byte $b0, $f0			; enable line/vblank/hblank/vblank-timer IRQs
	.byte $e6, $b2			; out $b2, al
	.byte $b0, $ff			; clear active IRQs
	.byte $e6, $b6			; out $b6, al

	.byte $b0, $03			; h-reload low = 3
	.byte $e6, $a4			; out $a4, al
	.byte $b0, $00			; h-reload high = 0
	.byte $e6, $a5			; out $a5, al
	.byte $b0, $02			; v-reload low = 2
	.byte $e6, $a6			; out $a6, al
	.byte $b0, $00			; v-reload high = 0
	.byte $e6, $a7			; out $a7, al
	.byte $b0, $0f			; enable h/v timers with auto-reload
	.byte $e6, $a2			; out $a2, al

	; event 0: boot-start
	.byte $b0, $01			; mov al, $01 (event id)
	.byte $a2, $00, $01		; mov [$0100], al
	.byte $e4, $02			; in al, $02 (LINE_CUR)
	.byte $a2, $01, $01		; mov [$0101], al
	.byte $e4, $b4			; in al, $b4 (active IRQ)
	.byte $a2, $02, $01		; mov [$0102], al
	.byte $e4, $b2			; in al, $b2 (enabled IRQ)
	.byte $a2, $03, $01		; mov [$0103], al
	.byte $e4, $a8			; in al, $a8 (h timer low)
	.byte $a2, $04, $01		; mov [$0104], al
	.byte $e4, $aa			; in al, $aa (v timer low)
	.byte $a2, $05, $01		; mov [$0105], al
	.byte $b0, $01			; trace count = 1
	.byte $a2, $01, $00		; mov [$0001], al

	; program LINE_CMP = $20
	.byte $b0, $20			; mov al, $20
	.byte $e6, $03			; out $03, al

	; event 1: line-cmp programmed
	.byte $b0, $02			; event id
	.byte $a2, $06, $01		; mov [$0106], al
	.byte $e4, $02			; in al, $02
	.byte $a2, $07, $01		; mov [$0107], al
	.byte $e4, $b4			; in al, $b4
	.byte $a2, $08, $01		; mov [$0108], al
	.byte $e4, $b2			; in al, $b2
	.byte $a2, $09, $01		; mov [$0109], al
	.byte $e4, $a8			; in al, $a8
	.byte $a2, $0a, $01		; mov [$010a], al
	.byte $e4, $aa			; in al, $aa
	.byte $a2, $0b, $01		; mov [$010b], al
	.byte $b0, $02			; trace count = 2
	.byte $a2, $01, $00		; mov [$0001], al

	; event 2: pre-wait sample
	.byte $b0, $03			; event id
	.byte $a2, $0c, $01		; mov [$010c], al
	.byte $e4, $02			; in al, $02
	.byte $a2, $0d, $01		; mov [$010d], al
	.byte $e4, $b4			; in al, $b4
	.byte $a2, $0e, $01		; mov [$010e], al
	.byte $e4, $b2			; in al, $b2
	.byte $a2, $0f, $01		; mov [$010f], al
	.byte $e4, $a8			; in al, $a8
	.byte $a2, $10, $01		; mov [$0110], al
	.byte $e4, $aa			; in al, $aa
	.byte $a2, $11, $01		; mov [$0111], al
	.byte $b0, $03			; trace count = 3
	.byte $a2, $01, $00		; mov [$0001], al

wait_line_cmp:
	.byte $e4, $02			; in al, $02 (LINE_CUR)
	.byte $3c, $20			; cmp al, $20
	jne wait_line_cmp

	; event 3: reached line-cmp target
	.byte $b0, $10			; event id
	.byte $a2, $12, $01		; mov [$0112], al
	.byte $e4, $02			; in al, $02
	.byte $a2, $13, $01		; mov [$0113], al
	.byte $e4, $b4			; in al, $b4
	.byte $a2, $14, $01		; mov [$0114], al
	.byte $e4, $b2			; in al, $b2
	.byte $a2, $15, $01		; mov [$0115], al
	.byte $e4, $a8			; in al, $a8
	.byte $a2, $16, $01		; mov [$0116], al
	.byte $e4, $aa			; in al, $aa
	.byte $a2, $17, $01		; mov [$0117], al
	.byte $b0, $04			; trace count = 4
	.byte $a2, $01, $00		; mov [$0001], al

wait_line_vblank:
	.byte $e4, $02			; in al, $02 (LINE_CUR)
	.byte $3c, $90			; cmp al, $90 (line 144)
	jne wait_line_vblank

	; event 4: reached vblank candidate line
	.byte $b0, $20			; event id
	.byte $a2, $18, $01		; mov [$0118], al
	.byte $e4, $02			; in al, $02
	.byte $a2, $19, $01		; mov [$0119], al
	.byte $e4, $b4			; in al, $b4
	.byte $a2, $1a, $01		; mov [$011a], al
	.byte $e4, $b2			; in al, $b2
	.byte $a2, $1b, $01		; mov [$011b], al
	.byte $e4, $a8			; in al, $a8
	.byte $a2, $1c, $01		; mov [$011c], al
	.byte $e4, $aa			; in al, $aa
	.byte $a2, $1d, $01		; mov [$011d], al
	.byte $b0, $05			; trace count = 5
	.byte $a2, $01, $00		; mov [$0001], al

	; pass
	.byte $b0, $00			; mov al, $00
	.byte $a2, $00, $00		; mov [$0000], al
	hlt

; reset vector (CS=$ffff, IP=$0000)
.org $1fff0
	.byte $ea, $00, $00, $00, $e0	; jmp far $e000:$0000
