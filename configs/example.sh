#! /bin/bash

case "$1" in
   start)
      echo 1 >> /tmp/foo
      ;;

   stop)
      rm -f /tmp/foo
      ;;

   toggle)
      if [[ -f /tmp/foo ]]; then
         rm -f /tmp/foo
      else
         echo 1 >> /tmp/foo
      fi
      ;;

   status)
      ;;

   *)
      echo "Usage: {start|stop|status|toggle}"
      ;;

esac

if [[ -f /tmp/foo ]]; then
   RETVAL=0
else
   RETVAL=1
fi

if [ $RETVAL != 0 ]; then
   echo -n '{ "kind":"status","value":0 }'
   exit 1
fi

echo -n '{ "kind":"status","value":1 }'

exit 0
