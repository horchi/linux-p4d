
if [ "$1" == "-h" ] || [ -z "$1" ] || [ -z "$2" ]; then

  echo "  Usage: setp <address> <value>"
  exit 0
fi

id="$1"
value="$2"
unow=`date "+%s"`
snow=`date "+%Y-%m-%d %T"`

mysql -u p4 -pp4 -Dp4 --default-character-set=utf8 -e " \
  insert into jobs set updsp = '$unow', requestat = '$snow', state = 'P', command = 'setp', address = '$id', data = '$value';"
