; ********************************************************************************
; *                  TRS-80 Model II BIOS Configuration                          *
; *                                                                              *
; * Placed ahead of src/bios/bios.mac by the Model II build, so its IFDEF tests  *
; * see these definitions on both assembler passes.                              *
; ********************************************************************************

; F400H-F7FFH is plain RAM on the Model II (the Model 4 keyboard lives there)
; and lies above everything BetterCP/M owns. The BIOS uses it as its private
; sector buffer (1K, the largest sector size), which no overlay shares, so it
; doubles as a one-sector cache. Each sector actually read is also copied to
; LY_BUF, the "last physical read" that DC_INFO publishes (DUP relies on it).
PL_SBUF        EQU     0F400H
