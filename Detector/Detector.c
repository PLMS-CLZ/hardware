int gpsStxStatus = 0;
int gpsRecvIndex = 0;
int gpsRecvStep = 0;
int gpsReceived = 0;
char gpsData[1000];

int gsmStxStatus = 0;
int gsmRecvType = 0; // Type 1 = Text Received; Type 2 = Text Result
int gsmRecvIndex = 0;
int gsmRecvStep = 0;
int gsmSending = 0;
char gsmSender[1000];
char gsmDatetime[1000];
char gsmCommand[1000];
char gsmData[1000];

char controllerNumber[20];

int vStatus = -1;
int vSamples = 0;
int minSample, maxSample;

void controllerInit();
void controllerSave();
void getGPS();
void gpsReceive(int input);
void gsmReceive(int input);
void senseVoltage();
void updateUnit();

void main()
{
    // Configure Peripherals
    Unlock_IOLOCK();
    PPS_Mapping_NoLock(9, _INPUT, _U2RX);  // GPS RX
    PPS_Mapping_NoLock(5, _OUTPUT, _U2TX); // Debug TX
    PPS_Mapping_NoLock(7, _INPUT, _U1RX);  // GSM RX
    PPS_Mapping_NoLock(6, _OUTPUT, _U1TX); // GSM TX
    Lock_IOLOCK();

    // Configure Ports
    TRISB.RB0 = 1; // ADC
    TRISB.RB15 = 0;
    TRISB.RB14 = 0;
    TRISB.RB13 = 0;
    TRISB.RB12 = 0;
    TRISB.RB11 = 0;
    TRISB.RB10 = 0;
    TRISB.RB8 = 0; // GPS Enable

    // Initialize ADC
    ADC1_Init();

    // Initialize UART
    UART1_Init(9600); // GPS
    UART2_Init(9600); // GSM

    // LED Test
    LATB.RB15 = 1;
    LATB.RB14 = 1;
    LATB.RB13 = 1;
    LATB.RB12 = 1;
    LATB.RB11 = 1;
    LATB.RB10 = 1;
    Delay_ms(5000);
    LATB.RB14 = 0;
    LATB.RB13 = 0;
    LATB.RB12 = 0;
    LATB.RB11 = 0;
    LATB.RB10 = 0;
    
    // Clear WDT
    asm CLRWDT;

    // UART
    UART1_Write_Text("\r\nPIC UART1 Ready!\r\n");
    UART2_Write_Text("\r\nPIC UART2 Ready!\r\n");

    // GPS
    getGPS();

    LATB.RB14 = 1;
    UART2_Write_Text("GPS: ");
    UART2_Write_Text(gpsData);
    UART2_Write('\n');

    // GSM
    UART1_Write_Text("ATE0\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    UART1_Write_Text("AT+CMGD=1,4\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    UART1_Write_Text("AT+CNMI=3,2,0,1,1\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);
    UART1_Write_Text("AT+CMGF=1\r\n");
    UART1_Read_Text(gsmCommand, "OK", 255);

    asm CLRWDT;

    LATB.RB13 = 1;
    UART2_Write_Text("GSM Configured\n");

    // Controller
    controllerInit();

    UART2_Write_Text("Controller: ");
    UART2_Write_Text(controllerNumber);
    UART2_Write('\n');

    while (1)
    {
        if (UART1_Data_Ready())
        {
            gsmReceive(UART1_Read());
        }
        else if (gsmRecvStep == 0 && gsmStxStatus == 0 && gsmSending == 0)
        {
            senseVoltage();
        }
        
        asm CLRWDT;
    }
}

void ControllerSave()
{
    LATB.RB12 = 0;

    strcpy(controllerNumber, gsmSender);

    FLASH_Erase(0x4400);
    Delay_ms(100);
    FLASH_Write_Compact(0x4480, gsmSender);

    LATB.RB12 = 1;

    UART2_Write_Text("Controller Saved\n");
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

    LATB.RB12 = 1;
}

void updateUnit()
{
    int i;
    char *_status;

    // Check
    if (controllerNumber[0] != '+')
        return;

    getGPS();

    _status = (vStatus == 0) ? "fault" : "normal";

    // Command
    UART1_Write_Text("\r\nAT+CMGS=\"");
    Delay_ms(100);
    UART1_Write_Text(controllerNumber);
    Delay_ms(100);
    UART1_Write_Text("\"\r");
    UART1_Read_Text(gsmData, ">", 255);
    // Data
    UART1_Write_Text("PLMS-UpdateUnit-CLZ\n");
    Delay_ms(100);
    UART1_Write_Text(_status);
    Delay_ms(100);
    UART1_Write_Text("\n$GPRMC,");
    Delay_ms(100);
    for (i = 0; i < 100; i++)
    {
        if (gpsData[i] == '\0')
            break;

        UART1_Write(gpsData[i]);
        Delay_ms(20);
    }
    UART1_Write('\x1A');

    gsmSending = 1;

    UART2_Write_Text("Sending Message\n");
}

void senseVoltage()
{
    float voltage;
    int curSample = ADC1_Get_Sample(2);

    if (vSamples == 0)
    {
        // Set initial
        maxSample = minSample = curSample;
    }
    else if (vSamples < 50)
    {
        if (curSample > maxSample)
            maxSample = curSample;
        if (curSample < minSample)
            minSample = curSample;
    }

    if (vSamples == 50)
    {
        // Reset
        vSamples = 0;

        // 0 - 512 range
        minSample = 512 - minSample;
        maxSample = maxSample - 512;

        // Get average
        voltage = (maxSample + minSample) / 2.0;

        // Apply factor
        voltage *= 1.157894;

        if (vStatus != 1 && voltage > 200)
        {
            vStatus = 1;

            UART2_Write_Text("Power Restored\n");

            LATB.RB10 = 1;

            updateUnit();
        }
        else if (vStatus != 0 && voltage < 100)
        {
            vStatus = 0;

            UART2_Write_Text("Outage Detected\n");

            LATB.RB10 = 0;

            updateUnit();
        }
    }
    else
    {
        vSamples++;
    }
}

void getGPS()
{
    gpsReceived = 0;
    LATB.RB8 = 1;

    UART2_Write_Text("Getting Location\n");

    do
    {
        if (UART2_Data_Ready())
        {
            gpsReceive(UART2_Read());
            asm CLRWDT;
        }
    } while (gpsReceived == 0);
    LATB.RB8 = 0;
}

void gpsReceive(int input)
{
    if (input < 0)
        return;

    if (input == '$')
    {
        gpsStxStatus = 1;
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
        if (input == '\r')
        {
            gpsData[gpsRecvIndex++] = '\0';

            if (gpsRecvIndex > 55)
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

void gsmReceive(int input)
{
    if (input < 0)
        return;

    if (input == '+')
    {
        gsmStxStatus = 1;
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

        gsmRecvStep = 0;
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
        // reset stx status
        gsmStxStatus = 0;

        // reset index
        gsmRecvIndex = 0;

        // proceed to step 1
        gsmRecvStep = 1;

        LATB.RB11 = 1;
    }
    else if (gsmRecvStep >= 1)
    {
        if (gsmRecvType == 1)
        {
            // CMT Region (Text Messages received by GSM)

            if (input == '"' && gsmRecvStep == 1)
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
                    if (gsmRecvStep == 9)
                    {
                        if (input == '\r')
                        {
                            gsmData[gsmRecvIndex] = '\0';
                            gsmRecvIndex = 0;
                            gsmRecvStep = 0;
                            gsmRecvType = 0;

                            controllerSave();
                            updateUnit();

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
                    if (gsmRecvStep == 9)
                    {
                        if (input == '\r')
                        {
                            gsmData[gsmRecvIndex] = '\0';
                            gsmRecvIndex = 0;
                            gsmRecvStep = 0;
                            gsmRecvType = 0;

                            if (strcmp(controllerNumber, gsmSender) == 0)
                                updateUnit();

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
                    gsmRecvType = 0;
                    LATB.RB11 = 0;
                }
            }
        }
        else if (gsmRecvType == 2)
        {
            // CMGS Result Region

            gsmSending = 0;

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

                LATB.RB11 = 0;

                UART2_Write_Text("Message Sent\n");
            }
        }
    }
}