#!/bin/bash

echo "$2" | mail -s "$1" -a "Content-Type: text/plain; charset=UTF-8" $3

#echo "$2" | iconv -f utf-8 -t iso-8859-1 | mail -s "$1" $3



