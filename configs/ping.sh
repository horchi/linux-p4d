#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"
JARGS="$4"
STATE="true"

LOGGER="logger -t homectld -p kern.warn"
IP=`echo ${JARGS} | jq -r .ip`

if [[ -z "${IP}" ]]; then
   ${LOGGER} "ping.sh: IP argument missing, call with '{ \"ip\": \"8.8.8.8\"}'"
   STATE="false"
elif ping -q -c 1 -W 1 ${IP} >/dev/null; then
	echo -n
else
	STATE="false"
fi

if [[ "${COMMAND}" == "init" ]]; then
   PARAMETER='{"cloneable": true, "symbol": "mdi:mdi-router-wireless-off", "symbolOn": "mdi:mdi-router-wireless"}'
   RESULT="{ \"type\":\"SC\",\"address\":$2,\"kind\":\"status\",\"valid\":true,\"value\":${STATE}, \"parameter\": ${PARAMETER} }"
   echo -n ${RESULT}
elif [[ "${COMMAND}" == "toggle" ]]; then
   ${LOGGER} "ping.sh: update; called with IP: ${IP}"
   mosquitto_pub --quiet -L ${MQTTURL} -m "{ \"type\":\"SC\",\"address\":${ADDRESS},\"kind\":\"status\",\"state\":${STATE} }"
elif [ "${COMMAND}" == "status" ]; then
   mosquitto_pub --quiet -L ${MQTTURL} -m "{ \"type\":\"SC\",\"address\":${ADDRESS},\"kind\":\"status\",\"state\":${STATE} }"
fi

exit 0
