char output[100];
char input;

void main() {
     // Disable Analog Pins
     ADCON1 = 0x0F;
     
     // Use internal oscillator 8MHz
     OSCCON = 0x70;

     // Setup PORTS
     TRISB = 0;
     
     // Setup UART
     UART1_Init(9600);
     
     LATB = 1;
     Delay_ms(2500);
     
     UART1_Write_Text("PIC Ready!");
  
     LATB = 0;
     LATB.RB7 = 1;
     
     while(1) {
         if (UART1_Data_Ready() == 1) {
             UART1_Write(UART1_Read());
         }
     }
}