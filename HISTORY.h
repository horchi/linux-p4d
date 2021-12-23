/*
 * -----------------------------------
 * p4 Daemon / p4d - Revision History
 * -----------------------------------
 *
 */

#define _VERSION     "0.9.2"
#define VERSION_DATE "23.12.2021"

#ifdef GIT_REV
#  define VERSION _VERSION "-GIT" GIT_REV
#else
#  define VERSION _VERSION
#endif

/*
 * ------------------------------------

2021-12-23:  version 0.9.2
  - bugfix:  Fixed dashboard charts

2021-12-23:  version 0.9.1
  - bugfix:  Fixed background image config

2021-12-18:  version 0.9.0
  - added:  Interface for deCONZ (ConBee)
  - added:  widget width 0.5
  - added:  Spacer widget with linefeed option
  - added:  Time widget
  - added:  background image
  - added:  Kiosk mode (e.g. for wall mounted touch panels) - activated by URL option 'kiosk=1'
  - added:  Auto switch back to first dashboard - activated by URL option 'backTime=<seconds>'
  - change: Optical adjustments for different display sizes
  - added:  Sensors active and record separately adjustable
  - change: Extended node-red support
  - added:  Config for widget heigh per dashboard
  - added:  HomeMatic support via node-red

2021-12-14:  version 0.8.20
  - added:  MQTT Topics for node-red communication

2021-12-13:  version 0.8.19
  - added:  add all widgets option
  - change: adjusted widget defaults
  - change: redesigned add and delete of widgets

2021-12-12:  version 0.8.18
  - added:  show only widget-type specific option in widget setup dialog

2021-12-12:  version 0.8.17
  - added:  media query for deviced between 741px anf 1100px width (by Michael Pölsterl)
  - added:  cyclic refresh for charts

2021-12-12:  version 0.8.16
  - added:  color of meter scales to stylesheet

2021-12-12:  version 0.8.15
  - bugfix:  fixed w1 sensor detection on busmaster > 1
  - bugfix:  fixed update of table schemaconf

2021-12-11:  version 0.8.14
  - change:  fixed sympol alignment for ios devices
  - added:   color chouce for dashboard widgets

2021-12-11:  version 0.8.13
  - change:  display optimized for mobile devices

2021-12-10:  version 0.8.12
  - change:  Improved logging for docker
  - bugfix:  Fixed dashboard symbol handling
  - change:  minor changes and fixes
  - added:   peak for 'value' and 'linear-meter' widgets
  - added:   new widget type 'SymbolValue'

2021-12-09:  version 0.8.11
  - added:  User image upload (setup -> images)
  - added:  Sort of dashboard pages (drag&drop)
  - change: Some minor webif improvements

2021-12-09:  version 0.8.10
  - change: Webfrontend improvement

2021-12-07:  version 0.8.9
  - bugfix: fixed ssl support
  - change: Improved chart widget
  - change: Support text and symbol for dashboard title
  - change: improved speed at page change between dashboard, list and schema

2021-12-05:  version 0.8.8
  - change: Multi dashboard configuration
  - change: Fixed list view

2021-12-05:  version 0.8.7
  - change: Minor improvement and fixes

2021-12-04:  version 0.8.6
  - bugfix: Fixed crash

2021-12-04:  version 0.8.5
  - bugfix: Fixed start with empty dashbord config

2021-12-04:  version 0.8.4
  - added:  All images below img to selection for icon widgets
  - change: Speed up dashboard display
  - change: Added chart view to value widgets
  - bugfix: Fixed problem with image selection for icon widgets

2021-12-04:  version 0.8.3
  - change: Compile on X86 without wiringPin lib

2021-12-04:  version 0.8.2
  - change: Remember last selected chart sensors and range

2021-12-03:  version 0.8.1
  - bugfix: Fixed w1 MQTT timeout without sensors

2021-12-01:  version 0.8.0
  - change: Redesign of web frontend
  - change: Merging with my other sources (woold/womod)

2021-05-10:  version 0.7.8
  - change: Removed MQTT recover improvement of 0.7.4

2021-05-08:  version 0.7.7
  - change: Added debug message

2021-04-21:  version 0.7.6
  - change: Merged path of anathusic

2021-04-21:  version 0.7.5
  - bugfix: Fixed possible crash on empty string

2021-04-18:  version 0.7.4
  - change: Added group config to io setup
  - change: Added CRC check for read of values

2021-02-23:  version 0.7.3
  - bugfix: removed extra character (terminating null) from MQTT messages

2021-02-21:  version 0.7.2
  - change: Evaluate type of 'VA' values

2021-02-20:  version 0.7.1
  - added:  Signed display of value for p4 tool

2021-02-05:  version 0.7.0
  - added:  Pellets consuption overview

2021-01-26:  version 0.6.23
  - added:  Create stylesheet link on start if missing
  - change: Updated img/icons

2021-01-25:  version 0.6.22
  - change: More flexible 'id' handling for schema item definitions

2021-01-25:  version 0.6.21
  - bugfix: Fixed color picker at schema config
  - change: Cleanup icons (removed obsolete)
  - change: Modified state icons (thx to Michael Pölsterl)
  - added:  New 'light' version of state icons (thx to Michael Pölsterl)
  - added:  Selection of state icon set in setup

2021-01-19:  version 0.6.20
  - bugfix: Fixed jquery compatibility issue

2021-01-19:  version 0.6.19
  - bugfix: Fixed TLS communication package size.

2021-01-18:  version 0.6.18
  - added: SSL support for web interface

2021-01-17:  version 0.6.17
  - bugfix: Fixed 'growing' state mails (bug of version 0.6.16)

2021-01-17:  version 0.6.16
  - bugfix: Fixed problem with potentially missing state mails

2021-01-17:  version 0.6.15
  - change: New config style for 'stateMailStates' option

2021-01-17:  version 0.6.14
  - change: New config style for dashboard and list addresses

2021-01-15:  version 0.6.13
  - bugfix: Fixed valuefacts for 'SD' values

2021-01-13:  version 0.6.12
  - change: Improved handling of state durations
  - change: Display progress dialog on load of all pages
  - change: Log rotate now zip fist backup
  - change: Display 'min' durations > 60 as HH:MM
  - change: Minor code cleanup
  - added:  MQTT command interface to set parameters

2021-01-11:  version 0.6.11
  - Added:  Implemented time edit dialog
  - change: Better chart performance for charts > 7 days
  - change: Disable chart update while typing at day range field
  - change: Show progress dialog while chart query is running

2021-01-11:  version 0.6.10
  - bugfix: Removed min max display for time range parameters

2021-01-10:  version 0.6.9
  - added: Own logfile and logrotate for p4d
  - added: Recording of state duration

2021-01-08:  version 0.6.8
  - bugfix: fixed parameter storage

2021-01-04:  version 0.6.7
  - bugfix: fixed missing stylesheet symlink after initial install
  - change: modern checkbox style
  - added:  hover for chart buttons (thx to pellet-heizer:holzheizer-forum)
  - change: re-structured js file tree
  - bugfix: fixed display of missing service menu items
  - change: Improved service menu style
  - added:  added display of parameter digits for service menu
  - added:  css style even for buttons of user edit page
  - added:  new display and edit style for boolean parameter digits at the service menu
  - added:  honor mix and max in edit dialog of service menu
  - change: improved parameter display of p4 tool

2021-01-02:  version 0.6.6
  - change: Improvement time range dialog
  - change: Set widget type for unit 'U' to graph

2021-01-02:  version 0.6.5
  - bugfix: Fixed formatting of heating date
  - change: Finetuning of log levels
  - added:  build target to makefile

2021-01-01:  version 0.6.4
  - bugfix: Fixed refresh of time ranges
  - bugfix: Fixed drag@drop of chart bookmarks for firefox
  - bugfix: Fixed some typos at webif

2020-12-31:  version 0.6.3
  - change: Minor stylesheet modifications

2020-12-31:  version 0.6.2
  - added: Shadow for buttons and dialogs
  - added: Progress dialog
  - added: Button to refresh configured time ranges

2020-12-30:  version 0.6.1
  - change: Minor css style improvement of list view header
  - change: Adjusted dashboard view for mobile devices

2020-12-30:  version 0.6.0
  - bugfix: Fixed client ID check
  - change: Improved signal handling for web interface

2020-11-08:  version 0.5.20
  - added: Workaround for WEBIF problem
  - bugfix: Fixed sending mail to more receivers

2020-11-05:  version 0.5.19
  - added: Added missing libwebsock package

2020-11-04:  version 0.5.18
  - added: Edit of Time range parameters

2020-10-30:  version 0.5.17
  - bugfix: Fixed JS error on empty image
  - bugfix: Slow WEBIF performance on some network environments

2020-09-26:  version 0.5.16
  - change: Improved schema element selection
  - change: UC elements not created on dialog cancel
  - change: Option to use backend image for schema element

2020-09-24:  version 0.5.15
  - added:  Improved log messages on websocket errors

2020-09-23:  version 0.5.14
  - change: Improved schema element move by keboard
              -> use CTRL key to move 10 pixels instead of 1
  - added:  Configuration of element layer for schema elements
  - added:  remove of user constant schema elements

2020-09-22:  version 0.5.13
  - change: removed unused table smartconf
  - added:  type image to schema elements

2020-09-21:  version 0.5.12
  - bugfix: Fixed store of 'by key' moved elements
  - added:  User constants to schema configuration

2020-09-20:  version 0.5.11
  - bugfix: Fixed selection of heeting type

2020-09-20:  version 0.5.10
  - bugfix: Fixed missing config options

2020-09-20:  version 0.5.9
  - added: Move schema elements with arrow keys

2020-09-20:  version 0.5.8
  - bugfix: Fixed schema refresh

2020-09-20:  version 0.5.7
  - bugfix: Fixed Install script
  - added:  Config of schema image

2020-09-19:  version 0.5.6
  - bugfix: Update schemaconf on storing sensor selection
  - change: Display address in hex on schema edit dialog
  - added:  JS method getItem for schema view

2020-09-17:  version 0.5.5
  - change:  Merged dev branch back to master

2020-09-17:  version 0.5.4
  - change:  Using json 'real' for numeric values instead of string

2020-09-17:  version 0.5.3
  - bugfix:  Fixed Homeassistant interface

2020-09-16:  version 0.5.2
  - bugfix:  Fixed display of 'all' sensors

2020-09-16:  version 0.5.1
  - change:  Show peak min in list view. Show last peak reset with mouse hint at button

2020-09-14:  version 0.5.0
  - change:  First release of WS/JS version, removed apache and php stuff

2020-09-13:  version 0.5.beta.1
  - change:  Final development version of new web interface

2020-09-05:  version 0.5.alpha.1
   - change: Implementing alpha version of WS/JS version

2020-08-25:  version 0.3.39
   - change: Changed the value display default for dashbord to text

2020-08-24:  version 0.3.38
   - bugfix: Fixed packet install with non empyt db root password
   - bugfix: Fixed packet install with remote db
   - change: Added support of 'unkown' units by parsing the description

2020-08-23:  version 0.3.37
   - added: Support configurable texts for boolean values in schema view

2020-08-19:  version 0.3.36
   - change: removed dependencies to libpaho,
             replaced with a more lightweight implementaion
   - added:  auto refresh of schema view

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
