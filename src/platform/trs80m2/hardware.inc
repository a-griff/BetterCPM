; ********************************************************************************
; *                    TRS-80 Model II Hardware Constants                        *
; *                                                                              *
; * Sources: Model II boot ROM source (Frank Durda, BOOTROM.TAR in pski's        *
; * model2archive) and MAME src/mame/trs/trs80m2.cpp.                            *
; ********************************************************************************

; -------------------------------------------------------------------------------
; I/O ports
; -------------------------------------------------------------------------------

FDCCMD          EQU     0E4H            ; FD1791 command (write) / status (read)
FDCSTS          EQU     0E4H
FDCTRK          EQU     0E5H            ; FD1791 track register
FDCSEC          EQU     0E6H            ; FD1791 sector register
FDCDAT          EQU     0E7H            ; FD1791 data register
FDCSEL          EQU     0EFH            ; drive select latch (write only)
ROMCTL          EQU     0F9H            ; bit 0: 1 = boot ROM at 0000-07FF
KBDATA          EQU     0FCH            ; keyboard data (read; clears key flag)
CRTCA           EQU     0FCH            ; 6845 address register (write)
CRTCD           EQU     0FDH            ; 6845 data register
CPUSTAT         EQU     0FFH            ; read: bit 7 = key available
CPUCTL          EQU     0FFH            ; write: see CPUCTL bits below

; -------------------------------------------------------------------------------
; Port FF (write) bits
; -------------------------------------------------------------------------------

CTL_BANK1       EQU     001H            ; 8000-FFFF from memory page 1
CTL_4080        EQU     010H            ; 1 = 40-column characters
CTL_RTCINT      EQU     020H            ; enable real-time-clock NMI
CTL_BLANK       EQU     040H            ; 1 = blank the display
CTL_VIDRAM      EQU     080H            ; video RAM mapped at F800-FFFF
CTL_RUN         EQU     CTL_VIDRAM OR CTL_BANK1  ; BetterCP/M operating value
KB_READY        EQU     7               ; CPUSTAT bit: key available

; -------------------------------------------------------------------------------
; Drive select latch (FDCSEL) bits
; -------------------------------------------------------------------------------

SEL_NONE        EQU     00FH            ; drive bits 0-3 are active low
SEL_SIDE0       EQU     040H            ; 1 = side 0, 0 = side 1
SEL_MFM         EQU     080H            ; 1 = MFM (double density), 0 = FM

; -------------------------------------------------------------------------------
; FD1791 commands and status (data bus already re-inverted by the FDC board)
; -------------------------------------------------------------------------------

FDCRESTORE      EQU     008H            ; restore, head load, 3 ms step
FDCSEEK         EQU     018H            ; seek, head load, 3 ms step
FDCREAD         EQU     080H            ; read sector
FDCWRITE        EQU     0A0H            ; write sector
FDCFORCE        EQU     0D0H            ; force interrupt (terminate)
ST_BUSY         EQU     0               ; status bit numbers
ST_DRQ          EQU     1
ST_ERRMASK      EQU     09CH            ; not ready, RNF, CRC, lost data

; -------------------------------------------------------------------------------
; Memory and boot contract
; -------------------------------------------------------------------------------

VIDEOBASE       EQU     0F800H          ; mapped while CTL_VIDRAM is set
VIDEOCOLS       EQU     80
VIDEOROWS       EQU     24
BT_LOAD         EQU     00E00H          ; ROM loads all 26 FM sectors of track 0 here
BT_BOOT         EQU     01000H          ; must hold 'BOOT'; ROM jumps to BT_BOOT+4
BT_DIAG         EQU     01400H          ; must hold 'DIAG'; ROM calls BT_DIAG+4 first
BT_END          EQU     01B00H          ; end of the loaded boot track
BT_STACK        EQU     02000H          ; stack the ROM leaves us

; -------------------------------------------------------------------------------
; Z80 DMA (port F8) and PIO port A (FDC INTRQ on bit 0)
; -------------------------------------------------------------------------------

DMAPORT         EQU     0F8H
PIOA            EQU     0E0H            ; bit 0 = FDC INTRQ (command complete)
DMA_ENABLE      EQU     087H
DMA_RESET       EQU     0C3H

; -------------------------------------------------------------------------------
; Model 4 port names used by shared BIOS/CONFIG source
; -------------------------------------------------------------------------------

FDCSTATUS       EQU     FDCSTS
FDCCOMMAND      EQU     FDCCMD
FDCTRACK        EQU     FDCTRK
FDCSECTOR       EQU     FDCSEC
FDCDATA         EQU     FDCDAT
FDCSELECT       EQU     FDCSEL
