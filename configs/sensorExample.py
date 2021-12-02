#  ------------------------------------------------------------------------------------------# ----------------------------------------------------
# - after importing 'sensor' module the sensor object is available
#
# - implement at least the function 'sensorCall(command, typ, addr)'
#  ------------------------------------------------------------------------------------------

import sensor
import json

def sensorCall(command, typ, addr):

        import locale
        locale.setlocale(locale.LC_ALL, 'de_DE.UTF-8')
        import time

        abgas = json.loads(sensor.getData("VA", 0x01))

        me = {
                "kind": "value",
                "title": "Python Test",
                "unit": "°C"
        }

        me["value"] = abgas["value"] / 2

        return json.dumps(me)
