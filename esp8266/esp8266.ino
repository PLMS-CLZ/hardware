#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>

#define picRx 14 // D5
#define picTx 12 // D6

SoftwareSerial picSerial;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
DynamicJsonDocument jsonData(200);

const char *mqtt_broker = "test.mosquitto.org";

long lastWifiOnUpdate = 0;
long lastWifiOffUpdate = 0;
long lastReconnectAttempt = 0;

int picStxStatus = 0;
int picRecvIndex = 0;
int picRecvStep = 0;

char picCommand[50];
char picData[100];

char ssid[50];
char password[50];

char jsonSerial[200];

void setup()
{
    Serial.begin(9600);

    // baud frame/parity/stop rx tx inverse
    picSerial.begin(9600, SWSERIAL_8N1, picRx, picTx, false);

    pinMode(LED_BUILTIN, OUTPUT);

    WiFi.mode(WIFI_STA);

    mqttClient.setServer(mqtt_broker, 1883);
    mqttClient.setCallback(mqttReceive);
}

void loop()
{
    wifiStatusLed();

    if (picSerial.available() > 0)
    {
        char x = picSerial.read();
        Serial.write(x);
        picReceive(x);
    }

    if (isWifiConnected())
    {
        if (isMqttConnected())
        {
            mqttClient.loop();
        }
        else
        {
            Serial.print("Connecting to ");
            Serial.print(ssid);
            Serial.print(" ");
            Serial.print(password);
            Serial.println("...");

            mqttConnect();
        }
    }
}

bool isWifiConnected()
{
    return WiFi.status() == WL_CONNECTED;
}

bool isMqttConnected()
{
    return mqttClient.connected();
}

void wifiStatusLed()
{
    long now = millis();

    if (now - lastWifiOnUpdate > 1000)
    {
        lastWifiOnUpdate = now;

        digitalWrite(LED_BUILTIN, LOW);
    }

    if (now - lastWifiOffUpdate > 250)
    {
        lastWifiOffUpdate = now;

        if (!isWifiConnected())
        {
            digitalWrite(LED_BUILTIN, HIGH);
        }
    }
}

void mqttConnect()
{
    long now = millis();

    if (now - lastReconnectAttempt > 5000)
    {
        lastReconnectAttempt = now;

        Serial.println("Connecting to Laravel API...");

        // Attempt to reconnect
        if (mqttClient.connect("maincontroller"))
        {
            mqttClient.subscribe("plms-clz/units/register");
        }

        if (isMqttConnected())
        {
            lastReconnectAttempt = 0;

            Serial.println("Connected to Laravel API");
        }
    }
}

void mqttReceive(char *topic, byte *payload, unsigned int length)
{
    // Send to PIC
    picSerial.print("STX\n");
    picSerial.print(topic);
    picSerial.write('\n');
    for (int i = 0; i < length; i++)
        picSerial.write(payload[i]);
    picSerial.write('\0');

    // Send to Debug
    Serial.println("STX");
    Serial.println(topic);
    for (int i = 0; i < length; i++)
        Serial.write(payload[i]);
    Serial.println();
}

void picReceive(char input)
{
    if (input == 'S' && picRecvStep == 0)
    {
        picStxStatus++;
    }
    else if (input == 'T' && picStxStatus == 1)
    {
        picStxStatus++;
    }
    else if (input == 'X' && picStxStatus == 2)
    {
        picStxStatus++;
    }
    else if (input == '\n' && picStxStatus == 3)
    {
        picStxStatus++;
    }

    if (picStxStatus == 4)
    {
        // reset stx status
        picStxStatus = 0;

        // reset index
        picRecvIndex = 0;

        // begin receive step
        picRecvStep = 1;
    }
    else if (picRecvStep == 1)
    {
        if (input == '\n')
        {
            // add terminating char
            picCommand[picRecvIndex] = '\0';

            // reset index
            picRecvIndex = 0;

            // proceed to next step
            picRecvStep++;
        }
        else
        {
            picCommand[picRecvIndex++] = input;
        }
    }
    else if (picRecvStep >= 2)
    {
        if (strcmp(picCommand, "WiFiConnect") == 0)
        {
            if (picRecvStep == 2)
            {
                if (input == '\n')
                {
                    ssid[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep++;
                }
                else
                {
                    ssid[picRecvIndex++] = input;
                }
            }
            else if (picRecvStep == 3)
            {
                if (input == '\0')
                {
                    password[picRecvIndex] = '\0';
                    picRecvStep = 0;

                    // connect to wifi
                    WiFi.begin(ssid, password);
                }
                else
                {
                    password[picRecvIndex++] = input;
                }
            }
        }
        else if (strcmp(picCommand, "UnitRegisterResponse") == 0)
        {
            if (picRecvStep == 2)
            {
                if (input == '\n')
                {
                    picData[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep++;

                    jsonData.clear();
                    jsonData["phone_number"] = String(picData);
                }
                else
                {
                    picData[picRecvIndex++] = input;
                }
            }
            else if (picRecvStep == 3)
            {
                if (input == '\n')
                {
                    picData[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep++;

                    jsonData["active"] = String(picData).toInt();
                }
                else
                {
                    picData[picRecvIndex++] = input;
                }
            }
            else if (picRecvStep == 4)
            {
                if (input == '\n')
                {
                    picData[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep++;

                    jsonData["latitude"] = String(picData).toDouble();
                }
                else
                {
                    picData[picRecvIndex++] = input;
                }
            }
            else if (picRecvStep == 5)
            {
                if (input == '\0')
                {
                    picData[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep = 0;

                    jsonData["longitude"] = String(picData).toDouble();

                    serializeJson(jsonData, jsonSerial);

                    mqttClient.publish("plms-clz/units/response", jsonSerial);
                }
                else
                {
                    picData[picRecvIndex++] = input;
                }
            }
        }
        else
        {
            // Unknown command, terminate
            picRecvStep = 0;
        }
    }
}
