; ********************************************************************************
; *                    TRS-80 Model II System Overlay Loader                     *
; *                                                                              *
; * Boot-media adapter for loading the command reloader, RSX carrier, and system *
; * control overlay from fixed physical sectors. Sector placement remains a      *
; * platform concern and is deliberately kept outside the common BIOS.           *
; ********************************************************************************

; ================================================================================
; =                           System Overlay Loads                              =
; =                                                                              =
; = Provides the Model II platform entry points that fetch protected system       =
; = components from their fixed boot-media sectors into their runtime buffers.   =
; ================================================================================

M2_RELOAD:                              ; load command-environment reloader
        LD SP,0100H
        LD HL,LY_LOAD
        LD DE,M2_RLTAB
        CALL M2_FETCH
        JR NZ,M2_RLFAIL
        LD SP,LY_STKT
        JP LY_LOAD

M2_RSLOAD:                              ; load RSX carrier
        PUSH AF
        LD HL,LY_CFG
        LD DE,M2_RLTAB
        CALL M2_FETCH                   ; selector occupies the reloader block tail
        POP BC
        RET NZ
        LD A,B
        JP LY_RSSEL

M2_OVLOAD:                              ; load control overlay
        LD HL,LY_CFG
        LD DE,M2_OVTAB
  ; Fetch two physical sectors.
  ; IN: HL = destination, DE = cylinder/side/sector triples.
  ; OUT: A = status, Z if good.  AF,BC,DE,HL used; IX preserved.
  ; The last byte of a triple may replace H after the pointer is spent.

; -------------------------------------------------------------------------------
; Physical Sector Fetch
; -------------------------------------------------------------------------------

M2_FETCH:                               ; fetch two boot-media sectors
        LD B,2

M2_RLLOOP:                              ; read next physical sector
        PUSH BC
        PUSH DE
        EX DE,HL
        LD A,(HL)
        INC HL
        LD C,(HL)
        INC HL
        LD H,(HL)
        LD B,C
        LD C,H
        EX DE,HL
        CALL BIOPHREAD
        POP DE
        POP BC
        RET NZ
        INC DE
        INC DE
        INC DE
        DJNZ M2_RLLOOP
        RET

M2_RLFAIL:                              ; return sector-read failure
        JR M2_RLFAIL

; -------------------------------------------------------------------------------
; Boot-Media Sector Tables
; -------------------------------------------------------------------------------

M2_RLTAB:                               ; reloader sector table
  ; @reloader-sector-table@

M2_OVTAB:                               ; control-overlay sector table
  ; @control-sector-table@
