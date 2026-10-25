; ********************************************************************************
; *                    TRS-80 Model II Physical Sector I/O                       *
; *                                                                              *
; * 8-inch FD1791 sector read/write by DRQ polling. Cylinder 0 is FM with        *
; * 128-byte sectors; every other cylinder is MFM with 512-byte sectors.         *
; *                                                                              *
; * Entry:  A = cylinder, B = side (0/1), C = sector ID, HL = buffer             *
; *         (B_PDrive) = drive bit (01H, 02H, 04H or 08H)                        *
; * Return: Z, A=0 on success, HL advanced past the data                         *
; *         NZ, A = masked controller status on failure                          *
; * Options: M2_READ_ONLY EQU 1 omits M2_WRITE.                                  *
; ********************************************************************************

; Data moves by the Z80 DMA, paced by the FD1791 DRQ on the DMA READY line,
; exactly as Pickles & Trout does it. Completion is FDC INTRQ on PIO A bit 0.
M2_READ:
        PUSH    HL
        CALL    M2_SETUP
        POP     HL
        RET     NZ
        LD      A,06DH                  ; WR0: port A -> port B
        LD      B,FDCREAD
        JR      M2_DMA

        IF      M2_READ_ONLY NE 1
M2_WRITE:
        PUSH    HL
        CALL    M2_SETUP
        POP     HL
        RET     NZ
        LD      A,06DH                  ; load as A -> B, then turn around
        LD      B,FDCWRITE
        ENDIF

; A = WR0 byte, B = FDC command, HL = buffer, (M2_LEN) = byte count.
M2_DMA:
        LD      (M2_DMAT+4),A
        LD      (M2_DMAT+11),HL         ; port B (memory) address
        LD      DE,(M2_LEN)
        ADD     HL,DE                   ; caller's HL advances past the data
        PUSH    HL
        DEC     DE                      ; DMA length is count - 1
        LD      (M2_DMAT+6),DE
        LD      HL,M2_DMAT
        LD      C,DMAPORT
        LD      A,B
        CP      FDCWRITE
        LD      B,M2_DMAN               ; read program
        JR      NZ,M2D_OUT
        LD      B,M2_DMAN+2             ; write program adds B -> A turnaround
M2D_OUT:
        OTIR
        LD      C,A                     ; FDC command
        LD      A,DMA_ENABLE
        OUT     (DMAPORT),A
        LD      A,C
        OUT     (FDCCMD),A
        LD      B,24                    ; let the FDC raise BUSY
M2D_SETTLE:
        DJNZ    M2D_SETTLE
        LD      DE,0
M2D_WT1:                               ; wait for INTRQ
        IN      A,(PIOA)
        RRCA
        JR      C,M2D_END
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,M2D_WT1
        LD      B,2                     ; ~1.6 s total before giving up
M2D_WT2:
        IN      A,(PIOA)
        RRCA
        JR      C,M2D_END
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,M2D_WT2
        DJNZ    M2D_WT2
        LD      A,DMA_RESET
        OUT     (DMAPORT),A
        POP     HL
        JR      M2_TIMEOUT
M2D_END:
        LD      A,DMA_RESET
        OUT     (DMAPORT),A
        POP     HL
        IN      A,(FDCSTS)
        JR      M2_DSTAT

; DMA program: reset; WR4 interrupt control 0; reinitialize status;
; WR0 transfer, port A = FDC data, length; WR1 port A I/O fixed;
; WR2 port B memory incrementing; WR4 byte mode, port B address;
; WR5 READY active high; LOAD. Writes append WR0 B -> A and LOAD again.
M2_DMAT:
        DB      DMA_RESET,091H,000H,08BH
        DB      06DH,FDCDAT
        DW      0
        DB      03CH,010H,08DH
        DW      0
        DB      08AH,0CFH
M2_DMAN EQU     $-M2_DMAT
        DB      001H,0CFH
M2_LEN: DW      512


M2_TIMEOUT:
        LD      A,FDCFORCE
        OUT     (FDCCMD),A
        LD      A,080H                  ; report as "not ready"
        OR      A
        RET
M2_DSTAT:
        AND     ST_ERRMASK
        RET

; -------------------------------------------------------------------------------
; Select drive/side/density, seek, and load the sector register.
; Sets (M2_LEN) to the sector length: 512 MFM, 128 FM (cylinder 0).
; -------------------------------------------------------------------------------
M2_SETUP:
        LD      (M2_CYL),A
        LD      A,C
        LD      (M2_SECT),A
        LD      A,B                     ; side 0 -> SEL_SIDE0 set
        AND     1
        LD      A,SEL_SIDE0
        JR      Z,M2S_SIDE
        XOR     A
M2S_SIDE:
        LD      B,A
        LD      A,(M2_CYL)
        OR      A
        JR      Z,M2S_FM
        LD      A,SEL_MFM
        OR      B
        LD      B,A
M2S_FM:
        LD      A,(B_PDrive)            ; active-low drive bit
        XOR     SEL_NONE
        AND     SEL_NONE
        OR      B
        OUT     (FDCSEL),A
        LD      DE,0
M2S_IDLE:
        IN      A,(FDCSTS)
        BIT     ST_BUSY,A
        JR      Z,M2S_SEEK
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,M2S_IDLE
        JR      M2_TIMEOUT
M2S_SEEK:
        LD      A,(M2_CYL)
        OUT     (FDCDAT),A
        LD      A,FDCSEEK
        OUT     (FDCCMD),A
        LD      DE,0
M2S_SKW:
        IN      A,(FDCSTS)
        BIT     ST_BUSY,A
        JR      Z,M2S_SKOK
        DEC     DE
        LD      A,D
        OR      E
        JR      NZ,M2S_SKW
        JR      M2_TIMEOUT
M2S_SKOK:
        AND     010H                    ; seek error
        RET     NZ
        LD      A,(M2_SECT)
        OUT     (FDCSEC),A
        LD      A,(M2_CYL)
        OR      A
        LD      HL,512
        JR      NZ,M2S_LEN
        LD      HL,128                  ; FM boot track
M2S_LEN:
        LD      (M2_LEN),HL
        XOR     A
        RET

M2_CYL:  DB     0
M2_SECT: DB     0
