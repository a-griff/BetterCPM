; ********************************************************************************
; *                      TRS-80 Model II Physical Console                        *
; *                                                                              *
; * Direct 80x24 video at F800H (kept mapped: BetterCP/M owns nothing above      *
; * F400H) and the Model II's ASCII keyboard. Same interface as m4cons.inc.      *
; ********************************************************************************

VIDEOSIZE       EQU     VIDEOCOLS*VIDEOROWS
VIDEOEND        EQU     VIDEOBASE+VIDEOSIZE

; -------------------------------------------------------------------------------
; Console initialization: map video RAM and page 1, 80 columns, display on,
; hide the 6845 hardware cursor (BetterCP/M draws a reverse-video cursor).
; -------------------------------------------------------------------------------

M2_INIT:
        LD      A,CTL_RUN
        OUT     (CPUCTL),A
        LD      A,10                    ; R10 cursor start: blink mode 01 = off
        OUT     (CRTCA),A
        LD      A,020H
        OUT     (CRTCD),A
M2_CLS:
        LD      HL,VIDEOBASE
        LD      (M2CURSOR),HL
        XOR     A
        LD      (M2COLUMN),A
        LD      DE,VIDEOBASE+1
        LD      BC,VIDEOSIZE-1
        LD      (HL),020H
        LDIR
        RET

; -------------------------------------------------------------------------------
; Character output (C = character). CR and LF move the cursor; others store.
; -------------------------------------------------------------------------------

M2_OUT:
        LD      A,C
        CP      13
        JR      Z,M2_CR
        CP      10
        JR      Z,M2_LF
        LD      HL,(M2CURSOR)
        LD      (HL),A
        INC     HL
        LD      A,(M2COLUMN)
        INC     A
        CP      VIDEOCOLS
        JR      C,M2_STCOL
        XOR     A
M2_STCOL:
        LD      (M2COLUMN),A
M2_SETC:
        LD      (M2CURSOR),HL
        RET

M2_CR:
        LD      A,(M2COLUMN)
        LD      E,A
        LD      D,0
        LD      HL,(M2CURSOR)
        OR      A
        SBC     HL,DE
        XOR     A
        JR      M2_STCOL

M2_LF:
        LD      HL,(M2CURSOR)
        LD      DE,VIDEOCOLS
        ADD     HL,DE
        JR      M2_SETC

; -------------------------------------------------------------------------------
; Keyboard. The keyboard sends finished ASCII and repeats by itself; a key is
; waiting while CPUSTAT bit 7 is set, and reading KBDATA takes it. CAPS and some
; CONTROL combinations set bit 7 (CAPS-A = C1H), so bit 7 is stripped.
; -------------------------------------------------------------------------------

M2_STAT:                                ; A <> 0 if a key is waiting
        IN      A,(CPUSTAT)
        AND     080H
        RET

M2_IN:                                  ; wait for and return a key in A
        IN      A,(CPUSTAT)
        RLA
        JR      NC,M2_IN
        IN      A,(KBDATA)
        AND     07FH
        RET

M2CURSOR: DW    VIDEOBASE
M2COLUMN: DB    0
