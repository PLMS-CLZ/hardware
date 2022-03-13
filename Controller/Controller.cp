#line 1 "D:/keanu/Documents/GitHub/hardware/Controller/Controller.c"
const char *apiCreds = "admin@gmail.com\n11111111";

int espStxStatus = 0;
int espRecvIndex = 0;
int espRecvStep = 0;

int gsmStxStatus = 0;
int gsmRecvType = 0;
int gsmRecvIndex = 0;
int gsmRecvStep = 0;

char espCommand[200];
char espData[200];

char gsmSender[20];
char gsmDatetime[25];
char gsmCommand[200];
char gsmData[200];

char flashData[200];

void UnitRegister()
{

 UART1_Write_Text("\r\nAT+CMGS=\"");
 Delay_ms(100);

 UART1_Write_Text(espData);
 Delay_ms(100);
 UART1_Write_Text("\"\r");
 Delay_ms(100);

 UART1_Write_Text("PLMS-UnitRegister-CLZ\x1A");
}

void UnitUpdate()
{

 UART2_Write_Text("\r\nSTX\nUnitUpdate\n");
 Delay_ms(100);

 UART2_Write_Text(gsmData);
 Delay_ms(100);
 UART2_Write('\n');
 Delay_ms(100);
 UART2_Write_Text(gsmSender);
 Delay_ms(100);
 UART2_Write('\0');
}

void MessageSent()
{

 UART2_Write_Text("\r\nSTX\nMqttPublish\n");
 Delay_ms(100);

 UART2_Write_Text("PLMS-ControllerResponse-CLZ\n");
 Delay_ms(100);

 UART2_Write_Text("MessageSent");
 Delay_ms(100);
 UART2_Write('\0');
}

void ControllerConnected()
{

 UART2_Write_Text("\r\nSTX\nMqttPublish\n");
 Delay_ms(100);

 UART2_Write_Text("PLMS-ControllerResponse-CLZ\n");
 Delay_ms(100);

 UART2_Write_Text("ControllerConnected");
 Delay_ms(100);
 UART2_Write('\0');
}

void WiFiConnect(char *wifi)
{

 UART2_Write_Text("\r\nSTX\nWiFiConnect\n");
 Delay_ms(100);

 UART2_Write_Text(wifi);
 Delay_ms(100);
 UART2_Write('\0');
}

void WiFiSave(char *wifi)
{
 FLASH_Erase(0x4400);
 Delay_ms(100);
 FLASH_Write_Compact(0x4480, wifi);

 UART2_Write_Text("\r\nWiFi Saved\n");
}

void WiFiInit()
{
 int i;

 FLASH_Read_Compact(0x4480, flashData, 100);

 if (flashData[0] == 0xFF)
 return;

 for (i = 0; i < 100; i++)
 {
 if (flashData[i] == '\0')
 break;
 else if (i == 99)
 return;
 }

 WiFiConnect(flashData);
}

void apiLogin()
{

 UART2_Write_Text("\r\nSTX\nApiLogin\n");
 Delay_ms(100);

 UART2_Write_Text(apiCreds);
 Delay_ms(100);
 UART2_Write('\0');
}

void gsmReceive(char input)
{
 if (gsmStxStatus == 0 && input == '+')
 {
 gsmStxStatus++;
 }
 else if (gsmStxStatus == 1 && input == 'C')
 {
 gsmStxStatus++;
 }
 else if (gsmStxStatus == 2 && input == 'M')
 {
 gsmStxStatus++;
 }
 else if (gsmStxStatus == 3 && (input == 'T' || input == 'G'))
 {
 if (input == 'T')
 {
 gsmRecvType = 1;
 }
 else if (input == 'G')
 {
 gsmRecvType = 2;
 }

 gsmStxStatus++;
 }
 else if (gsmStxStatus == 4 && ((gsmRecvType == 1 && input == ':') || (gsmRecvType == 2 && input == 'S')))
 {
 gsmStxStatus++;
 }
 else if (gsmStxStatus == 5 && gsmRecvType == 2 && input == ':')
 {
 gsmStxStatus++;
 }
 else
 {
 gsmStxStatus = 0;
 }

 if ((gsmStxStatus == 5 && gsmRecvType == 1) || (gsmStxStatus == 6 && gsmRecvType == 2))
 {

 gsmStxStatus = 0;


 gsmRecvIndex = 0;


 gsmRecvStep = 1;

 LATB.RB15 = 1;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 else if (gsmRecvStep >= 1)
 {
 if (gsmRecvType == 1)
 {


 if (input == '"' && gsmRecvStep == 1)
 {
 gsmRecvStep++;
 }
 else if (gsmRecvStep == 2)
 {
 if (input == '"')
 {

 gsmSender[gsmRecvIndex] = '\0';


 gsmRecvIndex = 0;


 gsmRecvStep++;
 }
 else
 {
 gsmSender[gsmRecvIndex++] = input;
 }
 }
 else if (input == '"' && gsmRecvStep >= 3 && gsmRecvStep <= 5)
 {
 gsmRecvStep++;
 }
 else if (gsmRecvStep == 6)
 {
 if (input == '"')
 {
 gsmDatetime[gsmRecvIndex] = '\0';
 gsmRecvIndex = 0;
 gsmRecvStep++;
 }
 else
 {
 gsmDatetime[gsmRecvIndex++] = input;
 }
 }
 else if (input == '\n' && gsmRecvStep == 7)
 {
 gsmRecvStep++;
 }
 else if (gsmRecvStep == 8)
 {
 if (input == '\n')
 {
 gsmCommand[gsmRecvIndex] = '\0';
 gsmRecvIndex = 0;
 gsmRecvStep++;

 LATB.RB14 = 1;
 }
 else
 {
 gsmCommand[gsmRecvIndex++] = input;
 }
 }
 else if (gsmRecvStep >= 9)
 {
 if (strcmp(gsmCommand, "WiFiConnect") == 0)
 {
 LATB.RB13 = 1;

 if (gsmRecvStep == 9)
 {
 if (input == '\r')
 {
 gsmData[gsmRecvIndex] = '\0';
 gsmRecvIndex = 0;
 gsmRecvStep = 0;
 gsmRecvType = 0;

 LATB.RB12 = 1;

 WiFiConnect(gsmData);

 LATB.RB15 = 0;
 LATB.RB14 = 0;

 WiFiSave(gsmData);

 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 else
 {
 gsmData[gsmRecvIndex++] = input;
 }
 }
 }
 else if (strcmp(gsmCommand, "PLMS-UnitUpdate-CLZ") == 0)
 {
 LATB.RB13 = 1;

 if (gsmRecvStep == 9)
 {
 if (input == '\r')
 {
 gsmData[gsmRecvIndex] = '\0';
 gsmRecvIndex = 0;
 gsmRecvStep = 0;
 gsmRecvType = 0;

 LATB.RB12 = 1;

 UnitUpdate();

 LATB.RB15 = 0;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 else
 {
 gsmData[gsmRecvIndex++] = input;
 }
 }
 }
 else
 {
 gsmRecvStep = 0;
 gsmRecvType = 0;

 LATB.RB15 = 0;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 }
 }
 else if (gsmRecvType == 2)
 {


 if (input == '\r' && (gsmRecvStep == 1 || gsmRecvStep == 3))
 {
 gsmRecvStep++;
 }
 else if (input == '\n' && (gsmRecvStep == 2 || gsmRecvStep == 4))
 {
 gsmRecvStep++;
 }
 else if (input == 'O' && gsmRecvStep == 5)
 {
 gsmRecvStep++;
 }
 else if (input == 'K' && gsmRecvStep == 6)
 {
 gsmRecvStep = 0;
 gsmRecvType = 0;

 MessageSent();

 LATB.RB15 = 0;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 }
 }
}

void espReceive(char input)
{
 if (input == 'S' && espStxStatus == 0)
 {
 espStxStatus++;
 }
 else if (input == 'T' && espStxStatus == 1)
 {
 espStxStatus++;
 }
 else if (input == 'X' && espStxStatus == 2)
 {
 espStxStatus++;
 }
 else if (input == '\n' && espStxStatus == 3)
 {
 espStxStatus++;
 }
 else
 {
 espStxStatus = 0;
 }

 if (espStxStatus == 4)
 {

 espStxStatus = 0;


 espRecvIndex = 0;


 espRecvStep = 1;

 LATB.RB15 = 1;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 else if (espRecvStep == 1)
 {
 if (input == '\n')
 {

 espCommand[espRecvIndex] = '\0';


 espRecvIndex = 0;


 espRecvStep++;

 LATB.RB14 = 1;
 }
 else
 {
 espCommand[espRecvIndex++] = input;
 }
 }
 else if (espRecvStep >= 2)
 {
 if (strcmp(espCommand, "UnitRegister") == 0)
 {
 LATB.RB13 = 1;

 if (espRecvStep == 2)
 {
 if (input == '\0')
 {
 espData[espRecvIndex] = '\0';
 espRecvIndex = 0;
 espRecvStep = 0;

 LATB.RB12 = 1;

 ControllerConnected();

 LATB.RB15 = 0;
 LATB.RB14 = 0;

 UnitRegister();

 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 else
 {
 espData[espRecvIndex++] = input;
 }
 }
 }
 else if (strcmp(espCommand, "ApiLogin") == 0)
 {
 LATB.RB13 = 1;

 if (espRecvStep == 2)
 {
 if (input == '\0')
 {
 espData[espRecvIndex] = '\0';
 espRecvIndex = 0;
 espRecvStep = 0;

 LATB.RB12 = 1;

 apiLogin();

 LATB.RB15 = 0;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 else
 {
 espData[espRecvIndex++] = input;
 }
 }
 }
 else
 {
 espRecvStep = 0;

 LATB.RB15 = 0;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 }
 }
}

void main()
{

 Unlock_IOLOCK();
 PPS_Mapping_NoLock(9, _INPUT, _U2RX);
 PPS_Mapping_NoLock(8, _OUTPUT, _U2TX);
 PPS_Mapping_NoLock(7, _INPUT, _U1RX);
 PPS_Mapping_NoLock(6, _OUTPUT, _U1TX);
 Lock_IOLOCK();


 TRISB.RB15 = 0;
 TRISB.RB14 = 0;
 TRISB.RB13 = 0;
 TRISB.RB12 = 0;


 UART1_Init(9600);
 UART2_Init(9600);


 LATB.RB15 = 0;
 LATB.RB14 = 0;
 LATB.RB13 = 0;
 LATB.RB12 = 0;
 Delay_ms(5000);
 LATB.RB15 = 1;
 LATB.RB14 = 1;
 LATB.RB13 = 1;
 LATB.RB12 = 1;


 UART1_Write_Text("\r\nPIC UART1 Ready!\r\n");
 Delay_ms(100);
 UART2_Write_Text("\r\nPIC UART2 Ready!\r\n");
 Delay_ms(30000);


 WiFiInit();


 UART1_Write_Text("ATE0\r\n");
 UART1_Read_Text(gsmCommand, "OK", 255);
 LATB.RB12 = 0;
 UART1_Write_Text("AT+CMGD=1,4\r\n");
 UART1_Read_Text(gsmCommand, "OK", 255);
 LATB.RB13 = 0;
 UART1_Write_Text("AT+CNMI=3,2,0,1,1\r\n");
 UART1_Read_Text(gsmCommand, "OK", 255);
 LATB.RB14 = 0;
 UART1_Write_Text("AT+CMGF=1\r\n");
 UART1_Read_Text(gsmCommand, "OK", 255);
 LATB.RB15 = 0;

 while (1)
 {
 if (UART1_Data_Ready())
 {
 gsmReceive(UART1_Read());
 }

 if (UART2_Data_Ready())
 {
 espReceive(UART2_Read());
 }
 }
}
