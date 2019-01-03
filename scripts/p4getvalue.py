#!/usr/bin/python

import mysql.connector
import sys
import logging

logging.basicConfig(format='%(asctime)s %(levelname)s %(process)d %(message)s',
					datefmt='%d.%m.%Y %H:%M:%S',
					filename='/tmp/p4get.log',
					level=logging.DEBUG)

ptype = "VA"
field = "value"
address = sys.argv[1]

if len(sys.argv) > 2:
    ptype = sys.argv[2]

if ptype == "UD":
    field = "text"

logging.info("  called with ptype " + ptype + " address " + address)

try:
	mydb = mysql.connector.connect(host="gate", user="p4", passwd="p4", database="p4")
	mycursor = mydb.cursor()
	logging.debug("  connected to database")

	sql = "select %s from samples where time = (select max(time) from samples) and address = %s and type = '%s'" % \
											   (field, address, ptype)

	logging.debug("  sql: " + sql)
	mycursor.execute(sql)
	myresult = mycursor.fetchall()

except Error as e:
    logging.error(e)
    print("na")
    quit()

logging.debug("  sql executed, got " + "{:d}".format(mycursor.rowcount) + " row(s)")

if mycursor.rowcount <= 0:
    logging.warn("No rows found!")
    print("na")
    quit()

value = myresult[0][0]

if isinstance(value, str) or isinstance(value, unicode):
	value = value.encode('ascii', 'ignore').decode('ascii')
elif isinstance(value, float):
	value = "{:.2f}".format(value)
elif isinstance(value, int):
	value = "{:d}".format(value)

if value == 'STRUNG':
    value = 'STOERUNG'

print(value)
logging.info("  result was " + value)
