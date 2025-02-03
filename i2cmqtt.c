//***************************************************************************
// i2c Interface
// File i2cmqtt.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 2024-2024 - Jörg Wendel
//***************************************************************************

#include <signal.h>

#include "lib/common.h"
#include "lib/json.h"
#include "lib/mqtt.h"

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#endif

#include "lib/i2c/ads1115.h"
#include "lib/i2c/mcp23017.h"
#include "lib/i2c/dht20.h"

#include "gpio.h"

//***************************************************************************
// Class I2CMqtt
//***************************************************************************

class I2CMqtt
{
   public:

      enum Pin
      {
         pinGpio16 = 36,
         pinMcpIrq = pinGpio16
      };

      enum ValueFormat
      {
         fReal,
         fBool,
         fInteger,
         fText
      };

      struct SensorData
      {
         ValueFormat format {fReal};
         std::string type;
         uint address {0};
         std::string title;
         std::string unit;
         int iValue {0};
         double dValue {0};
         std::string sValue;
      };

      I2CMqtt(const char* aDevice, const char* aMqttUrl, const char* aMqttTopic, int aInterval = 60);
      virtual ~I2CMqtt();

      static void downF(int aSignal) { shutdown = true; }

      int init();
      int exit();
      int loop();
      int update();
      int updateMcp();

      bool doShutDown() { return shutdown; }
      int show();
      int scan(const char* device);

      int initDht(const char* config);
      int initMcp(const char* config);
      int initAds(const char* config);

   protected:

      int mqttConnection();
      int mqttDisconnect();
      int mqttPublish(json_t* jObject);
      int mqttPublish(SensorData& sensor);
      int performMqttRequests();
      int dispatchMqttMessage(const char* message);

      const char* mqttUrl {};
      Mqtt* mqttWriter {};
      Mqtt* mqttReader {};
      std::string mqttTopicIn;
      std::string mqttTopicOut;
      int interval {60};
      std::string device;

      std::vector<Dht20> dhtChips;
      std::vector<Mcp23017> mcpChips;
      std::vector<Ads1115> adsChips;

      static void ioInterrupt();
      static bool shutdown;
};

//***************************************************************************
// Interrupt
//***************************************************************************

volatile bool ioInterruptTrigger {false};

void I2CMqtt::ioInterrupt()
{
   ioInterruptTrigger = true;
}

//***************************************************************************
// I2C MQTT
//***************************************************************************

bool I2CMqtt::shutdown {false};

I2CMqtt::I2CMqtt(const char* aDevice, const char* aMqttUrl, const char* aMqttTopic, int aInterval)
   : mqttUrl(aMqttUrl),
     mqttTopicIn(aMqttTopic),
     mqttTopicOut(aMqttTopic),
     interval(aInterval),
     device(aDevice)
{
   mqttTopicIn += "/in";
   mqttTopicOut += "/out";
}

I2CMqtt::~I2CMqtt()
{
}

int I2CMqtt::initDht(const char* config)
{
   if (isEmpty(config))
      return done;

   // --dht '0x38'
   // --dht '0x38,0x75'
   // --dht 'tca:0x70:0:0x38'
   // --dht 'tca:0x70:0:0x38,tca:0x71:1:0x38'

   auto tuples = split(config, ',');

   for (const auto& t : tuples)
   {
      auto options = split(t, ':');

      if (options[0] == "tca")
      {
         uint8_t tcaAddress = strtol(options[1].c_str(), nullptr, 0);
         uint8_t tcaChannel = strtol(options[2].c_str(), nullptr, 0);
         uint8_t dhtAddress = strtol(options[3].c_str(), nullptr, 0);

         dhtChips.emplace_back();
         dhtChips.back().setTcaChannel(tcaChannel);
         dhtChips.back().init(device.c_str(), dhtAddress, tcaAddress);

         tell(eloAlways, "Debug: Init DHT 0x%02x via 0x%02x/%d", dhtAddress, dhtChips.back().getAddress(), dhtChips.back().getTcaChannel());
      }
      else
      {
         uint8_t dhtAddress = strtol(options[0].c_str(), nullptr, 0);

         dhtChips.emplace_back();
         dhtChips.back().init(device.c_str(), dhtAddress);
      }
   }

   return done;
}

int I2CMqtt::initAds(const char* config)
{
   if (isEmpty(config))
      return done;

   // --mcp '0x48', ...

   auto tuples = split(config, ',');

   for (const auto& t : tuples)
   {
      adsChips.emplace_back();
      adsChips.back().init(device.c_str(), strtol(t.c_str(), nullptr, 0));
   }

   return success;
}

int I2CMqtt::initMcp(const char* config)
{
   if (isEmpty(config))
      return done;

   // --mcp '0x27'
   // --mcp '0x26,0x27'

   auto tuples = split(config, ',');

   for (const auto& t : tuples)
   {
      mcpChips.emplace_back();
      Mcp23017* mcp = &mcpChips.back();
      mcp->init(device.c_str(), strtol(t.c_str(), nullptr, 0));

      mcp->portMode(Mcp23017Port::A, 0b00000000);          // Port A as output
      mcp->portMode(Mcp23017Port::B, 0b11111111);          // Port B as input

      // enable interrupt for port B

      mcp->interruptMode(Mcp23017InterruptMode::Separated);
      mcp->interrupt(Mcp23017Port::B, Mcp23017::itChange);

      mcp->writeRegister(Mcp23017Register::GPIO_A, 0x00);  // Reset port A
      mcp->writeRegister(Mcp23017Register::GPIO_B, 0x00);  // Reset port B

      // GPIO reflects the same logic as the input pins state

      mcp->writeRegister(Mcp23017Register::IPOL_B, 0x00);
      mcp->writeRegister(Mcp23017Register::IPOL_A, 0x00);
      mcp->writePort(Mcp23017Port::A, 0xFF);

      mcp->clearInterrupts();
   }

   tell(eloAlways, "Debug: pinMode(%d, INPUT) (%d / %s)", pinMcpIrq, physPinToGpio(pinMcpIrq), physPinToGpioName(pinMcpIrq));
   pinMode(pinMcpIrq, INPUT);
   pullUpDnControl(pinMcpIrq, INPUT_PULLUP);

   if (wiringPiISR(pinMcpIrq, INT_EDGE_BOTH, &ioInterrupt) < 0)
      tell(eloAlways, "Error: Unable to setup ISR to pin %d / %s", physPinToGpio(pinMcpIrq), physPinToGpioName(pinMcpIrq));

   return done;
}
//***************************************************************************
// Init
//***************************************************************************

int I2CMqtt::init()
{
   tell(eloAlways, "Setup wiringPi");
   wiringPiSetupPhys();     // we use the 'physical' PIN numbers
   // wiringPiSetup();      // to use the 'special' wiringPi PIN numbers
   // wiringPiSetupGpio();  // to use the 'GPIO' PIN numbers

   mqttConnection();

   return success;
}

//***************************************************************************
// Exit
//***************************************************************************

int I2CMqtt::exit()
{
   mqttDisconnect();

   return done;
}

//***************************************************************************
// Loop
//***************************************************************************

int I2CMqtt::loop()
{
   // #TODO - check i2c and recover ...

   while (!doShutDown())
   {
      time_t nextAt = time(0) + interval;
      update();

      while (!doShutDown() && time(0) < nextAt)
      {
         usleep(100000);

         if (ioInterruptTrigger)
         {
            ioInterruptTrigger = false;
            tell(eloDebug, "Debug: Update on interrupt");
            updateMcp();
         }

         performMqttRequests();
      }
   }

   return done;
}

//***************************************************************************
// Update
//***************************************************************************

int I2CMqtt::update()
{
   tell(eloInfo, "Updating ..");

   if (mqttConnection() != success)
      return fail;

   for (auto& ads : adsChips)
   {
      for (uint pin = 0; pin < 4; ++pin)
      {
         Ads1115::Channel ch {};

         switch (pin)
         {
            case 0: ch = Ads1115::ai0Gnd; break;
            case 1: ch = Ads1115::ai1Gnd; break;
            case 2: ch = Ads1115::ai2Gnd; break;
            case 3: ch = Ads1115::ai3Gnd; break;
         }

         ads.setChannel(ch);

         int value {0};

         if (ads.read(0, value) != success)
            return fail;

         tell(eloDebug, "%d) %4d mV", pin, value);

         char type[100] {};
         sprintf(type, "ADS%02x", ads.getAddress());

         char name[100] {};
         sprintf(name, "ADS%02x Analog Input", ads.getAddress());

         SensorData sensor {};
         mqttPublish(sensor = {fInteger, type, pin, name, "mV", value, 0.0});
      }
   }

   updateMcp();

   for (auto& dht : dhtChips)
   {
      if (dht.read() == success)
      {
         SensorData sensor {};
         char name[100] {}; char type[10] {};

         if (dht.getTcaChannel() != 0xff)
            sprintf(type, "DHT%02x%x", dht.getAddress(), dht.getTcaChannel());
         else
            sprintf(type, "DHT%02x", dht.getAddress());

         sprintf(name, "%s Temperature", type);
         mqttPublish(sensor = {fReal, type, 0, name, "°C", 0, dht.getTemperature()});
         sprintf(name, "%s Humidity", type);
         mqttPublish(sensor = {fInteger, type, 1, name, "%", dht.getHumidity(), 0.0});
      }
      else
      {
         tell(eloAlways, "DHT request failed");
      }
   }

   tell(eloInfo, "... done");

   return done;
}

//***************************************************************************
// Dispatch MQTT Message
//***************************************************************************

int I2CMqtt::dispatchMqttMessage(const char* message)
{
   json_t* jData {jsonLoad(message)};

   if (!jData)
      return fail;

   tell(eloAlways, "<- [%s]", message);

   const char* type = getStringFromJson(jData, "type", "");
   int bit = getIntFromJson(jData, "address");
   std::string action = getStringFromJson(jData, "action", "");

   if (strncmp(type, "MCPO", 4) == 0)
   {
      std::string hex = std::string("0x") + std::string(type+4);
      uint8_t mcpAddress = strtol(hex.c_str(), nullptr, 0);
      Mcp23017* mcp {};

      for (auto& m : mcpChips)
      {
         if (m.getAddress() == mcpAddress)
            mcp = &m;
      }

      if (!mcp)
      {
         tell(eloAlways, "Device '%s' not found, skipping message [%s]", type, message);
         return done;
      }

      tell(eloDebug, "Debug: Device '%s' found at address 0x%02x", type, mcp->getAddress());
      uint8_t currentOutput = mcp->readPort(Mcp23017Port::A);

      // std::string byte = bin2string(currentOutput);
      // tell(eloAlways, "output is: '%s'", byte.c_str());

      tell(eloDebug, "Debug: Update digital output pin %s:0x%02x, action '%s'", type, bit, action.c_str());

      if (action == "impulse")
      {
         bitClear(currentOutput, bit);
         // byte = bin2string(currentOutput);
         // tell(eloAlways, "write: '%s'", byte.c_str());
         mcp->writePort(Mcp23017Port::A, currentOutput);
         usleep(50000);
         bitSet(currentOutput, bit);
      }
      else if (action == "init")
      {
         // the following (all) MCPO settings handled by homectrld:
         //   getBoolByPath(jData, "config/impulse");
         //   getBoolByPath(jData, "config/invert");
         //   getStringFromJson(jData, "config/feedbackInType", "");
         //   getIntFromJson(jData, "config/feedbackInAddress");
         return done;
      }
      else if (action == "set")
         bitSet(currentOutput, bit);
      else if (action == "clear")
         bitClear(currentOutput, bit);
      else
         tell(eloAlways, "Info: Ignoring unexpected action '%s' fpr MCPO", action.c_str());

      // byte = bin2string(currentOutput);
      // tell(eloAlways, "write: '%s'", byte.c_str());

      mcp->writePort(Mcp23017Port::A, currentOutput);
      updateMcp();
   }
   else if (strncmp(type, "MCPI", 4) == 0)
   {
      // uint pull = getIntByPath(jData, "config/pull");

      // if (pull == 0)
      //    ; // pullUpDnControl(bit, );
      // else if (pull == 1)
      //    pullUpDnControl(bit, INPUT_PULLUP);
      // else if (pull == 2)
      //    pullUpDnControl(bit, INPUT_PULLDOWN);

      // #TODO getBoolByPath(jData, "config/interrupt");

      // the following MCPI settings handled by homectrld:
      //   getBoolByPath(jData, "config/invert");
   }

   return success;
}

//***************************************************************************
// Update MCP
//***************************************************************************

int I2CMqtt::updateMcp()
{
   for (auto& mcp : mcpChips)
   {
      char type[100] {};
      uint8_t currentOutput {mcp.readPort(Mcp23017Port::A)};
      uint8_t currentInput {mcp.readPort(Mcp23017Port::B)};

      // Port A digital outputs

      sprintf(type, "MCPO%02x", mcp.getAddress());

      for (uint bit = 0; bit < 8; ++bit)
      {
         SensorData sensor {};
         char name[100] {};
         int state {(currentOutput >> bit) & 1};

         sprintf(name, "MCP%02x Digital Output %d", mcp.getAddress(), bit);
         mqttPublish(sensor = {fBool, type, bit, name, "", state, 0.0});
      }

      // Port B digital  inputs

      sprintf(type, "MCPI%02x", mcp.getAddress());

      for (uint bit = 0; bit < 8; ++bit)
      {
         SensorData sensor {};
         char name[100] {};
         int state {(currentInput >> bit) & 1};

         sprintf(name, "MCP%02x Digital Input %d", mcp.getAddress(), bit+8);
         mqttPublish(sensor = {fBool, type, bit+8, name, "", state, 0.0});
      }
   }

   return done;
}

//***************************************************************************
// MQTT Publish
//***************************************************************************

int I2CMqtt::mqttPublish(SensorData& sensor)
{
   json_t* obj = json_object();

   // { "value": 77.0, "type": "P4VA", "address": 1, "unit": "°C", "title": "Abgas" }

   json_object_set_new(obj, "type", json_string(sensor.type.c_str()));
   json_object_set_new(obj, "address", json_integer(sensor.address));
   json_object_set_new(obj, "title", json_string(sensor.title.c_str()));
   json_object_set_new(obj, "unit", json_string(sensor.unit.c_str()));

   if (sensor.format == fReal)
   {
      json_object_set_new(obj, "kind", json_string("value"));
      json_object_set_new(obj, "value", json_real(sensor.dValue));
   }
   else if (sensor.format == fInteger)
   {
      json_object_set_new(obj, "kind", json_string("value"));
      json_object_set_new(obj, "value", json_integer(sensor.iValue));
   }
   else if (sensor.format == fBool)
   {
      json_object_set_new(obj, "kind", json_string("status"));
      json_object_set_new(obj, "state", json_boolean(sensor.iValue));
   }
   else if (sensor.format == fText)
   {
      json_object_set_new(obj, "kind", json_string("text"));
      json_object_set_new(obj, "text", json_string(sensor.sValue.c_str()));
   }
   else
   {
      tell(eloAlways, "Unexpected format (%d)", sensor.format);
      json_decref(obj);
      return fail;
   }

   mqttPublish(obj);

   return success;
}

//***************************************************************************
// MQTT Publish
//***************************************************************************

int I2CMqtt::mqttPublish(json_t* jObject)
{
   char* message = json_dumps(jObject, JSON_REAL_PRECISION(8));
   mqttWriter->write(mqttTopicOut.c_str(), message);
   free(message);
   json_decref(jObject);

   return success;
}

//***************************************************************************
// Perform MQTT Requests
//***************************************************************************

int I2CMqtt::performMqttRequests()
{
   if (isEmpty(mqttUrl))
      return done;

   if (!mqttReader->isConnected())
      return done;

   MemoryStruct message;

   // tell(eloMqtt, "Try reading topic '%s'", mqttReader->getTopic());

   while (mqttReader->read(&message, 10) == success)
   {
      if (isEmpty(message.memory))
         continue;

      dispatchMqttMessage(message.memory);
   }

   return done;
}

//***************************************************************************
// MQTT Connection
//***************************************************************************

int I2CMqtt::mqttConnection()
{
   if (!mqttWriter)
      mqttWriter = new Mqtt();

   if (!mqttReader)
      mqttReader = new Mqtt();

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect(mqttUrl) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(eloAlways, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
      tell(eloAlways, "MQTT: Publish i2c data to topic '%s'", mqttTopicOut.c_str());
   }

   if (!mqttReader->isConnected())
   {
      if (mqttReader->connect(mqttUrl) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting subscriber to '%s' failed", mqttUrl);
         return fail;
      }

      if (mqttReader->subscribe(mqttTopicIn.c_str()) == success)
         tell(eloAlways, "MQTT: Topic '%s' at '%s' subscribed", mqttTopicIn.c_str(), mqttUrl);

      json_t* obj = json_object();
      json_object_set_new(obj, "type", json_string("I2C"));
      json_object_set_new(obj, "action", json_string("init"));
      json_object_set_new(obj, "topic", json_string(mqttTopicIn.c_str()));

      mqttPublish(obj);
   }

   return success;
}

//***************************************************************************
// MQTT Disconnect
//***************************************************************************

int I2CMqtt::mqttDisconnect()
{
   if (mqttReader) mqttReader->disconnect();
   if (mqttWriter) mqttWriter->disconnect();

   delete mqttReader; mqttReader = nullptr;
   delete mqttWriter; mqttWriter = nullptr;

   tell(eloMqtt, "Disconnected from MQTT");

   return done;
}

//***************************************************************************
// Show
//***************************************************************************

int I2CMqtt::show()
{
   // show

   tell(eloAlways, "-----------------------");

   for (auto& ads : adsChips)
   {
      tell(eloAlways, "ADS-1115");

      for (uint pin = 0; pin < 4; ++pin)
      {
         Ads1115::Channel ch {};

         switch (pin)
         {
            case 0: ch = Ads1115::ai0Gnd; break;
            case 1: ch = Ads1115::ai1Gnd; break;
            case 2: ch = Ads1115::ai2Gnd; break;
            case 3: ch = Ads1115::ai3Gnd; break;
         }

         ads.setChannel(ch);

         int value {0};

         if (ads.read(0, value) != success)
            return fail;

         tell(eloAlways, "%d: %4d mV", pin, value);
      }

      tell(eloAlways, "-----------------------");
   }

   for (auto& mcp : mcpChips)
   {
      tell(eloAlways, "MCP-23017");

      uint8_t currentA = mcp.readPort(Mcp23017Port::A);
      uint8_t currentB = mcp.readPort(Mcp23017Port::B);

      // std::string byteA = bin2string(currentA);
      // std::string byteB = bin2string(currentB);
      // tell(eloAlways, "Port A: %s", byteA.c_str());
      // tell(eloAlways, "Port B: %s", byteB.c_str());

      // Port A outputs

      tell(eloAlways, " ");

      for (int i = 0; i < 8; ++i)
         tell(eloAlways, "MCPO%02x:%d - %d", mcp.getAddress(), i, (currentA >> i) & 1);

      // Port B inputs

      tell(eloAlways, " ");

      for (int i = 0; i < 8; ++i)
         tell(eloAlways, "MCPI%02x:%d - %d", mcp.getAddress(), i+8, (currentB >> i) & 1);

      tell(eloAlways, "-----------------------");
   }

   for (auto& dht : dhtChips)
   {
      char type[10] {};

      if (dht.getTcaChannel() != 0xff)
         sprintf(type, "DHT%02x%x", dht.getAddress(), dht.getTcaChannel());
      else
         sprintf(type, "DHT%02x", dht.getAddress());

      tell(eloAlways, "%s", type);

      if (dht.read() == success)
      {
         tell(eloAlways, "Temperature %.2f °C", dht.getTemperature());
         tell(eloAlways, "Humidity %d %%", dht.getHumidity());
      }
      else
      {
         tell(eloAlways, "DHT request failed");
      }

      tell(eloAlways, "-----------------------");
   }

   return success;
}

//***************************************************************************
// Scan
//***************************************************************************

int I2CMqtt::scan(const char* device)
{
   I2C::scan(device);
   return done;
}

//***************************************************************************
// Usage
//***************************************************************************

void showUsage(const char* bin)
{
   printf("Usage: %s <command> [-l <log-level>] [-d <device>]\n", bin);
   printf("\n");
   printf("  options:\n");
   printf("     -l <eloquence>   set eloquence (bitmask)\n");
   printf("     -t               log to terminal\n");
   printf("     -s               show and exit\n");
   printf("     -S               scan i2c bus and exit\n");
   printf("     -i <interval>    interval seconds (default 60)\n");
   printf("     -u <url>         MQTT url\n");
   printf("     -T <topic>       MQTT topic\n");
   printf("                        <topic>/out - produce to\n");
   printf("                        <topic>/in  - read from\n");
   printf("     -d <device>      i2c device (defaults to /dev/i2c-0)\n");
   printf("     --ads <address>  ADS1115  address (0x48, defaults to -1/off)\n");
   printf("     --mcp <address>  MCP23017 address (0x20-0x27, defaults to -1/off)\n");
   printf("     --dht <config>   DHT20 config (defaults to -1/off)\n");
   printf("        where <config>:\n");
   printf("            <tuple>,<tuple>[,<tuple>,...]\n");
   printf("            where <tuple>\n");
   printf("               tca:<tca-address>:<tca-channel>:<dht-address>\n");
   printf("            or\n");
   printf("               <dht-address>\n");
   printf("     Note: tca is a TCA9548A i2c bus multiplexer\n");
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   bool nofork {false};
   int _stdout {na};
   bool showMode {false};
   bool scanMode {false};

   const char* mqttTopic {TARGET "2mqtt/i2c"};
   const char* mqttUrl {"tcp://localhost:1883"};
   const char* device {"/dev/i2c-0"};
   int interval {60};
   const char* dhtConfig {};
   const char* mcpConfig {};
   const char* adsConfig {};

   // usage ..

   if (argc <= 1 || (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
   {
      showUsage(argv[0]);
      return 0;
   }

   for (int i = 1; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) < 2)
         continue;

      switch (argv[i][1])
      {
         case 'u': mqttUrl = argv[i+1];                       break;
         case 'l':
            if (argv[i+1])
               eloquence = (Eloquence)strtol(argv[++i], nullptr, 0);
            break;
         case 'i': if (argv[i+1]) interval = atoi(argv[++i]); break;
         case 'T': if (argv[i+1]) mqttTopic = argv[i+1];      break;
         case 't': _stdout = yes;                             break;
         case 'd': if (argv[i+1]) device = argv[++i];         break;
         case 's': showMode = true;                           break;
         case 'S': scanMode = true;                           break;
         case 'n': nofork = true;                             break;
         case '-':
         {
            if (isEmpty(argv[i]+2))
               continue;
            if (strcmp(argv[i]+2, "ads") == 0 && argv[i+1])
               adsConfig = argv[++i];
            else if (strcmp(argv[i]+2, "mcp") == 0 && argv[i+1])
               mcpConfig = argv[++i];
            else if (strcmp(argv[i]+2, "dht") == 0 && argv[i+1])
               dhtConfig = argv[++i];

            break;
         }
      }
   }

   if (scanMode)
   {
      I2CMqtt* job = new I2CMqtt(device, mqttUrl, mqttTopic, interval);
      job->scan(device);
      delete job;
      return 0;
   }

   // do work ...

   if (_stdout != na)
      logstdout = _stdout;
   else
      logstdout = false;

   // fork daemon

   if (!nofork && !showMode)
   {
      int pid;

      if ((pid = fork()) < 0)
      {
         printf("Can't fork daemon, %s\n", strerror(errno));
         return 1;
      }

      if (pid != 0)
         return 0;
   }

   // create/init AFTER fork !!!

   I2CMqtt* job = new I2CMqtt(device, mqttUrl, mqttTopic, interval);

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return 1;
   }

   job->initDht(dhtConfig);
   job->initMcp(mcpConfig);
   job->initAds(adsConfig);

   if (showMode)
      return job->show();

   // register SIGINT

   ::signal(SIGINT, I2CMqtt::downF);
   ::signal(SIGTERM, I2CMqtt::downF);

   job->loop();

   // shutdown

   tell(eloAlways, "shutdown");
   delete job;

   return 0;
}
