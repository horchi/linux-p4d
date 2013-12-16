-----------------------------------------------------------------------------------
--                             Linux - P4 Daemon (p4d)                           --
-----------------------------------------------------------------------------------
-
- This daemon is fetching data from the S 3200 and store it in a MySQL database
- The data that should be fetched can be configured with a WEBIF or daemon
- The collected data of the S 3200 will be displayed with the WEBIF, too
-
- Written by: C++/SQL         - Jörg Wendel (linux at jwendel dot de)
-             Documentation
-             and Testing     - Thomas Unsin
-
- Homepage:   https://github.com/horchi/linux-p4d
-
- This code is distributed under the terms and conditions of the
- GNU GENERAL PUBLIC LICENSE. See the file LICENSE for details.
-----------------------------------------------------------------------------------


Description of the Readme:
--------------------------

- This document describes how to setup and configure the p4d solution


Prerequisits:
-------------

- USB-Serial Converter
- USB-Serial converter must be connected to COM1 on Fröling mainboard
- Raspberry Pi with a default OS setup (e.g. wheezy-raspbian)


Installation MySql Database:
----------------------------

apt-get install mysql-server-5.5 libmysqlclient-dev
	(set database password for root user during installation)

It's not required to host the database on the Raspberry. A remote database is as well supported.

	
P4 database setup:
------------------

- login as root: 
> mysql -u root -p
  CREATE DATABASE p4 charset utf8;
  CREATE USER 'p4'@'localhost' IDENTIFIED BY 'p4';
  GRANT ALL PRIVILEGES ON p4.* to 'p4'@'localhost';

If database isn't located on the Raspberry check the chapter remote database setup at the end of the Readme.


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

- change to directory /tmp
- run command to download current version
	git clone https://github.com/horchi/linux-p4d/
- change to directory /tmp/linux-p4d
- call "make" in the source directory
- call "make install" in the source directory (file configs/p4d.conf is copied to /etc)
- compiled daemon file is stored in folder /usr/local/bin
- check /etc/p4d.conf file for setting ttyDeviceSvc
	change port if required
	check in /dev which ttyUSB port number is used for USB-Serial converter (/dev/ttyUSB0, /dev/ttyUSB1)
- enable automatic p4d start during RPi boot, run the following commands
	cp contrib/p4d /etc/init.d/
	update-rc.d p4d defaults
- run the command "/usr/sbin/local/p4d -I" to create the initial database tables and load the heating parameters to the database


Setup and configure WEBIF:
--------------------------

- copy content of folder /tmp/linux-p4d/htdocs to folder /var/www
- download the tool pChart2.1.x from http://www.pchart.net/download
- store the extracted download in folder /var/www/pChart2.1.3
- create symbolic link
	ln -s /var/www/pChart2.1.3/ /var/www/pChart
- change access to cache folder 
	chmod 777 /var/www/pChart/cache
- change the file /var/www/config.php
	$cache_dir          = "pChart/cache";
	$chart_fontpath     = "pChart/fonts";
- enter the web address http://<IP_of_RPi>/index.php


Setup and configure sending mails:
----------------------------------

documentation will follow	
	
	
	
	

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

- To analyze this you can show all users at the sql promt:
 use mysql
 SELECT host, user FROM user;
