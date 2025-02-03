//***************************************************************************
// VOTRO... Interface
// File votro.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 17.03.2024 - Jörg Wendel
//***************************************************************************

#include <signal.h>

#include "lib/common.h"
#include "lib/serial.h"
#include "lib/json.h"
#include "lib/mqtt.h"

#define isBitSet(value, bit)  ((value >> (bit)) & 0x01)

//***************************************************************************
// Class - VotroCom
//   Supports the RS485 Bus and the Simple Serial Display Include)
//***************************************************************************

class VotroCom
{
   public:

      // ----------------------------------------------------
      // Definitions for the RS485 interface

      enum DeviceAddress
      {
         daMainsCharger = 0x04,
         daSolar        = 0x10,
         daBooster      = 0xc4,

         daDisplay      = 0xf4
      };

      enum Parameter
      {
         // daSolar

         pCurrent       = 0x02,
         pBattVoltage   = 0x03,
         pPanelVoltage  = 0x04,

         pKfzBattVolage = 0x62,
         pPower         = 0x87,

         // daMainsCharger

         pBattVoltage04   = 0x05,
         pKfzBattVolage04 = 0x09,
         pCurrent04       = 0xa1,

         // daBooster

      };

      enum ComBytes
      {
         cbStart       = 0x00,
         cpPacketType,
         cbDestination,
         cbSource,
         cbCommand,
         cbParameter,
         cbValueLow,          // little endian -> low byte first
         cbValueHight,
         cbChecksum
      };

      enum Command
      {
         cReadValue  = 0x03,
         cWriteValue = 0x09   // not shure - only speculation
      };

      enum Specials
      {
         sStartByte = 0xaa
      };

      enum MessageType
      {
         mtRequest  = 0x22,
         mtResponce = 0x62
      };

      enum ParameterType
      {
         ptBit,
         ptByte,
         ptWord
      };

      struct ParameterDefinition
      {
         std::string title;
         ParameterType type {ptWord};
         uint factor {0};             // factor or bit number (0-7)
         std::string unit;
      };

      struct ResponseRs485
      {
         DeviceAddress source {daSolar};
         Parameter parameter {pCurrent};
         byte valueHight {0x00};
         byte valueLow {0x00};
         double value {0.0};
         bool state {false};
      };

      // ----------------------------------------------------
      // Definitions for the Simple Serial Display Interface

      enum BytesSolar
      {
         // bsStart = 0x00,
         bsDeviceId = 0x00,   // 1
         bsBattVolotageLsb,   // 2
         bsBattVolotageMsb,   // 3
         bsSolarVolotageLsb,  // 4
         bsSolarVolotageMsb,  // 5
         bsSolarCurrentLsb,   // 6
         bsSolarCurrentMsb,   // 7
         bsFlags1,
         bsFlags2,
         bsFlags3,
         bsControllerTemp,
         bsChargeMode,
         bsBattStatus,
         bsControllerStatus,
         bsChecksum
      };

      enum BattStatusBits
      {
         bsbPhaseI      = 0,
         bsbPhaseU1     = 1,
         bsbPhaseU2     = 2,
         bsbPhaseU3     = 3,
         bsbUnknown0    = 4,
         bsbUnknown1    = 5,
         bsbUnknown2    = 6,
         bsbUnknown3    = 7
      };

      enum ControllerStatusBits
      {
         ssbMppt        = 0,
         ssbUnknown1    = 1,
         ssbUnknown2    = 2,
         ssbActive      = 3,
         ssbProtection  = 4,  // charged over 80%
         ssbAES         = 5,  // Automatic Energy Selector
         ssbUnknown6    = 6,
         ssbUnknown7    = 7
      };

      enum SimpleAddresses
      {
         saSolarCharger       = 0x1a,
         saMainsCharger       = 0x3a,
         saLoadConverter      = 0x7a,
         saBatteryController1 = 0xca,
         saBatteryController2 = 0xda
      };

      struct ResponseSimpleInterface
      {
         uint8_t deviceId {0x00};
         double battVoltage {0.0};
         double solarVoltage {0.0};
         double solarCurrent {0.0};
         std::string battStatus;
         bool solarActive {false};
         bool solarProtection {false};
         bool solarAES {false};
         bool solarMppt {false};
         int controllerTemp {0};
         byte flags1 {0};
         byte flags2 {0};
         byte flags3 {0};
         std::string chargeMode;
         byte controllerStatusByte {};  // for debugging
      };

      //

      VotroCom(const char* aDevice, uint baud = 0, bool aUseSdi = false);
      ~VotroCom() { close(); }

      int open();
      int close();

      void setSpecialSpeed(int b) { serial->setSpecialSpeed(b); }

      int request(byte address, byte parameter);
      void setSilent(bool s) { silent = s; }

      ResponseRs485* getRs485Response() { return &rs485Response; }
      static std::map<DeviceAddress,std::map<Parameter,ParameterDefinition>> parameterDefinitions;  // for RS48%
      int readSimple();
      ResponseSimpleInterface* getSimpleInterfaceResponse() { return &simpleInterfaceResponse; }

   private:

      byte calcRs485Checksum(unsigned char buffer[], size_t size);
      int parseRs485Response(byte buffer[], size_t size);

      int parseSimpleResponse(uint8_t buffer[], size_t size);
      uint8_t calcSimpleChecksum(unsigned char buffer[], size_t size);

      ResponseRs485 rs485Response;
      ResponseSimpleInterface simpleInterfaceResponse;

      std::string device;
      Serial* serial {};
      Sem sem;            // to protect synchronous access to device
      uint baud {0};
      bool useSdi {false};
      bool silent {false};

      static std::map<byte,std::string> chargeModes;  // for Simple ...
};

//***************************************************************************
// VotroCom
//***************************************************************************

std::map<VotroCom::DeviceAddress,std::map<VotroCom::Parameter,VotroCom::ParameterDefinition>> VotroCom::parameterDefinitions
{
   //   parameter,      title,                 format, factor, unit
   //                                                  or bit
   { daSolar, {
         { pCurrent,         { "PV Strom",            ptWord,  10, "A" } },
         { pBattVoltage,     { "Batterie",            ptWord, 100, "V" } },
         { pPanelVoltage,    { "PV Spannung",         ptWord, 100, "V" } },
         { pPower,           { "PV Leistung",         ptWord,  10, "W" } },
         { pKfzBattVolage,   { "KFZ Batterie",        ptWord, 100, "V" } }
      }},
   { daBooster, {
         { pCurrent,         { "Strom ??",            ptWord,  10, "A" } }
      }},
   { daMainsCharger, {
         { pCurrent04,       { "Netz Strom",          ptWord,  10, "A" } },
         { pBattVoltage04,   { "Batterie",            ptWord, 100, "V" } },
         { pKfzBattVolage04, { "KFZ Batterie",        ptWord, 100, "V" } }
      }}
};

std::map<byte,std::string> VotroCom::chargeModes
{
   { 0x35, "Lead GEL" },
   { 0x22, "Lead AGM1" },
   { 0x2F, "Lead AGM2" },
   { 0x50, "LifePo4 13.9 V" },
   { 0x52, "LifePo4 14.2 V" },
   { 0x54, "LifePo4 14.4 V" },
   { 0x56, "LifePo4 14.6 V" },
   { 0x58, "LifePo4 14.8 V" }
};

//***************************************************************************
// VotroCom
//***************************************************************************

VotroCom::VotroCom(const char* aDevice, uint aBaud, bool aUseSdi)
   : device(aDevice),
     sem(0x4da00001),
     baud(aBaud),
     useSdi(aUseSdi)
{
   if (useSdi)
   {
      // 1000 ?? :o bits per second, 8 bit, no parity and 1 stop bit

      if (!baud)
         baud = 1020;

      serial = new Serial(0);
      serial->setSpecialSpeed(baud);
   }
   else // RS485 Interface
   {
      // 19200 bits per second, 8 bit, even parity and 1 stop bit

      if (!baud)
         baud = 19200;

      serial = new Serial(0, CS8 | PARENB);
      serial->setSpecialSpeed(19200);
   }
}

//***************************************************************************
// Open / Close
//***************************************************************************

int VotroCom::open()
{
   return serial->open(device.c_str());
}

int VotroCom::close()
{
   return serial->close();
}

//***************************************************************************
// Read Simple - for the 'simple' serial display interface
//  - reads one data packet
//***************************************************************************

int VotroCom::readSimple()
{
   int status {success};

   SemLock lock(&sem);
   usleep(500000);                  // collect some bytes

   constexpr uint maxTries {3};     // after open it can take up to 3 cycle to snchronice
   uint nTry {0};

   while (nTry < maxTries)
   {
      uint8_t b {0};

      while ((status = serial->look(b, 100)) == success)
      {
         if (b == sStartByte)  // same start byte as for RS485
            break;

         tell(eloDebug, "Skipping unexpected byte 0x%02x at start", b);
      }

      uint8_t buffer[20] {};
      nTry++;

      if (b != sStartByte || serial->read(buffer, 15) != 15)
      {
         tell(eloAlways, "Error: Reading packet failed, skipping, start byte was 0x%02x", b);
         status = fail;
         continue;
      }
      else
      {
         return parseSimpleResponse(buffer, 15);
      }
   }

   return fail;
}

//***************************************************************************
// Parse Simple Response
//  - for the simple serial display interface
//***************************************************************************

int VotroCom::parseSimpleResponse(uint8_t buffer[], size_t size)
{
   // actually only the Solar Charger is implemented

   if (buffer[bsDeviceId] != saSolarCharger)
   {
      if (!silent) tell(eloAlways, "Error: Got unexpected device id, ignoring");
      return fail;
   }

   uint8_t checksum = calcSimpleChecksum(buffer, size-1);

   if (checksum != buffer[bsChecksum])
      tell(eloAlways, "Warning: Checksum failure in received packet (%zu) [0x%02x/0x%02x]", size, checksum, buffer[bsChecksum]);

   simpleInterfaceResponse.deviceId = buffer[bsDeviceId];
   simpleInterfaceResponse.battVoltage = (buffer[bsBattVolotageMsb] << 8) + buffer[bsBattVolotageLsb];
   simpleInterfaceResponse.battVoltage /= 100.0;
   simpleInterfaceResponse.solarVoltage = (buffer[bsSolarVolotageMsb] << 8) + buffer[bsSolarVolotageLsb];
   simpleInterfaceResponse.solarVoltage /= 100.0;
   simpleInterfaceResponse.solarCurrent = (buffer[bsSolarCurrentMsb] << 8) + buffer[bsSolarCurrentLsb];
   simpleInterfaceResponse.solarCurrent /= 10.0;

   simpleInterfaceResponse.controllerTemp = buffer[bsControllerTemp];

   switch (buffer[bsBattStatus] & 0x0F)
   {
      case bsbPhaseI:  simpleInterfaceResponse.battStatus = "Phase I";  break;
      case bsbPhaseU1: simpleInterfaceResponse.battStatus = "Phase U1"; break;
      case bsbPhaseU2: simpleInterfaceResponse.battStatus = "Phase U2"; break;
      case bsbPhaseU3: simpleInterfaceResponse.battStatus = "Phase U3"; break;
   }

   simpleInterfaceResponse.controllerStatusByte = buffer[bsControllerStatus];
   simpleInterfaceResponse.solarActive = isBitSet(buffer[bsControllerStatus], ssbActive);
   simpleInterfaceResponse.solarProtection = isBitSet(buffer[bsControllerStatus], ssbProtection);
   simpleInterfaceResponse.solarAES = isBitSet(buffer[bsControllerStatus], ssbAES);
   simpleInterfaceResponse.solarMppt = isBitSet(buffer[bsControllerStatus], ssbMppt);

   simpleInterfaceResponse.flags1 = buffer[bsFlags1];
   simpleInterfaceResponse.flags2 = buffer[bsFlags2];
   simpleInterfaceResponse.flags3 = buffer[bsFlags3];

   if (chargeModes.find(buffer[bsChargeMode]) != chargeModes.end())
      simpleInterfaceResponse.chargeMode = chargeModes[buffer[bsChargeMode]];

   return success;
}

//***************************************************************************
// Calc Checksum for the Simple Serial Interface
//***************************************************************************

uint8_t VotroCom::calcSimpleChecksum(unsigned char buffer[], size_t size)
{
   uint8_t checksum {0x00};

   for (uint i = 0; i < size; ++i)
   {
      checksum ^= buffer[i];
      tell(eloDebug, "byte %d) 0x%02x", i+1, buffer[i]);
   }

   return checksum;
}

//***************************************************************************
// Calc Checksum for RS485
//***************************************************************************

byte VotroCom::calcRs485Checksum(unsigned char buffer[], size_t size)
{
   byte checksum {0x55};

   for (uint i = 0; i < size; ++i)
      checksum ^= buffer[i];

   return checksum;
}

//***************************************************************************
// Parse RS485 Response
//***************************************************************************

int VotroCom::parseRs485Response(byte buffer[], size_t size)
{
   if (buffer[cbStart] != sStartByte ||
       buffer[cpPacketType] != mtResponce ||
       buffer[cbDestination] != daDisplay ||
       buffer[cbCommand] != cReadValue)
   {
      if (!silent) tell(eloAlways, "Error: Got unexpected response, ignoring");
      return fail;
   }

   rs485Response.source = (DeviceAddress)buffer[cbSource];
   rs485Response.parameter = (Parameter)buffer[cbParameter];
   rs485Response.valueLow = buffer[cbValueLow];
   rs485Response.valueHight = buffer[cbValueHight];
   rs485Response.value = 0;
   rs485Response.state = false;

   byte checksum = calcRs485Checksum(buffer, 8);

   if (buffer[cbChecksum] != checksum)
      tell(eloAlways, "Warning: Checksum failure in received packet");

   // lookup definition

   if (parameterDefinitions.find(rs485Response.source) != parameterDefinitions.end())
   {
      auto& deviceDef = parameterDefinitions[rs485Response.source];

      if (deviceDef.find(rs485Response.parameter) != deviceDef.end())
      {
         const auto& def = deviceDef[rs485Response.parameter];

         if (def.type == ptWord)
            rs485Response.value = (rs485Response.valueHight << 8) + rs485Response.valueLow;
         else if (def.type == ptBit && def.factor < 8)
            rs485Response.state = isBitSet(rs485Response.valueLow, def.factor);
         else if (def.type == ptBit && def.factor >= 8)
            rs485Response.state = isBitSet(rs485Response.valueHight, def.factor-8);
         else
            rs485Response.value = rs485Response.valueLow;

         rs485Response.value /= def.factor;
      }
      else
      {
         rs485Response.value = (rs485Response.valueHight << 8) + rs485Response.valueLow;
      }
   }

   return success;
}

//***************************************************************************
// Request for RS485
//  - request one parameter
//***************************************************************************

int VotroCom::request(byte address, byte parameter)
{
   if (!serial->isOpen())
   {
      tell(eloAlways, "Error: Can't request, device not open");
      return fail;
   }

   SemLock lock(&sem);

   tell(eloDebug, "Requesting 0x%02x:0x%02x", address, parameter);

   unsigned char buffer[10] {};
   size_t contentLen {0};

   buffer[contentLen++] = sStartByte;         // start byte
   buffer[contentLen++] = mtRequest;          // request (packet type)
   buffer[contentLen++] = address;            // votro device address
   buffer[contentLen++] = daDisplay;          // source id
   buffer[contentLen++] = 0x03;               // command (read value)
   buffer[contentLen++] = parameter;          // the parameter address to read
   buffer[contentLen++] = 0x00;               // value low byte
   buffer[contentLen++] = 0x00;               // value heigh byte
   byte checksum = calcRs485Checksum(buffer, contentLen);
   buffer[contentLen++] = checksum;           // checksum

   int status {success};

   if ((status = serial->write(buffer, contentLen)) != success)
   {
      tell(eloAlways, "Writing request failed, state was %d", status);
      return status;
   }

   // read response

   byte b {0};
   byte _response[10] {};
   size_t cnt {0};

   while ((status = serial->look(b, 500)) == success)
   {
      if (!cnt && b != sStartByte)
      {
         tell(eloAlways, "Skipping unexpected start byte 0x%02x", b);
         continue;
      }

      _response[cnt++] = b;

      if (cnt >= 9)
         break;
   }

   if (eloquence & eloDetail)
   {
      bool shown {false};

      if (!shown)
      {
         shown = true;
         printf("-> '");
         for (uint i = 0; i < contentLen; ++i)
            printf("%s0x%02x", i ? ",": "", buffer[i]);
         printf("'\n");
      }

      printf("<- '");
      for (uint i = 0; i < cnt; ++i)
         printf("%s0x%02x", i ? ",": "", _response[i]);
      printf("'\n");
      fflush(stdout);
   }

   status = parseRs485Response(_response, cnt);

   return status;
}

//***************************************************************************
// Class - Votro
//***************************************************************************

class Votro
{
   public:

      struct SensorData
      {
         uint address {0};
         std::string kind;
         std::string title;
         std::string unit;
         double value {0.0};
         std::string text;
      };

      Votro(const char* aDevice, const char* aMqttUrl, const char* aMqttTopic, int aInterval = 60, uint aBaud = 0, bool aUseSdi = false);
      virtual ~Votro();

      static void downF(int aSignal) { shutdown = true; }

      int init() { return success; }
      int loop();
      int update();

      static bool doShutDown() { return shutdown; }

   protected:

      int mqttConnection();
      int mqttPublish(SensorData& sensor, const char* type);

      std::string device;
      const char* mqttUrl {};
      Mqtt* mqttWriter {};
      std::string mqttTopic;
      int interval {60};
      uint baud {0};
      bool useSdi {false};

      static bool shutdown;
};

//***************************************************************************
// Votro
//***************************************************************************

bool Votro::shutdown {false};

Votro::Votro(const char* aDevice, const char* aMqttUrl, const char* aMqttTopic, int aInterval, uint aBaud, bool aUseSdi)
   : device(aDevice),
     mqttUrl(aMqttUrl),
     mqttTopic(aMqttTopic),
     interval(aInterval),
     baud(aBaud),
     useSdi(aUseSdi)
{
}

Votro::~Votro()
{
}

//***************************************************************************
// Loop
//***************************************************************************

int Votro::loop()
{
   while (!doShutDown())
   {
      time_t nextAt = time(0) + interval;
      update();

      while (!doShutDown() && time(0) < nextAt)
         sleep(1);
   }

   return done;
}

//***************************************************************************
// Update
//***************************************************************************

int Votro::update()
{
   tell(eloInfo, "Updating ...");

   if (mqttConnection() != success)
      return fail;

   VotroCom com(device.c_str(), baud, useSdi);
   SensorData sensor {};
   int status {success};

   com.open();

   if (useSdi)
   {
      if ((status = com.readSimple()) == success)
      {
         VotroCom::ResponseSimpleInterface* response = com.getSimpleInterfaceResponse();
         std::string type = "VOTRO" + horchi::to_string(response->deviceId, 0, true);

         mqttPublish(sensor = {0, "value", "Battery", "V", response->battVoltage}, type.c_str());
         mqttPublish(sensor = {1, "value", "PV Spannung", "V", response->solarVoltage}, type.c_str());
         mqttPublish(sensor = {2, "value", "PV Strom", "A", response->solarCurrent}, type.c_str());
         mqttPublish(sensor = {3, "text", "Charging Status", "", 0, response->battStatus}, type.c_str());
         mqttPublish(sensor = {4, "status", "PV Active", "", (double)response->solarActive}, type.c_str());
         mqttPublish(sensor = {5, "status", "PV 80%", "", (double)response->solarProtection}, type.c_str());
         mqttPublish(sensor = {6, "status", "PV AES", "", (double)response->solarAES}, type.c_str());
         mqttPublish(sensor = {7, "value", "Controller Temp", "°C", (double)response->controllerTemp}, type.c_str());
         mqttPublish(sensor = {8, "text", "Charge Mode", "", 0, response->chargeMode}, type.c_str());
         mqttPublish(sensor = {9, "status", "PV MPPT", "", (double)response->solarMppt}, type.c_str());
      }
   }
   else
   {
      for (const auto& deviceDef : VotroCom::parameterDefinitions)
      {
         for (const auto& def : deviceDef.second)
         {
            tell(eloDebug, "Requesting parameter '0x%02x/%s' (0x%02x)", deviceDef.first, def.second.title.c_str(), def.first);

            status = com.request(deviceDef.first, def.first);
            VotroCom::ResponseRs485* response = com.getRs485Response();

            tell(eloDebug, "%s: %.2f %s", def.second.title.c_str(), response->value, def.second.unit.c_str());

            std::string type = "VOTRO" + horchi::to_string(deviceDef.first, 0, true);
            mqttPublish(sensor = {def.first, "value", def.second.title.c_str(), def.second.unit.c_str(), response->value}, type.c_str());
         }
      }
   }

   com.close();
   tell(eloInfo, " ... %s", status == success ? "done" : "failed");

   return done;
}

//***************************************************************************
// MQTT Publish
//***************************************************************************

int Votro::mqttPublish(SensorData& sensor, const char* type)
{
   json_t* obj = json_object();

   json_object_set_new(obj, "type", json_string(type));
   json_object_set_new(obj, "address", json_integer(sensor.address));
   json_object_set_new(obj, "title", json_string(sensor.title.c_str()));
   json_object_set_new(obj, "unit", json_string(sensor.unit.c_str()));
   json_object_set_new(obj, "kind", json_string(sensor.kind.c_str()));

   if (sensor.kind == "value")
      json_object_set_new(obj, "value", json_real(sensor.value));
   else if (sensor.kind == "status")
      json_object_set_new(obj, "state", json_boolean(sensor.value));
   else
      json_object_set_new(obj, "text", json_string(sensor.text.c_str()));

   char* message = json_dumps(obj, JSON_REAL_PRECISION(8));
   mqttWriter->write(mqttTopic.c_str(), message);
   free(message);
   json_decref(obj);

   return success;
}

//***************************************************************************
// MQTT Connection
//***************************************************************************

int Votro::mqttConnection()
{
   if (!mqttWriter)
      mqttWriter = new Mqtt();

   if (!mqttWriter->isConnected())
   {
      if (mqttWriter->connect(mqttUrl) != success)
      {
         tell(eloAlways, "Error: MQTT: Connecting publisher to '%s' failed", mqttUrl);
         return fail;
      }

      tell(eloAlways, "MQTT: Connecting publisher to '%s' succeeded", mqttUrl);
      tell(eloAlways, "MQTT: Publish VOTRO data to topic '%s'", mqttTopic.c_str());
   }

   return success;
}

//***************************************************************************
// Show
//***************************************************************************

int showParameter(VotroCom* com, byte address, byte parameter, int factor, bool asCsv)
{
   com->request(address, parameter);

   VotroCom::ResponseRs485* response = com->getRs485Response();
   std::string byte1 = bin2string(response->valueLow);
   std::string byte2 = bin2string(response->valueHight);

   word wValue = (response->valueHight << 8) + response->valueLow;

   if (asCsv)
   {
      tell(eloInfo, "%d,%d,\"%s\",%d,\"%s\",%d,%.2f",
           response->parameter, response->valueLow, byte1.c_str(),
           response->valueHight, byte2.c_str(), wValue,
           (double)(wValue)/(double)factor);
   }
   else
   {
      if (eloquence & eloInfo)
         tell(eloInfo, "0x%02x) byte1 %3d (%s), byte2 %3d (%s), word %5d - (%.2f)",
              response->parameter, response->valueLow, byte1.c_str(),
              response->valueHight, byte2.c_str(), wValue,
              (double)(wValue)/(double)factor);
      else
         tell(eloAlways, "0x%02x) byte1 %3d, byte2 %3d, word %5d - (%.2f)",
              response->parameter, response->valueLow, response->valueHight, wValue,
              (double)(wValue)/(double)factor);
   }

   return success;
}

int show(const char* device, byte address, int parameter, int factor, uint baud, bool asCsv)
{
   VotroCom com(device, baud, false);
   com.open();

   if (asCsv)
      tell(eloInfo, "id,byte1,as bin,byte1,as bin,word,value");

   if (parameter != na)
   {
      while (!Votro::doShutDown())
      {
         showParameter(&com, address, parameter, factor, asCsv);
         sleep(1);
      }
   }
   else
   {
      for (uint par = 0; par <= 0xff; ++par)
         showParameter(&com, address, par, factor, asCsv);
   }

   com.close();

   return done;
}

//***************************************************************************
// Show Simple
//   - show data of Simple Serial Display Interface
//***************************************************************************

int showSimple(const char* device, uint baud)
{
   VotroCom com(device, baud, true);

   com.open();

   while (!Votro::doShutDown())
   {
      if (com.readSimple() != success)
      {
         tell(eloAlways, "Error: Reading failed");
         sleep(3);
         continue;
      }

      VotroCom::ResponseSimpleInterface* response = com.getSimpleInterfaceResponse();

      tell(eloAlways, "battVoltage: %.2f V", response->battVoltage);
      tell(eloAlways, "solarVoltage: %.2f V", response->solarVoltage);
      tell(eloAlways, "solarCurrent: %.2f A", response->solarCurrent);
      tell(eloAlways, "battStatus: %s", response->battStatus.c_str());
      tell(eloAlways, "solarActive: %s", response->solarActive ? "yes" : "no");
      tell(eloAlways, "solarProtection: %s", response->solarProtection ? "yes" : "no");
      tell(eloAlways, "solarAES: %s", response->solarAES ? "yes" : "no");
      tell(eloAlways, "solarMppt: %s", response->solarMppt ? "yes" : "no");
      tell(eloAlways, "controllerTemp: %d °C", response->controllerTemp);
      tell(eloAlways, "chargeMode: %s", response->chargeMode.c_str());
      tell(eloAlways, "flags1: 0x%02x", response->flags1);
      tell(eloAlways, "flags2: 0x%02x", response->flags2);
      tell(eloAlways, "flags3: 0x%02x", response->flags3);

      tell(eloAlways, "ssbUnknown1: %s", isBitSet(response->controllerStatusByte, VotroCom::ssbUnknown1) ? "yes" : "no");
      tell(eloAlways, "ssbUnknown2: %s", isBitSet(response->controllerStatusByte, VotroCom::ssbUnknown2) ? "yes" : "no");
      tell(eloAlways, "ssbUnknown6: %s", isBitSet(response->controllerStatusByte, VotroCom::ssbUnknown6) ? "yes" : "no");
      tell(eloAlways, "ssbUnknown7: %s", isBitSet(response->controllerStatusByte, VotroCom::ssbUnknown7) ? "yes" : "no");

      sleep(1);
   }

   com.close();

   return done;
}

//***************************************************************************
// Detect
//***************************************************************************

int detect(const char* device, uint baud)
{
   VotroCom com(device, baud, false);

   com.setSilent(true);
   com.open();

   for (uint addr = 0x00; addr <= 0xff; ++addr)
   {
      if (com.request(addr, 0x01) == success)
         tell(eloAlways, "Device at address 0x%02x is reachable", addr);
   }

   fflush(stdout);
   com.close();

   return success;
}

//***************************************************************************
// Usage
//***************************************************************************

int showUsage(int ret, const char* bin)
{
   printf("Usage: %s <command> [-l <log-level>] [-d <device>]\n", bin);
   printf("\n");
   printf("  options:\n");
   printf("     -d <device>          serial device file (defaults to /dev/ttyVotro)\n");
   printf("     -l <eloquence>       set eloquence\n");
   printf("     -t                   log to terminal\n");
   printf("     -i <interval>        interval [s] (default 60)\n");
   printf("     -u <url>             MQTT url (default tcp://localhost:1883)\n");
   printf("     -T <topic>           MQTT topic (default " TARGET "2mqtt/votro)\n");
   printf("     -n                   Don't fork in background\n");
   printf("     -I                   Use simple serial display interface instead of RS485\n");
   printf("     \n");
   printf("  Simple-Serial-Display-Interface options:\n");
   printf("     -s                   show data in a loop every second\n");
   printf("     \n");
   printf("  RS485 options:\n");
   printf("     -s <device-addr>     show data, if <parameter> is specified the value is displayed in a loop every second\n");
   printf("        [<parameter-id>]   show only <parameter> instead of all 256 possible\n");
   printf("        [<factor>]         calculate with the assumption that it is transferred with this factor\n");
   printf("     -C                   Output as CSV - only for -s option (show)\n");
   printf("     -D <parameter-id>    Try to detect devices\n");

   return ret;
}

//***************************************************************************
// Main
//***************************************************************************

int main(int argc, char** argv)
{
   bool nofork {false};
   int _stdout {na};
   const char* mqttTopic {TARGET "2mqtt/votro"};
   const char* mqttUrl {"tcp://localhost:1883"};
   const char* device {"/dev/ttyVotro"};
   bool showMode {false};
   bool detectMode {false};
   int interval {60};
   int parameter {na};
   byte address {0x10};
   uint showFactor {1};
   bool asCsv {false};
   bool useSdi {false};
   uint baud {0};

   eloquence = eloAlways;

   // usage ..

   if (argc <= 1 || (argv[1][0] == '?' || (strcmp(argv[1], "-h") == 0) || (strcmp(argv[1], "--help") == 0)))
      return showUsage(0, argv[0]);

   for (int i = 1; argv[i]; i++)
   {
      if (argv[i][0] != '-' || strlen(argv[i]) != 2)
         continue;

      switch (argv[i][1])
      {
         case 'l':
            if (argv[i+1])
               eloquence = (Eloquence)strtol(argv[++i], nullptr, 0);
            break;
         case 'u': mqttUrl = argv[i+1];                       break;
         case 'i': if (argv[i+1]) interval = atoi(argv[++i]); break;
         case 'b': baud = atoi(argv[i+1]);                    break;
         case 'T': if (argv[i+1]) mqttTopic = argv[i+1];      break;
         case 't': _stdout = yes;                             break;
         case 'd': if (argv[i+1]) device = argv[++i];         break;
         case 'I': useSdi = true;                             break;
         case 's':
            showMode = true;
            if (useSdi)
               break;
            if (argc < 3)
               return showUsage(1, argv[0]);
            address = strtol(argv[++i], nullptr, 0);
            if (argc > i+1)
               parameter = strtol(argv[++i], nullptr, 0);
            if (argc > i+1)
               showFactor = strtol(argv[++i], nullptr, 0);
            break;
         case 'C': asCsv = true;                              break;
         case 'D': detectMode = true;                         break;
         case 'n': nofork = true;                             break;
      }
   }

   // do work ...

   if (detectMode)
      return detect(device, baud);

   if (showMode)
   {
      if (useSdi)
         return showSimple(device, baud);
      else
         return show(device, address, parameter, showFactor, baud, asCsv);
   }

   if (_stdout != na)
      logstdout = _stdout;
   else
      logstdout = false;

   Votro* job = new Votro(device, mqttUrl, mqttTopic, interval, baud, useSdi);

   if (job->init() != success)
   {
      printf("Initialization failed, see syslog for details\n");
      delete job;
      return 1;
   }

   // fork daemon

   if (!nofork)
   {
      int pid {0};

      if ((pid = fork()) < 0)
      {
         printf("Can't fork daemon, %s\n", strerror(errno));
         return 1;
      }

      if (pid != 0)
         return 0;
   }

   // register SIGINT

   ::signal(SIGINT, Votro::downF);
   ::signal(SIGTERM, Votro::downF);

   job->loop();

   // shutdown

   tell(eloAlways, "shutdown");
   delete job;

   return 0;
}
