#! /bin/bash

MQTTURL="$1"
MSG="$2"

LOGGER="logger -t sensormqtt -p kern.warn"

#${LOGGER} "/etc/homectld/mqtt.d/tele_tasmota_SENSOR.sh: Convert ${MSG} to ${MQTTURL}"

sensor()
{
   ADDRESS=${1}
   ELEMENT="${2}"
   TITLE="${3}"
   UNIT="${4}"

   value=`echo ${MSG} | jq -r "${ELEMENT}"`

   RESULT=$(jo \
               type="TASMOTA" \
               address=${ADDRESS} \
               kind="value" \
               title="${TITLE}" \
               unit="${UNIT}" \
               valid=true \
               value=${value})

   ${LOGGER} "tele_tasmota_SENSOR.sh: -> ${RESULT}"
   mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"
}

sensor 1 '.data.Total_in' 'Total Consumption' 'kWh'
sensor 2 '.data.Total_out' 'Total fed in' 'kWh'
sensor 3 '.data.Power_curr' 'Actual Load' 'W'
