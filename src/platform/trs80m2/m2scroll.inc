; ********************************************************************************
; *                   TRS-80 Model II Scrolling Console                         *
; *                                                                              *
; * Resident 80-column, 24-row output wrapper around the compact Model II console.*
; * Enforces the screen boundary, scrolls at the bottom row, handles backspace   *
; * cursor motion, and provides BetterCP/M's private cursor-character service.   *
; ********************************************************************************

; ================================================================================
; =                         Bounded Console Output                              =
; =                                                                              =
; = Extends the compact loader console with resident screen-boundary handling,   =
; = scrolling, backspace motion, and reverse-video cursor presentation.          =
; ================================================================================

; -------------------------------------------------------------------------------
; Scrolling Output
; -------------------------------------------------------------------------------


M2_SCROLL_OUT:                          ; write bounded scrolling console character
        LD      A,C
        CP      8
        JR      Z,M2_BACKSPACE
        CP      01CH
        JP      Z,M2_CLS                ; clear screen and home the cursor
        CALL    M2_OUT
        LD      HL,(M2CURSOR)
        LD      DE,VIDEOEND
        OR      A
        SBC     HL,DE
        RET     C

        LD      HL,VIDEOBASE+VIDEOCOLS
        LD      DE,VIDEOBASE
        LD      BC,VIDEOSIZE-VIDEOCOLS
        LDIR
        LD      HL,VIDEOEND-VIDEOCOLS
        LD      DE,VIDEOEND-VIDEOCOLS+1
        LD      BC,VIDEOCOLS-1
        LD      (HL),020H
        LDIR

  ; The last screen row (FF30-FF7F) lies wholly within one 256-byte page.
  ; Add the column to L directly; no separate 16-bit offset is needed.
        LD      A,(M2COLUMN)
        LD      HL,VIDEOEND-VIDEOCOLS
        ADD     A,L
        LD      L,A
        JP      M2_SETC

  ; Patch 2026-09-07: CONIN echoed BS correctly, but the old output leaf
  ; planted an 08H character on the screen.  Move left without erasing; the
  ; caller supplies SPACE/BS when rubout is wanted.  Keep this in the resident
  ; wrapper so the little boot loaders retain their original console size.

M2_BACKSPACE:                           ; move cursor left for backspace
        LD      HL,M2COLUMN
        LD      A,(HL)
        OR      A
        RET     Z                       ; do not back over the beginning of a line
        DEC     (HL)
        LD      HL,(M2CURSOR)
        DEC     HL
        JP      M2_SETC

  ; BetterCP/M private cursor-character operation. C=0 queries support. Any
  ; other byte is written at the current console position in reverse video and
  ; advances exactly like ordinary output. The Model II shows
  ; characters with bit 7 set in reverse video.

; -------------------------------------------------------------------------------
; Cursor Presentation
; -------------------------------------------------------------------------------

M2_CUROUT:                              ; write reverse-video cursor character
        LD      A,C
        OR      A
        JR      Z,M2_CURQUERY
        OR      080H
        LD      C,A
        CALL    M2_SCROLL_OUT

M2_CURQUERY:                            ; report cursor-output support
        LD      A,1
        RET
