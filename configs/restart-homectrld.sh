#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"

STATE="true"

if [[ "${COMMAND}" == "init" ]]; then
   PARAMETER='{"cloneable": false, "symbol": "mdi:mdi-redo", "symbolOn": "mdi:mdi-redo", "color": "rgb(235, 197, 5)", "colorOn": "rgb(235, 197, 5)"}'
   RESULT="{ \"type\":\"SC\",\"address\":$2,\"kind\":\"status\",\"valid\":true,\"value\":${STATE}, \"parameter\": ${PARAMETER} }"
   echo -n ${RESULT}
elif [ "${COMMAND}" == "toggle" ]; then
   DIR=`dirname "$0"`
   ${DIR}/sysctl "restart" "$2" "$3" "homectld.service"
elif [ "${COMMAND}" == "status" ]; then
   mosquitto_pub --quiet -L ${MQTTURL} -m "{ \"type\":\"SC\",\"address\":${ADDRESS},\"kind\":\"status\",\"state\":${STATE} }"
fi

exit 0
