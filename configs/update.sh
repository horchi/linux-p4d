#! /bin/bash

COMMAND="$1"
ADDRESS="$2"
MQTTURL="$3"

LOGGER="logger -t homectld -p kern.warn"

GIT_ROOT="/root/source/linux-homectld"
STATE="false"
COLOR="\"green\""

if [[ ! -d "${GIT_ROOT}" ]]; then
	STATE="false"
elif ping -q -c 1 -W 1 8.8.8.8 >/dev/null; then

   cd "${GIT_ROOT}"
   export HOME="/root"
   LOCAL=`git rev-parse HEAD`
   REMOTE=`git ls-remote origin -h refs/heads/master`
   REMOTE=`echo $REMOTE | sed s/" .*"/""/`

   if [[ "${LOCAL}" != "${REMOTE}" ]]; then
      COLOR="null"
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
		${LOGGER} "Abort update, directory ${GIT_ROOT} not found"
	elif [[ "${STATE}" == "false" ]]; then
		${LOGGER} "update.sh: internet connection is not available"
	else
		cd "${GIT_ROOT}"
		${LOGGER} "update.sh: git pull"
		git pull >/dev/null 2>&1 | ${LOGGER}
		${LOGGER} "update.sh: rebuild"
		make -s clean >/dev/null 2>&1 | ${LOGGER}
		make -j 3 >/dev/null 2>&1 | ${LOGGER}
		${LOGGER} "update.sh: install"
		make linstall >/dev/null 2>&1 | ${LOGGER}
		${LOGGER} "update.sh: restart"
		/bin/systemctl restart homectld.service 2>&1 | ${LOGGER}
	fi
elif [ "${COMMAND}" == "status" ]; then
   RESULT="{ \"type\": \"SC\", \"address\": ${ADDRESS}, \"kind\": \"status\", \"state\": ${STATE}, \"color\": ${COLOR} }"
   # ${LOGGER} "update.sh: Publish ${RESULT}"
   mosquitto_pub --quiet -L ${MQTTURL} -m "${RESULT}"
fi

exit 0
