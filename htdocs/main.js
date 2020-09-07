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
// import WebSocketClient from "./websocket.js"

const osd2webUrl = "ws://192.168.200.145:4444";

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

window.documentReady = function(doc)
{
   daemonState.state = -1;
   s3200State.state = -1;
   documentName = doc;
   console.log("documentReady: " + documentName);

   var url = "ws://" + location.hostname + ":1111";
   var protocol = "p4d";

   connectWebSocket(url, protocol);
}

function onSocketConnect(protocol)
{
   var token = localStorage.getItem('p4dToken');
   var user = localStorage.getItem('p4dUser');

   if (!token || token == null)  token = "";
   if (!user || user == null)    user = "";

   prepareMenu(token != "");

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

   jsonArray[0] = jsonRequest;

   if (documentName != "login") {
      socket.send({ "event" : "login", "object" :
                    { "type" : "active",
                      "user" : user,
                      "token" : token,
                      "requests" : jsonArray }
                  });
   }
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

   var d = new Date();

   console.log("got event: " + event);

   if (event == "result") {
      if (jMessage.object.status == 0)
         dialog.alert({ title: "",
                        message: jMessage.object.message,
	                     cancel: "Schließen"
	                   });
      else
         dialog.alert({ title: "Information (" + jMessage.object.status + ")",
                          message: jMessage.object.message,
	                       cancel: "Schließen"
	                     });
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
   else if (event == "config") {
      config = jMessage.object;
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
      localStorage.setItem('p4dToken', jMessage.object.value);
      localStorage.setItem('p4dUser', jMessage.object.user);
      localStorage.setItem('p4dRights', jMessage.object.rights);

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

function prepareMenu(haveToken)
{
   var html = "";

   html += "<a href=\"index.html\"><button class=\"rounded-border button1\">Dashboard</button></a>";
   html += "<a href=\"list.html\"><button class=\"rounded-border button1\">Liste</button></a>";
   html += "<a href=\"chart.html\"><button class=\"rounded-border button1\">Charts</button></a>";
   html += "<a href=\"menu.html\"><button class=\"rounded-border button1\">Menü</button></a>";
   html += "<a href=\"errors.html\"><button class=\"rounded-border button1\">Fehler</button></a>";

   html += "<div class=\"menuLogin\">";
   if (haveToken)
      html += "<a href=\"user.html\"><button class=\"rounded-border button1\">[" + localStorage.getItem('p4dUser') + "]</button></a>";
   else
      html += "<a href=\"login.html\"><button class=\"rounded-border button1\">Login</button></a>";
   html += "</div>";

   if (localStorage.getItem('p4dRights') & 0x08 || localStorage.getItem('p4dRights') & 0x10) {
      html += "<a href=\"maincfg.html\"><button class=\"rounded-border button1\">Setup</button></a>";

      if ($("#navMenu").data("setup") != undefined) {
         html += "<div>";
         html += "  <a href=\"maincfg.html\"><button class=\"rounded-border button2\">Allg. Konfiguration</button></a>";
         html += "  <a href=\"iosetup.html\"><button class=\"rounded-border button2\">Aufzeichnung</button></a>";
         html += "  <a href=\"groups.html\"><button class=\"rounded-border button2\">Baugruppen</button></a>";
         html += "  <a href=\"usercfg.html\"><button class=\"rounded-border button2\">User</button></a>";
         html += "  <a href=\"syslog.html\"><button class=\"rounded-border button2\">Syslog</button></a>";
         html += "</div>";
      }
   }

   // storr button below menu #TODO user dialog instead

   if ($("#navMenu").data("iosetup") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border button2\" onclick=\"storeIoSetup()\">Speichern</button>";
      html += "</div>";
   }
   if ($("#navMenu").data("groups") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border button2\" onclick=\"storeGroups()\">Speichern</button>";
      html += "</div>";
   }
   else if ($("#navMenu").data("maincfg") != undefined) {
      html += "<div class=\"confirmDiv\">";
      html += "  <button class=\"rounded-border button2\" onclick=\"storeConfig()\">Speichern</button>";
      html += "  <button id=\"buttonResPeaks\" class=\"rounded-border button2\" onclick=\"resetPeaks()\">Reset Peaks</button>";
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
   root.innerHTML = log.lines.replace(/(?:\r\n|\r|\n)/g, '<br>');
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

window.chartSelect = function(action)
{
   // console.log("chartSelect clicked for " + action);

   var sensors = "";
   var root = document.getElementById("chartSelector");
   var elements = root.querySelectorAll("[id^='checkChartSel_']");

   for (var i = 0; i < elements.length; i++) {
      if (elements[i].checked) {
         var id = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
         sensors += id + ",";
       }
   }

   theChartRange = parseInt($("#chartRange").val());

   var now = new Date();

   if (action == "next")
      theChartStart.setDate(theChartStart.getDate()+1);
   else if (action == "prev")
      theChartStart.setDate(theChartStart.getDate()-1);
   else if (action == "now")
      theChartStart.setDate(now.getDate()-theChartRange);
   else if (action == "range")
      theChartStart.setDate(now.getDate()-theChartRange);

   // console.log("sensors:  '" + sensors + "'");

   var jsonRequest = {};
   prepareChartRequest(jsonRequest, sensors, theChartStart, theChartRange, "chart");
   socket.send({ "event" : "chartdata", "object" : jsonRequest });
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
   localStorage.removeItem('p4dToken');
   localStorage.removeItem('p4dUser');
   localStorage.removeItem('p4dRights');

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

function drawCharts(dataObject, root)
{
   if (theChart != null) {
      theChart.destroy();
      theChart = null;
   }

   var data = {
      type: "line",
      data: {
         labels: [],
         datasets: []
      },
      options: {
         responsive: false,
         tooltips: {
            mode: "index",
            intersect: false,
         },
         hover: {
            mode: "nearest",
            intersect: true
         },
         legend: {
            display: true,
            labels: {
               fontColor: "white"
            }
         },
         scales: {
            xAxes: [{
               type: "time",
               time: { displayFormats: {
                  millisecond: 'MMM DD - HH:MM',
                  second: 'MMM DD - HH:MM',
                  minute: 'HH:MM',
                  hour: 'MMM DD - HH:MM',
                  day: 'HH:MM',
                  week: 'MMM DD - HH:MM',
                  month: 'MMM DD - HH:MM',
                  quarter: 'MMM DD - HH:MM',
                  year: 'MMM DD - HH:MM' } },
               distribution: "linear",
               display: true,
               ticks: {
                  maxTicksLimit: 25,
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

   // console.log("dataObject: " + JSON.stringify(dataObject, undefined, 4));

   var colors = ['yellow','white','red','lightblue','lightgreen','purple','blue'];

   for (var i = 0; i < dataObject.rows.length; i++)
   {
      var dataset = {};

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = colors[i];
      dataset["borderColor"] = colors[i];
      dataset["label"] = dataObject.rows[i].title;
      dataset["borderWidth"] = 1.2;
      dataset["fill"] = false;
      dataset["pointRadius"] = 0;

      data.data.datasets.push(dataset);
   }

   var end = new Date();
   end.setDate(theChartStart.getDate()+theChartRange);

   $("#chartTitle").html(theChartStart.toLocaleString('de-DE') + "  -  " + end.toLocaleString('de-DE'));
   $("#chartSelector").html("");

   for (var i = 0; i < dataObject.sensors.length; i++)
   {
      var html = "<div class=\"chartSel\"><input id=\"checkChartSel_" + dataObject.sensors[i].id + "\"type=\"checkbox\" onclick=\"chartSelect('choice')\" " + (dataObject.sensors[i].active ? "checked" : "") + "/>" + dataObject.sensors[i].title + "</div>";
      $("#chartSelector").append(html);
   }

   theChart = new Chart(root.getContext("2d"), data);
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
               time: { displayFormats: {
                  millisecond: 'MMM DD - HH:MM',
                  second: 'MMM DD - HH:MM',
                  minute: 'HH:MM',
                  hour: 'MMM DD - HH:MM',
                  day: 'HH:MM',
                  week: 'MMM DD - HH:MM',
                  month: 'MMM DD - HH:MM',
                  quarter: 'MMM DD - HH:MM',
                  year: 'MMM DD - HH:MM' } },
               distribution: "linear",
               display: true,
               ticks: {
                  maxTicksLimit: 25,
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
      // console.log("append dataset for chart dialog " + i);
   }

   var canvas = root.querySelector("#chartDialog");
   theChart = new Chart(canvas, data);
}
