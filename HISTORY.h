/*
 * -----------------------------------
 * p4 Daemon / p4d -  Revision History
 * -----------------------------------
 *
 */

#define _VERSION     "0.3.35"
#define VERSION_DATE "20.08.2020"

#ifdef GIT_REV
#  define VERSION _VERSION "-GIT" GIT_REV
#else
#  define VERSION _VERSION
#endif

/*
 * ------------------------------------

2020-08-18:  version 0.3.35
   - bugfix: Fixed init of schema configuration

2020-08-18:  version 0.3.34
   - change: Increased 'data' field for webif communication

2020-05-19:  version 0.3.33
   - bugfix: Fixed problem with error address

2020-05-14:  version 0.3.32
   - change: Code review
   - change: WEBIF - changed main view to dashboard
   - change: WEBIF - removed status display from dashboard view
   - added:  images for PE1-25kW, PE1c, PE1, and PT4e

2020-05-11:  version 0.3.31
   - bugfix: fixed compile of lib paho.mqtt.c

2020-05-07:  version 0.3.30
   - added:  Script sensors
   - bugfix: fixed compile of lib paho.mqtt.c

2020-05-05:  version 0.3.29
   - added:  Syslog to WEBIF

2020-05-02:  version 0.3.28
   - bugfix: Fixed compile without MQTT

2020-05-01:  version 0.3.27
   - bugfix: Fixed possible crash with empty MQTT user

2020-05-01:  version 0.3.26
   - change: Moved many config options fron p4d.conf to WEBIF
   - change: Minor rework of config page
   - added:  optional user/password for MQTT

2020-04-30:  version 0.3.25
   - bugfix: Fixed crash in 'GROUP' mode for mqtt topics

2020-04-29:  version 0.3.24
   - change: option to store each group in seperate toping (FHEM special instead of structures JSON object)

2020-04-28:  version 0.3.23
   - added: 'Baugruppen' to MQTT interface for FHEM and Openhab

2020-04-27:  version 0.3.22
   - added: MQTT interface for FHEM and Openhab

2020-04-26:  version 0.3.21
   - bugfix: Fixed tore of MQTT config parameters

2020-04-25:  version 0.3.20
   - added: MQTT config parameter to WEBIF

2020-04-25:  version 0.3.19
   - added: MQTT Config topic now optional
   - added: added MQTT data topic name to config
   - change: removed ßäüö from the MQTT topic Names and IDs
   - change: improved make install
   - bugfix: fixed p4d-backup script

2020-04-16:  version 0.3.18
   - bugfix: Fixed handling of hex IDs

2020-04-16:  version 0.3.17
   - change: The display order now configured by the order of the
             sensor IDs instead of 'Pos' at the settings page

2020-04-13:  version 0.3.16
   - added: maxsacle for '%' values
   - change: removed confirm for store of settinges
   - added: type to value selection

2020-04-13:  version 0.3.15
   - change: Minor change of parameter setup

2020-04-05:  version 0.3.14
   - added: Dashbord/Widget view

2020-03-31:  version 0.3.13
   - change: int valuefacts now update existing rows

2020-03-29:  version 0.3.12
   - change: removed ntp from install script
   - bugfix: Fixed handling of unit charset

2020-03-29:  version 0.3.11
   - bugfix: Minor install fix

2020-03-28:  version 0.3.10
   - added:  storage of min and max peaks values
   - bugfix: Fixed scripts config path

2020-03-24:  version 0.3.9
   - added: modification of php max_input_vars to deb package

2020-03-22:  version 0.3.8
   - change: improved package build

2020-03-22:  version 0.3.7
   - added: user confirmation to post purge script

2020-03-21:  version 0.3.6
   - added: patches for libpaho

2020-03-21:  version 0.3.5
   - added: build-deb make target to create debian package

2020-03-18:  version 0.3.4
   - change: Code cleanup and minor makefile fixes

2020-03-16:  version 0.3.3
   - change: Improved error message

2020-03-11:  version 0.3.2
   - change: Compatible to buster (thx to Panzerknacker)

2019-10-31:  version 0.3.1
   - change: Merged dev branch into master

2019-01-11:  version 0.3.0
   - added: Configurable url for menu button to a external site (e.g. a home automation=
   - added: MQTT interface to home assistant

2018-12-20:  version 0.2.335
   - added: script p4getvalue.py to query actual values from database

2018-12-19:  version 0.2.334
   - change: compatible with old g++ 4.6 again

2018-12-19:  version 0.2.333
   - added: Added php hint regarding setup menue problem with large value counts to README
   - change: storing all samples in one transaction

2018-11-12:  version 0.2.332
   - change: Fixed pChart compatibility problem again

2018-10-31:  version 0.2.32
   - added: Create scripts.d folder at "make install" do avoid warning message

2018-10-30:  version 0.2.31
   - added: Install target for systemV init scripts

2018-10-04:  version 0.2.30
   - change: Updated README.md, changes Makefile for 'old' pChart version

2018-01-31:  version 0.2.29
   - added:  Added One-Wire sensor type '3b'

2017-11-08:  version 0.2.28
   - added:  Added text for heating state 71

2017-03-09:  version 0.2.27
   - change:  Time sync for S3200 now at 3:00 instead of 23:00
   - added:   install tagets for pchart and apache config

2017-02-13:  version 0.2.26
   - bugfix: WEBIF: Fixed store of sensor alerts

2017-01-24:  version 0.2.25
   - change:  WEBIF: Updated config page style on small devices (< 740 pixel)

2017-01-23:  version 0.2.24
   - added:  WEBIF: Charts support now all sensors types (not only 'VA')

2017-01-23:  version 0.2.23
   - bugfix: Fixed potentially endless loop on communication trouble
   - change: WEBIF: Show script buttons only with login

2017-01-23:  version 0.2.22
   - added:  WEBIF: More css/html improvements
   - added:  Added script interface

2017-01-22:  version 0.2.21
   - added:  WEBIF: Added even more gradients

2017-01-22:  version 0.2.20
   - bugfix: Fixed redundant error mails
   - change: Improved style of base-config page
   - change: WEBIF: Switch main page to mobile mode < 800px
   - added:  WEBIF: Display menu on fixed position at top of page (even while scrolling)
   - added:  WEBIF: Added gradient to table on main page
   - bugfix: WEBIF: Fixed range setting on main page
   - bugfix: Minor fix of error mail style

2017-01-17:  version 0.2.19
   - added:  WEBIF: Added red/green dot to error dialog

2017-01-17:  version 0.2.18
   - added:  'Störungs'-Mails for pending errors in S-3200 error list
   - added:  WEBIF: Select box for sensor type in sensort alert dialog
   - added:  WEBIF: 'Test Mail' button for sensort alerts
   - bugfix: WEBIF: fixed problem with escape of sql strings at sensor alert dialog

2017-01-16:  version 0.2.17
   - change: prepared error list for 'Störungs-Mail feature

2017-01-16:  version 0.2.16
   - change: WEBIF: Improved browser icon compatibility

2017-01-15:  version 0.2.15
   - bugfix: WEBIF: Fixed store of mail setting

2017-01-15:  version 0.2.14
   - change: WEBIF: Down override selected style with install-web
   - change: WEBIF: Stylesheet (CSS) modifications for schema-values and table-buttons
   - change: Error list now shows only one line per error with the newest state
   - bugfix: WEBIF: Fixed charts34
   - bugfix: WEBIF: Fixed animated status gif
   - bugfix: WEBIF: Fixed schema config (selection of Text/Value)

2017-01-13:  version 0.2.13
   - change: WEBIF: Some CSS finetuning again ;)

2017-01-13:  version 0.2.12
   - change:  Improved state handling

2017-01-13:  version 0.2.11
   - change:  WEBIF: redesign of page 'Sensor-Alerts'

2017-01-12:  version 0.2.10
   - bugfix:  Fixed config problem with '#' sign in mysql password

2017-01-11:  version 0.2.9
   - added:  Mail check button

2017-01-09:  version 0.2.8
   - bugfix: WEBIF: Fixed chart range

2017-01-09:  version 0.2.7
   - bugfix: WEBIF: Fixed php warning 'Undefined index: stateAni'

2017-01-09:  version 0.2.6
   - bugfix: WEBIF: Fixed missing parameter (since 0.1.46)

2017-01-07:  version 0.2.5
   - change: WEBIF: fixed make install-web
   - change: WEBIF: show version of webif on main page

2017-01-06:  version 0.2.4
   - change: WEBIF: Some CSS finetuning
   - change: WEBIF: added chown to WEBOWNER (of Make.config) to web-install

2017-01-05:  version 0.2.3
   - bugfix: WEBIF: Fixed configuration of schema

2017-01-03:  version 0.2.2
   - added:  WEBIF: new CSS style 'dark'
   - change: Splited CSS files into base.cc and separate CSS files for color schemas

2017-01-02:  version 0.2.1
   - added:  Synchronisation to HomeMatic System-Variables

2016-12-23:  version 0.2.0
   - added:  update of web interface
   - change: merged modifications since version 0.1.38 into master branch

2016-12-22:  version 0.1.46
   - added:  storage for time range parameters

2016-12-22:  version 0.1.45
   - change: WEBIF: redesign all tabs, added a 'light' stylesheet
   - added:  display of time range parameters

2016-12-15:  version 0.1.44
   - change: WEBIF: finished redesign of 'Aktuell' tab

2016-12-14:  version 0.1.43
   - change: WEBIF: further modifications for mobile devices

2016-12-14:  version 0.1.42
   - change: WEBIF:  fixed problem with SQL syntax since MySQL >= 5.7

2016-12-13:  version 0.1.41
   - added:  WEBIF: different mobile view for actual tab
   - added:  WEBIF: configurable sensors list for action tab

2016-12-13:  version 0.1.40
   - change: WEBIF: finished RC1 of PHP7 porting (php7 branch)

2016-12-12:  version 0.1.39
   - change: WEBIF: start of PHP7 porting in (php7 branch)

2016-03-10:  version 0.1.38
   - bugfix: fixed display of 'usrtitle' in status mails

2016-03-04:  version 0.1.37
   - bugfix: remove steering chars in unit (delivered by S-3200)

2016-03-04:  version 0.1.36
   - bugfix: fixed mode display in state mails

2016-03-02:  version 0.1.35
   - added:  update of table smartconf

2016-03-01:  version 0.1.34
   - change: ported to new db api
   - added:  added table smartconfig (by S.Döring)

2016-03-01:  version 0.1.33
   - bugfix: minor fix for html header
   - added:  git handling to makefile

2016-03-01:  version 0.1.32
   - change: html header of status mails now configurable in "/etc/p4d/mail-head.html"
             example can be found in configs

2016-02-29:  version 0.1.31
   - change: source environment in update.sh

2016-02-24:  version 0.1.30
   - change: update of after-update.sh example
   - change: updated html (outfit) of status mails

2016-02-18:  version 0.1.29
   - change: added support of usrtitle to after-update.sh example

2016-02-17:  version 0.1.28
   - bugfix: fixed ddl

2016-02-17:  version 0.1.27
   - added: homematic example script
   - change: updated webif

2016-02-17:  version 0.1.26
   - bugfix: fixed call of script <confdir>/after-update.sh

2016-02-16:  version 0.1.25
   - added: added script hook called after update cycle is performed

2015-06-24:  version 0.1.24
   - change: don't touche value facts during init for 'none' updates

2015-04-14:  version 0.1.23
   - bugfix: fixed displayed timezone

2015-03-24:  version 0.1.22
   - change: Ported to horchi's new DB API ;)

2015-03-23:  version 0.1.21
   - change: moved p4d config from /etc/ to /etc/p4d/

2015-03-12:  version 0.1.20
   - added:  web config for sensor alerts by 'Abholzer'
   - added:  %range% and %delta% telpmates for alert mails

2015-03-10:  version 0.1.19
   - added:  implemented sensor range alert check

2015-03-05:  version 0.1.18
   - added:  implementation of sensor alert mails

2015-03-05:  version 0.1.17
   - added:  Started implementation of sensor alerts

2015-02-27:  version 0.1.16
   - added:  New table for sensor alerts (not implemented yet)

2015-02-27:  version 0.1.15
   - change: Improved trouble handling
   - added:  Setup of aggregation to README
   - added:  Setup of One-Wire-Sensors to README

2015-01-12:  version 0.1.14
   - change: On missing one-wire sensor show 'Info' instead of 'Error'

2014-12-01:  version 0.1.13
   - change: implemented aggregation
   - change: support of html mails

2014-12-01:  version 0.1.12
   - change: p4d 'prepared' for aggregation of samples

2014-11-28:  version 0.1.11
   - added:  support for 1W sensors
   - change: moved webif images to subfolder
   - change: minor changes and fixes

2014-02-07:  version 0.1.10
   - change: supporting WEBIF communication event if
             serial line is disturbed

2014-02-05:  version 0.1.9
   - bugfix: fixed problem with negative parameter values

2014-02-01:  version 0.1.8
   - change: improved recover handling of serial communication
   - added:  more state and heater images.
   - added:  config of heating type to WEBIF
   - added:  more heater states
   - added:  configuration of schema image to WENIF

2014-01-30:  version 0.1.7
   - added :  index for jobs table
   - bugfix:  fixed mail body of 'STÖRUNGs' mails

2014-01-24:  version 0.1.6
   - change: fixed table shadow for firefox
   - bugfix: fixed passwort check fpr initial login

2014-01-07:  version 0.1.5
   - change: added default unit % for analog outputs
   - change: update of README thx Thomas!

2014-01-07:  version 0.1.4
   - added:  recording of digital inputs
   - added:  recording of analog outouts
   - bugfix: Fixed another security issue

2014-01-06:  version 0.1.3
   - bugfix: Fixed security issues reported by b_a_s_t_i !Thx!

2014-01-03:  version 0.1.2
   - added:  time synchronisation to adjust s3200's system time
   - added:  error history to WEBIF
   - added:  auto refresh to view 'Aktuell'
   - change: improved status display

2013-12-23: version 0.1.1
   - added: settings to WEBIF

2013-12-18: version 0.1.0
   - first official release

2010-11-04: version 0.0.1
   - start of implementation

 * ------------------------------------
 */
