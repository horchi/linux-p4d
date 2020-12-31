/*
 *  main.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var WebSocketClient = window.WebSocketClient
//  import WebSocketClient from "./websocket.js"

var storagePrefix = "p4d";
var isActive = null;
var socket = null;
var config = {};
var daemonState = {};
var s3200State = {};
var lastUpdate = "";   // #TODO - set to data age instead of receive time
var documentName = "";
var widgetCharts = {};
var theChart = null;
var theChartRange = 2;
var theChartStart = new Date(); theChartStart.setDate(theChartStart.getDate()-theChartRange);
var chartDialogSensor = "";
var chartBookmarks = {};

window.documentReady = function(doc)
{
   daemonState.state = -1;
   s3200State.state = -1;
   documentName = doc;
   console.log("documentReady: " + documentName);

   var url = "ws://" + location.hostname + ":" + location.port;
   var protocol = "p4d";

   connectWebSocket(url, protocol);
}

function onSocketConnect(protocol)
{
   var token = localStorage.getItem(storagePrefix + 'Token');
   var user = localStorage.getItem(storagePrefix + 'User');

   if (!token || token == null)  token = "";
   if (!user || user == null)    user = "";

   // request some data at login

   var jsonArray = [];
   var nRequests = 0;
   var jsonRequest = {};

   if (documentName == "syslog")
      jsonRequest["name"] = "syslog";
   else if (documentName == "maincfg")
      jsonRequest["name"] = "configdetails";
   else if (documentName == "usercfg")
      jsonRequest["name"] = "userdetails";
   else if (documentName == "iosetup")
      jsonRequest["name"] = "iosettings";
   else if (documentName == "groups")
      jsonRequest["name"] = "groups";
   else if (documentName == "chart")
      prepareChartRequest(jsonRequest, "", 0, 2, "chart")
   else if (documentName == "dashboard")
      jsonRequest["name"] = "data";
   else if (documentName == "list")
      jsonRequest["name"] = "data";
   else if (documentName == "errors")
      jsonRequest["name"] = "errors";
   else if (documentName == "menu")
      jsonRequest["name"] = "menu";
   else if (documentName == "alerts")
      jsonRequest["name"] = "alerts";
   else if (documentName == "schema")
      jsonRequest["name"] = "schema";

   jsonArray[0] = jsonRequest;

      socket.send({ "event" : "login", "object" :
                    { "type" : "active",
                      "user" : user,
                      "token" : token,
                      "page"  : documentName,
                      "requests" : jsonArray }
                  });
}

function connectWebSocket(useUrl, protocol)
{
   socket = new WebSocketClient({
      url: useUrl,
      protocol: protocol,
      autoReconnectInterval: 1000,
      onopen: function (){
         console.log("socket opened :)");
         if (isActive === null)     // wurde beim Schliessen auf null gesetzt
            onSocketConnect(protocol);
      }, onclose: function (){
         isActive = null;           // auf null setzen, dass ein neues login aufgerufen wird
      }, onmessage: function (msg) {
         dispatchMessage(msg.data)
      }.bind(this)
   });

   if (!socket)
      return !($el.innerHTML = "Your Browser will not support Websockets!");
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

var infoDialog = null;

async function showInfoDialog(message, titleMsg, onCloseCallback)
{
   while (infoDialog)
      await sleep(100);

   var msDuaration = 2000;
   var bgColor = null;
   var height = 70;
   var cls = "no-titlebar";

   if (titleMsg && titleMsg != "") {
      msDuaration = 10000;
      cls = "";
      height = 100;
      if (titleMsg.indexOf("Error") != -1 || titleMsg.indexOf("Fehler") != -1)
         bgColor = 'background-color:rgb(224, 102, 102);'
   }

   $('<div style="margin-top:13px;' + (bgColor != null ? bgColor : "")  + '"></div>').html(message).dialog({
      dialogClass: cls,
      width: "60%",
      height: height,
      title: titleMsg,
		modal: true,
      resizable: true,
		closeOnEscape: true,
      hide: "fade",
      open:  function() {
         infoDialog = $(this); setTimeout(function() {
            if (infoDialog)
               infoDialog.dialog('close');
            infoDialog = null }, msDuaration);
      },
      close: function() {
         $(this).dialog('destroy').remove();
         infoDialog = null;
      }
   });
}

var progressDialog = null;

async function hideProgressDialog()
{
   if (progressDialog != null) {
      console.log("hide progress");
      progressDialog.dialog('destroy').remove();
      progressDialog = null;
   }
}

async function showProgressDialog()
{
   while (progressDialog)
      await sleep(100);

   var msDuration = 30000;   // timeout 30 seconds
   var form = document.createElement("form");
   form.style.overflow = "hidden";
   var div = document.createElement("div");
   form.appendChild(div);
   div.className = "progress";

   console.log("show progress");

   $(form).dialog({
      dialogClass: "no-titlebar rounded-border",
      width: "130px",
      title: "",
		modal: true,
      resizable: false,
		closeOnEscape: false,
      minHeight: "0px",
      hide: "fade",
      open: function() {
         progressDialog = $(this); setTimeout(function() {
            if (progressDialog)
               progressDialog.dialog('close');
            progressDialog = null }, msDuration);
      },
      close: function() {
         $(this).dialog('destroy').remove();
         progressDialog = null;
      }
   });
}

function dispatchMessage(message)
{
   var jMessage = JSON.parse(message);
   var event = jMessage.event;
   var rootList = document.getElementById("listContainer");
   var rootDashboard = document.getElementById("widgetContainer");
   var rootConfig = document.getElementById("configContainer");
   var rootIoSetup = document.getElementById("ioSetupContainer");
   var rootGroupSetup = document.getElementById("groupContainer");
   var rootChart = document.getElementById("chart");
   var rootDialog = document.querySelector('dialog');
   var rootErrors = document.getElementById("errorContainer");
   var rootMenu = document.getElementById("menuContainer");
   var rootAlerts = document.getElementById("alertContainer");
   var rootSchema = document.getElementById("schemaContainer");
   var d = new Date();

   console.log("got event: " + event);

   if (event == "result") {
      hideProgressDialog();
      if (jMessage.object.status == 0)
         showInfoDialog(jMessage.object.message);
      else
         showInfoDialog(jMessage.object.message , "Information (" + jMessage.object.status + ")");
   }
   else if ((event == "update" || event == "all") && rootDashboard) {
      lastUpdate = d.toLocaleTimeString();
      updateDashboard(jMessage.object);
   }
   else if ((event == "update" || event == "all") && rootList) {
      lastUpdate = d.toLocaleTimeString();
      updateList(jMessage.object);
   }
   else if (event == "init" && rootDashboard) {
      lastUpdate = d.toLocaleTimeString();
      initDashboard(jMessage.object, rootDashboard);
      updateDashboard(jMessage.object);
   }
   else if (event == "init" && rootList) {
      lastUpdate = d.toLocaleTimeString();
      initList(jMessage.object, rootList);
      updateList(jMessage.object);
   }
   else if ((event == "init" || event == "update" || event == "all") && rootSchema) {
      lastUpdate = d.toLocaleTimeString();
      updateSchema(jMessage.object);
   }
   else if (event == "schema" && rootSchema) {
      initSchema(jMessage.object, rootSchema);
   }
   else if (event == "chartbookmarks") {
      chartBookmarks = jMessage.object;
      updateChartBookmarks();
   }
   else if (event == "config") {
      config = jMessage.object;
      prepareMenu();
   /* var head = document.getElementsByTagName('HEAD')[0];
      var link = document.createElement('link');
      link.rel = 'stylesheet';
      link.type = 'text/css';
      link.href = 'stylesheet-' + config.style + '.css';
      head.appendChild(link);
      console.log("Useing stylesheet: " + link.href); */
   }
   else if (event == "configdetails" && rootConfig) {
      initConfig(jMessage.object, rootConfig)
   }
   else if (event == "userdetails" && rootConfig) {
      initUserConfig(jMessage.object, rootConfig)
   }
   else if (event == "daemonstate") {
      daemonState = jMessage.object;
   }
   else if (event == "s3200-state") {
      s3200State = jMessage.object;
   }
   else if (event == "syslog") {
      showSyslog(jMessage.object);
   }
   else if (event == "token") {
      localStorage.setItem(storagePrefix + 'Token', jMessage.object.value);
      localStorage.setItem(storagePrefix + 'User', jMessage.object.user);
      localStorage.setItem(storagePrefix + 'Rights', jMessage.object.rights);

      if (jMessage.object.state == "confirm") {
         window.location.replace("index.html");
      } else { // if (documentName == "login") {
         if (document.getElementById("confirm"))
            document.getElementById("confirm").innerHTML = "<div class=\"infoError\"><b><center>Login fehlgeschlagen</center></b></div>";
      }
   }
   else if (event == "groups" && rootGroupSetup) {
      initGroupSetup(jMessage.object, rootGroupSetup);
   }
   else if (event == "valuefacts" && rootIoSetup) {
      initIoSetup(jMessage.object, rootIoSetup);
   }
   else if (event == "errors" && rootErrors) {
      initErrors(jMessage.object, rootErrors);
   }
   else if (event == "menu" && rootMenu) {
      initMenu(jMessage.object, rootMenu);
   }
   else if (event == "alerts" && rootAlerts) {
      initAlerts(jMessage.object, rootAlerts);
   }
   else if (event == "pareditrequest" && rootMenu) {
      editMenuParameter(jMessage.object, rootMenu);
   }
   else if (event == "chartdata") {
      var id = jMessage.object.id;
      if (rootChart) {                                       // the charts page
         drawCharts(jMessage.object, rootChart);
      }
      else if (rootDashboard && id == "chartwidget") {       // the dashboard widget
         drawChartWidget(jMessage.object, rootDashboard);
      }
      else if (rootDialog && id == "chartdialog") {          // the dashboard chart dialog
         drawChartDialog(jMessage.object, rootDialog);
      }
   }

   // console.log("event: " + event + " dispatched");
}

function prepareMenu()
{
   var html = "";
   var haveToken = localStorage.getItem(storagePrefix + 'Token') != "";

   html += "<a href=\"index.html\"><button class=\"rounded-border button1\">Dashboard</button></a>";
   html += "<a href=\"list.html\"><button class=\"rounded-border button1\">Liste</button></a>";
   html += "<a href=\"chart.html\"><button class=\"rounded-border button1\">Charts</button></a>";
   html += "<a href=\"schema.html\"><button class=\"rounded-border button1\">Funktionsschema</button></a>";
   html += "<a href=\"menu.html\"><button class=\"rounded-border button1\">Service Menü</button></a>";
   html += "<a href=\"errors.html\"><button class=\"rounded-border button1\">Fehler</button></a>";

   html += "<div class=\"menuLogin\">";
   if (haveToken)
      html += "<a href=\"user.html\"><button class=\"rounded-border button1\">[" + localStorage.getItem(storagePrefix + 'User') + "]</button></a>";
   else
      html += "<a href=\"login.html\"><button class=\"rounded-border button1\">Login</button></a>";
   html += "</div>";

   if (localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10) {
      html += "<a href=\"maincfg.html\"><button class=\"rounded-border button1\">Setup</button></a>";

      if ($("#navMenu").data("setup") != undefined) {
         html += "<div>";
         html += "  <a href=\"maincfg.html\"><button class=\"rounded-border button2\">Allg. Konfiguration</button></a>";
         html += "  <a href=\"iosetup.html\"><button class=\"rounded-border button2\">Aufzeichnung</button></a>";
         html += "  <a href=\"alerts.html\"><button class=\"rounded-border button2\">Sensor Alerts</button></a>";
         html += "  <a href=\"groups.html\"><button class=\"rounded-border button2\">Baugruppen</button></a>";
         html += "  <a href=\"usercfg.html\"><button class=\"rounded-border button2\">User</button></a>";
         html += "  <a href=\"syslog.html\"><button class=\"rounded-border button2\">Syslog</button></a>";
         html += "</div>";
      }
   }

   // buttons below menu

   if ($("#navMenu").data("iosetup") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"storeIoSetup()\">Speichern</button>";
      html += "  <button class=\"rounded-border buttonOptions\" id=\"filterIoSetup\" onclick=\"filterIoSetup()\">Filter [alle]</button>";
      html += "</div>";
   }
   else if ($("#navMenu").data("alerts") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"storeAlerts()\">Speichern</button>";
      html += "</div>";
   }
   else if ($("#navMenu").data("schema") != undefined) {
      if (localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10) {
         html += "<div class=\"confirmDiv\">";
         html += "  <button class=\"rounded-border buttonOptions\" onclick=\"schemaEditModeToggle()\">Anpassen</button>";
         html += "  <button class=\"rounded-border buttonOptions\" id=\"buttonSchemaAddItem\" title=\"Konstante (Text) hinzufügen\" style=\"visibility:hidden;\" onclick=\"schemaAddItem()\">&#10010;</button>";
         html += "  <button class=\"rounded-border buttonOptions\" id=\"buttonSchemaStore\" style=\"visibility:hidden;\" onclick=\"schemaStore()\">Speichern</button>";
         html += "</div>";
      }
   }
   else if ($("#navMenu").data("sevicemenu") != undefined) {
      if (localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10) {
         html += "<div class=\"confirmDiv\">";
         html += "  <button class=\"rounded-border buttonOptions\" onclick=\"updateTimeRanges()\">Zeiten aktualisieren</button>";
         html += "</div>";
      }
   }
   else if ($("#navMenu").data("groups") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"storeGroups()\">Speichern</button>";
      html += "</div>";
   }
   else if ($("#navMenu").data("maincfg") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"storeConfig()\">Speichern</button>";
      html += "  <button class=\"rounded-border buttonOptions\" title=\"Letzter Reset: " + config.peakResetAt + "\" id=\"buttonResPeaks\" onclick=\"resetPeaks()\">Reset Peaks</button>";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"sendMail('Test Mail', 'test')\">Test Mail</button>";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"initTables('menu')\">Init Service Menü</button>";
      html += "  <button class=\"rounded-border buttonOptions\" onclick=\"initTables('valuefacts')\">Init Messwerte</button>";
      html += "</div>";
   }
   else if ($("#navMenu").data("login") != undefined)
      html += "<div id=\"confirm\" class=\"confirmDiv\"/>";

   $("#navMenu").html(html);

   var msg = "DEBUG: Browser: '" + $.browser.name + "' : '" + $.browser.versionNumber + "' : '" + $.browser.version + "'";
   socket.send({ "event" : "logmessage", "object" : { "message" : msg } });
}

function showSyslog(log)
{
   var root = document.getElementById("syslogContainer");
   root.innerHTML = log.lines.replace(/(?:\r\n|\r|\n)/g, '<br/>');
}

function initErrors(errors, root)
{
   // console.log(JSON.stringify(errors, undefined, 4));

   tableRoot = document.getElementById("errors");
   tableRoot.innerHTML = "";

   for (var i = 0; i < errors.length; i++) {
      var item = errors[i];

      var style = item.state == "quittiert" || item.state == "gegangen" ? "greenCircle" : "redCircle";
      var html = "<td><div class=\"" + style + "\"></div></td>";
      html += "<td class=\"tableMultiColCell\">" + item.time + "</td>";
      html += "<td class=\"tableMultiColCell\">" + (item.duration / 60).toFixed(0) + "</td>";
      html += "<td class=\"tableMultiColCell\">" + item.text + "</td>";
      html += "<td class=\"tableMultiColCell\">" + item.state + "</td>";

      if (tableRoot != null) {
         var elem = document.createElement("tr");
         elem.innerHTML = html;
         tableRoot.appendChild(elem);
      }
   }
}

window.sendMail = function(subject, body)
{
   socket.send({ "event" : "sendmail", "object" : { "subject" : subject, "body" : body } });
}

window.toggleMode = function(address, type)
{
   socket.send({ "event": "togglemode", "object":
                 { "address": address,
                   "type": type }
               });
}

window.toggleIo = function(address, type)
{
   socket.send({ "event": "toggleio", "object":
                 { "address": address,
                   "type": type }
               });
}

window.toggleIoNext = function(address, type)
{
   socket.send({ "event": "toggleionext", "object":
                 { "address": address,
                   "type": type }
               });
}

window.vdrKeyPress = function(key)
{
   // { "event" : "keypress", "object" : { "key" : "menu", "repeat" : 1 } }

   if (key == undefined || key == "")
      return;

   socket.send({ "event": "keypress", "object":
                 { "key": key,
                   "repeat": 1 }
               });
}

window.doLogin = function()
{
   // console.log("login: " + $("#user").val() + " : " + $.md5($("#password").val()));
   socket.send({ "event": "gettoken", "object":
                 { "user": $("#user").val(),
                   "password": $.md5($("#password").val()) }
               });
}

window.doLogout = function()
{
   localStorage.removeItem(storagePrefix + 'Token');
   localStorage.removeItem(storagePrefix + 'User');
   localStorage.removeItem(storagePrefix + 'Rights');

   window.location.replace("login.html");
}

// ---------------------------------
// charts

function prepareChartRequest(jRequest, sensors, start, range, id)
{
   var gaugeSelected = false;

   if (!$.browser.safari || $.browser.versionNumber > 9)
   {
      const urlParams = new URLSearchParams(window.location.search);
      if (urlParams.has("sensors"))
      {
         gaugeSelected = true;
         sensors = urlParams.get("sensors");
      }
   }

   jRequest["id"] = id;
   jRequest["name"] = "chartdata";

   // console.log("requesting chart for '" + start + "' range " + range);

   if (gaugeSelected) {
      jRequest["sensors"] = sensors;
      jRequest["start"] = 0;   // default (use today-range)
      jRequest["range"] = 1;
   }
   else {
      jRequest["start"] = start == 0 ? 0 : Math.floor(start.getTime()/1000);  // calc unix timestamp
      jRequest["range"] = range;
      jRequest["sensors"] = sensors;
   }
}

function drawChartWidget(dataObject, root)
{
   var id = "widget" + dataObject.rows[0].sensor;

   if (widgetCharts[id] != null) {
      widgetCharts[id].destroy();
      widgetCharts[id] = null;
   }

   var data = {
      type: "line",
      data: {
         datasets: []
      },
      options: {
         legend: {
            display: false
         },
         responsive: true,
         maintainAspectRatio: false,
         aspectRatio: false,
         scales: {
            xAxes: [{
               type: "time",
               distribution: "linear",
               display: false
            }],
            yAxes: [{
               display: false
            }]
         }
      }
   };

   for (var i = 0; i < dataObject.rows.length; i++)
   {
      var dataset = {};

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = "#415969";  // fill color
      dataset["borderColor"] = "#3498db";      // line color
      dataset["fill"] = true;
      dataset["pointRadius"] = 2.0;
      data.data.datasets.push(dataset);
   }

   var canvas = document.getElementById(id);
   widgetCharts[id] = new Chart(canvas, data);
}

function drawChartDialog(dataObject, root)
{
   if (dataObject.rows[0].sensor != chartDialogSensor) {
      return ;
   }

   if (theChart != null) {
      theChart.destroy();
      theChart = null;
   }

   var data = {
      type: "line",
      data: {
         datasets: []
      },
      options: {
         legend: {
            display: true
         },
         responsive: true,
         maintainAspectRatio: false,
         aspectRatio: false,
         scales: {
            xAxes: [{
               type: "time",
               time: {
                  unit: 'hour',
                  unitStepSize: 1,
                  displayFormats: {
                  millisecond: 'MMM DD - HH:mm',
                  second: 'MMM DD - HH:mm',
                  minute: 'HH:mm',
                  hour: 'MMM DD - HH:mm',
                  day: 'HH:mm',
                  week: 'MMM DD - HH:mm',
                  month: 'MMM DD - HH:mm',
                  quarter: 'MMM DD - HH:mm',
                  year: 'MMM DD - HH:mm' } },
               distribution: "linear",
               display: true,
               ticks: {
                  maxTicksLimit: 24,
                  padding: 10,
                  fontColor: "white"
               },
               gridLines: {
                  color: "gray",
                  borderDash: [5,5]
               },
               scaleLabel: {
                  display: true,
                  fontColor: "white",
                  labelString: "Zeit"
               }
            }],
            yAxes: [{
               display: true,
               ticks: {
                  padding: 10,
                  maxTicksLimit: 20,
                  fontColor: "white"
               },
               gridLines: {
                  color: "gray",
                  zeroLineColor: 'gray',
                  borderDash: [5,5]
               },
               scaleLabel: {
                  display: true,
                  fontColor: "white",
                  labelString: "Temperatur [°C]"
               }
            }]
         }
      }
   };

   for (var i = 0; i < dataObject.rows.length; i++)
   {
      var dataset = {};

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = "#415969";  // fill color
      dataset["borderColor"] = "#3498db";      // line color
      dataset["fill"] = true;
      dataset["label"] = dataObject.rows[i].title;
      dataset["pointRadius"] = 0;
      data.data.datasets.push(dataset);
   }

   var canvas = root.querySelector("#chartDialog");
   theChart = new Chart(canvas, data);
}
