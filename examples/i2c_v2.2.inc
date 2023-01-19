;
; i2c_v2.2.inc
;
;
; AtariVox EEPROM Driver
;
; By Alex Herbert, 2004
;
; Optimisations by Thomas Jentzsch, 2006/2008
;


; I/O Constants

I2C_SDA_MASK    equ     $04
I2C_SCL_MASK    equ     $08


; Signalling Macros

;  MAC     I2C_SCL_0
;    lda     #$00
;    sta     SWCHA
;  ENDM

;  MAC     I2C_SCL_1
;    lda     #I2C_SCL_MASK
;    sta     SWCHA
;  ENDM

;  MAC     I2C_SDA_IN
;    lda     #I2C_SCL_MASK
;    sta     SWACNT
;  ENDM

;  MAC     I2C_SDA_OUT
;    lda     #I2C_SCL_MASK|I2C_SDA_MASK
;    sta     SWACNT
;  ENDM


  MAC     I2C_START
; I2C_SCL_1
    lda     #(I2C_SCL_MASK|I2C_SDA_MASK)*2  ; 2         I2C_SCL_MASK
    sta     SWCHA                           ; 4
; I2C_SDA_OUT
    lsr                                     ; 2         I2C_SCL_MASK|I2C_SDA_MASK
    sta     SWACNT                          ; 4
; total: 12 cycles
  ENDM


  MAC     I2C_TXBIT
; I2C_SCL_0
    lda     #%0011                          ; 2
    sta     SWCHA                           ; 4
; set bit
    adc     #%1000                          ; 2
    sta     SWACNT                          ; 4         SDA = !C (a = %1000 / %1100)
; I2C_SCL_1
    lda     #I2C_SCL_MASK                   ; 2
    sta     SWCHA                           ; 4
; total: 18 cycles
  ENDM


  MAC     I2C_TXNACK
; I2C_SCL_0
    lda     #I2C_SCL_MASK*2                 ; 2         $00
    sta     SWCHA                           ; 4
; I2C_SDA_IN
    lsr                                     ; 2         I2C_SCL_MASK
    sta     SWACNT                          ; 4
; I2C_SCL_1
    nop                                     ; 2         required for timing!
    sta     SWCHA                           ; 4         I2C_SCL_MASK
; total: 18 cycles
  ENDM


; Subroutine Macros

  MAC     I2C_SUBS

i2c_startread
; use V to flag if previous byte needs ACK
    ldy     #%10100001                      ; 2         eeprom read command
    .byte   $2c                             ; 2
i2c_startwrite
    ldy     #%10100000                      ; 2         eeprom write command
    I2C_START                               ;12         start signal (clears V flag)
    tya                                     ; 2 = 16
i2c_txbyte
    eor     #$ff                            ; 2         invert data byte
    sec                                     ; 2
    rol                                     ; 2 =  6    shift loop bit into a
i2c_txbyteloop
    tay                                     ; 2
    I2C_TXBIT                               ;18         transmit
    tya                                     ; 2
    asl                                     ; 2         shift next bit into C
    bne     i2c_txbyteloop                  ; 2³= 26/27

; receive acknowledge bit

    beq     i2c_rxbit                       ;43 = 43
; i2c_txbyte: 264 cycles
;-------------------------------------------------------------------------------

i2c_rxbyte
    bvc     i2c_rxskipack                   ; 2³        previous byte needs acknowledge?
    jsr     i2c_txack                       ;24         transmit acknowledge bit

i2c_rxskipack
    bit     i2c_rxbyte                      ; 4         set V - next byte(s) require acknowledge

    lda     #1                              ; 2
i2c_rxbyteloop
    tay                                     ; 2 =  2
i2c_rxbit:                                  ;           receive bit in C
    I2C_TXNACK                              ;18 = 18

    lda     SWCHA                           ; 4
    lsr                                     ; 2
    lsr                                     ; 2
    lsr                                     ; 2 = 10    C = SDA

    tya                                     ; 2
    rol                                     ; 2         rotate into Y
    bcc     i2c_rxbyteloop                  ; 2³=  6/7

; received byte in A
    rts                                     ; 6 =  6
; i2c_rxbit:  40 cycles
; i2c_rxbyte: 310/333 cycles
;-------------------------------------------------------------------------------

i2c_stopread
    bvc     i2c_stopwrite                   ; 2³
    ldy     #$80                            ; 2
    jsr     i2c_rxbit                       ;46         transmit no-acknowledge

i2c_stopwrite
    jsr     i2c_txack                       ;30

; return port to input mode
    lda     #0                              ; 2         0
    sta     SWACNT                          ; 4
    rts                                     ; 6 = 12
; i2c_stopread:  45/92
; i2c_stopwrite: 42
;-------------------------------------------------------------------------------

i2c_txack
; I2C_SCL_0
    lda     #0                              ; 2         $00
    sta     SWCHA                           ; 4
; I2C_SDA_OUT
    lda     #(I2C_SCL_MASK|I2C_SDA_MASK)    ; 2         I2C_SCL_MASK|I2C_SDA_MASK
    sta     SWACNT                          ; 4
; I2C_SCL_1
    asl                                     ; 2         I2C_SCL_MASK
    sta     SWCHA                           ; 4

    rts                                     ; 6
; total: 24 cycles
;-------------------------------------------------------------------------------

  ENDM



