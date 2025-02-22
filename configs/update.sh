#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"
JARGS="$4"

LOGGER="logger -t sensormqtt -p kern.warn"

GIT_ROOT=`echo ${JARGS} | jq -r .gitroot`
SERVICE=`echo ${JARGS} | jq -r .service`

if [[ -z "${GIT_ROOT}" ]]; then
   GIT_ROOT="/root/source/linux-homectld"
fi

if [[ -z "${SERVICE}" ]]; then
   SERVICE="homectld.service"
fi

UPD_PENDING=0
STATE="false"
COLOR="\"red\""

if [[ ! -d "${GIT_ROOT}" ]]; then
   STATE="false"
   COLOR="\"red\""
elif ping -q -c 1 -W 1 8.8.8.8 >/dev/null; then

   cd "${GIT_ROOT}"
   export HOME="/root"
   LOCAL=`git rev-parse HEAD`
   REMOTE=`git ls-remote origin -h refs/heads/master`
   REMOTE=`echo $REMOTE | sed s/" .*"/""/`

   if [[ "${LOCAL}" != "${REMOTE}" ]]; then
      ${LOGGER} "update.sh: Update to ${REMOTE} pending"
      COLOR="\"blue\""
      UPD_PENDING=1
   else
      # ${LOGGER} "update.sh: NO update pending, commit id is ${LOCAL}"
      COLOR="\"green\""
   fi

   STATE="true"
   echo -n
else
   STATE="false"
fi

if [[ "${COMMAND}" == "init" ]]; then
   PARAMETER="{\"cloneable\": false, \"symbol\": \"mdi:mdi-progress-upload\", \"symbolOn\": \"mdi:mdi-progress-upload\"}"
   RESULT="{ \"type\":\"SC\",\"address\":$2,\"kind\":\"status\",\"valid\":true,\"value\":${STATE}, \"color\": ${COLOR}, \"parameter\": ${PARAMETER}  }"
   echo -n ${RESULT}
elif [[ "${COMMAND}" == "toggle" ]]; then
   if [[ ! -d "${GIT_ROOT}" ]]; then
      ${LOGGER} "update.sh: Abort update, directory ${GIT_ROOT} not found"
   elif [[ "${STATE}" == "false" ]]; then
      ${LOGGER} "update.sh: internet connection is not available"
   elif [[ ${UPD_PENDING} -eq 1 ]]; then
      cd "${GIT_ROOT}"
      ${LOGGER} "update.sh: git pull"
      git pull >/dev/null 2>&1 | ${LOGGER}
      ${LOGGER} "update.sh: rebuild, will take up to minutes"
      make -s clean >/dev/null 2>&1 | ${LOGGER}
      make -j 3 >/dev/null 2>&1 | ${LOGGER}
      ${LOGGER} "update.sh: install"
      make linstall >/dev/null 2>&1 | ${LOGGER}
      ${LOGGER} "update.sh: restart"
      /bin/systemctl restart ${SERVICE} 2>&1 | ${LOGGER}

      ${LOGGER} "update.sh: restart result was $?"

      if [[ $? -ne 0 ]]; then
         STATE="false"
         COLOR="\"red\""
      fi
   fi

   RESULT="{ \"type\": \"SC\", \"address\": ${ADDRESS}, \"kind\": \"status\", \"state\": ${STATE}, \"color\": ${COLOR} }"
   mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"

elif [ "${COMMAND}" == "status" ]; then
   RESULT="{ \"type\": \"SC\", \"address\": ${ADDRESS}, \"kind\": \"status\", \"state\": ${STATE}, \"color\": ${COLOR} }"
   # ${LOGGER} "update.sh: Publish ${RESULT}"
   mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"
fi

exit 0
