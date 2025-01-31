#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"

STATE="true"

if [[ "${COMMAND}" == "init" ]]; then
   PARAMETER='{"cloneable": false, "symbol": "mdi:mdi-restart", "symbolOn": "mdi:mdi-restart", "color": "rgb(225, 5, 3)", "colorOn": "rgb(225, 5, 3)"}'
   RESULT="{ \"type\":\"SC\",\"address\":$2,\"kind\":\"status\",\"valid\":true,\"value\":${STATE}, \"parameter\": ${PARAMETER} }"
echo -n ${RESULT}
elif [ "${COMMAND}" == "toggle" ]; then
   systemctl reboot
elif [ "${COMMAND}" == "status" ]; then
   mosquitto_pub --quiet -L ${MQTTURL} -m "{ \"type\":\"SC\",\"address\":${ADDRESS},\"kind\":\"status\",\"state\":${STATE} }"
fi

exit 0
