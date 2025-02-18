#! /bin/bash

MQTTURL="$1"
MSG="$2"

LOGGER="logger -t homectld -p kern.warn"

# ${LOGGER} "/etc/homectld/mqtt.d/tele_tasmota_SENSOR.sh: Convert ${MSG} to ${MQTTURL}"

totalIn=`echo ${MSG} | jq -r .SML.Total_in`

RESULT=$(jo \
            type="TASMOTA" \
            address=1 \
            kind="value" \
            title="Total Consumption" \
            unit="kWh" \
            valid=true \
            value=${totalIn})

${LOGGER} "tele_tasmota_SENSOR.sh: -> ${RESULT}"
mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"


totalIn=`echo ${MSG} | jq -r .SML.Total_out`

RESULT=$(jo \
            type="TASMOTA" \
            address=2 \
            kind="value" \
            title="Total fed in" \
            unit="kWh" \
            valid=true \
            value=${totalIn})

${LOGGER} "tele_tasmota_SENSOR.sh: -> ${RESULT}"
mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"

totalIn=`echo ${MSG} | jq -r .SML.Power_curr`

RESULT=$(jo \
            type="TASMOTA" \
            address=3 \
            kind="value" \
            title="Actual load" \
            unit="W" \
            valid=true \
            value=${totalIn})

${LOGGER} "tele_tasmota_SENSOR.sh: -> ${RESULT}"
mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"
