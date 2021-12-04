//***************************************************************************
// Automation Control
// File gpio.c
// This code is distributed under the terms and conditions of the
// GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
// Date 04.12.2021  Jörg Wendel
//***************************************************************************

#pragma once

#ifdef _NO_RASPBERRY_PI_

#include <stdlib.h>   // uint

#define PUD_UP 0
#define INT_EDGE_FALLING 0
#define OUTPUT 0
#define INPUT 0

int wiringPiISR(int, int, void (*)());
void pinMode(int, int);
void pullUpDnControl(int pin, int value);
int digitalWrite(uint pin, bool value);
int digitalRead(uint pin);
int wiringPiSetupPhys();

#endif // _NO_RASPBERRY_PI_
