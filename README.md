-------------------------------------------------------
--             Linux - P4 Daemon                     --
-------------------------------------------------------
 
Requirements:
-------------

  - libmysql >= 5.07

Description:
------------

Deamon which fetch sensor data of the 'Lambdatronic s3200' 
and store to a MySQL database


Configuration:
--------------

  Log level #
      Logging level (Errors, Infos, Debug ...)

  DbHost
    ip of database host

  DbPort

  DbName

  DbUser

  DbPass


Installation:
-------------

- Unpack 
- Call "make" in the source directory
- sudo cp configs/p4.conf /etc/
- Create the database (see below)

If you use upstart:

- sudo cp contrib/p4d.conf /etc/init/
- start p4d

otherwise start the script how you like ;)

Create database and user:
--------------------------

- login as root: 
> mysql -u root -p
 CREATE DATABASE p4 charset utf8;
 CREATE USER 'p4'@'%' IDENTIFIED BY 'p4';
 GRANT ALL PRIVILEGES ON p4.* TO 'p4'@'%';

- login as p4 for maintanance:
> mysql -u p4 -pp4 -Dp4 --default-character-set=utf8
- or remote
> mysql -u p4 -pp4 -Dp4 --default-character-set=utf8 -h <host>

MYSQL HINTS:
-----------
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


-------------------------------------------------------
--                 Charting Client                   --
-------------------------------------------------------

The client reads data from database an creeate some weather charts using mathgl 2.1.x
It is more or less a example code.
(I use it to create images for VDRs graphTFT Plugin)

Requirements:
-------------

  - libmysql >= 5.07
  - libmgl version 2.1.x

libmgl:
-------
Install libmgl 2 maually since it is'nt avalible in most distributions :(
You can download the source here http://mathgl.sourceforge.net/doc_en/doc_en_6.html
and build it like the documentation at http://mathgl.sourceforge.net/

cmake -D enable-jpeg=on .
cmake -D enable-jpeg=on .
make 
sudo make install
sudo ldconfig

Building the client:
--------------------

make sql-chart

Installing the client:
----------------------

- sudo cp sql-chart /usr/local/bin/
- sudo cp scripts/p4-chart.sh /usr/local/bin/

If you use upstart:

- sudo cp contrib/p4-chart.conf /etc/init/
- start p4-chart

otherwise start the script how you like ;)
