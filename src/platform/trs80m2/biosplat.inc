; ********************************************************************************
; *                    TRS-80 Model II BIOS Platform Binding                     *
; *                                                                              *
; * Binds the portable BetterCP/M compatibility BIOS to the Model II console,    *
; * scrolling, initialization, command-reload, overlay, and RSX-load services.   *
; ********************************************************************************

        INCLUDE hardware.inc
        INCLUDE m2cons.inc
        INCLUDE m2scroll.inc

PL_CONST       EQU     M2_STAT         ; console-status service
PL_CONIN       EQU     M2_IN           ; console-input service
PL_CONOT       EQU     M2_SCROLL_OUT   ; scrolling console-output service
PL_CURSOR      EQU     M2_CUROUT       ; cursor-presentation service

PL_INIT        EQU     M2_INIT         ; platform initialization service
        INCLUDE reload.inc
PL_RELOAD      EQU     M2_RELOAD       ; command-environment reload service
PL_OVLOAD      EQU     M2_OVLOAD       ; overlay-load service
PL_RSXLOAD     EQU     M2_RSLOAD       ; RSX-load service
