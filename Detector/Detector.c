int gpsStxStatus = 0;
int gpsRecvIndex = 0;
int gpsRecvStep = 0;

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
float gpsLatitude;
float gpsLongitude;

void ControllerSave(char *controller)
{
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
    char *_status, _gpsLatitude[20], _gpsLongitude[20];

    // Check
    if (strlen(controllerNumber) == 0)
    {
        return;
    }

    // Process
    _status = (status == 0) ? "fault" : "normal";
    FloatToStr(gpsLatitude, _gpsLatitude);
    FloatToStr(gpsLongitude, _gpsLongitude);

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
    UART1_Write('\n');
    Delay_ms(100);
    UART1_Write_Text(_gpsLatitude);
    Delay_ms(100);
    UART1_Write('\n');
    Delay_ms(100);
    UART1_Write_Text(_gpsLongitude);
    Delay_ms(100);
    UART1_Write('\x1A');
}

float parseGPS(const char *position)
{
    float degrees = 0;

    if (strlen(position) > 5)
    {
        char degreesPart[4];
        int degreesCount = position[4] == '.' ? 2 : 3;

        memcpy(degreesPart, position, degreesCount);
        degreesPart[degreesCount] = 0;

        // Move pointer to minutes part
        position += degreesCount;

        // add degrees and minutes
        degrees = atoi(degreesPart) + atof(position) / 60.0;
    }

    return degrees;
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
        if (strcmp(gsmCommand, "PLMS-UnitRegister-CLZ") == 0)
        {
            LATB.RB13 = 1;

            if (gsmRecvStep == 9)
            {
                if (input == '\r')
                {
                    gsmData[gsmRecvIndex] = '\0';
                    gsmRecvIndex = 0;
                    gsmRecvStep = 0;

                    LATB.RB12 = 1;

                    ControllerSave(gsmSender);

                    LATB.RB15 = 0;
                    LATB.RB14 = 0;

                    UnitUpdate();

                    LATB.RB13 = 0;
                    LATB.RB12 = 0;
                }
                else
                {
                    gsmData[gsmRecvIndex++] = input;
                }
            }
        }
        if (strcmp(gsmCommand, "PLMS-UnitUpdate-CLZ") == 0)
        {
            LATB.RB13 = 1;

            if (gsmRecvStep == 9)
            {
                if (input == '\r')
                {
                    gsmData[gsmRecvIndex] = '\0';
                    gsmRecvIndex = 0;
                    gsmRecvStep = 0;

                    LATB.RB12 = 1;

                    if (strcmp(controllerNumber, gsmSender) == 0)
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

            LATB.RB15 = 0;
            LATB.RB14 = 0;
            LATB.RB13 = 0;
            LATB.RB12 = 0;
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
    else if (gpsStxStatus == 3 && input == 'G')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 4 && input == 'L')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 5 && input == 'L')
    {
        gpsStxStatus++;
    }
    else if (gpsStxStatus == 5 && input == ',')
    {
        gpsStxStatus++;
    }
    else
    {
        gpsStxStatus = 0;
    }

    if (gpsStxStatus == 6)
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
        if (input == ',')
        {
            gpsData[gpsRecvIndex] = '\0';
            gpsRecvIndex = 0;
            gpsRecvStep++;

            gpsLatitude = parseGPS(gpsData);
        }
        else
        {
            gpsData[gpsRecvIndex++] = input;
        }
    }
    else if (gpsRecvStep == 2)
    {
        // Latitude Direction
        if (input == ',')
        {
            gpsData[gpsRecvIndex] = '\0';
            gpsRecvIndex = 0;
            gpsRecvStep++;

            if (strcmp(gpsData, 'S') == 0)
            {
                gpsLatitude = -gpsLatitude;
            }
        }
        else
        {
            gpsData[gpsRecvIndex++] = input;
        }
    }
    else if (gpsRecvStep == 3)
    {
        // Latitude
        if (input == ',')
        {
            gpsData[gpsRecvIndex] = '\0';
            gpsRecvIndex = 0;
            gpsRecvStep++;

            gpsLongitude = parseGPS(gpsData);
        }
        else
        {
            gpsData[gpsRecvIndex++] = input;
        }
    }
    else if (gpsRecvStep == 4)
    {
        // Latitude Direction
        if (input == ',')
        {
            gpsData[gpsRecvIndex] = '\0';
            gpsRecvIndex = 0;
            gpsRecvStep = 0;

            if (strcmp(gpsData, 'W') == 0)
            {
                gpsLongitude = -gpsLongitude;
            }
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
    PPS_Mapping_NoLock(9, _INPUT, _U1RX);  // GSM RX
    PPS_Mapping_NoLock(8, _OUTPUT, _U1TX); // GSM TX
    PPS_Mapping_NoLock(7, _INPUT, _U2RX);  // GPS RX
    PPS_Mapping_NoLock(6, _OUTPUT, _U2TX); // GPS TX
    Lock_IOLOCK();

    // Configure Ports
    TRISB.RB15 = 0;
    TRISB.RB14 = 0;
    TRISB.RB13 = 0;
    TRISB.RB12 = 0;

    // Configure UART
    UART1_Init(9600); // GSM
    UART2_Init(9600); // GPS

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
    UART1_Write_Text("\r\nPIC UART1 Ready!\r\n");
    Delay_ms(100);
    UART2_Write_Text("\r\nPIC UART2 Ready!\r\n");
    Delay_ms(30000);

    // Initialize Controller
    ControllerInit();

    // Setup GSM
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
            gpsReceive(UART2_Read());
        }
    }
}