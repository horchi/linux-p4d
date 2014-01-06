#!/bin/bash


# -----------------------------------------
# require: mail of 'GNU Mailutils' package


echo "$2" | mail -s "$1" -a "Content-Type: text/plain; charset=UTF-8" $3

