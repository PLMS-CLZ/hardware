#line 1 "D:/keanu/Documents/GitHub/controller/PIC4550_ESP8266/main.c"
char output[100];
char input;

void main() {
 ADCON1 = 0x0F;
 OSCCON = 0x70;






 TRISB = 0;


 UART1_Init(9600);

 LATB = 1;
 Delay_ms(2500);

 UART1_Write_Text("PIC 4550 Ready!!!");



 LATB = 0;
 LATB.RB7 = 1;

 UART1_Write_Text("STX\nWiFiConnect\nLorenzo\nmasterjellalJKL123\n");

 while(1) {
 if (UART1_Data_Ready() == 1) {
 UART1_Write(UART1_Read());
 }
 }
}
