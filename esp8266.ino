#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char *mqtt_broker = "test.mosquitto.org";

long lastWifiOnUpdate = 0;
long lastWifiOffUpdate = 0;
long lastReconnectAttempt = 0;

int picStxStatus = 0;
int picRecvIndex = 0;
int picRecvStep = 0;

char picCommand[100];

char ssid[50];
char password[50];

void setup()
{
  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.mode(WIFI_STA);

  mqttClient.setServer(mqtt_broker, 1883);
  mqttClient.setCallback(mqttReceive);
}

void loop()
{
  wifiStatusLed();

  if (Serial.available() > 0)
  {
    picReceive((char)Serial.read());
  }

  if (isWifiConnected())
  {
    if (isMqttConnected())
    {
      mqttClient.loop();
    }
    else
    {
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

    // Attempt to reconnect
    if (mqttClient.connect("maincontroller"))
    {
      mqttClient.subscribe("plms-clz/units/register");
    }

    if (isMqttConnected())
    {
      lastReconnectAttempt = 0;
    }
  }
}

void mqttReceive(char *topic, byte *payload, unsigned int length)
{
  // Send to PIC
  Serial.println("STX");
  Serial.println(topic);
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println("ETX");
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
        if (input == '\n')
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
    else
    {
      // Unknown command, terminate
      picRecvStep = 0;
    }
  }
}
