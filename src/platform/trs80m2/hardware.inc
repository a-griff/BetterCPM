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
; Z80 SIO (two serial ports) and CTC (baud rates, clock)
; -------------------------------------------------------------------------------

SIOADATA        EQU     0F4H            ; port A data
SIOBDATA        EQU     0F5H            ; port B data
SIOACTL         EQU     0F6H            ; port A control/status
SIOBCTL         EQU     0F7H            ; port B control/status
CTCCH0          EQU     0F0H            ; port A receive clock; CTC vector register
CTCCH1          EQU     0F1H            ; port A transmit clock
CTCCH2          EQU     0F2H            ; port B receive/transmit clock
CTCCH3          EQU     0F3H            ; software clock timer
PIOACTL         EQU     0E2H            ; PIO port A control
PIOBCTL         EQU     0E3H            ; PIO port B control

; -------------------------------------------------------------------------------
; Platform extension (src/platform/trs80m2/m2ext.mac)
;
; 512 bytes at F600H-F7FFH, loaded by stage one at cold boot from system
; package slot 1 (package bytes 10752-11263). It holds the interrupt-mode-2
; vector table shared by the clock and the serial ports, the serial driver and
; the routing of the TTY:/UC1:/RDR:/PUN:/LST: devices. The BIOS sector cache
; therefore holds at most 512-byte sectors (F400H-F5FFH).
; -------------------------------------------------------------------------------

MX_BASE         EQU     0F600H
MX_VECT         EQU     MX_BASE         ; 16-byte vector table; I = HIGH MX_VECT
MX_CTCV         EQU     LOW MX_VECT     ; CTC vector base (channel 3 uses +6)
MX_SIOV         EQU     LOW MX_VECT+8   ; SIO vector (status does not affect it)
MX_CONST        EQU     MX_BASE+010H    ; console status, CON: not CRT:
MX_CONIN        EQU     MX_BASE+013H    ; console input, CON: not CRT:
MX_CONOT        EQU     MX_BASE+016H    ; console output (C), CON: not CRT:
MX_LIST         EQU     MX_BASE+019H    ; list output (C), any LST:
MX_LSTST        EQU     MX_BASE+01CH    ; list status, any LST:
MX_PUNCH        EQU     MX_BASE+01FH    ; punch output (C)
MX_READR        EQU     MX_BASE+022H    ; reader input
MX_IMON         EQU     MX_BASE+025H    ; select mode 2, vectors, enable interrupts
MX_SIGN         EQU     MX_BASE+00AH    ; 'M2X',version (in an unused vector slot)
MX_CFG          EQU     MX_BASE+028H    ; per-port settings, 12 bytes each (A, then B):
MX_CFGLEN       EQU     12              ; +0 CTC divisor, +1 SIO setup sequence
                                        ; (register, value pairs after a channel reset)

; -------------------------------------------------------------------------------
; Controller port names used by the shared BIOS control overlay
; -------------------------------------------------------------------------------

FDCSTATUS       EQU     FDCSTS
FDCCOMMAND      EQU     FDCCMD
FDCTRACK        EQU     FDCTRK
FDCSECTOR       EQU     FDCSEC
FDCDATA         EQU     FDCDAT
FDCSELECT       EQU     FDCSEL
