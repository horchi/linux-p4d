//***************************************************************************
// Automation Control
// File gpio.h
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 16.04.2020 Jörg Wendel
//***************************************************************************

#include "gpio.h"

// ----------------------------------
// dummies to be removed
// ----------------------------------

#ifdef _NO_RASPBERRY_PI_

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
