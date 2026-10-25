; ********************************************************************************
; *                  TRS-80 Model II BIOS Configuration                          *
; *                                                                              *
; * Placed ahead of src/bios/bios.mac by the Model II build, so its IFDEF tests  *
; * see these definitions on both assembler passes.                              *
; ********************************************************************************

; F400H-F7FFH is plain RAM on the Model II (the Model 4 keyboard lives there)
; and lies above everything BetterCP/M owns: the BIOS keeps its one-sector
; read cache there, sized for the largest sector BetterCP/M supports.
PL_SBUF        EQU     0F400H          ; private copy of the last sector read
PL_SLEN        EQU     1024
