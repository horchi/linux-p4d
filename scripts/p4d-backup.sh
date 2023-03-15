#!/bin/bash

export MYSQL_PWD=p4

function dumpTable()
{
   name=$1
   file=./$name-dump.sql.gz

   echo "dumping table $name to $file"

   mysqldump --opt -u p4 -p$MYSQL_PWD p4 $name | gzip > $file
   status=${PIPESTATUS[0]}

   if [ $status != 0 ]; then
      rm -f "$file"
      echo "failed to dump table $name"
   else
      echo "succeeded"
   fi

   return $status
}

# ----------------------------------------------------------------
# main
# ----------------------------------------------------------------

if ! dumpTable "config";      then  exit 1; fi
if ! dumpTable "errors";      then  exit 1; fi
if ! dumpTable "menu";        then  exit 1; fi
if ! dumpTable "samples";     then  exit 1; fi
if ! dumpTable "schemaconf";  then  exit 1; fi
if ! dumpTable "sensoralert"; then  exit 1; fi
if ! dumpTable "valuefacts";  then  exit 1; fi
if ! dumpTable "timeranges";  then  exit 1; fi
if ! dumpTable "scripts";     then  exit 1; fi
if ! dumpTable "groups";      then  exit 1; fi
if ! dumpTable "peaks";       then  exit 1; fi
if ! dumpTable "pellets";     then  exit 1; fi
if ! dumpTable "users";       then  exit 1; fi
if ! dumpTable "dashboards";  then  exit 1; fi
if ! dumpTable "dashboardwidgets"; then  exit 1; fi
if ! dumpTable "states";      then  exit 1; fi
if ! dumpTable "valuetypes";  then  exit 1; fi

echo "to import the tables call mysql per file:"
echo "  zcat the-dumpfile.gz | mysql -u p4 -pp4 -Dp4"
echo " "
echo "Attention: At the import all data get lost and will be replaced with the content of the dump files!"
