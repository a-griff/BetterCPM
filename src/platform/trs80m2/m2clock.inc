; ********************************************************************************
; *                    TRS-80 Model II Software Clock                            *
; *                                                                              *
; * The Model II has no battery clock. Its 30 Hz real-time interrupt is an NMI   *
; * at 0066H, inside the CP/M default FCB, so CP/M cannot use it. Instead, CTC   *
; * channel 3 runs as a timer: 4 MHz / 256 / 125 = 125 interrupts per second,   *
; * taken in interrupt mode 2 through MC_VEC. The handler keeps the TIME ABI 1.0 *
; * record (day count, BCD hour, minute, second) here in the BIOS, at a fixed    *
; * address that never moves. M2CLK.RSX publishes it as the TIME service and    *
; * starts the timer on the first SET; until then interrupts stay disabled.     *
; *                                                                              *
; * The handler uses 10 bytes of the interrupted program's stack.              *
; ********************************************************************************

MC_ISR: PUSH    AF                      ; CTC channel 3 tick
        PUSH    HL
        LD      HL,MC_TICK
        DEC     (HL)
        JR      NZ,MC_EXIT
        LD      (HL),MC_HZ
        PUSH    BC
        LD      HL,MC_CLK+4             ; seconds
        LD      C,60H
        CALL    MC_BUMP
        JR      NZ,MC_EXBC
        CALL    MC_BUMP                 ; minutes
        JR      NZ,MC_EXBC
        LD      C,24H
        CALL    MC_BUMP                 ; hours
        JR      NZ,MC_EXBC
        DEC     HL                      ; midnight: next day
        INC     (HL)
        JR      NZ,MC_EXBC
        INC     HL
        INC     (HL)
MC_EXBC:POP     BC
MC_EXIT:POP     HL
        POP     AF
        EI
        RETI

  ; Advance the BCD field at HL modulo C, then step HL to the next field.
  ; Return Z when the field wrapped to zero (carry into the next field).
MC_BUMP:LD      A,(HL)
        ADD     A,1
        DAA
        CP      C
        JR      C,MC_BKEEP
        XOR     A
MC_BKEEP:
        LD      (HL),A
        DEC     HL
        RET

MC_HZ   EQU     125                     ; ticks per second
MC_TICK:DB      MC_HZ                   ; ticks left in this second
MC_CLK: DB      0,0,0,0,0               ; day low, day high, hour, minute, second
MC_RUN: DB      0                       ; nonzero once SET has started the clock

  ; Mode-2 vector: the CTC supplies (base AND 0F8H) + 6 for channel 3, so the
  ; word must sit at an address whose low three bits are 110.
        DS      (14 - ($ AND 7)) AND 7
MC_VEC: DW      MC_ISR
        IF      (MC_VEC AND 7) NE 6
        DB      MC_VEC_MISALIGNED       ; undefined: forces an assembly error
        ENDIF
