//***************************************************************************
// Arduino Sketch for homed
// File main.ino
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 2020-2021 - Jörg Wendel
//***************************************************************************

#include <ADS1115_WE.h>
#include <Wire.h>
#define I2C_ADDRESS 0x48
ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

bool adsPresent {false};

#include <WiFiNINA.h>
#include <MQTT.h>
#include <ArduinoJson.h>

#include "Sodaq_wdt.h"          // the watchdog

#include "settings.h"           // set your WiFi an MQTT connection data here!

const char* topicIn  {TOPIC "/arduino/in"};
const char* topicOut {TOPIC "/arduino/out"};
const char* mqttUrl  {MQTT_URL};

int ledPin {LED_BUILTIN};
unsigned int updateInterval {30 * 1000};  // update/push sensor data every x seconds
unsigned long lastUpdateAt {0};

const byte inputCount {7};
unsigned int sampleCount[inputCount] {0};
unsigned int sampleSum[inputCount] {0};

const unsigned short adsInputOffset {inputCount};   // offset vor name (A7, A8, ...)
const byte inputCountAds {4};
unsigned int sampleCountAds[inputCountAds] {0};
float sampleSumAds[inputCountAds] {0};

// MQTT

WiFiClient net;
MQTTClient mqtt(1024);

// NTP

#ifdef _USE_NTP
  #include <NTPClient.h>
  WiFiUDP ntpUDP;
  // NTPClient       (WiFiUDP, server, offset [s], update interval [ms])
  NTPClient ntpClient(ntpUDP, "europe.pool.ntp.org", 0, 60 * 1000);
  bool isTimeSet {false};
#endif

//***************************************************************************
// RGB LED
//***************************************************************************

void rgbLed(byte r, byte g, byte b)
{
   WiFiDrv::analogWrite(25, r);   // green
   WiFiDrv::analogWrite(26, g);   // red
   WiFiDrv::analogWrite(27, b);   // blue
}

//***************************************************************************
// Setup
//***************************************************************************

void setup()
{
   // LED

   pinMode(ledPin, OUTPUT);
   digitalWrite(ledPin, LOW);

   // rgb LED

   WiFiDrv::pinMode(25, OUTPUT);
   WiFiDrv::pinMode(26, OUTPUT);
   WiFiDrv::pinMode(27, OUTPUT);
   rgbLed(20, 0, 0);

   // setup ADS1115

   analogReadResolution(12);

   // setup ADS1115

   Wire.begin();

   if (adc.init())
   {
      adsPresent = true;

      adc.setCompareChannels(ADS1115_COMP_0_GND);
      adc.setConvRate(ADS1115_128_SPS);
      adc.setMeasureMode(ADS1115_CONTINUOUS);
      adc.setPermanentAutoRangeMode(true);
   }

   // check/setup serial interface

   unsigned long timeoutAt = millis() + 1500;
   Serial.begin(115200);

   while (!Serial)
   {
      if (millis() > timeoutAt)
         break;
   }

   // #> sudo minicom -D /dev/ttyACM0 -b 115200

   connect();
   started();
}

//******************************************************************
// Connect WiFi and MQTT
//******************************************************************

void connect()
{
   byte r {20};
   byte b {20};

   sodaq_wdt_disable();
   sodaq_wdt_enable(WDT_PERIOD_8X);  // enable Watchdog with period of 8 seconds

   if (Serial) Serial.println("Connecting to WiFi '" + String(wifiSsid) + "' ..");

   while (!net.connected() || WiFi.status() != WL_CONNECTED)
   {
      mqtt.disconnect();
      WiFi.disconnect();
      //ntpClient.end();

      rgbLed(r, 0, 0);
      delay(500);

      while (WiFi.begin(wifiSsid, wifiPass) != WL_CONNECTED)
      {
         r = r ? 0 : 20;
         rgbLed(r, 0, 0);
         delay(200);
      }

      if (Serial) Serial.println("Connected to WiFi!");
      mqtt.begin(mqttServer, mqttPort, net);
      if (Serial) Serial.println("Connecting to MQTT broker '" + String(mqttServer) + ":" + mqttPort + "' ..");

      sodaq_wdt_reset();

      rgbLed(0, 0, b);
      delay(500);

      while (WiFi.status() == WL_CONNECTED && !mqtt.connect(mqttUrl, key, secret))
      {
         b = b ? 0 : 20;
         rgbLed(0, 0, b);
         delay(200);
      }

      if (Serial) Serial.println("Connected to MQTT broker");

      mqtt.loop();
      delay(500);
   }

   mqtt.onMessage(atMessage);
   mqtt.subscribe(topicIn);

#ifdef _USE_NTP
   ntpClient.begin();    // start NTP
   delay(500);
   isTimeSet = ntpClient.forceUpdate();
#endif

   sodaq_wdt_disable();
   sodaq_wdt_enable(WDT_PERIOD_2X);  // enable Watchdog with period of 2 seconds
}

//******************************************************************
// Integer To Analog Pin
//******************************************************************

int intToA(int num)
{
   switch (num)
   {
      case 0: return A0;
      case 1: return A1;
      case 2: return A2;
      case 3: return A3;
      case 4: return A4;
      case 5: return A5;
      case 6: return A6;
   }

   return A0;
}

ADS1115_MUX intToAds(int num)
{
   switch (num)
   {
      case 0: return ADS1115_COMP_0_GND;
      case 1: return ADS1115_COMP_1_GND;
      case 2: return ADS1115_COMP_2_GND;
      case 3: return ADS1115_COMP_3_GND;
   }

   return ADS1115_COMP_0_GND;
}

//***************************************************************************
// Loop
//***************************************************************************

unsigned long nextAt = 0;

void loop()
{
   mqtt.loop();

   if (nextAt > millis())
      return;

   // we pass here every 500ms

   nextAt = millis() + 500;    // schedule next in 500ms

   // trigger watchdog

   sodaq_wdt_reset();

   // refresh NTP

#ifdef _USE_NTP
   isTimeSet += ntpClient.update();
#endif

   // check connection

   if (!net.connected() || WiFi.status() != WL_CONNECTED)
   {
      digitalWrite(ledPin, LOW);
      connect();
   }

   // flash green

   static byte g {0};
   g = g ? 0 : 20;
   rgbLed(0, g, 0);
   digitalWrite(ledPin, HIGH);

   // real work

   update();

   if (millis() - lastUpdateAt > updateInterval)
      send();

   mqtt.loop();
}

//***************************************************************************
// Update
//***************************************************************************

void update()
{
   for (int inp = 0; inp < inputCount; inp++)
   {
      sampleSum[inp] += analogRead(intToA(inp));
      sampleCount[inp]++;
   }

   if (adsPresent)
   {
      for (int inp = 0; inp < inputCountAds; inp++)
      {
         sampleSumAds[inp] += readAdsChannel(intToAds(inp));
         sampleCountAds[inp]++;
      }
   }
}

//***************************************************************************
// Send
// JSON Format:
// {
//    "event" : "update",
//    "analog" : [
//       {
//          "name" : "A0",
//          "unit" : "dig",
//          "value" : 1250
//       },
//       {
//          "name" : "A1",
//          "unit" : "dig",
//          "value" : 2707
//       }
//    ]
// }
//***************************************************************************

void send()
{
   if (!mqtt.connected())
      return ;

   lastUpdateAt = millis();
   digitalWrite(ledPin, LOW);

   StaticJsonDocument<1024> doc;

   doc["event"] = "update";

   for (int inp = 0; inp < inputCount; inp++)
   {
      unsigned int avg = sampleSum[inp] / sampleCount[inp];
      sampleSum[inp] = 0;
      sampleCount[inp] = 0;

      doc["analog"][inp]["name"] = String("A") + inp;
      doc["analog"][inp]["value"] = avg;
      doc["analog"][inp]["unit"] = "dig";
#ifdef _USE_NTP
      if (isTimeSet)
         doc["analog"][inp]["time"] = ntpClient.getEpochTime();
#endif
   }

   if (adsPresent)
   {
      for (int inp = 0; inp < inputCountAds; inp++)
      {
         float avg = sampleSumAds[inp] / sampleCountAds[inp];
         sampleSumAds[inp] = 0.0;
         sampleCountAds[inp] = 0;

         unsigned int jInput = inp + adsInputOffset;
         doc["analog"][jInput]["name"] = String("A") + jInput;
         doc["analog"][jInput]["value"] = avg;
         doc["analog"][jInput]["unit"] = "mV";
#ifdef _USE_NTP
         if (isTimeSet)
            doc["analog"][jInput]["time"] = ntpClient.getEpochTime();
#endif
      }
   }

   publish(&doc, true);
}

//***************************************************************************
// started
//***************************************************************************

void started()
{
   StaticJsonDocument<1024> doc;

   doc["event"] = "init";
   publish(&doc, false);
}

//***************************************************************************
// publish
//***************************************************************************

int publish(JsonDocument* doc, int retained)
{
   String outString;
   serializeJson(*doc, outString);

   if (!mqtt.publish(topicOut, outString, retained, 0))
   {
      if (Serial)
         Serial.println("Failed to publish message " + String(mqtt.lastError()) + " : " + String(mqtt.returnCode()));

      return -1;
   }

   // if (Serial) Serial.println(ntpClient.getFormattedTime());
   if (Serial) Serial.println(outString);

   return 0;
}

//***************************************************************************
// At Message
//***************************************************************************

void atMessage(String& topic, String& payload)
{
   if (Serial) Serial.println("incoming: " + topic + " - " + payload);

   StaticJsonDocument<512> root;
   deserializeJson(root, payload);

   const char* event = root["event"];
   String parameter = root["parameter"];

   if (strcmp(event, "setUpdateInterval") == 0)
      updateInterval = parameter.toInt() * 1000;
   else if (strcmp(event, "triggerUpdate") == 0)
      send();
}

float readAdsChannel(ADS1115_MUX channel)
{
   float voltage {0.0};
   adc.setCompareChannels(channel);
   voltage = adc.getResult_mV();
   return voltage;
}
