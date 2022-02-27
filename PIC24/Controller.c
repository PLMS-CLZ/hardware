int espStxStatus = 0;
int espRecvIndex = 0;
int espRecvStep = 0;

int gsmStxStatus = 0;
int gsmRecvIndex = 0;
int gsmRecvStep = 0;

char espCommand[50];
char espData[100];

char gsmSender[20];
char gsmDatetime[25];
char gsmCommand[50];
char gsmData[100];

char flashData[100];

void UnitRegister()
{
    UART1_Write_Text("AT+CMGS=\"");
    Delay_ms(100);
    UART1_Write_Text(espData);
    Delay_ms(100);
    UART1_Write_Text("\"\r");
    Delay_ms(100);
    UART1_Write_Text("PLMS-UnitRegister-CLZ\x1A");
}

void UnitRegisterResponse()
{
    UART2_Write_Text("\r\nSTX\nUnitRegisterResponse\n");
    Delay_ms(100);
    UART2_Write_Text(gsmSender);
    Delay_ms(100);
    UART2_Write('\n');
    Delay_ms(100);
    UART2_Write_Text(gsmData);
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
    UART2_Write_Text("\r\nWiFi Saving\n");

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

void gsmReceive(char input)
{
    if (input == '+' && gsmStxStatus == 0)
    {
        gsmStxStatus++;
    }
    else if (input == 'C' && gsmStxStatus == 1)
    {
        gsmStxStatus++;
    }
    else if (input == 'M' && gsmStxStatus == 2)
    {
        gsmStxStatus++;
    }
    else if (input == 'T' && gsmStxStatus == 3)
    {
        gsmStxStatus++;
    }
    else if (input == ':' && gsmStxStatus == 4)
    {
        gsmStxStatus++;
    }
    else
    {
        gsmStxStatus = 0;
    }

    if (gsmStxStatus == 5)
    {
        // reset stx status
        gsmStxStatus = 0;

        // reset index
        gsmRecvIndex = 0;

        // proceed to step 1
        gsmRecvStep = 1;

        LATB.RB15 = 1;
        LATB.RB14 = 0;
        LATB.RB13 = 0;
        LATB.RB12 = 0;
    }
    else if (input == '"' && gsmRecvStep == 1)
    {
        gsmRecvStep++;
    }
    else if (gsmRecvStep == 2)
    {
        if (input == '"')
        {
            // add terminating char
            gsmSender[gsmRecvIndex] = '\0';

            // reset index
            gsmRecvIndex = 0;

            // proceed to next step
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
        else if (strcmp(gsmCommand, "PLMS-UnitRegisterResponse-CLZ") == 0)
        {
            LATB.RB13 = 1;

            if (gsmRecvStep == 9)
            {
                if (input == '\r')
                {
                    gsmData[gsmRecvIndex] = '\0';
                    gsmRecvIndex = 0;
                    gsmRecvStep = 0;

                    UnitRegisterResponse();

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

            LATB.RB15 = 0;
            LATB.RB14 = 0;
            LATB.RB13 = 0;
            LATB.RB12 = 0;
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
        // reset stx status
        espStxStatus = 0;

        // reset index
        espRecvIndex = 0;

        // proceed to step 1
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
            // add terminating char
            espCommand[espRecvIndex] = '\0';

            // reset index
            espRecvIndex = 0;

            // proceed to next step
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
        if (strcmp(espCommand, "plms-clz/units/register") == 0)
        {
            LATB.RB13 = 1;

            if (espRecvStep == 2)
            {
                if (input == '\0')
                {
                    espData[espRecvIndex] = '\0';
                    espRecvIndex = 0;
                    espRecvStep = 0;

                    UnitRegister();

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
    // Configure Peripherals
    Unlock_IOLOCK();
    PPS_Mapping_NoLock(9, _INPUT, _U1RX);  // GSM RX
    PPS_Mapping_NoLock(8, _OUTPUT, _U1TX); // GSM TX
    PPS_Mapping_NoLock(7, _INPUT, _U2RX);  // NodeMCU RX
    PPS_Mapping_NoLock(6, _OUTPUT, _U2TX); // NodeMCU TX
    Lock_IOLOCK();

    // Configure Ports
    TRISB.RB15 = 0;
    TRISB.RB14 = 0;
    TRISB.RB13 = 0;
    TRISB.RB12 = 0;

    // Configure UART
    UART1_Init(9600); // GSM
    UART2_Init(9600); // NodeMCU

    // LED Test
    LATB.RB15 = 0;
    LATB.RB14 = 0;
    LATB.RB13 = 0;
    LATB.RB12 = 0;
    Delay_ms(5000);
    LATB.RB15 = 1;
    LATB.RB14 = 1;
    LATB.RB13 = 1;
    LATB.RB12 = 1;

    // UART Ready
    UART1_Write_Text("PIC UART1 Ready!");
    Delay_ms(100);
    UART2_Write_Text("PIC UART2 Ready!");
    Delay_ms(30000);

    // Setup GSM
    LATB.RB12 = 0;
    UART1_Write_Text("ATE0\r");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB13 = 0;
    UART1_Write_Text("AT+CMGF=1\r");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB14 = 0;
    UART1_Write_Text("AT+CNMI=1,2,0,0,0\r");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB15 = 0;

    WiFiInit();

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