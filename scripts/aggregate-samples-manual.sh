

# echo "select time, aggregate from samples where time <= '$1'
#       and aggregate != 'A' limit 100;" | mysql -u p4 -pp4 -Dp4


if [[ ${#} -ne 2 ]]; then
   echo "Usage: "
   echo "  aggregate-samples-manual.sh date (like '2024-01-01') interval [minutes]"
   echo "    - aggregates all samples before the given date in the given interval"
   exit
fi

DATE=${1}
INT=${2}

# check statement
echo "select time, aggregate from samples where aggregate != 'A' and time <= '${DATE}' order by time limit 10;" | mysql -u p4 -pp4 -Dp4
echo "..."
echo "..."

echo "replace into samples
             select address, type, 'A' as aggregate,
               CONCAT(DATE(time), ' ', SEC_TO_TIME((TIME_TO_SEC(time) DIV ${INT}) * ${INT})) + INTERVAL 15 MINUTE time,
               unix_timestamp(sysdate()) as inssp, unix_timestamp(sysdate()) as updsp,
               round(sum(value)/count(*), 2) as value, text, count(*) samples
             from
               samples
             where
               aggregate != 'A' and  time <= '${DATE}'
             group by CONCAT(DATE(time), ' ', SEC_TO_TIME((TIME_TO_SEC(time) DIV ${INT}) * ${INT})) + INTERVAL 15 MINUTE, address, type;" | mysql -u p4 -pp4 -Dp4

echo "delete from samples where aggregate != 'A' and time <= '${DATE}';" | mysql -u p4 -pp4 -Dp4
