# Linux - P4 Daemon (p4d)

This daemon is fetching data from the S 3200 and store it in a MySQL database. The data that should be fetched can be configured with a WEBIF or daemon. The collected data of the S 3200 will be displayed with the WEBIF, too.

Written by: *Jörg Wendel (linux at jwendel dot de)*
Documentation and Testing: *Thomas Unsin*
Homepage: https://github.com/horchi/linux-p4d

## License
This code is distributed under the terms and conditions of the GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.

## Disclaimer
USE AT YOUR OWN RISK. No warranty.

Die Software wurde für den Eigengebrauch erstellt. Sie wird kostenlos unter der
GPLv2 veröffentlicht.

Es ist kein fertiges Produkt, die Software entstand als Studie was hinsichtlich der Kommunikation
mit der s3200 Steuerung möglich ist und kann Bastlern als Basis und Anregung für eigene
Projekte dienen.

Es besteht kein Anspruch auf Funktion, jeder der sie einsetzen möchte
muss das Risiko selbst abschätzen können und wissen was er tut, insbesondere auch in
Hinblick auf die Einstellungen der Heizungsparameter und den damit verbundenen Risiken
hinsichtlich Fehlfunktion, Störung, Brand, etc. Falsche Einstellung können unter anderem
durch Bedienfehler und Fehler in dieser Software ausgelöst werden!
Die Vorgaben, Vorschriften und AGB des Herstellers der Heizung bleiben Maßgebend!
Ich kann  nicht ausschließen das es zu Fehlfunktionen oder unerwartetem Verhalten,
auch hinsichtlich der zur Heizung übertragenen Daten und damit verbundenen, mehr oder
weniger kritischen Fehlfunktionen derselben kommen kann!


### Prerequisits:
- USB-Serial Converter based on FTDI chip (other may cause connection problems)
- USB-Serial converter must be connected to COM1 on Fröling mainboard
- A Linux based host is required
  e.g. a Raspberry Pi with a default OS setup (e.g. raspbian stretch) is a good option for the p4d
- de_DE.UTF-8 is required as language package (Raspberry command: `dpkg-reconfigure locales`)

### Installation MySQL Database und client library:
It's not required to host the database on the Raspberry. A remote database is as well supported.

`apt install mysql-server libmysqlclient-dev`
some distributions like raspbian stretch switched to mariadb, in this case:
`apt install mariadb-server libmariadbclient-dev `

(set database password for root user during installation)

### P4 database setup:
If database isn't located on the Raspberry check the chapter remote database setup at the end of the Readme.

- login as root:
```
> mysql -u root -p
  CREATE DATABASE p4 charset utf8;
  CREATE USER 'p4'@'localhost' IDENTIFIED BY 'p4';
  GRANT ALL PRIVILEGES ON p4.* TO 'p4'@'localhost' IDENTIFIED BY 'p4';
```

### Installation Apache Webserver:
Run the following commands to install the Apache webserver and required packages
```
apt update
apt install apache2 libapache2-mod-php7.2 php7.2-mysql php7.2-gd
```

Check from a remote PC if connection works a webpage with the content `It Works!` will be displayed

## Installation the p4d daemon:
### install the build dependencies
```
apt install build-essential libssl-dev libxml2-dev libcurl4-openssl-dev libssl-dev
```
### get the p4d and build it
```
cd /usr/src/
git clone https://github.com/horchi/linux-p4d/
cd linux-p4d
make clean all
make install
make inst-sysv-init
```

- Now P4 daemon is installed in folder `/usr/local/bin` and its config in /etc/p4d/
- Check `/etc/p4d.conf` file for setting db-login, ttyDeviceSvc device (change device if required),
  check which `/dev/ttyUSB?` devices is used for USB-Serial converter (`/dev/ttyUSB0`, `/dev/ttyUSB1`, `/dev/ttyACM0`)

### Aggregation / Cleanup
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

### Enable automatic p4d startup during boot:
If MySQL database is located on the same device as p4d is running you have to do the next steps
- Edit file `/usr/src/linux-p4d/contrib/p4d`
- Append the parameter `mysql` at the end of the next line
```
# Required-Start:    hostname $local_fs $network $syslog
```
- enable p4d start during RPi boot, run the commands
```
cp contrib/p4d /etc/init.d/
chmod 750 /etc/init.d/p4d
update-rc.d p4d defaults
```

### Install the WEB interface:
```
 cd /usr/src/linux-p4d
 make install-web
 make install-pcharts
 make install-apache-conf
```

The default username and password for the login is
User: *p4*
Pass: *p4-3200*

### PHP Settings:
file /etc/php/7.0/apache2/php.ini
set max_input_vars = 5000

### Fist steps to enable data logging:

1. Log in
2. Setup -> Aufzeichnung -> Init
  - Select the values you like to record and store your selection (click 'Speichern')
3. Menu -> Init
4. Menu -> Aktualisieren

After this you can set up the schema configuration. The schema configuration seems not to be working with the firefox!

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
- Remove `heirloom-mailx` (`aptitude remove heirloom-mailx`)

### Configure Time sync:
With the next steps you can enable a time synchronization of the p4d and the heating:
If you enable this feature you can set the max time difference between the p4d systemtime and the time of the heating. The p4d will set the time of the heating (once a day) to his the systemtime. Therefore it is recommended to hold your system time in sync, e.g. by running the `ntp` daemon.

- Check if ntpd (openntpd, ...) is running `/etc/init.d/ntp status`. If not you have to install it.
- Start the WEBIF and login, go to the Setup page
- Enable "Tägliche Zeitsynchronisation" and set the max time difference in seconds in the line "Maximale Abweichung"
- Save configuration

### One Wire Sensors:
The p4d checks automatically if there are 'One Wire Sensors' connected, each detected sensor will be
configurable via the web interface after reading the sensor facts by clicking [init].

To make the sensors availaspble to the raspi you have to load the `w1-gpio` module, this can be done by calling `modprobe w1-gpio` or automatically at boot by registering it in `/etc/modules`:
```
echo "w1-gpio" >> /etc/modules
```

### Points to check
- reboot the device to check if p4d is starting automatically during startup


## Additional information
### Remote database setup
Login as root:
```
> mysql -u root -p
 CREATE DATABASE p4 charset utf8;
 CREATE USER 'p4'@'%' IDENTIFIED BY 'p4';
 GRANT ALL PRIVILEGES ON p4.* TO 'p4'@'%';
```

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
