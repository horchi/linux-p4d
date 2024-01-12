//***************************************************************************
// Automation Control
// File growatt.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.11.2010 - 01.12.2023  Jörg Wendel
//***************************************************************************

#include "growatt.h"

//***************************************************************************
// Gro Watt
//***************************************************************************

std::map<std::string,GroWatt::Def> GroWatt::sensors =
{
   { "ACPowerToGrid",                 {  1, "W" } },
   { "ACPowerToGridTotal",            {  2, "W" } },
   { "ACPowerToUser",                 {  3, "W" } },
   { "ACPowerToUserTotal",            {  4, "W" } },
   { "BatteryState",                  {  5, "" } },
   { "BatteryTemperature",            {  6, "°C" } },
   { "BatteryVoltage",                {  7, "V" } },
   { "BoostTemperature",              {  8, "°C" } },
   { "ChargeEnergyToday",             {  9, "kWh" } },
   { "ChargeEnergyTotal",             { 10, "kWh" } },
   { "ChargePower",                   { 11, "W" } },
   { "Cnt",                           { 12, "" } },
   { "DischargeEnergyToday",          { 13, "kWh" } },
   { "DischargeEnergyTotal",          { 14, "kWh" } },
   { "DischargePower",                { 15, "W" } },
   { "EnergyToGridToday",             { 16, "kWh" } },
   { "EnergyToGridTotal",             { 17, "kWh" } },
   { "EnergyToUserToday",             { 18, "kWh" } },
   { "EnergyToUserTotal",             { 19, "kWh" } },
   { "GridFrequency",                 { 20, "Hz" } },
   { "INVPowerToLocalLoad",           { 21, "W" } },
   { "INVPowerToLocalLoadTotal",      { 22, "W" } },
   { "InputPower",                    { 23, "W" } },
   { "InverterStatus",                { 24, "" } },
   { "InverterTemperature",           { 25, "°C" } },
   { "L1ThreePhaseGridOutputCurrent", { 26, "A" } },
   { "L1ThreePhaseGridOutputPower",   { 27, "W" } },
   { "L1ThreePhaseGridVoltage",       { 28, "V" } },
   { "L2ThreePhaseGridOutputCurrent", { 29, "A" } },
   { "L2ThreePhaseGridOutputPower",   { 30, "W" } },
   { "L2ThreePhaseGridVoltage",       { 31, "V" } },
   { "L3ThreePhaseGridOutputCurrent", { 32, "A" } },
   { "L3ThreePhaseGridOutputPower",   { 33, "W" } },
   { "L3ThreePhaseGridVoltage",       { 34, "V" } },
   { "LocalLoadEnergyToday",          { 35, "kWh" } },
   { "LocalLoadEnergyTotal",          { 36, "kWh" } },
   { "Mac",                           { 37, "" } },
   { "OutputPower",                   { 38, "W" } },
   { "PV1EnergyToday",                { 39, "kWh" } },
   { "PV1EnergyTotal",                { 40, "kWh" } },
   { "PV1InputCurrent",               { 41, "A" } },
   { "PV1InputPower",                 { 42, "W" } },
   { "PV1Voltage",                    { 43, "V" } },
   { "PV2EnergyToday",                { 44, "kWh" } },
   { "PV2EnergyTotal",                { 45, "kWh" } },
   { "PV2InputCurrent",               { 46, "A" } },
   { "PV2InputPower",                 { 47, "W" } },
   { "PV2Voltage",                    { 48, "V" } },
   { "PVEnergyTotal",                 { 49, "kWh" } },
   { "SOC",                           { 50, "%" } },
   { "WorkTimeTotal",                 { 51, "s" } },
   { "TemperatureInsideIPM",          { 52, "°C" } },
   { "TodayGenerateEnergy",           { 53, "kWh" } },
   { "TotalGenerateEnergy",           { 54, "kWh" } }
};

//***************************************************************************
// Get Address Of
//***************************************************************************

int GroWatt::getAddressOf(const char* key)
{
   const auto it = sensors.find(key);

   if (it == sensors.end())
      return na;

   return it->second.address;
}
