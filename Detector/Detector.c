int gpsStxStatus = 0;
int gpsRecvIndex = 0;
int gpsRecvStep = 0;
int gpsReceived = 0;

int gsmStxStatus = 0;
int gsmRecvIndex = 0;
int gsmRecvStep = 0;

char gpsData[200];

char gsmSender[20];
char gsmDatetime[25];
char gsmCommand[200];
char gsmData[200];

char controllerNumber[20];

int status = 0;

void ControllerSave(char *controller)
{
    strcpy(controllerNumber, controller);

    FLASH_Erase(0x4400);
    Delay_ms(100);
    FLASH_Write_Compact(0x4480, controller);
}

void ControllerInit()
{
    int i;

    FLASH_Read_Compact(0x4480, controllerNumber, 20);

    if (controllerNumber[0] == 0xFF)
        return;

    for (i = 0; i < 20; i++)
    {
        if (controllerNumber[i] == '\0')
            break;
        else if (i == 19)
            return;
    }
}

void UnitUpdate()
{
    char *_status;

    // Check
    if (strlen(controllerNumber) == 0)
    {
        return;
    }

    // Process
    _status = (status == 0) ? "fault" : "normal";

    // Command
    UART1_Write_Text("\r\nAT+CMGS=\"");
    Delay_ms(100);
    // Receiver
    UART1_Write_Text(controllerNumber);
    Delay_ms(100);
    UART1_Write_Text("\"\r");
    Delay_ms(100);
    // Data
    UART1_Write_Text("PLMS-UnitUpdate-CLZ\n");
    Delay_ms(100);
    UART1_Write_Text(_status);
    Delay_ms(100);
    UART1_Write_Text("\n$GPRMC,");
    Delay_ms(100);
    UART1_Write_Text(gpsData);
    Delay_ms(100);
    UART1_Write('\x1A');
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
    else if (gsmStxStatus == 3 && input == 'T')
    {
        gsmStxStatus++;
    }
    else if (gsmStxStatus == 4 && input == ':')
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

        LATB.RB14 = 1;
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

            LATB.RB13 = 1;
        }
        else
        {
            gsmCommand[gsmRecvIndex++] = input;
        }
    }
    else if (gsmRecvStep >= 9)
    {
        if (strcmp(gsmCommand, "PLMS-UnitRegister-CLZ") == 0)
        {
            LATB.RB12 = 1;

            if (gsmRecvStep == 9)
            {
                if (input == '\r')
                {
                    LATB.RB11 = 1;

                    gsmData[gsmRecvIndex] = '\0';
                    gsmRecvIndex = 0;
                    gsmRecvStep = 0;

                    ControllerSave(gsmSender);

                    LATB.RB14 = 0;
                    LATB.RB13 = 0;

                    UnitUpdate();

                    LATB.RB12 = 0;
                    LATB.RB11 = 0;
                }
                else
                {
                    gsmData[gsmRecvIndex++] = input;
                }
            }
        }
        else if (strcmp(gsmCommand, "PLMS-UnitUpdate-CLZ") == 0)
        {
            LATB.RB12 = 1;

            if (gsmRecvStep == 9)
            {
                if (input == '\r')
                {
                    LATB.RB11 = 1;

                    gsmData[gsmRecvIndex] = '\0';
                    gsmRecvIndex = 0;
                    gsmRecvStep = 0;

                    if (strcmp(controllerNumber, gsmSender) == 0)
                        UnitUpdate();

                    LATB.RB14 = 0;
                    LATB.RB13 = 0;
                    LATB.RB12 = 0;
                    LATB.RB11 = 0;
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

            LATB.RB14 = 0;
            LATB.RB13 = 0;
            LATB.RB12 = 0;
            LATB.RB11 = 0;
        }
    }
}

void gpsReceive(char input)
{
    if (gpsStxStatus == 0 && input == '$')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 1 && input == 'G')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 2 && input == 'P')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 3 && input == 'R')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 4 && input == 'M')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 5 && input == 'C')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 6 && input == ',')
    {
        gpsStxStatus++;
    }
    else
    {
        gpsStxStatus = 0;
    }

    if (gpsStxStatus == 7)
    {
        // reset stx status
        gpsStxStatus = 0;

        // reset index
        gpsRecvIndex = 0;

        // proceed to step 1
        gpsRecvStep = 1;
    }
    else if (gpsRecvStep == 1)
    {
        // Latitude
        if (input == '\r')
        {
            gpsData[gpsRecvIndex++] = '\0';

            if (gpsRecvIndex > 50)
                gpsReceived = 1;

            gpsRecvIndex = 0;
            gpsRecvStep = 0;
        }
        else
        {
            gpsData[gpsRecvIndex++] = input;
        }
    }
}

void main()
{
    // Configure Peripherals
    Unlock_IOLOCK();
    PPS_Mapping_NoLock(9, _INPUT, _U2RX);  // GPS RX
    PPS_Mapping_NoLock(8, _OUTPUT, _U2TX); // GPS TX
    PPS_Mapping_NoLock(7, _INPUT, _U1RX);  // GSM RX
    PPS_Mapping_NoLock(6, _OUTPUT, _U1TX); // GSM TX
    Lock_IOLOCK();

    // Configure Ports
    TRISB.RB15 = 0;
    TRISB.RB14 = 0;
    TRISB.RB13 = 0;
    TRISB.RB12 = 0;
    TRISB.RB11 = 0;
    TRISB.RB10 = 0; // GPS Enable

    // Configure UART
    UART1_Init(9600); // GPS
    UART2_Init(9600); // GSM

    // LED Test
    LATB.RB15 = 0;
    LATB.RB14 = 0;
    LATB.RB13 = 0;
    LATB.RB12 = 0;
    LATB.RB11 = 0;
    LATB.RB10 = 0;
    Delay_ms(5000);
    LATB.RB15 = 1;
    LATB.RB14 = 1;
    LATB.RB13 = 1;
    LATB.RB12 = 1;
    LATB.RB11 = 1;

    // UART Ready
    UART1_Write_Text("\r\nPIC UART1 Ready!\r\n");
    UART2_Write_Text("\r\nPIC UART2 Ready!\r\n");

    // Initialize GPS
    LATB.RB10 = 1;
    do
    {
        if (UART2_Data_Ready())
        {
            gpsReceive(UART2_Read());
        }
    } while (gpsReceived == 0);
    LATB.RB10 = 0;
    UART2_Write_Text(gpsData);

    // Initialize Controller
    ControllerInit();

    // Setup GSM
    UART1_Write_Text("ATE0\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB14 = 0;
    UART1_Write_Text("AT+CMGD=1,4\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB13 = 0;
    UART1_Write_Text("AT+CNMI=3,2,0,1,1\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB12 = 0;
    UART1_Write_Text("AT+CMGF=1\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    LATB.RB11 = 0;

    while (1)
    {
        if (UART1_Data_Ready())
        {
            gsmReceive(UART1_Read());
        }
    }
}