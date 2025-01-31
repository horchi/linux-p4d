//***************************************************************************
// Automation Control
// File gpio.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.12.2021 Jörg Wendel
//***************************************************************************

/*
  https://wiki.odroid.com/odroid-n2/hardware/expansion_connectors
*/

#pragma once

#ifndef _NO_RASPBERRY_PI_
#  include <wiringPi.h>
#endif

#define bitSet(value, bit)   (value |= (1UL << bit))
#define bitClear(value, bit) (value &= ~(1UL << bit))
#define bitRead(value, bit)  ((value >> bit) & 0x01)
#define lowByte(word)        ((uint8_t)(word & 0xff))
#define highByte(word)       ((uint8_t)(word >> 8))

//***************************************************************************
// Physical Pin to GPIO Name
//  ODROID n2
//***************************************************************************

extern const char* physToGpioName_odroid_n2[64];
extern const char* physToGpioName_raspberry_pi[64];
const char* physPinToGpioName(int pin);

//***************************************************************************
// dummy functions
//***************************************************************************

#ifdef _NO_RASPBERRY_PI_

#include <stdlib.h>   // uint

#define PUD_UP 0
#define PUD_DOWN 0

#define INT_EDGE_FALLING 0
#define INT_EDGE_BOTH 2
#define OUTPUT 0
#define INPUT 0
#define HIGH 1
#define LOW 0

int physPinToGpio(uint pin);
int wiringPiISR(int, int, void (*)());
void pinMode(int, int);
void pullUpDnControl(int pin, int value);
int digitalWrite(uint pin, bool value);
int digitalRead(uint pin);
int wiringPiSetupPhys();

#endif // _NO_RASPBERRY_PI_

#ifdef MODEL_ODROID_N2

#else

#  define INPUT_PULLUP PUD_UP
#  define INPUT_PULLDOWN PUD_DOWN

#endif  // MODEL_ODROID_N2
