
_main:

;main.c,4 :: 		void main() {
;main.c,5 :: 		ADCON1 = 0x0F; // Disable Analog Pins
	MOVLW       15
	MOVWF       ADCON1+0 
;main.c,6 :: 		OSCCON = 0x70; // Use internal oscillator 8MHz
	MOVLW       112
	MOVWF       OSCCON+0 
;main.c,13 :: 		TRISB = 0;
	CLRF        TRISB+0 
;main.c,16 :: 		UART1_Init(9600);
	BSF         BAUDCON+0, 3, 0
	CLRF        SPBRGH+0 
	MOVLW       207
	MOVWF       SPBRG+0 
	BSF         TXSTA+0, 2, 0
	CALL        _UART1_Init+0, 0
;main.c,18 :: 		LATB = 1;
	MOVLW       1
	MOVWF       LATB+0 
;main.c,19 :: 		Delay_ms(2500);
	MOVLW       26
	MOVWF       R11, 0
	MOVLW       94
	MOVWF       R12, 0
	MOVLW       110
	MOVWF       R13, 0
L_main0:
	DECFSZ      R13, 1, 1
	BRA         L_main0
	DECFSZ      R12, 1, 1
	BRA         L_main0
	DECFSZ      R11, 1, 1
	BRA         L_main0
	NOP
;main.c,21 :: 		UART1_Write_Text("PIC 4550 Ready!!!");
	MOVLW       ?lstr1_main+0
	MOVWF       FARG_UART1_Write_Text_uart_text+0 
	MOVLW       hi_addr(?lstr1_main+0)
	MOVWF       FARG_UART1_Write_Text_uart_text+1 
	CALL        _UART1_Write_Text+0, 0
;main.c,25 :: 		LATB = 0;
	CLRF        LATB+0 
;main.c,26 :: 		LATB.RB7 = 1;
	BSF         LATB+0, 7 
;main.c,28 :: 		UART1_Write_Text("STX\nWiFiConnect\nLorenzo\nmasterjellalJKL123\n");
	MOVLW       ?lstr2_main+0
	MOVWF       FARG_UART1_Write_Text_uart_text+0 
	MOVLW       hi_addr(?lstr2_main+0)
	MOVWF       FARG_UART1_Write_Text_uart_text+1 
	CALL        _UART1_Write_Text+0, 0
;main.c,30 :: 		while(1) {
L_main1:
;main.c,31 :: 		if (UART1_Data_Ready() == 1) {
	CALL        _UART1_Data_Ready+0, 0
	MOVF        R0, 0 
	XORLW       1
	BTFSS       STATUS+0, 2 
	GOTO        L_main3
;main.c,32 :: 		UART1_Write(UART1_Read());
	CALL        _UART1_Read+0, 0
	MOVF        R0, 0 
	MOVWF       FARG_UART1_Write_data_+0 
	CALL        _UART1_Write+0, 0
;main.c,33 :: 		}
L_main3:
;main.c,34 :: 		}
	GOTO        L_main1
;main.c,35 :: 		}
L_end_main:
	GOTO        $+0
; end of _main
