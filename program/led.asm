    ; clear TRISC2
    movlb 1
    bcf 0x0E, 2

    movlb 2

loop:
    ; set LATC2
    bsf 0x0E, 2

    ; clr LATC2
    bcf 0x0E, 2

    ; loop forever
    bra loop
    end
