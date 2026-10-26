; ********************************************************************************
; *                  TRS-80 Model II BIOS Configuration                          *
; *                                                                              *
; * Placed ahead of src/bios/bios.mac by the Model II build, so its IFDEF tests  *
; * see these definitions on both assembler passes.                              *
; ********************************************************************************

; F400H-F5FFH is plain RAM on the Model II (the Model 4 keyboard lives there)
; and lies above everything BetterCP/M owns. The BIOS uses it as its private
; sector buffer (512 bytes, the largest Model II sector size), which no overlay
; shares, so it doubles as a one-sector cache. Each sector actually read is
; also copied to LY_BUF, the "last physical read" that DC_INFO publishes (DUP
; relies on it). F600H-F7FFH holds the platform extension (m2ext.mac).
PL_SBUF        EQU     0F400H

; The platform extension routes every character device except the CRT
; (serial ports A and B for TTY:, UC1:, RDR:, PUN: and LST:).
PL_SER      EQU     1
