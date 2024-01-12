#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"

STATE="true"

RESULT="{ \"type\":\"SC\",\"address\":$2,\"kind\":\"status\",\"valid\":true,\"value\":${STATE} }"
echo -n ${RESULT}

if [ "${COMMAND}" == "init" ]; then
   exit 0
fi

if [ "${COMMAND}" == "start" ]; then
   systemctrl restart homectld.service
fi

if [ "${COMMAND}" != "init" ]; then
   mosquitto_pub --quiet -L ${MQTTURL} -m "{ \"type\":\"SC\",\"address\":${ADDRESS},\"kind\":\"status\",\"state\":${STATE} }"
fi

exit 0
