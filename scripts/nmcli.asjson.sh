#!/bin/bash

COMMAND="$1"

if [ "${COMMAND}" == "wifi-list" ]; then
   printf '%s' "$(nmcli -f bssid,ssid,mode,chan,rate,signal,bars,security,active,in-use -t dev wifi)" | sed s/"[\]:"/"-"/g |\
      jq -sR 'split("\n") | map(split(":")) | map({"id": .[0],
                                               "network": .[1],
                                               "mode": .[2],
                                               "channel": .[3],
                                               "rate": .[4],
                                               "signal": .[5],
                                               "bars": .[6],
                                               "security": .[7],
                                               "active": .[8],
                                               "inuse": .[9]
                                             })' | json_pp -json_opt canonical,utf8
   exit 0
elif [ "${COMMAND}" == "wifi-con" ]; then
   printf '%s' "$(nmcli -t -f name,autoconnect,autoconnect-priority,active,device,state,type connection show | grep 'wireless')" |\
      jq -sR 'split("\n") | map(split(":")) | map({"network": .[0],
                                               "autoconnect": .[1],
                                               "priority": .[2],
                                               "active": .[3],
                                               "device": .[4],
                                               "state": .[5],
                                               "type": .[6]
                                             })' | json_pp -json_opt canonical,utf8

   exit 0
fi

echo "Usage: $0 { wifi-list|wifi-con }"
