    ; clear TRISC2
    movlb 1
    bcf 0x0E, 2

    ; set LATC2
    movlb 2
    bsf 0x0E, 2

    ; loop forever
    bra $
    end
