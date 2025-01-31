//***************************************************************************
// Automation Control
// File gpio.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2024 Jörg Wendel
//***************************************************************************

#include "gpio.h"

//***************************************************************************
// Physical Pin To GPIO Name
//***************************************************************************

const char* physToGpioName_odroid_n2[64]
{
   // physical header pin number to gpio name

   nullptr,                                           // kein pin 0
   "3.3V",                 "5.0V",                    //  1 |  2
   "GPIOX.17 (I2C-2_SDA)", "5.0V",                    //  3 |  4
   "GPIOX.18 (I2C-2_SCL)", "GND",                     //  5 |  6
   "GPIOA.13",             "GPIOX.12 (UART_TX_B)",     //  7 |  8
   "GND",                  "GPIOX.13 (UART_RX_B)",     //  9 | 10
   "GPIOX.3",              "GPIOX.16 (PWM_E)",         // 11 | 12
   "GPIOX.4",              "GND",                     // 13 | 14
   "GPIOX.7 (PWM_F)",      "GPIOX.0",                 // 15 | 16
   "3.3V",                 "GPIOX.1",                 // 17 | 18
   "GPIOX.8 (SPI_MOSI)",   "GND",                     // 19 | 20
   "GPIOX.9 (SPI_MISO)",   "GPIOX.2",                 // 21 | 22
   "GPIOX.11 (SPI_SCLK)",  "GPIOX.10 (SPI_CE0)",       // 23 | 24
   "GND",                  "GPIOA.4 (SPI_CE1)",        // 25 | 26
   "GPIOA.14 (I2C-3_SDA)", "GPIOA.15 (I2C-3_SCL)",     // 27 | 28
   "GPIOX.14",             "GND",                     // 29 | 30
   "GPIOX.15",             "GPIOA.12",                // 31 | 32
   "GPIOX.5 (PWM_C)",      "GND",                     // 33 | 34
   "GPIOX.6 (PWM_D)",      "GPIOX.19",                // 35 | 36
   "ADC.AIN3",             "1.8V REF OUT",            // 37 | 38
   "GND",                  "ADC.AIN2",                // 39 | 40

   // Not used

   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  // 41...48
   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  // 49...56
   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr            // 57...63
};

const char* physToGpioName_raspberry_pi[64]
{
   // physical header pin number to gpio name

   nullptr,                                           // kein pin 0
   "3.3V",                 "5.0V",                    //  1 |  2
   "GPIOX.17 (I2C-2_SDA)", "5.0V",                    //  3 |  4
   "GPIOX.18 (I2C-2_SCL)", "GND",                     //  5 |  6
   "GPIOA.13",             "GPIOX.12 (UART_TX_B)",     //  7 |  8
   "GND",                  "GPIOX.13 (UART_RX_B)",     //  9 | 10
   "GPIOX.3",              "GPIOX.16 (PWM_E)",         // 11 | 12
   "GPIOX.4",              "GND",                     // 13 | 14
   "GPIOX.7 (PWM_F)",      "GPIOX.0",                 // 15 | 16
   "3.3V",                 "GPIOX.1",                 // 17 | 18
   "GPIOX.8 (SPI_MOSI)",   "GND",                     // 19 | 20
   "GPIOX.9 (SPI_MISO)",   "GPIOX.2",                 // 21 | 22
   "GPIOX.11 (SPI_SCLK)",  "GPIOX.10 (SPI_CE0)",       // 23 | 24
   "GND",                  "GPIOA.4 (SPI_CE1)",        // 25 | 26
   "GPIOA.14 (I2C-3_SDA)", "GPIOA.15 (I2C-3_SCL)",     // 27 | 28
   "GPIOX.14",             "GND",                     // 29 | 30
   "GPIOX.15",             "GPIOA.12",                // 31 | 32
   "GPIOX.5 (PWM_C)",      "GND",                     // 33 | 34
   "GPIOX.6 (PWM_D)",      "GPIOX.19",                // 35 | 36
   "ADC.AIN3",             "1.8V REF OUT",            // 37 | 38
   "GND",                  "ADC.AIN2",                // 39 | 40

   // Not used

   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  // 41...48
   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,  // 49...56
   nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr            // 57...63
};

#ifndef _NO_RASPBERRY_PI_

//***************************************************************************
// compile WITH wiringPi
//***************************************************************************

const char* physPinToGpioName(int pin)
{
   if (pin < 0 || pin >= 64)
      return "";

#ifndef MODEL_ODROID_N2
   if (!physToGpioName_raspberry_pi[pin])
      return "";
   return physToGpioName_raspberry_pi[pin];
#else
   if (!physToGpioName_odroid_n2[pin])
      return "";
   return physToGpioName_odroid_n2[pin];
#endif
}

#else // _NO_RASPBERRY_PI_

//***************************************************************************
// compile WITHOPUT wiringPi
//***************************************************************************

const char* physPinToGpioName(int pin)
{
   return "";
}

int physPinToGpio(uint pin)
{
   return 0;
}

int wiringPiISR(int, int, void (*)())
{
   return 0;
}

void pinMode(int, int)
{
}

void pullUpDnControl(int pin, int value)
{
}

int digitalWrite(uint pin, bool value)
{
   return 0;
}

int digitalRead(uint pin)
{
   return 0;
}

int wiringPiSetupPhys()
{
   return 0;
}

#endif // _NO_RASPBERRY_PI_
