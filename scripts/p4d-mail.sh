
echo "$2" | iconv -f utf-8 -t iso-8859-1 | mail -s "$1" $3

