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
- check /etc/p4d.conf file for setting ttyDeviceSvc
	change device if required
	check in /dev which ttyUSB devices is used for USB-Serial converter (/dev/ttyUSB0, /dev/ttyUSB1)


Enable automatic p4d startup during boot:
-----------------------------------------

- if MySQL database is located on the same device as p4d is running you have to do the next steps
	edit file /tmp/linux-p4d/contrib/p4d
	append the parameter 'mysql' at the end of the next line
		# Required-Start:    hostname $local_fs $network
- enable p4d start during RPi boot, run the commands
	cp contrib/p4d /etc/init.d/
	update-rc.d p4d defaults


Setup and configure WEBIF:
--------------------------

- copy content of folder /tmp/linux-p4d/htdocs to your webroot (eg. /var/www)
- download the tool pChart2.1.x from http://www.pchart.net/download
- store the extracted download in folder /var/www/pChart2.1.3
- create symbolic link
	ln -s /var/www/pChart2.1.3/ /var/www/pChart
- change access to cache folder 
	chmod 777 /var/www/pChart/cache
- for test enter the web address http://<IP_of_RPi>/index.php 


Setup and configure sending mails:
----------------------------------

The next steps are an example to setup the sending of mails. If another tool is preferred it can be used as well. The config is based on GMX. If you have another provider please search in the Internet for the required config.
- install required components
	apt-get install ssmtp mailutils
- enable sending status mails of p4d, change the following parameters in the file /etc/p4d.conf or remove # character
	mail = 1
	stateMailTo = me@gmx.de,whoever@foobar.de
	errorMailTo = hausmeister@blablub.de
- the line '#stateMailAtStates' in file /etc/p4d.conf will be described in a new version of that file
	for now you will get a mail for each status change of the heating
- copy required mailscript
	cp /tmp/linux-p4d/scripts/p4d-mail.sh /usr/local/bin
- change revaliases file	
  edit file /etc/ssmtp/revaliases and add line (gmx is used as an example)
	p4d:MyMailAddress@gmx.de:mail.gmx.net:25
- change ssmtp.conf file
  edit file /etc/ssmtp/ssmtp.conf (gmx is used as an example)
	root=MyMailAddress@gmx.de
	mailhub=mail.gmx.net
	hostname=gmx.net
	UseSTARTTLS=YES
	AuthUser=MyMailAddress@gmx.de
	AuthPass=MyPassword


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
