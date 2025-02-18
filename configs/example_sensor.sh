#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"
JARGS="$4"

VALUE=77.4

RESULT="{ \"type\":\"SC\",\"address\":$2,\"kind\":\"value\",\"valid\":true,\"value\":${VALUE},\"unit\":\"°C\" }"
echo -n ${RESULT}

if [ "$1" != "init" ]; then
  mosquitto_pub -L "${MQTTURL}" -m "${RESULT}"
fi
