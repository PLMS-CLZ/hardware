#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <WiFiClientSecureBearSSL.h>

#define picRx 14 // D5
#define picTx 12 // D6

SoftwareSerial picSerial;
WiFiClient wifiClient;
HTTPClient httpClient;
PubSubClient mqttClient(wifiClient);
DynamicJsonDocument jsonData(1024);

const char *mqtt_broker = "test.mosquitto.org";

long lastWifiOnUpdate = 0;
long lastWifiOffUpdate = 0;
long lastReconnectAttempt = 0;

char apiEmail[50];
char apiPassword[50];
char apiToken[50];

int picStxStatus = 0;
int picRecvIndex = 0;
int picRecvStep = 0;

char picCommand[50];
char picData[100];

char ssid[50];
char password[50];
char mqttTopic[50];
char jsonSerial[1024];

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
            mqttClient.subscribe("PLMS-ControllerCommands-CLZ");
        }

        if (isMqttConnected())
        {
            lastReconnectAttempt = 0;

            Serial.println("Connected to Laravel API");
        }
    }
}

void apiLogin()
{
    Serial.println("Logging in...");

    std::unique_ptr<BearSSL::WiFiClientSecure> wifiClientSecure(new BearSSL::WiFiClientSecure);
    wifiClientSecure->setInsecure();

    httpClient.useHTTP10(true);

    if (httpClient.begin(*wifiClientSecure, "https://plms-clz.herokuapp.com/api/auth/user/login"))
    {
        httpClient.addHeader("Accept", "application/json");
        httpClient.addHeader("Content-Type", "application/json");

        jsonData.clear();
        jsonData["email"] = apiEmail;
        jsonData["password"] = apiPassword;
        serializeJson(jsonData, jsonSerial);

        int responseCode = httpClient.POST(jsonSerial);

        Serial.print("Response Code: ");
        Serial.println(responseCode);

        if (responseCode == HTTP_CODE_OK)
        {
            deserializeJson(jsonData, httpClient.getString());
            String response = jsonData["token"];
            response.toCharArray(apiToken, response.length());
            Serial.println(apiToken);
        }
        else
        {
            Serial.println(httpClient.getString());
        }

        httpClient.end();
    }
}

void mqttReceive(char *topic, byte *payload, unsigned int length)
{
    picSerial.print("STX\n");
    Serial.print("MQTT STX\n");
    for (int i = 0; i < length; i++)
    {
        picSerial.write(payload[i]);
        Serial.write(payload[i]);
    }
    picSerial.write('\0');
    Serial.print("\0");
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
                    picRecvIndex = 0;
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
        else if (strcmp(picCommand, "ApiLogin") == 0)
        {
            if (picRecvStep == 2)
            {
                if (input == '\n')
                {
                    apiEmail[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep++;
                }
                else
                {
                    apiEmail[picRecvIndex++] = input;
                }
            }
            else if (picRecvStep == 3)
            {
                if (input == '\0')
                {
                    apiPassword[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep = 0;

                    apiLogin();
                }
                else
                {
                    apiPassword[picRecvIndex++] = input;
                }
            }
        }
        else if (strcmp(picCommand, "MqttPublish") == 0)
        {
            if (picRecvStep == 2)
            {
                if (input == '\n')
                {
                    mqttTopic[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep++;
                }
                else
                {
                    mqttTopic[picRecvIndex++] = input;
                }
            }
            else if (picRecvStep == 3)
            {
                if (input == '\0')
                {
                    picData[picRecvIndex] = '\0';
                    picRecvIndex = 0;
                    picRecvStep = 0;

                    mqttClient.publish(mqttTopic, picData);
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
