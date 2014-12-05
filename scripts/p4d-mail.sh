#!/bin/bash


# -----------------------------------------
# require: mail of 'GNU Mailutils' package


echo "$2" | mail -s "$1" -a "Content-Type: $3; charset=UTF-8" $4
