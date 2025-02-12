/*
 *  main.js
 *
 *  (c) 2020-2025 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var WebSocketClient = window.WebSocketClient
var pingTimeoutMs = 10000;
var colorStyle = null;
var isDaytime = false;
var daytimeCalcAt = null;
var schemaEditActive = false;

var onSmalDevice = false;
var isActive = null;
var socket = null;
var lastPingAt = 0;
var config = {};
var commands = {};
var daemonState = {};

var widgetTypes = {};
var valueTypes = [];
var valueFacts = {};
var dashboards = {};
var allSensors = [];
var wifis = [];
var images = [];
var systemServices = [];
var syslogs = [];
var syslogFilter = "";
var currentPage = "dashboard";
var startPage = null;
var actDashboard = -1;
var s3200State = {};
var widgetCharts = {};
var theChart = null;
var theChartRange = null;
var theChartStart = null;
var chartDialogSensor = "";
var chartBookmarks = {};
var infoDialogTimer = null;
var grouplist = {};

var lmcData = {};

var setupMode = false;
var dashboardGroup = 0;
var kioskBackTime = 0;
var kioskMode = null;    // 0 - with menu,    normal dash symbols, normal-widget-height
							    // 1 - without menu, big dash symbols,    kiosk-widget-height
							    // 2 - with menu,    big dash symbols,    kiosk-widget-height
							    // 3 - with menu,    big dash symbols,    normal-widget-height

var heightFactor = null;
var sab = 0;             // env(safe-area-inset-bottom)

$('document').ready(function() {
   daemonState.state = -1;
   s3200State.state = -1;

   const urlParams = new URLSearchParams(window.location.href);
   kioskMode = urlParams.get('kiosk');
   heightFactor = urlParams.get('heightFactor');
   dashboardGroup = urlParams.get('group') != null ? urlParams.get('group') : 0;
   kioskBackTime = urlParams.get('backTime');
   console.log("kioskMode", kioskMode, "heightFactor", heightFactor);
   console.log("dashboardGroup" + " : " + dashboardGroup);
   startPage = urlParams.get('page') != null ? urlParams.get('page') : null;
   actDashboard = urlParams.get('dash') != null ? urlParams.get('dash') : -1;
   console.log("currentPage: " + currentPage);
   console.log("startPage: " + startPage);

   let connectUrl = "";

   if (window.location.href.startsWith("https"))
      connectUrl = "wss://" + location.hostname + ":" + location.port;
   else
      connectUrl = "ws://" + location.hostname + ":" + location.port;

   let protocol = myProtocol;

   moment.locale('de');
   connectWebSocket(connectUrl, protocol);
   colorStyle = getComputedStyle(document.body);
   $('#socketState').click(function() { daemonStateDlg(); });
});

var socketStateInterval = null;

function onSocketConnect(protocol)
{
   let token = localStorage.getItem(storagePrefix + 'Token');
   let user = localStorage.getItem(storagePrefix + 'User');

   if (token == null) token = "";
   if (user == null)  user = "";

   socket.send({ "event" : "login", "object" :
                 { "type" : "active",
                   "user" : user,
                   "page" : currentPage,
                   "token" : token,
                   "url" : window.location.href }
               });

   if (socketStateInterval)
      clearInterval(socketStateInterval);

   socketStateInterval = setInterval(function() {
      updateSocketState();
   }, 1*1000);

   onSmalDevice = window.matchMedia("(max-width: 740px)").matches;
   // console.log("onSmalDevice : " + onSmalDevice);

   mainMenuSel(currentPage);
   sab = parseInt(getComputedStyle(document.documentElement).getPropertyValue("--sab"));
}

function connectWebSocket(useUrl, protocol)
{
   console.log("try socket opened " + protocol);

   socket = new WebSocketClient({
      url: useUrl,
      protocol: protocol,
      autoReconnectInterval: 1000,
      onopen: function () {
         console.log("socket opened " + socket.protocol);
         if (isActive === null)     // wurde beim Schliessen auf null gesetzt
            onSocketConnect(protocol);
      }, onclose: function () {
         isActive = null;           // auf null setzen, damit ein neues login aufgerufen wird
      }, onmessage: function (msg) {
         dispatchMessage(msg.data)
      }.bind(this)
   });

   if (!socket)
      return !($el.innerHTML = "Your Browser will not support Websockets!");
}

function daemonStateDlg()
{
   let isOnline = daemonState.state != null && daemonState.state == 0;
   let isConnected = socket && socket.ws.readyState == WebSocket.OPEN;
   let form = '<div id="daemonStateDlg">';

   form += ' <div class="dialog-content">';

   if (isOnline) {
      form += '  <div style="display:flex;padding:2px;"><span style="width:30%;display:inline-block;">Läuft seit:</span><span display="inline-block">' + daemonState.runningsince + '</span></div>\n';
      form += '  <div style="display:flex;padding:2px;"><span style="width:30%;display:inline-block;">Version:</span> <span display="inline-block">' + daemonState.version + '</span></div>\n';
      form += '  <div style="display:flex;padding:2px;"><span style="width:30%;display:inline-block;">CPU Last:</span><span display="inline-block">' + daemonState.average0 + ' / ' + daemonState.average1 + ' / '  + daemonState.average2 + '</span></div>\n';
      form += '  <div style="display:flex;padding:2px;"><span style="width:30%;display:inline-block;">Verbunden:</span><span display="inline-block">' + (isConnected ? 'JA' : 'NEIN') + '</span></div>\n';
   }

   form += ' </div>';
   form += '</div>';

   $(form).dialog({
      title: config.instanceName + (isOnline ? ' - ONLINE' : ' - OFFLINE'),
      width: "400px",
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      open: function(event, ui) {
         $('.ui-widget-overlay').bind('click', function()
                                      { $("#daemonStateDlg").dialog('close'); });
      },
      close: function() {
         $("body").off("click");
         $(this).dialog('destroy').remove();
      }
   });

}

var socketStateCount = 0;
var socketStateTimeInterval = null;

function updateSocketState()
{
   if (!$('#socketState').length)
      return ;

   // VDR did not support ping

   if (currentPage != "vdr") {
		let now = new Date().getTime();
      if (lastPingAt + pingTimeoutMs <= now) {
         console.log("Warning: Reconnecting web socket due to timeout, last keep alive was ", now - lastPingAt, "ms ago");
         socket.reopen();
         lastPingAt = new Date().getTime();
      }
   }

   let circle = 'yellowCircle';

   if (socketStateCount++ % 2)
      circle = 'grayCircle';
   else if (!socket || socket.ws.readyState != WebSocket.OPEN)
      circle = 'redCircle';
   else
      circle = 'greenCircle';

   $('#socketState').attr('class', '');
   $('#socketState').addClass('socketState ' + circle);

   let now = new Date();
   $('#footerTime').html(now.toTimeLocal());
}

function sleep(ms) {
   return new Promise(resolve => setTimeout(resolve, ms));
}

var infoDialog = null;

async function showInfoDialog(object)
{
   let message = object.message;
   let titleMsg = '';
   let msDuration = 2000;

   if (object.status == 0)
      msDuration = 4000;
   else if (object.status <= 1)
      msDuration = 10000;
   else if (object.status >= 10)
      msDuration = 5000;

   hideInfoDialog();

   if (object.status == 0) {
      $('#action-state').css('opacity', 0);
      $('#action-state').html(message + ' - success');
      $('#action-state').animate({'opacity': '1'}, 300);
   }

   if (object.status == 0) {
      titleMsg = '<a style="color:darkgreen;">Success</a>';
   }
   else if (object.status == -1) {
      titleMsg = "Error";
      titleMsg = '<a style="color:var(--red1);">Error</a>';
   }
   else if (object.status < -1) {
      titleMsg = '<a style="color:var(--red1);">Error (' + object.status + ')</a>';
   }
   else if (object.status == 1) {
      let array = message.split("#:#");
      titleMsg = array[0];
   }
   else if (object.status >= 10) {
      titleMsg = '<a style="color:yellow;">Note</a>';
   }

   let now = new Date();
   titleMsg = now.toDatetimeLocal() + '  ' + titleMsg;

   if ($('#infoBox').html() == '')
      $('#infoBox').append($('<div></div>')
                           .attr('id', 'infoBoxFirstElement')
                           .addClass('rounded-border')
                           .append($('<span></span>')
                                   .css('width', '-webkit-fill-available')
                                   .css('width', '-moz-available')
                                   .append($('<button></button>')
                                           .addClass('rounded-border tool-button-svg-smal')
                                           .html('<svg><use xlink:href="#close"></use></svg>')
                                           .click(function(event) { $('#infoBox').html(''); }))
                                   .append($('<span></span>')
                                           .html('Clear messages'))));

   let newDiv = $('<div></div>')
       .addClass('rounded-border')
       .css('background-color', 'rgb(173 169 87)')
       .append($('<span></span>')
               .css('width', '-webkit-fill-available')
               .css('width', '-moz-available')
               .append($('<button></button>')
                       .addClass('rounded-border tool-button-svg-smal')
                       .html('<svg><use xlink:href="#close"></use></svg>')
                       .click(function(event) { $(this).closest('div').remove(); }))
               .append($('<span></span>')
                       .css('font-weight', 'bold')
                       .html(titleMsg))
               .append($('<br></br>')
                       .addClass(titleMsg == '' ? 'hidden' : ''))
               .append($('<div></div>')
                       .css('white-space', 'pre')
                       .css('padding-left', '20px')
                       .text(message)));

   newDiv.insertAfter('#infoBoxFirstElement');
   newDiv.animate({backgroundColor: 'transparent'}, 2000);

   if (object.status != 0) {
      viewInfoDialog();
      $("#infoBox").scrollTop(0);
   }

   infoDialogTimer = setTimeout(function() {
      hideInfoDialog();
   }, msDuration);
}

function toggleInfoDialog()
{
   if (infoDialogTimer) {
      clearTimeout(infoDialogTimer);
      infoDialogTimer = null;
   }

   if ($("#infoBox").hasClass('infoBox-active'))
      hideInfoDialog();
   else
      viewInfoDialog();
}

function viewInfoDialog()
{
   $("#infoBox").css('top', $("#content").position().top);
   $('#infoBox').addClass('infoBox-active');
   $("#content").click(function() { hideInfoDialog(); });
   $("#toggleMsgBtn").html('<svg><use xlink:href="#angle-up"></use></svg>');
}

function hideInfoDialog()
{
   $("#content").off("click");

   if (infoDialogTimer) {
      clearTimeout(infoDialogTimer);
   infoDialogTimer = null;
   }

   $('#infoBox').removeClass('infoBox-active');
   $('#toggleMsgBtn').html('<svg><use xlink:href="#angle-down"></use></svg>');
   $('#action-state').animate({'opacity': '0'}, 1000);
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

   let msDuration = 300000;   // timeout 5 minutes

   let form = document.createElement("div");
   form.style.overflow = "hidden";
   let div = document.createElement("div");
   form.appendChild(div);
   div.className = "progress";

   console.log("show progress");

   $(form).dialog({
      dialogClass: "no-titlebar rounded-border",
      width: "125px",
      title: "",
		modal: true,
      resizable: false,
		closeOnEscape: true,
      minHeight: "0px",
      hide: "fade",
      open: function() {
         progressDialog = $(this);
         setTimeout(function() {
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

function pwdDialog(onConfirm, message, okBtn = 'Continue', cancelBtn = 'Cancel')
{
   let form = '<div>' +
       '  <div class="dialog-content">' +
       '    <input class="rounded-border input" id="pwdDlgValue"></input>'
       '  </div>';

   $(form).dialog({
      hide: "fade",
      title: 'Wifi key',
      buttons: {
         [okBtn]:     function() { $(this).dialog('close'); onConfirm($('#pwdDlgValue').val()); },
         [cancelBtn]: function() { $(this).dialog('close'); }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });
}

function dispatchMessage(message)
{
   let jMessage = JSON.parse(message);
   let event = jMessage.event;

   if (currentPage == "vdr") {
      if (event == 'actual') {
         let actual = jMessage.object;
         // console.log("VDR: Got", event, JSON.stringify(actual, undefined, 4));
         const actualStart = new Date(actual.present.starttime * 1000)
         $('#vdrChannel').html(actual.channel.channelname);
         $('#vdrStartTime').html(actualStart.toTimeLocal());
         $('#vdrTitle').html(actual.present.title);
         $('#vdrShorttext').html(actual.present.shorttext);
      }
      return ;
   }

   // if (event != "chartdata" && event != "ping")
   //    console.log("got event: " + event);

   if (event == "result") {
      hideProgressDialog();
      showInfoDialog(jMessage.object);
   }
   else if (event == "ping") {
      // console.log("Got ping");
      lastPingAt = new Date().getTime();
   }
   else if (event == "init") {
      // console.log("init " + JSON.stringify(jMessage.object, undefined, 4));
      allSensors = jMessage.object;
      if (currentPage == 'dashboard')
         initDashboard();
      else if (currentPage == 'schema')
         updateSchema();
      else if (currentPage == 'list')
         initList();
   }
   else if (event == "update" || event == "all") {
      // console.log("update " + JSON.stringify(jMessage.object, undefined, 4));
      if (event == "all") {
         allSensors = jMessage.object;
      }
      else {
         for (let key in jMessage.object)
            allSensors[key] = jMessage.object[key];
      }
      if (currentPage == 'dashboard')
         updateDashboard(jMessage.object, event == "all");
      else if (currentPage == 'schema')
         updateSchema();
      else if (currentPage == 'list')
         updateList();
      else if (currentPage == 'iosetup')
         updateIoSetupValue();
   }
   else if (event == "schema") {
      initSchema(jMessage.object);
      // console.log("schema " + JSON.stringify(jMessage.object, undefined, 4));
   }
   else if (event == "chartbookmarks") {
      chartBookmarks = jMessage.object;
      updateChartBookmarks();
   }
   else if (event == "commands") {
      commands = jMessage.object;
      // console.log("commands " + JSON.stringify(commands, undefined, 4));
   }
   else if (event == "config") {
      config = jMessage.object;
      // console.log("config " + JSON.stringify(config, undefined, 4));
      if (config.background != '') {
         $(document.body).css('background', 'transparent url(' + config.background + ') no-repeat 50% 0 fixed');
         $(document.body).css('background-size', 'cover');
      }
      changeFavicon(config.instanceIcon);
      prepareMenu();
      if (currentPage == 'setup')
         initConfig();
   }
   else if (event == "configdetails" && currentPage == 'setup') {
      initConfig(jMessage.object)
   }
   else if (event == "userdetails" && currentPage == 'userdetails') {
      initUserConfig(jMessage.object);
   }
   else if (event == "daemonstate") {
      daemonState = jMessage.object;
		let now = new Date();
		daemonState.timeOffset = Math.round(now/1000 - daemonState.systime);
		// console.log("timeOffset", daemonState.timeOffset, "s");
		updateFooter();
   }
   else if (event == "widgettypes") {
      widgetTypes = jMessage.object;
   }
   else if (event == "syslogs") {
      syslogs = jMessage.object;
   }
   else if (event == "syslog") {
      showSyslog(jMessage.object);
   }
   else if (event == "database") {
      showDatabaseStatistic(jMessage.object);
      hideProgressDialog();
   }
   else if (event == "wifis") {
      pingTimeoutMs = 10000;
      wifis = jMessage.object;
      showWifiList();
      hideProgressDialog();
   }
   else if (event == "token") {
      localStorage.setItem(storagePrefix + 'Token', jMessage.object.value);
      localStorage.setItem(storagePrefix + 'User', jMessage.object.user);
      localStorage.setItem(storagePrefix + 'Rights', jMessage.object.rights);

      if (jMessage.object.state == "confirm") {
         window.location.replace("index.html");
      }
      else {
         currentPage = 'login';
         initLogin();
         if (document.getElementById("confirm"))
            document.getElementById("confirm").innerHTML = "<div class=\"infoError\"><b><center>Login fehlgeschlagen</center></b></div>";
      }
   }
   else if (event == "dashboards") {
      dashboards = jMessage.object;
      // console.log("dashboards " + JSON.stringify(dashboards, undefined, 4));
   }

   else if (event == "valuetypes") {
      valueTypes = jMessage.object;
      // console.log("valueTypes " + JSON.stringify(valueTypes, undefined, 4));
   }

   else if (event == "valuefacts") {
      valueFacts = jMessage.object;
      // console.log("valueFacts " + JSON.stringify(valueFacts, undefined, 4));

      if (currentPage == "iosetup")
         initIoSetup();
   }
   else if (event == "images") {
      images = jMessage.object;
      if (currentPage == 'images')
         initImages();
      // console.log("images " + JSON.stringify(images, undefined, 4));
   }
   else if (event == "chartdata") {
      hideProgressDialog();
      let id = jMessage.object.id;

      if (currentPage == 'chart') {                                 // the charts page
         drawCharts(jMessage.object);
      }
      else if (currentPage == 'dashboard' && id == "chartwidget") { // the dashboard widget
         drawChartWidget(jMessage.object);
      }
      else if (currentPage == 'dashboard' && id == "chartwidgetbar") { // the dashboard bar chart widget
         drawBarChartWidget(jMessage.object);
      }
      else if (currentPage == 'dashboard' && id == "chartdialog") { // the dashboard chart dialog
         drawChartDialog(jMessage.object);
      }
   }
   else if (event == "groups") {
      initGroupSetup(jMessage.object);
   }
   else if (event == "system-services") {
      // console.log("systemServices", jMessage.object);
      hideProgressDialog();
      systemServices = jMessage.object;
      showSystemServicesList();
   }
   else if (event == "grouplist") {
      grouplist = jMessage.object;
   }
   else if (event == "alerts") {
      initAlerts(jMessage.object);
   }
   else if (event == "lmcdata") {
      lmcData = jMessage.object;
      if (currentPage == 'lmc')
         updateLmc();
   }
   else if (event == "lmcmenu") {
      lmcMenu(jMessage.object);
   }
   else if (event == "s3200-state") {
      s3200State = jMessage.object;
   }
   else if (event == "errors") {
      initErrors(jMessage.object);
   }
   else if (event == "menu") {
      initMenu(jMessage.object);
      hideProgressDialog();
   }
   else if (event == "pareditrequest") {
      editMenuParameter(jMessage.object);
   }
   else if (event == "pellets") {
      hideProgressDialog();
      initPellets(jMessage.object);
   }
   else if (event == "ready") {
      if (startPage) {
         mainMenuSel(startPage);
         startPage = null;
      }
   }

   // console.log("event: " + event + " dispatched");
}

function addSetupMenuButton(title, page, action = null)
{
   $("#setupMenu")
      .append($('<button></button>')
              .addClass('rounded-border button2')
              .html(title)
              .click(function() { mainMenuSel(page, action); }));

}

function addMainMenuButton(title, page, condition = true)
{
   if (!condition)
      return;

   $("#mainMenu")
      .append($('<button></button>')
              .attr('id', 'mainmenu_' + page)
              .addClass('rounded-border tabButton ' + (page == currentPage ? 'active' : ''))
              .html(title)
              .click(function() { mainMenuSel(page); }));
}

function prepareMenu()
{
   if (kioskMode == 1) {
      $('#menu').css('height', '0px');
      $('#menu').css('padding', '0px');
      $('#dashboardMenu').css('margin', '2px');
      return ;
   }

   document.title = config.instanceName;
   console.log("---- prepareMenu: " + currentPage);

   $("#navMenu").empty()
      .append($('<div></div>')
              .attr('id', 'mainMenu'));

   addMainMenuButton('Dash', 'dashboard', true);
   addMainMenuButton('List', 'list', config.showList == '1');
   addMainMenuButton('Charts', 'chart');
   addMainMenuButton('Schema', 'schema', config.schema);
   addMainMenuButton('Music', 'lmc', config.lmcHost != '');
   addMainMenuButton('VDR', 'vdr', config.vdr != '');
   addMainMenuButton('Service Menü', 'menu');
   addMainMenuButton('Fehler', 'errors');
   addMainMenuButton('Pellets', 'pellets', localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x02);
   addMainMenuButton('Setup', 'setup', localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10);

   // burger menu

   $("#mainMenu")
      .append($('<div></div>')
              .css('display', 'flex')
              .css('float', 'right')
              .append($('<button></button>')
                      .attr('id', 'burgerMenu')
                      .addClass('rounded-border button1 burgerMenu')
                      .click(function() { openMenuBurger(); } )
                      .append($('<div></div>'))
                      .append($('<div></div>'))
                      .append($('<div></div>')))
              .append($('<button></button>')
                      .attr('id', 'toggleMsgBtn')
                      .addClass('rounded-border tool-button-svg')
                      .attr('title', 'view messages')
                      .css('background-color', 'transparent')
                      .css('width', '22px')
                      .html('<svg><use xlink:href="#angle-down"></use></svg>')
                      .click(function() { toggleInfoDialog(); } )));
}

function prepareSetupMenu()
{
   // sub menu for setup

   $('#confirmDiv').remove();
   $('#setupMenu').remove();

   if (['setup', 'iosetup', 'userdetails', 'groups', 'alerts', 'syslog', 'system', 'images', 'commands'].includes(currentPage)) {
      if (localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10) {

         $("#navMenu").append($('<div></div>')
                              .attr('id', 'setupMenu')
                              .addClass('setupMenu'));

         addSetupMenuButton('Konfiguration', 'setup');
         addSetupMenuButton('IO Setup', 'iosetup');
         addSetupMenuButton('User', 'userdetails');
         addSetupMenuButton('Alerts', 'alerts');
         addSetupMenuButton('Baugruppen', 'groups');
         addSetupMenuButton('Syslog', 'syslog');
         addSetupMenuButton('Database', 'system', 'database');
         addSetupMenuButton('Commands', 'commands');
         addSetupMenuButton('Wifi', 'system', 'wifis');
         addSetupMenuButton('System Services', 'system', 'system-services');
      }
   }
}

function updateFooter()
{
   $("#version").html(daemonState.version);
}

function openMenuBurger()
{
   let haveToken = localStorage.getItem(storagePrefix + 'Token') && localStorage.getItem(storagePrefix + 'Token') != "";
   let form = '<div id="burgerPopup" style="padding:0px;">';

   if (haveToken)
      form += '<button style="width:120px;margin:2px;" class="rounded-border button1" onclick="mainMenuSel(\'user\')">[' + localStorage.getItem(storagePrefix + 'User') + ']</button>';
   else
      form += '<button style="width:120px;margin:2px;" class="rounded-border button1" onclick="mainMenuSel(\'login\')">Login</button>';

   if (currentPage == 'dashboard' && localStorage.getItem(storagePrefix + 'Rights') & 0x10)
      form += '  <br/><div><button style="width:120px;margin:2px;color" class="rounded-border button1 mdi mdi-lead-pencil" onclick=" setupDashboard()">' + (setupMode ? ' Stop Setup' : ' Setup Dashboard') + '</button></div>';

   if (currentPage == 'schema' && (localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10))
      form += '  <br/><div><button style="width:120px;margin:2px;color" class="rounded-border button1 mdi mdi-lead-pencil" onclick=" setupSchema()">' + (schemaEditActive ? ' Stop Setup' : ' Setup Schema') + '</button></div>';

   form += '</div>';

   $(form).dialog({
      position: { my: "right top", at: "right bottom", of: document.getElementById("burgerMenu") },
      minHeight: "auto",
      width: "auto",
      dialogClass: "no-titlebar rounded-border",
		modal: true,
      resizable: false,
		closeOnEscape: true,
      hide: "fade",
      open: function(event, ui) {
         $(event.target).parent().css('background-color', 'var(--light1)');
         $('.ui-widget-overlay').bind('click', function()
                                      { $("#burgerPopup").dialog('close'); });
      },
      close: function() {
         $("body").off("click");
         $(this).dialog('destroy').remove();
      }
   });
}

function mainMenuSel(what, action = null)
{
   $("#burgerPopup").dialog("close");

   let lastPage = currentPage;
   currentPage = what;
   hideAllContainer();
   schemaEditActive = false;

   prepareSetupMenu();

   console.log("---- switch to " + currentPage);

   if (document.getElementById('mainMenu')) {
      let children = document.getElementById('mainMenu').children;

      for (let i = 0; i < children.length; i++)
         children[i].className = children[i].className.replace(" active", "");

      $('#mainmenu_' + currentPage).addClass('active');
   }

   if (currentPage != lastPage && (currentPage == "vdr" || lastPage == "vdr")) {
      console.log("closing socket " + socket.protocol);
      socket.close();
      socket = null;         // delete socket;

      let protocol = myProtocol;
      let url = "ws://" + location.hostname + ":" + location.port;

      if (currentPage == "vdr") {
         protocol = "osd2vdr";
         url = "ws://" + config.vdr;
      }

      console.log("connecting ", url);
      connectWebSocket(url, protocol);
   }

   if (currentPage != "vdr")
      socket.send({ "event" : "pagechange", "object" : { "page"  : currentPage }});

   let event = null;
   let jsonRequest = {};

   if (currentPage == "setup")
      event = "setup";
   else if (currentPage == "iosetup") {
      showProgressDialog();
      socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'valuefacts' } });
   }
   else if (currentPage == "commands")
      initCommands();
   else if (currentPage == "user")
      initUser();
   else if (currentPage == "userdetails")
      event = "userdetails";
   else if (currentPage == "images")
      initImages();
   else if (currentPage == "syslog") {
		updateSyslog();
		return;
   }
   else if (currentPage == "system") {
      event = "system";
      showProgressDialog();
      if (action == "wifis")
         pingTimeoutMs = 20000;
   }
   else if (currentPage == "list")
      initList();
   else if (currentPage == "dashboard")
      initDashboard();
   else if (currentPage == "vdr")
      initVdr();
   else if (currentPage == "lmc")
      initLmc();
   else if (currentPage == "chart") {
      event = "chartdata";
      // console.log("config.chartSensors: " + config.chartSensors);
      // console.log("1 config.chartRange " + config.chartRange.replace(',', '.') + " :" + parseFloat(config.chartRange.replace(',', '.')));
      if (!theChartRange) {
         theChartRange = parseFloat(config.chartRange.replace(',', '.'));
         theChartStart = new Date().subHours(theChartRange * 24);
      }
      else if (!theChartStart)
         theChartStart = new Date().subHours(theChartRange * 24);

      prepareChartRequest(jsonRequest, config.chartSensors, theChartStart, theChartRange, "chart");
      currentRequest = jsonRequest;
   }
   else if (currentPage == "groups")
      event = "groups";
   else if (currentPage == "alerts")
      event = "alerts";
   else if (currentPage == "schema")
      event = "schema";
   else if (currentPage == "menu")
      event = "menu";
   else if (currentPage == "errors")
      event = "errors";
   else if (currentPage == "pellets") {
      showProgressDialog();
      event = "pellets";
   }

   if (currentPage != 'vdr' && currentPage != 'iosetup') {
      if (!isObjectEmpty(jsonRequest) && event != null)
         socket.send({ 'event' : event, 'object' : jsonRequest });
      else if (event != null && action != null)
         socket.send({ 'event' : event, 'object' : { 'action' : action } });
      else if (event != null)
         socket.send({ 'event' : event, 'object' : { } });

      if (currentPage == 'login')
         initLogin();
   }
}

function setupDashboard()
{
   $("#burgerPopup").dialog("close");
   setupMode = !setupMode;
   initDashboard();
}

function initLogin()
{
   $('#container').removeClass('hidden');

   document.getElementById("container").innerHTML =
      '<div id="loginContainer" class="rounded-border inputTableConfig">' +
      '  <table>' +
      '    <tr>' +
      '      <td>User:</td>' +
      '      <td><input id="user" class="rounded-border input" type="text" value=""></input></td>' +
      '    </tr>' +
      '    <tr>' +
      '      <td>Passwort:</td>' +
      '      <td><input id="password" class="rounded-border input" type="password" value=""></input></td>' +
      '    </tr>' +
      '  </table>' +
      '  <button class="rounded-border button1" onclick="doLogin()">Anmelden</button>' +
      '</div>';
}

function showSyslog(log)
{
   $('#controlContainer').removeClass('hidden');
   $('#container').removeClass('hidden');

   prepareSetupMenu();

   $("#controlContainer")
      .empty()
      .append($('<div></div>')
              .addClass('labelB1')
              .html('System Log'))
      .append($('<select></select>')
              .attr('id', 'selectSyslog')
              .addClass('input rounded-border')
              .css('width', '-webkit-fill-available')
              .css('width', '-moz-available')
              .css('margin-bottom', '8px')
              .on('input', function() { updateSyslog(); }))

	   .append($('<div></div>')
              .addClass('button-group-spacing'))

      .append($('<div></div>')
              .addClass('labelB1')
              .html('Filter'))
      .append($('<input></input>')
              .attr('id', 'syslogFilter')
              .attr('placeholder', 'expression...')
              .attr('type', 'search')
              .addClass('input rounded-border clearableOD')
              .css('width', '-webkit-fill-available')
              .css('width', '-moz-available')
              .css('margin-bottom', '8px')
				  .val(syslogFilter))
              // .on('input', function() { updateSyslog(); }))

	   .append($('<div></div>')
              .addClass('button-group-spacing'))

	   .append($('<div></div>')
              .append($('<button></button>')
                      .addClass('rounded-border tool-button')
                      .html('Refresh')
                      .click(function() { updateSyslog(); })));

	for (let i = 0; i < syslogs.length; ++i) {
		$('#selectSyslog').append($('<option></option>')
										  .attr('selected', syslogs[i].name == log.name)
										  .html(syslogs[i].name));
	}

   $('#container').html('<div id="syslogContainer" class="setupContainer log"></div>');
	$('#syslogContainer').css('width', 'fit-content');

   let root = document.getElementById("syslogContainer");
   root.innerHTML = log.lines.replace(/(?:\r\n|\r|\n)/g, '<br/>');
   hideProgressDialog();

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('footer') - sab - 10);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('footer') - sab - 10);
   };
}

function updateSyslog()
{
	syslogFilter = $('#syslogFilter').val();
	showProgressDialog();
	socket.send({ "event" : "syslog", "object" :
					  { 'log' : $('#selectSyslog').val(), 'filter' : syslogFilter } });
}

function showDatabaseStatistic(statistic)
{
   // console.log("DatabaseStatistic: " + JSON.stringify(statistic, undefined, 2));

   $('#container').removeClass('hidden');
   $('#container').html('<div id="systemContainer"></div>');

   prepareSetupMenu();

   let html = '<div>';

   html += '  <div class="rounded-border seperatorFold">Tabellen</div>';
   html += '  <table class="tableMultiCol">' +
      '    <thead>' +
      '     <tr style="height:30px;font-weight:bold;">' +
      '       <td style="width:20%;">Name</td>' +
      '       <td style="width:10%;">Table</td>' +
      '       <td style="width:10%;">Index</td>' +
      '       <td style="width:10%;">Rows</td>' +
      '     </tr>' +
      '    </thead>' +
      '    <tbody>';

   for (let i = 0; i < statistic.tables.length; i++) {
      let table = statistic.tables[i];

      html += '<tr style="height:39px;">';
      html += ' <td>' + table.name + '</td>';
      html += ' <td>' + table.tblsize + '</td>';
      html += ' <td>' + table.idxsize + '</td>';
      html += '<td>' + table.rows.toLocaleString() + '</td>';
      html += '</tr>';
   }

   html += '    </tbody>' +
      '  </table>';

   html += '</div>';

   $('#systemContainer').html(html)
      .addClass('setupContainer');
}

function showWifiList()
{
   // console.log("WifiList: " + JSON.stringify(wifis, undefined, 2));

   $('#container').removeClass('hidden');
   $('#container').html('<div id="systemContainer"></div>');

   prepareSetupMenu();

   let html = '<div>';

   html += '  <div class="rounded-border seperatorFold">Wifi Networks</div>';
   html += '  <table class="tableMultiCol">' +
      '    <thead>' +
      '     <tr style="height:30px;font-weight:bold;">' +
      '       <td style="width:18%;">Network</td>' +
      '       <td style="width:10%;">Rate</td>' +
      '       <td style="width:8%;">Security</td>' +
      '       <td style="width:5%;">Signal</td>' +
      '       <td style="width:5%">Bars</td>' +
      '       <td style="width:5%"></td>' +
      '     </tr>' +
      '    </thead>' +
      '    <tbody>';

   for (let i = 0; i < wifis.reachable.length; i++) {
      let wifi = wifis.reachable[i];
      let known = isWifiKnown(wifi.network);
      let rowColor = wifi.active == 'yes' ? 'green' : '';
      let signalColor = parseInt(wifi.signal) >= 50 ? 'lightgreen' : parseInt(wifi.signal) >= 20 ? 'orange' : '';
      let action = wifi.active == 'yes' ? 'Disconnect' : known ? 'Connect' : 'Setup';
      let btnColor = wifi.active == 'yes' ? 'orange' : known ? 'lightgreen' : 'yellow';

      html += '<tr style="height:28px;color:' + rowColor + ';">';
      html += ' <td>' + wifi.network + '</td>';
      html += ' <td>' + wifi.rate + '</td>';
      html += ' <td>' + wifi.security + '</td>';
      html += ' <td>' + wifi.signal + '</td>';
      html += ' <td style="font-family:monospace;color:' + signalColor + ';">' + wifi.bars + '</td>';
      html += ' <td>' + '<button class="buttonOptions rounded-border" style="color:' + btnColor + ';" onclick="wifiAction(\'' + wifi.id + '\')">' + action + '</button>' + '</td>';
      html += '</tr>';
   }

   html += '    </tbody>' +
      '  </table>';

   html += '</div>';

   $('#systemContainer').html(html)
      .addClass('setupContainer');
}

function isWifiKnown(network)
{
   for (let i = 0; i < wifis.known.length; i++) {
      if (wifis.known[i].network == network) // && wifis.known[i].device != null && wifis.known[i].device != '')
         return true;
   }
   return false;
}

function wifiAction(id)
{
   let i = 0;

   for (i = 0; i < wifis.reachable.length; i++) {
      if (wifis.reachable[i].id == id)
         break;
   }

   if (wifis.reachable[i].id != id)
      return ;

   let active = wifis.reachable[i].active == 'yes';

   function doWifiAction(password = '') {
      pingTimeoutMs = 30000;
      showProgressDialog();
      console.log('wifiAction', wifis.reachable[i].network, wifis.reachable[i].active);
      socket.send({ "event": "system", "object":
                    {
                       'action': active ? 'wifi-disconnect' : 'wifi-connect',
                       'ssid': wifis.reachable[i].network,
                       'password': password
                    }
                  });
   }

   if (!active && !isWifiKnown(wifis.reachable[i].network))
      pwdDialog(doWifiAction, "Password");
   else
      doWifiAction();
}

function showSystemServicesList()
{
   // console.log("SystemServices: " + JSON.stringify(systemServices, undefined, 2));

   $('#container').removeClass('hidden');
   $('#container').html('<div id="systemContainer"></div>');

   prepareSetupMenu();

   // systemServices.sort(function(a, b) {
   //    return b.unitFileState.localeCompare(a.unitFileState);
   // });

   let html = '<div>';

   html += '  <div class="rounded-border seperatorFold">System Services</div>';
   html += '  <table class="tableMultiCol">' +
      '    <thead>' +
      '     <tr style="height:30px;font-weight:bold;">' +
      '       <td style="width:8%;">Service</td>' +
      '       <td style="width:14%;"></td>' +
      '       <td style="width:6%;">Status</td>' +
      '       <td style="width:3%"></td>' +
      '     </tr>' +
      '    </thead>' +
      '    <tbody>';

   for (let i = 0; i < systemServices.length; i++) {
      let svc = systemServices[i];
      let rowColor = svc.status == 'active' ? 'lightgreen' : 'var(--light4)';
      // let action = svc.status == 'active' ? 'Stop' : 'Start';
      let btnColor = svc.status == 'active' ? 'orange' : 'lightgreen';

      html += '<tr style="height:28px;color:' + rowColor + ';">';
      html += ' <td>' + svc.service + '</td>';
      html += ' <td>' + svc.title + '</td>';
      html += ' <td>' + svc.status + ' / ' + svc.subState + ' / ' + svc.unitFileState + '</td>';
      html += ' <td>' + '<button id="btn_' + svc.service + '"class="rounded-border button1 burgerMenuInline" onclick="systemServiceMenu(\'' + svc.service + '\')"><div></div><div></div><div></div></button>' + '</td>';
      html += '</tr>';
   }

   html += '    </tbody>' +
      '  </table>';

   html += '</div>';

   $('#systemContainer').html(html)
      .addClass('setupContainer');
}

function systemServiceMenu(service)
{
   let i = 0;

   for (i = 0; i < systemServices.length; i++) {
      if (systemServices[i].service == service)
         break;
   }

   let active = systemServices[i].status == 'active';
   let enabled = systemServices[i].unitFileState == 'enabled';
   let form = document.createElement("div");

   $(form)
      .attr('id', 'actionPopup')
      .css('display', 'grid')
      .css('padding', '2px')
      .append($('<button></button>')
              .addClass('rounded-border button1')
              .css('width', '80px')
              .css('margin', '2px')
              .html(active ? 'Stop' : 'Start')
              .click({ "service" : service }, function(event) {
                 systemServiceAction(event.data.service, active ? 'sys-service-Stop' : 'sys-service-Start');
                 $("#actionPopup").dialog('close');
              }))
      .append($('<button></button>')
              .addClass('rounded-border button1')
              .css('width', '80px')
              .css('margin', '2px')
              .html('Kill')
              .click({ "service" : service }, function(event) {
                 systemServiceAction(event.data.service, active ? 'sys-service-Stop' : 'sys-service-Kill');
                 $("#actionPopup").dialog('close');
              }));
   if (systemServices[i].unitFileState == 'enabled' || systemServices[i].unitFileState == 'disabled')
      $(form).append($('<button></button>')
                     .addClass('rounded-border button1')
                     .css('width', '80px')
                     .css('margin', '2px')
                     .html(enabled ? 'Disable' : 'Enable')
                     .click({ "service" : service }, function(event) {
                        systemServiceAction(event.data.service, enabled ? 'sys-service-Disable' : 'sys-service-Enable');
                        $("#actionPopup").dialog('close');
                     })
                    );

   $(form).dialog({
      position: { my: "right top", at: "left top", of: document.getElementById('btn_' + service)},
      minHeight: "auto",
      width: "auto",
      dialogClass: "no-titlebar rounded-border",
		modal: true,
      resizable: false,
		closeOnEscape: true,
      hide: "fade",
      open: function(event, ui) {
         $(event.target).parent().css('background-color', 'var(--light1)');
         $('.ui-widget-overlay').bind('click', function()
                                      { $("#actionPopup").dialog('close'); });
      },
      close: function() {
         $("body").off("click");
         $(this).dialog('destroy').remove();
      }
   });
}

function systemServiceAction(service, action)
{
   socket.send({ "event": "system", "object":
                 {
                    'action': action,
                    'service': service
                 }
               });
}

window.toggleMode = function(address, type)
{
   socket.send({ "event": "togglemode", "object":
                 { "address": address,
                   "type": type }
               });
}

function toggleIo(address, type, scale = -1)
{
   socket.send({ "event": "toggleio", "object":
                 {
                    'action': scale == -1 ? 'toggle' : 'dim',
                    'value': scale,
                    'address': address,
                    'type': type }
               });
}

window.toggleIoNext = function(address, type)
{
   socket.send({ "event": "toggleionext", "object":
                 { "address": address,
                   "type": type }
               });
}

function doLogin()
{
   // console.log("login: " + $("#user").val() + " : " + $.md5($("#password").val()));
   socket.send({ "event": "gettoken", "object":
                 { "user": $("#user").val(),
                   "password": $.md5($("#password").val()) }
               });
}

function doLogout()
{
   localStorage.removeItem(storagePrefix + 'Token');
   localStorage.removeItem(storagePrefix + 'User');
   localStorage.removeItem(storagePrefix + 'Rights');
   console.log("logout");
   mainMenuSel('login');
}

function hideAllContainer()
{
   $('#dashboardMenu').addClass('hidden');
   $('#controlContainer').addClass('hidden');
   $('#container').addClass('hidden');
}

// ---------------------------------
// charts

function prepareChartRequest(jRequest, sensors, start, range, id)
{
   let gaugeSelected = false;

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

   if (gaugeSelected) {
      // console.log("requesting gauge chart for '" + start + "' range " + range);
      jRequest["start"] = 0;   // default (use today-range)
      jRequest["range"] = 1;
      jRequest["sensors"] = sensors;
   }
   else {
      // console.log("requesting chart for '" + start + "' range " + range);
      jRequest["start"] = start == 0 ? 0 : Math.floor(start.getTime()/1000);  // calc unix timestamp
      jRequest["range"] = range;
      jRequest["sensors"] = sensors;
   }
}

//***************************************************************************
// Chart Widget
//***************************************************************************

function drawChartWidget(dataObject)
{
   let root = document.getElementById("container");
   let id = "widget" + dataObject.rows[0].sensor;

   if (widgetCharts[id] != null) {
      widgetCharts[id].destroy();
      widgetCharts[id] = null;
   }

   const config = {
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
               time: {
                  unit: 'hour',
                  unitStepSize: 1,
                  displayFormats: {
                     hour: 'HH:mm',
                     day: 'HH:mm'
                  }},
               distribution: "linear",
               display: true,
               gridLines: {
                  color: "gray"
               },
               ticks: {
                  padding: 0,
                  maxTicksLimit: 6,
                  maxRotation: 0,
                  fontColor: "darkgray"
               }

            }],
            yAxes: [{
               display: true,
               gridLines: {
                  color: "darkgray",
                  zeroLineColor: 'darkgray'
               },
               ticks: {
                  padding: 5,
                  maxTicksLimit: 5,
                  fontColor: "darkgray"
               }
            }]
         }
      }
   };

   for (let i = 0; i < dataObject.rows.length; i++) {
      let dataset = {};

      // console.log("draw chart row with", dataObject.rows[i].data.length, "points");

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = "#3498db33";  // fill color
      dataset["borderColor"] = "#3498db";        // line color
      dataset["borderWidth"] = 1;
      dataset["fill"] = true;
      dataset["pointRadius"] = 1.5;
      config.data.datasets.push(dataset);
   }

   // console.log("draw chart widget to", id, $('#'+id).innerHeight());

   let canvas = document.getElementById(id);
   widgetCharts[id] = new Chart(canvas, config);
}

//***************************************************************************
// Bar Chart Widget
//***************************************************************************

function drawBarChartWidget(dataObject)
{
   // #TODO
   // Bar chart nur für monatlich implementiert,
   // hier fehlt noch die Konfiguration (widget.interval) für die Zusammenfassung auf z.B.:
   // ..., jährlich, monatlich, wöchentlich, täglich, stündlich, ...
   // nebst passendem 'group by' select seitens des daemon mit Auswahl ob avg oder max verwendet werden soll
   //  letzteres ist derzeit krude hardcoded
   // soiwie die Anpassung der Axis im chart hier

   let root = document.getElementById("container");
   let id = "widget" + dataObject.rows[0].sensor;

   if (widgetCharts[id] != null) {
      widgetCharts[id].destroy();
      widgetCharts[id] = null;
   }

   const _config = {
      type: 'bar',
      data: {
         datasets: []
      },
      options: {
         responsive: true,
         maintainAspectRatio: false,
         plugins: {
            legend: {
               position: 'top',
            },
            title: {
               display: true,
               text: 'Chart.js Bar Chart'
            }
         }
      },
   };

   const config = {
      type: "bar",
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
               time: {
                  unit: 'month',
                  unitStepSize: 1,
                  displayFormats: {
                     month: 'MMMM',
                     day: 'HH'
                  }},
               gridLines: {
                  display: false,
                  color: "gray"
               },
               ticks: {
                  padding: 0,
                  fontColor: "darkgray"
               }
            }],
            yAxes: [{
               display: true,
               gridLines: {
                  color: "darkgray",
                  zeroLineColor: 'darkgray'
               },
               ticks: {
                  padding: 5,
                  maxTicksLimit: 5,
                  fontColor: "darkgray"
               }
            }]
         }
      }
   };

   for (let i = 0; i < dataObject.rows.length; i++) {
      let dataset = {};

      console.log("draw bar chart row with", dataObject.rows[i].data.length, "points");
      console.log(dataObject.rows[i].data);
      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = "#3498db33";
      dataset["borderColor"] = "#3498db";        // line color
      dataset["borderWidth"] = 1;
      dataset["fill"] = true;
      dataset["pointRadius"] = 1.5;
      config.data.datasets.push(dataset);
   }

   let canvas = document.getElementById(id);
   widgetCharts[id] = new Chart(canvas, config);
}

//***************************************************************************
// Chart Dialog
//***************************************************************************

function drawChartDialog(dataObject)
{
   let root = document.querySelector('dialog')
   if (dataObject.rows[0].sensor != chartDialogSensor) {
      return ;
   }

   if (theChart != null) {
      theChart.destroy();
      theChart = null;
   }

   let data = {
      type: "line",
      data: {
         datasets: []
      },
      options: {
         tooltips: {
            mode: "index",
            intersect: false,
         },
         legend: {
            display: true
         },
         hover: {
            mode: "nearest",
            intersect: true
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
                     hour: 'HH:mm',
                     day: 'HH:mm',
                  }},
               distribution: "linear",
               display: true,
               ticks: {
                  maxTicksLimit: 12,
                  padding: 5,
                  maxRotation: 0,
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
                  padding: 5,
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

   let key = '';

   for (let i = 0; i < dataObject.rows.length; i++)
   {
      let dataset = {};

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = "#3498db33";  // fill color
      dataset["borderColor"] = "#3498db";      // line color
      dataset["borderWidth"] = 1;
      dataset["fill"] = true;
      dataset["label"] = dataObject.rows[i].title;
      dataset["pointRadius"] = 0;
      data.data.datasets.push(dataset);

      key = dataObject.rows[i].key;  // we have olny one row here!
   }

   data.options.scales.yAxes[0].scaleLabel.labelString = '[' + valueFacts[key].unit + ']';

   let canvas = root.querySelector("#chartDialog");
   theChart = new Chart(canvas, data);
}

function toKey(type, address)
{
   return type + ":0x" + address.toString(16).padStart(2, '0');
}

function parseBool(val)
{
   if ((typeof val === 'string' && (val.toLowerCase() === 'true' || val.toLowerCase() === 'yes' || val.toLowerCase() === 'ja')) || val === 1)
      return true;
   else if ((typeof val === 'string' && (val.toLowerCase() === 'false' || val.toLowerCase() === 'no' || val.toLowerCase() === 'nein')) || val === 0)
      return false;

   return null;
}

Number.prototype.pad = function(size)
{
  let s = String(this);
  while (s.length < (size || 2)) {s = "0" + s;}
  return s;
}

Date.prototype.toDatetimeLocal = function toDatetimeLocal()
{
   let date = this,
       ten = function (i) {
          return (i < 10 ? '0' : '') + i;
       },
       YYYY = date.getFullYear(),
       MM = ten(date.getMonth() + 1),
       DD = ten(date.getDate()),
       HH = ten(date.getHours()),
       II = ten(date.getMinutes()),
       SS = ten(date.getSeconds()),
       MS = date.getMilliseconds()
   ;

   return YYYY + '-' + MM + '-' + DD + 'T' +
      HH + ':' + II + ':' + SS; //  + ',' + MS;
};

Date.prototype.toDateLocal = function toDateLocal()
{
   let date = this,
       ten = function (i) {
          return (i < 10 ? '0' : '') + i;
       },
       YYYY = date.getFullYear(),
       MM = ten(date.getMonth() + 1),
       DD = ten(date.getDate())
   ;

   return YYYY + '-' + MM + '-' + DD;
};

Date.prototype.toTimeLocal = function toTimeLocal()
{
   let date = this,
       ten = function (i) {
          return (i < 10 ? '0' : '') + i;
       },
       HH = ten(date.getHours()),
       II = ten(date.getMinutes()),
       SS = ten(date.getSeconds())
   ;

   return HH + ':' + II; //  + ':' + SS;
};


function fileExist(url)
{
   let xhr = new XMLHttpRequest();
   xhr.open('HEAD', url, false);
   xhr.send();
   return xhr.status != "404";
}

function getTotalHeightOf(id)
{
   return $('#' + id).outerHeight();
}

function lNow()
{
   let currentDateTime = new Date();
   return currentDateTime.getTime() / 1000;
}

function changeFavicon(src)
{
   src = 'img/' + src;
   $('link[rel="shortcut icon"]').attr('href', src)
   $('link[rel="icon"]').attr('href', src)
   $('link[rel="apple-touch-icon-precomposed"]').attr('href', src)
}

function isObjectEmpty(object)
{
   return Object.keys(object).length === 0 && object.constructor === Object;
}

function trim()
{
   return this.replace(/^\s\s*/, '').replace(/\s\s*$/, '');
}

function alterColor(rgb, type, percent)
{
	rgb = rgb.replace('rgb(', '').replace(')', '').split(',');

	let red = trim.call(rgb[0]);
	let green = trim.call(rgb[1]);
	let blue = trim.call(rgb[2]);

	if (type === "darken") {
		red = parseInt(red * (100 - percent) / 100, 10);
		green = parseInt(green * (100 - percent) / 100, 10);
		blue = parseInt(blue * (100 - percent) / 100, 10);
	}
   else {
		red = parseInt(red * (100 + percent) / 100, 10);
		green = parseInt(green * (100 + percent) / 100, 10);
		blue = parseInt(blue * (100 + percent) / 100, 10);
	}

	return 'rgb(' + red + ', ' + green + ', ' + blue + ')';
}
