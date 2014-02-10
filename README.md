-----------------------------------------------------------------------------------
--                             Linux - P4 Daemon (p4d)                           --
-----------------------------------------------------------------------------------
-
- This daemon is fetching data from the S 3200 and store it in a MySQL database
- The data that should be fetched can be configured with a WEBIF or daemon
- The collected data of the S 3200 will be displayed with the WEBIF, too
-
- Written by:                 Jörg Wendel (linux at jwendel dot de)
-
- Documentation and Testing:  Thomas Unsin
-
- Homepage:   https://github.com/horchi/linux-p4d
-
- This code is distributed under the terms and conditions of the
- GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
-
- Disclaimer: USE AT YOUR OWN RISK. No warranty. 
-
- Die Software wurde für den Eigengebrauch erstellt. Sie wird kostenlos unter der 
- GPLv2 veröffentlicht.
-
- Es ist kein fertiges Produkt, die Software entstand als Studie was hinsichtlich der Kommunikation
- mit der s3200 Steuerung möglich ist und kann Bastlern als Basis und Anregung für eigene
- Projekte dienen.
-
- Es besteht kein Anspruch auf Funktion, jeder der sie einsetzen möchte 
- muss das Risiko selbst abschätzen können und wissen was er tut, insbesondere auch in 
- Hinblick auf die Einstellungen der Heizungsparameter und den damit verbundenen Risiken
- hinsichtlich Fehlfunktion, Störung, Brand, etc. Falsche Einstellung können unter anderem 
- durch Bedienfehler und Fehler in dieser Software ausgelöst werden!
- Die Vorgaben, Vorschriften und AGP des Herstellers der Heizung bleiben Maßgebend!
- Ich kann  nicht ausschließen das es zu Fehlfunktionen oder unerwartetem Verhalten, 
- auch hinsichtlich der zur Heizung übertragenen Daten und damit verbundenen, mehr oder 
- weniger kritischen Fehlfunktionen derselben kommen kann!
-
-----------------------------------------------------------------------------------


Description of the README:
--------------------------

- This document describes how to setup and configure the p4d solution


Prerequisits:
-------------

- USB-Serial Converter based on FTDI chip
- USB-Serial converter must be connected to COM1 on Fröling mainboard
- a Linux based device is required
- a Raspberry Pi with a default OS setup (e.g. wheezy-raspbian) is a very good option for the p4d
- de_DE.UTF-8 is required as language package (Raspberry command: dpkg-reconfigure locales)


Installation MySql Database:
----------------------------

It's not required to host the database on the Raspberry. A remote database is as well supported.

apt-get install mysql-server-5.5 libmysqlclient-dev
	(set database password for root user during installation)

	
P4 database setup:
------------------

If database isn't located on the Raspberry check the chapter remote database setup at the end of the Readme.

- login as root: 
> mysql -u root -p
  CREATE DATABASE p4 charset utf8;
  CREATE USER 'p4'@'localhost' IDENTIFIED BY 'p4';
  GRANT ALL PRIVILEGES ON p4.* to 'p4'@'localhost';


Installation Apache Webserver:
------------------------------

- run the following commands to install the Apache webserver and required packages
	groupadd www-data
	usermod -a -G www-data www-data
	apt-get update
	apt-get install apache2 php5 php5-mysql php5-gd
- check from a remote PC if connection works
	a webpage with the content "It Works!" will be displayed


Installation p4d Application:
-----------------------------

- install build essentials like make, g++, ...
- install libssl-dev 
  apt-get install libssl-dev
- change to directory /tmp (or another folder if download should be persistent stay on the device)
- run command to download current version
	git clone https://github.com/horchi/linux-p4d/
- change to directory /tmp/linux-p4d
- call "make" in the source directory
- call "make install" in the source directory (file configs/p4d.conf is copied to /etc)
- p4 daemon is installed in folder /usr/local/bin
- check /etc/p4d.conf file for setting db-login, web-login, ttyDeviceSvc (change device if required),
	check in /dev which ttyUSB devices is used for USB-Serial converter (/dev/ttyUSB0, /dev/ttyUSB1)


Enable automatic p4d startup during boot:
-----------------------------------------

- if MySQL database is located on the same device as p4d is running you have to do the next steps
	edit file /tmp/linux-p4d/contrib/p4d
	append the parameter 'mysql' at the end of the next line
		# Required-Start:    hostname $local_fs $network $syslog
- enable p4d start during RPi boot, run the commands
	cp contrib/p4d /etc/init.d/
	chmod 750 /etc/init.d/p4d
	update-rc.d p4d defaults


Setup and configure WEBIF:
--------------------------

- copy content of folder /tmp/linux-p4d/htdocs to your webroot (/var/www is used as example in the next steps)
- download the tool pChart2.1.x from http://www.pchart.net/download (version 2.1.3 in our example)
- store the extracted download in the webroot folder (eg. /var/www/pChart2.1.3)
- create symbolic link
	ln -s /var/www/pChart2.1.3/ /var/www/pChart
- change access to cache folder 
	chmod 777 /var/www/pChart/cache
- change owner of www folder
	chown www-data /var/www
- for a test enter the web address http://<IP_of_RPi>/index.php 

The default username and password for the login is
Benutzername: p4
Passwort: p4-3200

Fist WEBIF steps to enable data logging:

1.) log in
2.) Setup -> Aufzeichnung -> Init
  select the values you like to record and save by clicking 'Speichern'
3.) Menu -> Init
4.) Menu -> Aktualisieren

After this you can set up the schema configuration. The schema configuration seems not to be working with the firefox!


Setup and configure sending mails:
----------------------------------

The next steps are an example to setup the sending of mails. If another tool is preferred it can be used as well. The config is based on GMX. If you have another provider please search in the Internet for the required config.
- install required components
	apt-get install ssmtp mailutils
- the mailscript p4d-mail.sh is copied during the "make install" to the folder /usr/local/bin. This script is used in our case to send mails
- change revaliases file	
  edit file /etc/ssmtp/revaliases and add line (gmx is used as an example)
	root:MyMailAddress@gmx.de:mail.gmx.net:25
- change ssmtp.conf file
  edit file /etc/ssmtp/ssmtp.conf (gmx is used as an example)
	root=MyMailAddress@gmx.de
	mailhub=mail.gmx.net
	hostname=gmx.net
	UseSTARTTLS=YES
	AuthUser=MyMailAddress@gmx.de
	AuthPass=MyPassword

- start the WEBIF and Login, go to the Setup page
- configure the "Mail Benachrichtigungen" options (status and Fehler Mail Empfänger)
- if no Status options are set you will get a mail for each status change
- set the script option /usr/local/bin/p4d-mail.sh

(If the heating values are added as attachment to the mail please check the next steps.
- check if heirloom-mailx is installed (ls -lah /etc/alternatives/mail)
- if output link is /etc/alternatives/mail -> /usr/bin/mail.mailutils
- remove heirloom-mailx (aptitude remove heirloom-mailx))


Configure Time sync:
--------------------

With the next steps you can enable a time synchronization of the p4d and the heating:
If you enable this feature you can set the max time difference between the p4d systemtime and the time 
of the heating. The p4d will set the time of the heating (once a day) to his the systemtime. Therefore it is
revcommended to hold your system time in sync, e.g. by running the ntp daemon.

- check if ntpd (openntpd, ...) is running "/etc/init.d/ntp status"
	if not you have to install it
- start the WEBIF and Login, go to the Setup page
- enable "Tägliche Zeitsynchronisation" and set the max time difference in seconds in the line "Maximale Abweichung"
- save configuration


points to check:
----------------

- reboot the device to check if p4d is starting automatically during startup


*********** Additional information *********************************************************************

remote database setup:
----------------------

- login as root: 
> mysql -u root -p
 CREATE DATABASE p4 charset utf8;
 CREATE USER 'p4'@'%' IDENTIFIED BY 'p4';
 GRANT ALL PRIVILEGES ON p4.* TO 'p4'@'%';


MYSQL HINTS:
------------
- If you cannot figure out why you get Access denied, remove all entries from the user table 
  that have Host values with wildcars contained (entries that match '%' or '_' characters). 
  A very common error is to insert a new entry with Host='%' and User='some_user', 
  thinking that this allows you to specify localhost to connect from the same machine. 
  The reason that this does not work is that the default privileges include an 
  entry with Host='localhost' and User=''. Because that entry has a Host value 'localhost' 
  that is more specific than '%', it is used in preference to the new entry when connecting 
  from localhost! The correct procedure is to insert a second entry with Host='localhost' 
  and User='some_user', or to delete the entry with Host='localhost' and User=''. 
  After deleting the entry, remember to issue a "FLUSH PRIVILEGES" statement to reload the grant tables. 

- To analyze this you can show all users:
 use mysql
 SELECT host, user FROM user;
