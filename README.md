# Linux - P4 Daemon (p4d)

This daemon is fetching data from the S 3200 and store it in a MySQL database.

Written by: *Jörg Wendel (linux at jwendel dot de)*

Homepage: https://github.com/horchi/linux-p4d

## License
This code is distributed under the terms and conditions of the GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.

## Disclaimer
USE AT YOUR OWN RISK. No warranty.
This software is a private contribution and not related Fröling. It may not work with future updates of the S3200 firmware and can also cause unintended behavior. Use at your own risk!
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Die Software wurde für den Eigengebrauch erstellt. Sie wird kostenlos unter der GPLv2 veröffentlicht.

Es ist kein fertiges Produkt, die Software entstand als Studie was hinsichtlich der Kommunikation
mit der s3200 Steuerung möglich ist und kann Bastlern als Basis und Anregung für eigene Projekte dienen.

Es besteht kein Anspruch auf Funktion, jeder der sie einsetzen möchte
muss das Risiko selbst abschätzen können und wissen was er tut, insbesondere auch in
Hinblick auf die Einstellungen der Heizungsparameter und den damit verbundenen Risiken
hinsichtlich Fehlfunktion, Störung, Brand, etc. Falsche Einstellung können unter anderem
durch Bedienfehler und Fehler in dieser Software ausgelöst werden!
Die Vorgaben, Vorschriften und AGB des Herstellers der Heizung bleiben maßgebend!
Ich kann  nicht ausschließen das es zu Fehlfunktionen oder unerwartetem Verhalten,
auch hinsichtlich der zur Heizung übertragenen Daten und damit verbundenen, mehr oder
weniger kritischen Fehlfunktionen derselben kommen kann!

## Donation
If this project help you, you can give me a cup of coffee :)

[![paypal](https://www.paypalobjects.com/de_DE/DE/i/btn/btn_donate_SM.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=KUF9ZAQ5UTHUN)

## Prerequisits:
- USB-Serial Converter based on FTDI chip
- USB-Serial converter must be connected to COM1 on Fröling mainboard
- A Linux based operating system is required

# Installation by package (actually available for Raspian Buster)

## install
```
wget www.jwendel.de/p4d/install-deb.sh -O /tmp/install-deb.sh
sudo bash /tmp/install-deb.sh
```

## uninstall
`dpkg --remove p4d`

## uninstall (with remove of all configurations, data and the p4d database)
`dpkg --purge p4d`

# Installation by source (working for most linux plattforms)

## Prerequisits:
-  the described installation is tested with Raspbian buster, the p4d should work also with other Linux distributions and versions but the installation process should adapted to them, for example they use other init processes
   or use different tools for the package management, other package names, ....
- de_DE.UTF-8 is required as language package (Raspberry command: `dpkg-reconfigure locales`)

## Preliminary:
Update your package data:
`sudo apt update`
and, if you like, update your installation:
`sudo apt dist-upgrade`

Perform all the following steps as root user! Either by getting root or by prefix each command with sodo.

## Installation of the MySQL Database:
It's not required to host the database on the Raspberry. A remote database is supported as well!

`apt install mysql-server`
some distributions like raspbian buster switched to mariadb, in this case:
`apt install mariadb-server`

(set database password for root user during installation)

### Local database setup:
If the database server is located localy on same host as the p4d:

```
> mysql -u root -Dmysql -p
  CREATE DATABASE p4 charset utf8;
  CREATE USER 'p4'@'localhost' IDENTIFIED BY 'p4';
  GRANT ALL PRIVILEGES ON p4.* TO 'p4'@'localhost' IDENTIFIED BY 'p4';
  flush privileges;
```
### Remote database setup:
if the database is running remote (on a other host or you like to habe remote access):
```
> mysql -u root -Dmysql -p
 CREATE DATABASE p4 charset utf8;
 CREATE USER 'p4'@'%' IDENTIFIED BY 'p4';
 GRANT ALL PRIVILEGES ON p4.* TO 'p4'@'%' IDENTIFIED BY 'p4';
 flush privileges;
```

## Installation of the p4d daemon:
### install the build dependencies
```
apt install build-essential libssl-dev libjansson-dev libxml2-dev libcurl4-openssl-dev libssl-dev libmariadbclient-dev libmariadb-dev-compat
```

### get the p4d and build it
```
cd /usr/src/
git clone https://github.com/horchi/linux-p4d/
cd linux-p4d
make clean all HASSMQTT=yes
make install
```
#### For older linux distributions which don't support the systemd init process use this instead of 'make install'
```
make install INIT_SYSTEM=sysV
```
#### If you like a other destination than '/usr/local' append PREFIX=your-destination (e.g. PREFIX=/usr) to all make calls above

#### or if you don't like to use the MQTT interface remove 'HASSMQTT=yes' from the commond above!

- Now P4 daemon is installed in folder `/usr/local/bin` and its config in /etc/p4d/
- Check `/etc/p4d.conf` file for setting db connection papameters, ttyDeviceSvc device (change device if required),
  check which `/dev/ttyUSB?` devices is used for USB-Serial converter (`/dev/ttyUSB0`, `/dev/ttyUSB1`, `/dev/ttyACM0`)

## Time to first start of p4d
```
systemctl start p4d
```
### to check it's state call
```
systemctl status p4d
```
it should now 'enabled' and in state 'running'!

### also check the syslog about errors of the p4d, this will show all its current log messages
```
grep "p4d:" /var/log/syslog
```

## Aggregation / Cleanup
The samples will recorded in the configured interval (parameter interval in p4d.conf), the default is 60 Seconds.
After a while the database will grow and the selects become slower. Therefore you can setup a automatic aggregation in the `p4d.conf` with this two parameters:
The `aggregateHistory` is the history for aggregation in days, the default is 0 days -> aggegation turned OFF
The `aggregateInterval` is the aggregation interval in minutes - 'one sample per interval will be build' (default 15 minutes)

For example:
```
aggregateHistory = 365
aggregateInterval = 15
```

Means that all samples older than 365 days will be aggregated to one sample per 15 Minutes.
If you like to delete 'old' samples you have to do the cleanup job by hand, actually i don't see the need to delete anything, I like to hold my data (forever :o ?).
Maybe i implement it later ;)

## WEB interface:

The default port of the WEB interface is 1111, The default username and password for the login is
User: *p4*
Pass: *p4-3200*

### Fist steps to enable data logging:

1. Log in to the web interface
2. Goto Setup
  - click 'Init Messwerte'
3. Goto Setup -> Aufzeichnung
  - Select the values you like to record and store your selection (click 'Speichern')
4. Goto Setup
  - click 'Init Service Menü'

After this you can set up the schema configuration. The schema configuration is done by drag&drop and isn't working on mobile devices!
Also deleting of chart bookmarks isn't supported on mobile devices.

### Setup and configure sending mails:
The next steps are an example to setup the sending of mails. If another tool is preferred it can be used as well. The config is based on GMX. If you have another provider please search in the Internet for the required config.
- Install required components
  - `apt-get install ssmtp mailutils`
- The mailscript `p4d-mail.sh` is copied during the `make install` to the folder `/usr/local/bin`. This script is used in our case to send mails
- Change revaliases file edit file /etc/ssmtp/revaliases and add line (gmx is used as an example)
  - `root:MyMailAddress@gmx.de:mail.gmx.net:25`
- Change `ssmtp.conf` file. Edit file `/etc/ssmtp/ssmtp.conf` (gmx is used as an example)
```
root=MyMailAddress@gmx.de
mailhub=mail.gmx.net
hostname=gmx.net
UseSTARTTLS=YES
AuthUser=MyMailAddress@gmx.de
AuthPass=MyPassword
```
- Start the WEBIF and Login, go to the Setup page
- Configure the "Mail Benachrichtigungen" options (status and Fehler Mail Empfänger)
- If no Status options are set you will get a mail for each status change
- Set the script option `/usr/local/bin/p4d-mail.sh`

If the heating values are added as attachment to the mail please check the next steps.
- Check if `heirloom-mailx` is installed (`ls -lah /etc/alternatives/mail`)
- If output link is `/etc/alternatives/mail -> /usr/bin/mail.mailutils`
- Remove `heirloom-mailx` (`apt remove heirloom-mailx`)

### Configure Time sync:
With the next steps you can enable a time synchronization of the p4d and the heating:
If you enable this feature you can set the max time difference between the p4d systemtime and the time of the heating. The p4d will set the time of the heating (once a day) to his the systemtime. Therefore it is recommended to hold your system time in sync, e.g. by running the `ntp` daemon or systemd-timesyncd.

- Check if ntpd (openntpd, systemd-timesyncd, ...) is active and running. If not you have to install one of it.
- Start the WEBIF and login, go to the Setup page
- Enable "Tägliche Zeitsynchronisation" and set the max time difference in seconds in the line "Maximale Abweichung"
- Save configuration

### MQTT Interface

Configure the parameters at the WEBIF
- 'MQTT Url' : The URL of your MQTT broker
- 'MQTT Data Topic Name' : The name of the topic which should be written. You can use the '&lt;NAME&gt;' template here, this will be replaced by the name of the sensor, therefore one topic for each sensor will be created and used (as needed by the homeassistant). If you (e.g.) using Openhab or FHEM omit '&lt;NAME&gt;' and all data is written to one topic.
                           The '&lt;GROUP&gt;' template, will use the configured group name and sort the sensors regarding its group configuration to the topics. Don't use the GROUP and NAME template together!
- 'Config Topic:' : Check box to write the sensor defaults to a config topic and register the separate config topics there (as used by the homeassistant).

### One Wire Sensors:
The p4d checks automatically if there are 'One Wire Sensors' connected, each detected sensor will be
configurable via the web interface after reading the sensor facts by clicking [init].

To make the sensors availaspble to the raspi you have to load the `w1-gpio` module, this can be done by calling `modprobe w1-gpio` or automatically at boot by registering it in `/etc/modules`:
```
echo "w1-gpio" >> /etc/modules
echo "w1_therm" >> /etc/modules
echo "dtoverlay=w1-gpio,gpioin=4,pullup=on" >> /boot/config.txt
```

### Script Sensors:
To use other additional sensors which are not supported by the p4d directly you can use the script-sensor interface of p4d.
To activate the script-sensor interface copy the example script scripts/script-sensor.sh from the git to /etc/p4d (don't rename it).
Now you can add script sensors with this command:
```
insert into valuefacts set type = 'SC', address = 1, inssp = UNIX_TIMESTAMP(), updsp = UNIX_TIMESTAMP(), groupid = 1, state = 'D', unit = '%', factor = 1, name = 'TestScriptSensor0x01', title = 'Test Script Sensor', maxscale = 100; | mysql -u p4 -pp4 -Dp4
```
Change address to a uniq one, name, title, unit as you like. maxscale and state can set later on in the WEBIF.
After this restart the p4d!
Now edit the script /etc/p4/script-sensor.sh that it output the actual value of this sensors using it's address.

### Points to check
- reboot the device to check if p4d is starting automatically during startup


## Backup
Backup the data of the p4 database including all recorded values:

```
p4d-backup
```

This will create the following files:

```
config-dump.sql.gz
errors-dump.sql.gz
jobs-dump.sql.gz
menu-dump.sql.gz
samples-dump.sql.gz
schemaconf-dump.sql.gz
sensoralert-dump.sql.gz
smartconfig-dump.sql.gz
valuefacts-dump.sql.gz
timeranges-dump.sql.gz
hmsysvars-dump.sql.gz
scripts-dump.sql.gz
```

To import the backup:
```
gunzip NAME-dump.sql.gz
mysql -u p4 -pp4 -Dp4 <  NAME-dump.sql
```
replace NAME with the name of the dump

#### ATTENTION:
The import deletes at first all the data and then imports the dumped data. To append the dumped data you have to modify the SQL statements inside the dump files manually.

## Additional information

### MySQL HINTS
If you cannot figure out why you get Access denied, remove all entries from the user table that have Host values with wildcars contained (entries that match `%` or `_` characters). A very common error is to insert a new entry with `Host='%'` and `User='some_user'`, thinking that this allows you to specify localhost to connect from the same machine. The reason that this does not work is that the default privileges include an entry with `Host='localhost'` and `User=''`. Because that entry has a Host value `localhost` that is more specific than `%`, it is used in preference to the new entry when connecting from localhost! The correct procedure is to insert a second entry with `Host='localhost'` and `User='some_user'`, or to delete the entry with `Host='localhost'` and `User=''`. After deleting the entry, remember to issue a `FLUSH PRIVILEGES` statement to reload the grant tables.

To analyze this you can show all users:
```
use mysql
SELECT host, user FROM user;
```

## Alternative install by script of Philipp:

```
cd ..
cd p4d
wget http://hungerphilipp.de/files/p4d/install.sh
chmod +x install.sh
./install.sh" or "sudo ./install.sh/
```

## Problems - anything is not working
 - check the syslog about errors of the p4d
 - for the WEBIF check also the browser console for errors
If you post any problem at at git-hub or the forum please post the errors also!
