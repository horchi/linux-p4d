
# https://github.com/sudar/Arduino-Makefile

TARGET        = ioctrl
BOARD_TAG     = mkrwifi1010
#MCU           = cortex-m0plus

#BOARD_TAG     = nano
#BOARD_SUB     = atmega328
#MCU           = atmega328p

ARDUINO_DIR   = /home/wendel/arduino-1.8.15
MONITOR_PORT  = /dev/ttyUSB0
AVRDUDE_ARD_BAUDRATE = 57600
CXXFLAGS     += -Wno-deprecated-declarations

#include /usr/share/arduino/Arduino.mk
include /home/wendel/Arduino-Makefile/Sam.mk

BIN  = $(TARGET_HEX)
DEST = ../$(OBJDIR).$(TARGET).hex
