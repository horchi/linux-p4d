/*
 *  dashboard.js
 *
 *  (c) 2020-2024 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

// var layout = 'grid';
var layout = 'flex';

var weatherData = null;
var weatherInterval = null;
var moseDownOn = { 'object' : null };
var lightClickTimeout = null;
var lightClickPosX = null;
var lightClickPosY = null;
var dashboardGauges = {};

const symbolColorDefault = '#ffffff';
const symbolOnColorDefault = '#059eeb';

function initDashboard(update = false)
{
   // console.log("initDashboard " + JSON.stringify(allSensors, undefined, 4));

   if (allSensors == null) {
      console.log("Fatal: Missing widgets!");
      return;
   }

	for (const gkey in dashboardGauges) {
		if (dashboardGauges.hasOwnProperty(gkey)) {
			dashboardGauges[gkey].destroy();
		}
	}

	dashboardGauges = {};

   if (!update) {
      $('#container').removeClass('hidden');
      $('#dashboardMenu').removeClass('hidden');
      $('#dashboardMenu').empty();
   }

   if (!Object.keys(dashboards).length)
      setupMode = true;

   if (setupMode) {
      $('#dashboardMenu').append($('<button></button>')
                                 .addClass('rounded-border buttonDashboard buttonDashboardTool')
                                 .addClass('mdi mdi-close-outline')
                                 .attr('title', 'Setup beenden')
                                 .click(function() { setupDashboard(); })
                                );
   }

   let jDashboards = [];
   for (let did in dashboards) {
      if (!dashboardGroup || dashboards[did].group == dashboardGroup)
         jDashboards.push([dashboards[did].order, did]);
   }
   jDashboards.sort();

   for (let i = 0; i < jDashboards.length; i++) {
      let did = jDashboards[i][1];
      if (actDashboard < 0)
         actDashboard = did;

      if (kioskBackTime > 0 && actDashboard != jDashboards[0][1]) {
         setTimeout(function() {
            actDashboard = jDashboards[0][1];
            initDashboard(false);
         }, kioskBackTime * 1000);
      }

      let classes = dashboards[did].symbol != '' ? dashboards[did].symbol.replace(':', ' ') : '';

      if (dashboards[actDashboard] == dashboards[did])
         classes += ' buttonDashboardActive';

      $('#dashboardMenu').append($('<button></button>')
                                 .addClass('rounded-border buttonDashboard widgetDropZone')
                                 .addClass(classes)
                                 .attr('id', 'dash' + did)
                                 .attr('draggable', setupMode)
                                 .attr('data-droppoint', setupMode)
                                 .attr('data-dragtype', 'dashboard')
                                 .on('dragstart', function(event) {dragDashboard(event)})
                                 .on('dragover', function(event) {dragOverDashboard(event)})
                                 .on('dragleave', function(event) {dragLeaveDashboard(event)})
                                 .on('drop', function(event) {dropDashboard(event)})
                                 .html(dashboards[did].title)
                                 .click({"id" : did}, function(event) {
                                    actDashboard = event.data.id;
                                    console.log("Activate dashboard " + actDashboard);
                                    initDashboard();
                                 }));

      if (kioskMode) {
         $('#dash'+did).css('font-size', '-webkit-xxx-large');
         $('#dash'+did).css('font-size', 'xxx-large');
      }

      if (setupMode && did == actDashboard)
         $('#dashboardMenu').append($('<button></button>')
                                    .addClass('rounded-border buttonDashboard buttonDashboardTool')
                                    .addClass('mdi mdi-lead-pencil')  //  mdi-draw-pen
                                    .click({"id" : 'dash'+did}, function(event) { dashboardSetup(event.data.id); })
                                   );
   }

   // additional elements in setup mode

   if (setupMode) {
      $('#dashboardMenu')
         .append($('<div></div>')
                 .css('width', '-webkit-fill-available')
                 .css('width', '-moz-available'))
         .append($('<div></div>')
                 .css('display', 'flex')
                 .css('float', 'right')
                 .append($('<input></input>')
                         .addClass('rounded-border input')
                         .css('margin', '15px,15px,15px,0px')
                         .css('align-self', 'center')
                         .attr('id', 'newDashboard')
                         .attr('title', 'Neues Dashboard')
                        )
                 .append($('<button></button>')
                         .addClass('rounded-border buttonDashboard buttonDashboardTool')
                         .addClass('mdi mdi-file-plus-outline')
                         .attr('title', 'dashboard hinzufügen')
                         .click(function() { newDashboard(); })
                        )
                 .append($('<button></button>')
                         .addClass('rounded-border buttonDashboard buttonDashboardTool')
                         .addClass('mdi mdi-shape-plus')
                         .attr('title', 'widgets zum dashboard hinzufügen')
                         .css('font-size', 'x-large')
                         .click(function() { addWidget(); })
                        )
                );
   }

   // widgets

   document.getElementById("container").innerHTML = '<div id="widgetContainer" class="widgetContainer"></div>';

   if (layout == 'flex') {
      $('#widgetContainer').css('display', 'flex');
      $('#widgetContainer').css('flex-wrap', 'wrap');
   }
   else {
      $('#widgetContainer').css('display', 'grid');
      $('#widgetContainer').css('gap', '0px');
   }

   if (dashboards[actDashboard] != null) {
      for (let key in dashboards[actDashboard].widgets) {
         initWidget(key, dashboards[actDashboard].widgets[key]);
         updateWidget(allSensors[key], true, dashboards[actDashboard].widgets[key]);
      }
   }

   initLightColorDialog();

   resizeDashboard();
   window.onresize = function() { resizeDashboard(); };
}

function resizeDashboard()
{
   let space = 20;

   if (getTotalHeightOf('menu') == 0)
      space = 10;

   $("#container").height($(window).outerHeight() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - getTotalHeightOf('footer') - sab - space);

   if (layout != 'flex') {
      let style = getComputedStyle(document.documentElement);
      let widgetWidthBase = style.getPropertyValue('--widgetWidthBase');
      let widgetHeightBase = style.getPropertyValue('--widgetHeightBase');

      let colCount = parseInt($('#container').innerWidth() / widgetWidthBase);
      let colWidthPercent = parseInt(100 / colCount);
      let rep = 'repeat(' + colCount + ',' + colWidthPercent + '%)';

      $('#widgetContainer').css('grid-template-columns', rep);
      $('#widgetContainer').css('grid-auto-rows', widgetHeightBase + 'px');
   }
}

function newDashboard()
{
   if (!$('#newDashboard').length)
      return;

   let name = $('#newDashboard').val();
   $('#newDashboard').val('');

   if (name != '') {
      console.log("new dashboard: " + name);
      socket.send({ "event" : "storedashboards", "object" : { [-1] : { 'title' : name, 'widgets' : {} } } });
      socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
   }
}

function getWidgetColor(widget, colorCondition, defaultColor, value)
{
   if (colorCondition == null || colorCondition == '') {
		if (defaultColor != null)
			return defaultColor;
		return 'white';
	}
	else {
		let conditions = colorCondition.split(",");
      for (c = 0; c < conditions.length; ++c) {
         if (conditions[c].indexOf('=') != -1) {
            let [val,col] = conditions[c].split("=");
            if (value == val)
               return col;
         }
         else if (conditions[c].indexOf('~') != -1) {   // ~ für string enthalten in
            let [val,col] = conditions[c].split("~");
            if (value.indexOf(val) != -1)
               return col;
         }
         else if (conditions[c].indexOf('<') != -1) {1
            let [val,col] = conditions[c].split("<");
            if (val < value)
               return col;
         }
         else if (conditions[c].indexOf('>') != -1) {
            let [val,col] = conditions[c].split(">");
            if (val > value)
               return col;
         }
         else if (conditions[c].indexOf('<=') != -1) {1
            let [val,col] = conditions[c].split("<=");
            if (val <= value)
               return col;
         }
         else if (conditions[c].indexOf('>=') != -1) {
            let [val,col] = conditions[c].split(">=");
            if (val >= value)
               return col;
         }
         else if (conditions[c].indexOf('<>') != -1) {
            let [val,col] = conditions[c].split("<>");
            if (val != value)
               return col;
         }
      }
   }
}

function getWidgetTitle(widget, fact)
{
   let title = ' ';

   if (widget.title && widget.title != '')
      title += widget.title;
   else if (fact && fact.usrtitle && fact.usrtitle != '')
      title += fact.usrtitle;
   else if (fact)
      title += fact.title;

   return title;
}

function getWidgetTitleClass(widget, fact)
{
   let titleClass = '';
   let title = '';

   if (fact != null)
      title = (fact.usrtitle != null ? fact.usrtitle : fact.title).toUpperCase();

   // #TODO move to configuration

   if (!setupMode && widget.unit == '°C' && title.indexOf('BOILER') != -1)
      titleClass = 'mdi mdi-shower-head';
   else if (!setupMode && widget.unit == '°C')
      titleClass = 'mdi mdi-thermometer';
   else if (!setupMode && (widget.unit == 'hPa' || widget.unit == 'A' || widget.unit == 'mA' ||
                           widget.unit == 'W' || widget.unit == 'V' || widget.unit == 'Ah'))
      titleClass = 'mdi mdi-gauge';
   else if (!setupMode && widget.unit == '%' && title.indexOf('FEUCHT') != -1)
      titleClass = 'mdi mdi-water-percent';
   else if (!setupMode && title.indexOf('GAS') != -1)
      titleClass = 'mdi mdi-gas-cylinder';
   else if (!setupMode && widget.unit == '%')
      titleClass = 'mdi mdi-label-percent-outline';
   else if (!setupMode && widget.unit == 'l')
      titleClass = 'mdi mdi-water';

   return titleClass;
}

//***************************************************************************
// Init a Widget
//***************************************************************************

function initWidget(key, widget, fact)
{
   if (key == null || key == '')
      return ;

   if (fact == null)
      fact = valueFacts[key];

   if (fact == null && widget.widgettype != 10 && widget.widgettype != 11) {
      console.log("Fact '" + key + "' not found, ignoring");
      return;
   }

   if (widget == null) {
      console.log("Missing widget for '" + key + "'  ignoring");
      return;
   }

   // console.log("Widget " + key + ': '+ JSON.stringify(widget));
   // console.log("initWidget " + key + " : " + (fact ? fact.name : ''));
   // console.log("fact: " + JSON.stringify(fact, undefined, 4));
   // console.log("widget: " + JSON.stringify(widget, undefined, 4));

   if (fact && widget.unit == '')
      widget.unit = fact.unit;

   const marginWidth = 3 *2;
	const useKioskHeight = kioskMode == 1 || kioskMode == 2;
   let root = document.getElementById("widgetContainer");
   let id = 'div_' + key;
   let elem = document.getElementById(id);

   if (elem == null) {

      elem = document.createElement("div");
      root.appendChild(elem);
      elem.setAttribute('id', id);

      if (layout == 'flex') {
         let style = getComputedStyle(document.documentElement);
         let widgetHeightBase = style.getPropertyValue('--widgetHeightBase') * 2;

         eHeight = widgetHeightBase;

         // verschiedene heightfactor für verscheidene dashboards und / kiosk mode

         if (heightFactor && heightFactor != 1)
            eHeight = widgetHeightBase * heightFactor;
         else if (!useKioskHeight && dashboards[actDashboard].options && dashboards[actDashboard].options.heightfactor)
            eHeight = widgetHeightBase * dashboards[actDashboard].options.heightfactor;
         else if (useKioskHeight && dashboards[actDashboard].options && dashboards[actDashboard].options.heightfactorKiosk) {
            eHeight = widgetHeightBase * dashboards[actDashboard].options.heightfactorKiosk;
            // console.log("eHeight", eHeight, "dashboards[actDashboard].options.heightfactorKiosk", dashboards[actDashboard].options.heightfactorKiosk);
         }

         // widget.heightfactor für widget spezifische Höhen -> to be implemented (nur vorbereitet)

         if (widget.heightfactor != null && widget.heightfactor != 1)
            eHeight = eHeight * widget.heightfactor + ((widget.heightfactor-1) * marginWidth);

         elem.style.height = eHeight + 'px';
      }
      else  // grid layout
      {
         if (widget.widthfactor != null)
            $(elem).css('grid-column', 'span ' + widget.widthfactor * 2);
         else
            $(elem).css('grid-column', 'span 2');

         if (widget.heightfactor != null)
            $(elem).css('grid-row', 'span ' + widget.heightfactor * 2);
         else
            $(elem).css('grid-row', 'span 2');
      }
   }

   elem.innerHTML = '';

   if (layout == 'flex')
   {
      let style = getComputedStyle(document.documentElement);
      let widgetWidthBase = style.getPropertyValue('--widgetWidthBase') * 2;
      let widthFactor = widget.widthfactor != null ? widget.widthfactor : 1;

      if (useKioskHeight && dashboards[actDashboard].options && dashboards[actDashboard].options.widthfactorKiosk)
         widthFactor *= dashboards[actDashboard].options.widthfactorKiosk;

      if (widthFactor != 1)
         elem.style.width = widgetWidthBase * widthFactor + ((widthFactor-1) * marginWidth) + 'px'; // adjust margin not the formula
      else
         elem.style.width = widgetWidthBase + 'px';

      // console.log("widget", key, "width", elem.style.width, "widgetWidthBase", widgetWidthBase, "widthFactor", widthFactor);
   }

   if (setupMode && (widget.widgettype < 900 || widget.widgettype == null)) {
      elem.setAttribute('draggable', true);
      elem.dataset.droppoint = true;
      elem.dataset.dragtype = 'widget';
      elem.addEventListener('dragstart', function(event) {dragWidget(event)}, false);
      elem.addEventListener('dragover', function(event) {dragOver(event)}, false);
      elem.addEventListener('dragleave', function(event) {dragLeave(event)}, false);
      elem.addEventListener('drop', function(event) {dropWidget(event)}, false);
   }

   let titleClass = getWidgetTitleClass(widget, fact);
   let title = getWidgetTitle(widget, fact);

   if (!widget.color)
      widget.color = 'white';

   $(document).on({'mouseup touchend' : function(e){
      if (!moseDownOn.object)
         return;
      e.preventDefault();
      let scale = parseInt((e.pageX - $(moseDownOn.div).position().left) / ($(moseDownOn.div).innerWidth() / 100));
      if (scale > 100) scale = 100;
      if (scale < 0) scale = 0;
      console.log("dim: " + scale + '%');
      toggleIo(moseDownOn.fact.address, moseDownOn.fact.type, scale);
      moseDownOn = { 'object' : null };
   }});

   $(document).on({'mousemove touchmove' : function(e){
      if (!moseDownOn.object)
         return;
      e.preventDefault();
      let scale = parseInt((e.pageX - $(moseDownOn.div).position().left) / ($(moseDownOn.div).innerWidth() / 100));
      if (scale > 100) scale = 100;
      if (scale < 0) scale = 0;
      $(moseDownOn.object).css('width', scale + '%');
   }})

   switch (widget.widgettype)
   {
      case 0: {          // Symbol
         $(elem)
            .addClass("widget widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .click(function(event) { console.log("click"); titleClick(event.ctrlKey, key); })
                    .css('user-select', 'none')
                    .html(title))
            .append($('<button></button>')
                    .attr('id', 'button' + fact.type + fact.address)
                    .attr('type', 'button')
                    .css('color', widget.color)
                    .css('user-select', 'none')
                    .css('border-radius', '100%')
                    .css('width', '100%')
                    .css('height', '100%')
                    .addClass('rounded-border')
                    .addClass('widget-main')
                    .append($('<img></img>')
                            .attr('id', 'widget' + fact.type + fact.address)
                            .attr('draggable', false)
                            .css('user-select', 'none'))
                   );

         if (!setupMode) {
            $('#button' + fact.type + fact.address)
               .on('mousedown touchstart', {"fact" : fact}, function(e) {
                  if (e.touches != null) {
                     lightClickPosX = e.touches[0].pageX;
                     lightClickPosY = e.touches[0].pageY;
                  }
                  else
                  {
                     e.preventDefault();
                     lightClickPosX = e.clientX;
                     lightClickPosY = e.clientY;
                  }
                  // e.preventDefault();
                  if ((e.which != 0 && e.which != 1) || $('#lightColorDiv').css('display') != 'none')
                     return;
                  if (fact.options & 0x04) {

                     lightClickTimeout = setTimeout(function(event) {
                        lightClickTimeout = lightClickPosX = lightClickPosY = null;
                        showLightColorDialog(key);
                     }, 400);
                  }
               })
               .on('mouseup mouseleave touchend', {"fact" : fact}, function(e) {
                  if ($('#lightColorDiv').css('display') != 'none')
                     return;
                  if (e.changedTouches != null) {
                     posX = e.changedTouches[0].pageX;
                     posY = e.changedTouches[0].pageY;
                  }
                  else
                  {
                     posX = e.clientX;
                     posY = e.clientY;
                  }
                  if (lightClickPosX != null || lightClickPosY != null) {
                     if (Math.abs(lightClickPosX - posX) < 15 && Math.abs(lightClickPosY - posY) < 15) {
                        e.preventDefault();
                        e.stopPropagation();
                        toggleIo(fact.address, fact.type);
                     }
                  }
                  if (lightClickTimeout)
                     clearTimeout(lightClickTimeout);
                  lightClickTimeout = lightClickPosX = lightClickPosY = null;
               });
         }

         break;
      }

      case 1:         // Chart
      case 13: {
         elem.className = "widgetChart widgetDropZone";
         let eTitle = document.createElement("div");
         let cls = setupMode ? 'mdi mdi-lead-pencil widget-edit' : '';
         eTitle.className = "widget-title " + cls + ' ' + titleClass;
         eTitle.innerHTML = title;
         eTitle.addEventListener("click", function(event) {titleClick(event.ctrlKey, key)}, false);
         elem.appendChild(eTitle);

         let ePeak = document.createElement("div");
         ePeak.setAttribute('id', 'peak' + fact.type + fact.address);
         ePeak.className = "chart-peak";
         elem.appendChild(ePeak);

         let eValue = document.createElement("div");
         eValue.setAttribute('id', 'value' + fact.type + fact.address);
         eValue.className = "chart-value";
         eValue.style.color = widget.color;
         elem.appendChild(eValue);

         let eChart = document.createElement("div");
         eChart.className = "chart-canvas-container";
         let cFact = fact;
         if (!setupMode && fact.record) {
            eChart.setAttribute("onclick", "toggleChartDialog('" + cFact.type + "'," + cFact.address + ")");
				$(eChart).css('cursor', 'pointer');
			}
         elem.appendChild(eChart);

         let eCanvas = document.createElement("canvas");
         eCanvas.setAttribute('id', 'widget' + fact.type + fact.address);
         eCanvas.className = "chart-canvas";
         eChart.appendChild(eCanvas);
         break;
      }

      case 3: {        // type 3 (Value)
         $(elem)
            .addClass("widgetValue widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .css('user-select', 'none')
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .html(title))
            .append($('<div></div>')
                    .attr('id', 'widget' + fact.type + fact.address)
                    .addClass('widget-value')
                    .css('user-select', 'none')
                    .css('color', widget.color)
						  .css('cursor', !setupMode && fact.record ? 'pointer' : '')
                    .click(function() {
                       let cFact = fact;
                       if (!setupMode && fact.record)
                          toggleChartDialog(cFact.type, cFact.address, key);}))
            .append($('<div></div>')
                    .attr('id', 'peak' + fact.type + fact.address)
                    .css('user-select', 'none')
                    .addClass('chart-peak'));

         break;
      }
      case 4: {        // Gauge
         $(elem)
            .addClass("widgetGauge participation widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .css('user-select', 'none')
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .html(title))
            .append($('<div></div>')
                    .attr('id', 'svgDiv' + fact.type + fact.address)
                    .addClass('widget-main-gauge')
                    .css('user-select', 'none')
                    .append($('<svg></svg>')
                            .attr('viewBox', '0 0 1000 600')
                            .attr('preserveAspectRatio', 'xMidYMin slice')
                            .append($('<path></path>')
                                    .attr('id', 'pb' + fact.type + fact.address))
                            .append($('<path></path>')
                                    .attr('id', 'pv' + fact.type + fact.address)
                                    .addClass('data-arc'))
                            .append($('<path></path>')
                                    .attr('id', 'pp' + fact.type + fact.address)
                                    .addClass('data-peak'))
                            .append($('<text></text>')
                                    .attr('id', 'value' + fact.type + fact.address)
                                    .addClass('gauge-value')
                                    .attr('font-size', "140")
                                    .attr('font-weight', 'bold')
                                    .attr('text-anchor', 'middle')
                                    .attr('alignment-baseline', 'middle')
                                    .attr('x', '500')
                                    .attr('y', '450'))
                            .append($('<text></text>')
                                    .attr('id', 'sMin' + fact.type + fact.address)
                                    .addClass('scale-text')
                                    .attr('text-anchor', 'middle')
                                    .attr('alignment-baseline', 'middle')
                                    .attr('x', '50')
                                    .attr('y', '550'))
                            .append($('<text></text>')
                                    .attr('id', 'sMax' + fact.type + fact.address)
                                    .addClass('scale-text')
                                    .attr('text-anchor', 'middle')
                                    .attr('alignment-baseline', 'middle')
                                    .attr('x', '950')
                                    .attr('y', '550'))));
         let cFact = fact;
         if (!setupMode && fact.record) {
            $(elem).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});
				$(elem).css('cursor', 'pointer');
			}

         let divId = '#svgDiv' + fact.type + fact.address;
         $(divId).html($(divId).html());  // redraw to activate the SVG !!
         break;
      }

      case 5:            // Meter
      case 6: {          // MeterLevel
         initMeter(key, widget, fact, widget.scalemax, null);
         break;
      }

      case 7: {          // PlainText
         $(elem).addClass('widgetPlain widgetDropZone');
         if (setupMode)
            $(elem).append($('<div></div>')
                           .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                           .addClass(titleClass)
                           .css('user-select', 'none')
                           .css('padding', fact.type == 'WEA' ? '0px' : '')
                           .click(function(event) {titleClick(event.ctrlKey, key);})
                           .html(title));

         if (fact.type == 'WEA' && fact.address != 1) {
            if (fact.address == 2)
               initWindy(key, widget, fact);
            else if (fact.address == 3)
               initWindyMap(key, widget, fact);
         }
         else {
            let wFact = fact;
            $(elem).append($('<div></div>')
                           .attr('id', 'widget' + fact.type + fact.address)
                           .addClass(fact.type == 'WEA' ? 'widget-weather' : 'widget-text')
                           .css('user-select', 'none')
                           .css('color', widget.color)
                           .css('height', 'inherit')
                           .click(function() { if (wFact.type == 'WEA' && wFact.address == 1) weatherForecast(); }));
         }
         break;
      }

      case 8: {     // 8 (Choice)
         console.log("choices:", fact.choices);
         $(elem)
            .addClass("widget widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .css('user-select', 'none')
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .html(title))
            .append($('<div></div>')
                    .attr('id', 'choice' + fact.type + fact.address)
                    .addClass('widget-choice rounded-border')
                    .css('color', widget.color));
         let choices = fact.choices.split(",");
         for (c = 0; c < choices.length; ++c) {
            // console.log("c: ", choices[c]);
            $('#choice' + fact.type + fact.address)
               .append($('<div></div>')
                       .attr('id', 'widget' + fact.type + fact.address + '_' + c)
                       .addClass('rounded-border')
                       .html(choices[c])
                       .click({ "value" : choices[c] }, function(event) {
                          socket.send({'event': 'toggleio', 'object': {'address': fact.address, 'type': fact.type, 'value': event.data.value, 'action': 'switch'}});
                       }));
         }

         break;
      }

      case 9:          // Symbol-Value
      case 12: {       // Symbol-Text
         $(elem)
            .addClass("widgetSymbolValue widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .css('user-select', 'none')
                    .html(title))
            .append($('<button></button>')
                    .attr('id', 'button' + fact.type + fact.address)
                    .addClass('widget-main')
                    .attr('type', 'button')
                    .css('color', widget.color)
                    .css('user-select', 'none')
                    .append($('<img></img>')
                            .attr('id', 'widget' + fact.type + fact.address)
                            .attr('draggable', false)
                            .addClass('rounded-border')
                            .css('user-select', 'none'))
                    .click(function() { toggleIo(fact.address, fact.type); }))
            .append($('<div></div>')
                    .attr('id', 'value' + fact.type + fact.address)
                    .addClass('symbol-value')
                    .css('user-select', 'none')
                   );

         break;
      }

      case 10: {   // space
         $(elem).addClass("widgetSpacer widgetDropZone");
         $(elem).append($('<div></div>')
                        .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                        .addClass(titleClass)
                        .click(function(event) {titleClick(event.ctrlKey, key);})
                        .html(setupMode ? ' spacer' : ''));
         if (!setupMode)
            $(elem).css('background-color', widget.color);
         if (widget.linefeed) {
            $(elem)
               .css('flex-basis', '100%')
               .css('height', setupMode ? '40px' : '0px')
               .css('padding', '0px')
               .css('margin', '0px');
         }
         break;
      }

      case 11: {   // actual time
         $(elem)
            .addClass("widgetPlain widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .css('user-select', 'none')
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .html(setupMode ? ' time' : ''));
         $(elem).append($('<div></div>')
                        .attr('id', 'widget' + key)
                        .addClass('widget-time')
                        .css('color', widget.color)
                        .css('user-select', 'none')
                        .html(getTimeHtml()));
         setInterval(function() {
            let timeId = '#widget'+key.replace(':', '\\:');
            $(timeId).html(getTimeHtml());
         }, 1*1000);

         break;
      }

      case 14: {      // Special Symbol
         $(elem)
            .addClass("widget widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .css('user-select', 'none')
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .html(title));

         let maxWidth = 280;
         let html = '';
         let yStart = 14;
         let heightStart = 170;
         let symbolWidth = maxWidth;
         let xPos = 10;

         if (widget.barwidth != null) {
            symbolWidth = widget.barwidth;
            xPos = (maxWidth - symbolWidth) / 2 + 10;
         }

         if (widget.symbol == null || widget.symbol == 0) {
            // plain rectangular tank symbol

            html = '<svg xmlns="http://www.w3.org/2000/svg" style="height:100%;aspect-ratio:1/1;" viewBox="0 0 300 200">' +
               // äußerer Rahmen
               '  <rect x="' + xPos + '"   y="10"    width="' + symbolWidth + '" rx="5" ry="5" height="180" fill="white" style="fill:white;stroke:black;stroke-width:5" />' +
               // untere permanente Füllung wegen der Rundung in widget Farbe
               '  <rect x="' + (xPos +0.5) + '" y="178.5" width="' + (symbolWidth - 3) + '" rx="4.5" height="10" fill="' + widget.color + '" />' +
               // Rest weiß - leer
               '  <rect x="' + (xPos +0.5) + '" y="14"    width="' + (symbolWidth - 3) + '" height="169" fill="rgb(255 255 255)" />' +
               // Füllung in widget Farbe
               '  <rect id="svg' + key + '" x="' + (xPos +0.5) + '" y="14" width="' + (symbolWidth - 3) + '" height="170" fill="' + widget.color + '" />' +
               // die Oberfläche mit 3D Effekt
               '  <ellipse id="svgEllipse' + key + '" rx="' + ((symbolWidth-6)/2) + '" ry="' + 8 + '" cx="' + (xPos + symbolWidth/2) + '" cy="100" stroke="' + widget.color + '" fill="' + alterColor(widget.color, 'lighten', 100) + '" style="stroke-width:2" />' +
               // Text
               '  <ellipse rx="40" ry="20" cx="' + (xPos + symbolWidth - 20) + '" cy="10" stroke="' + widget.color + '" style="fill:white;stroke-width:2" />' +
               '  <text id="value' + key + '" x="' + (xPos + symbolWidth - 20) + '" y="11" fill="black" alignment-baseline="middle" dominant-baseline="middle" text-anchor="middle" font-family="Helvetica,Arial,sans-serif" font-size="18">--</text>' +
               '</svg>';
         }

         else if (widget.symbol == 1) {
            // ...
         }

         $(elem).append($('<div></div>')
                        .attr('id', 'widget' + key)
                        .data('yStart', yStart)
                        .data('heightStart', heightStart)
                        .addClass('widget-main')
                        .html(html)
                        .click(function() {
                           let cFact = fact;
                           if (!setupMode && fact.record)
                              toggleChartDialog(cFact.type, cFact.address, key);})
                       );

         break;
      }

      default: {      // type 2 (Text)
         $(elem)
            .addClass("widget widgetDropZone")
            .append($('<div></div>')
                    .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                    .addClass(titleClass)
                    .css('user-select', 'none')
                    .click(function(event) {titleClick(event.ctrlKey, key);})
                    .html(title))
            .append($('<div></div>')
                    .attr('id', 'widget' + fact.type + fact.address)
                    .css('user-select', 'none')
                    .css('color', widget.color)
                    .addClass('widget-value'));

         let cFact = fact;
         if (!setupMode && fact.record) {
            $(elem).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});
				$(elem).css('cursor', 'pointer');
			}
         break;
      }
   }
}

//***************************************************************************
// Init WindyApp Widget
//***************************************************************************

function initWindy(key, widget, fact)
{
   // https://windy.app/de/widgets

   // Fields:
   //   wind-direction, wind-speed, wind-gust, air-temp,
   //   clouds, precipitation, waves-direction, waves-height,
   //   waves-period, tides, moon-phase

   let html = '<div ' +
       '     data-windywidget="forecast"' +
       '     data-customcss="windy-dark.css"' +
       '     data-thememode="dark"' +            // white|dark
       '     data-tempunit="C"' +                // C|F
       '     data-windunit="bft"' +              // knots|bft|m/s|mph|km/h
       '     data-heightunit="m"' +              // m|ft
       '     data-spotid="' + config.windyAppSpotID + '"' +
       // '     data-lat="54.0951002"' +         //
       // '     data-lng="8.9516540"' +          //
       '     data-fields="wind-speed,wind-gust,wind-direction,air-temp,clouds,precipitation"' +
       '     data-appid="widgets_7e484018b8"' +
       ' >' +
       '</div>' +
       '<script async="true" data-cfasync="false" type="text/javascript"' +
       '        src="windy_forecast_async.js?v1.4.6"></script>';

   let elem = document.getElementById('div_' + key);

   // special - use the whole width and self adjusted height

   $(elem).css('height', '');
   $(elem).css('width', '');
   $(elem).html(html);
}

function initWindyMap(key, widget, fact)
{
   return;

   let html = '<div ' +
      '    data-windywidget="map"' +
      '    data-thememode="white"' +
      '    data-spotid="' + config.windyAppSpotID + '"' +
      '    data-appid="widgets_8bdd3cb645">' +
      '</div>' +
      '<script async="true" data-cfasync="false" type="text/javascript" src="//windy.app/widget3/windy_map_async.js?v289"></script>';

   let elem = document.getElementById('div_' + key);

   // special - use the whole width and self adjusted height

   $(elem).css('height', '');
   $(elem).css('width', '');
   $(elem).html(html);
}

//***************************************************************************
// Init Meter Widget
//***************************************************************************

function initMeter(key, widget, fact, neededScaleMax, value)
{
   let radial = widget.widgettype == 5;
   let showValue = widget.showvalue == null || widget.showvalue == true;
   let titleClass = getWidgetTitleClass(widget, fact);
   let title = getWidgetTitle(widget, fact);
   let elem = document.getElementById('div_' + key);

   $(elem).empty();

   if (radial || !showValue)
      elem.className = 'widgetMeter widgetDropZone';
   else if (showValue)
      elem.className = 'widgetMeterLinear widgetDropZone';

   let eTitle = document.createElement("div");
   eTitle.className = "widget-title " + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : titleClass);
   eTitle.addEventListener("click", function(event) {titleClick(event.ctrlKey, key)}, false);
   eTitle.innerHTML = title;
   elem.appendChild(eTitle);

   let main = document.createElement("div");
   main.className = "widget-main-meter";
   elem.appendChild(main);

   let canvas = document.createElement('canvas');
   main.appendChild(canvas);
   canvas.setAttribute('id', 'widget' + fact.type + fact.address);

   let cFact = fact;
   if (!setupMode && fact.record) {
      $(canvas).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});
		canvas.style.cursor = 'pointer';
	}

   if (!radial && showValue) {
      let _value = document.createElement("div");
      _value.setAttribute('id', 'widgetValue' + fact.type + fact.address);
      _value.className = "widget-main-value-lin";
      elem.appendChild(_value);
      $("#widgetValue" + fact.type + fact.address).css('color', widget.color);
      let ePeak = document.createElement("div");
      ePeak.setAttribute('id', 'peak' + fact.type + fact.address);
      ePeak.className = "widget-main-peak-lin";
      elem.appendChild(ePeak);
   }

   if (widget.scalemin == null)
      widget.scalemin = 0;

   let neededScaleMin = widget.scalemin;

   if (widget.scalemin < 0)
      neededScaleMin = neededScaleMax*-1;

   let ticks = [];
   let scaleRange = neededScaleMax - neededScaleMin;
   let stepWidth = widget.scalestep;

   if (widget.rescale && widget.scalestep != null && widget.scalemax != neededScaleMax) {
      let factor = widget.scalemax / neededScaleMax;
      stepWidth = widget.scalestep / factor;

      if (neededScaleMax >= 10)
         stepWidth = Math.round(stepWidth);
   }

   if (!stepWidth) {
      let steps = 10;
      if (scaleRange <= 100)
         steps = scaleRange % 10 == 0 ? 10 : scaleRange % 5 == 0 ? 5 : scaleRange;
      if (steps < 10)
         steps = 10;
      if (!radial && widget.unit == '%')
         steps = 4;
      stepWidth = scaleRange / steps;
   }

   if (stepWidth <= 0)
      stepWidth = 1;

   stepWidth = Math.round(stepWidth*10) / 10;

   if (neededScaleMax < widget.scalemax)
      neededScaleMax += stepWidth;       // if we auto scale add one step

   if (widget.scalemin < 0)
      neededScaleMin = neededScaleMax*-1;

   let steps = -1;
   for (let step = neededScaleMin; step.toFixed(2) <= neededScaleMax; step += stepWidth) {
      ticks.push(step % 1 ? parseFloat(step).toFixed(1) : parseInt(step));
      steps++;
   }

   let highlights = {};
   let critmin = (widget.critmin == null || widget.critmin == -1) ? neededScaleMin : widget.critmin;
   let critmax = (widget.critmax == null || widget.critmax == -1) ? neededScaleMax : widget.critmax;
   let minColor = widget.unit == '°C' ? 'blue' : 'rgba(255,0,0,.6)';

   highlights = [
      { 'from': neededScaleMin, 'to': critmin,        'color': minColor },
      { 'from': critmin,        'to': critmax,        'color': 'rgba(0,255,0,.6)' },
      { 'from': critmax,        'to': neededScaleMax, 'color': 'rgba(255,0,0,.6)' }
   ];

   let bWidth = radial ? 0 : (widget.unit == '%' || widget.unit == 'l' ? 12 : 7);

   if (widget.barwidth != null)
      bWidth = widget.barwidth;

   let progressColor = widget.unit == '%' ? 'blue' : 'red';

	if (widget.barcolor != null && widget.barcolor != '')
		progressColor = widget.barcolor

	progressColor = getWidgetColor(widget, widget.colorConditionBar, progressColor, value);

   options = {
      renderTo: 'widget' + fact.type + fact.address,

      units: radial ? widget.unit : '',
      minValue: neededScaleMin,
      maxValue: neededScaleMax,
      majorTicks: ticks,
      minorTicks: 5,
      highlights: highlights,
      strokeTicks: false,
      highlightsWidth: radial ? 8 : 6,

      colorPlate: radial ? '#2177AD' : 'rgba(0,0,0,0)',
      colorBar: colorStyle.getPropertyValue('--scale'),        // 'gray',
      colorBarProgress: progressColor,
      colorBarStroke: 'red',
      colorMajorTicks: colorStyle.getPropertyValue('--scale'), // '#f5f5f5',
      colorMinorTicks: colorStyle.getPropertyValue('--scale'), // '#ddd',
      colorTitle: colorStyle.getPropertyValue('--scaleText'),
      colorUnits: colorStyle.getPropertyValue('--scaleText'),  // '#ccc',
      colorNumbers: colorStyle.getPropertyValue('--scaleText'),
      colorNeedle: 'rgba(240, 128, 128, 1)',
      colorNeedleEnd: 'rgba(255, 160, 122, .9)',

      fontNumbersSize: radial ? 34 : (showValue ? 40 : 30),
      fontUnitsSize: 45,
      fontUnitsWeight: 'bold',
      borderOuterWidth: 0,
      borderMiddleWidth: 0,
      borderInnerWidth: 0,
      borderShadowWidth: 0,
      fontValueWeight: 'bold',
      valueBox: radial,
      valueInt: 0,
      valueDec: widget.unit == '%' ? 0 : 2,
      colorValueText: colorStyle.getPropertyValue('--scale'),
      colorValueBoxBackground: 'transparent',
      valueBoxStroke: 0,
      fontValueSize: 45,
      valueTextShadow: false,
      animationRule: 'bounce',
      animationDuration: 500,
      barWidth: bWidth,
      numbersMargin: -3,
      needle: radial,

      // linear gauge specials

      barBeginCircle: widget.unit == '°C' ? 15 : 0,

      tickSide: 'left',
      numberSide: 'left',
      needleSide: 'left',
      colorPlate: 'transparent'
   };

   // patch width and height due to strange behavior of (at least) the LinearGauge when wider than high

   if ($(main).innerHeight() < $(main).innerWidth())
      options.width = options.hight = $(main).innerHeight();

	if (dashboardGauges[fact.type + fact.address] != null) {
      dashboardGauges[fact.type + fact.address].destroy();
      dashboardGauges[fact.type + fact.address] = null;
   }

   if (radial)
      dashboardGauges[fact.type + fact.address] = new RadialGauge(options);
   else
      dashboardGauges[fact.type + fact.address] = new LinearGauge(options);

   dashboardGauges[fact.type + fact.address].draw();
}

function initLightColorDialog()
{
   $("#container").append($('<div></div>)')
                          .attr('id', 'lightColorDiv')
                          .addClass('rounded-border lightColorDiv')
                          .append($('<input></input>)')
                                  .attr('id', 'lightColor')
                                  .addClass('lightColor')
                                  .attr('type', 'text'))
                          .append($('<button></button>)')
                                  .html('Ok')
                                  .click(function() { $('#lightColorDiv').css('display', 'none'); }))
                         );

   let options = {
      'cssClass' : 'lightColor',
      'layout' :  'block',
      'format' : 'hsv',
      'sliders' : 'wsvp',
      'autoResize' : false
   }

   $('#lightColor').wheelColorPicker(options);
   $('#lightColorDiv').css('display', 'none');

   $('#lightColor').on('sliderup', function(e) {
      if (!$(this).data('key'))
         return;

      let fact = valueFacts[$(this).data('key')];
      let hue = parseInt($(this).wheelColorPicker('getColor').h * 360);
      let sat = parseInt($(this).wheelColorPicker('getColor').s * 100);
      let bri = parseInt($(this).wheelColorPicker('getColor').v * 100);

      console.log("color of " + fact.address + " changed to '" + hue + "' / " + bri + '% / ' + sat + '%');

      socket.send({ "event": "toggleio", "object":
                    {  'action': 'color',
                       'type': fact.type,
                       'address': fact.address,
                       'hue': hue,
                       'saturation' : sat,
                       'bri': bri
                    }});
   });


   /*
     das stört den 'number' Input - z.B. Range auf der Chart Ansicht !!

     $('#container').on('mouseup', function(e) {
      e.preventDefault();
      if ($(e.target).attr('id') != 'lightColorDiv' && !$('#lightColorDiv').has(e.target).length)
         $('#lightColorDiv').css('display', 'none');
   }); */
}

function showLightColorDialog(key)
{
   let posX = ($('#container').innerWidth() - $('#lightColorDiv').outerWidth()) / 2;
   let posY = ($('#container').innerHeight() - $('#lightColorDiv').outerHeight()) / 2;
   let sensor = allSensors[key];

   $('#lightColor').data('key', key);
   $('#lightColorDiv').css('left', posX + 'px');
   $('#lightColorDiv').css('top', posY + 'px');
   $('#lightColorDiv').css('display', 'block');
   $('#lightColor').wheelColorPicker('setColor', { 'h': sensor.hue/360.0, 's': sensor.sat/100.0, 'v': sensor.score/100.0 });
}

function getTimeHtml()
{
   let now = new Date();

   // calc daytime every 60 seconds

   if (!daytimeCalcAt || now.getTime() >= daytimeCalcAt.getTime()+60000){
      daytimeCalcAt = now;
      let sunset = new Date().sunset(config.latitude.replace(',', '.'), config.longitude.replace(',', '.'));
      let sunrise = new Date().sunrise(config.latitude.replace(',', '.'), config.longitude.replace(',', '.'));
      isDaytime = daytimeCalcAt > sunrise && daytimeCalcAt < sunset;
      // console.log("isDaytime: " + isDaytime + ' : ' + sunrise + ' : '+ sunset);
   }

   let cls = isDaytime ? 'mdi mdi-weather-sunny' : 'mdi mdi-weather-night';

   return '<div>'
      + '<span class="' + cls + ' " style="color:orange;"></span>'
      + '<span> ' + moment().format('dddd Do') + '</span>'
      + '<span> ' + moment().format('MMMM YYYY') + '</span>'
      + '</div>'
      + '<div style="font-size:2em">' + moment().format('HH:mm:ss') + '</div>';
}

function getWeatherHtml(symbolView, wfact, weather)
{
   // https://openweathermap.org/weather-conditions#Weather-Condition-Codes-2

   let html = '';

   if (symbolView) {
      let wIconRef = 'img/weather/' + weather.icon + '@4x.png';
      if (!images.find(img => img == wIconRef))
         wIconRef = 'http://openweathermap.org/img/wn/' + weather.icon + '@4x.png';
      html += '<div style="display:block;font-weight:bold;height:75%;padding-left:5px;"><span>' + weather.detail + '</span><img style="height:100%" src="' + wIconRef + '"></img></div>';
      html += '<div style="display:flex;justify-content:space-between;padding-top:3px;">';
      html += ' <span style="color:orange;font-weight:bold;padding-left:5px;">' + weather.temp.toFixed(1) + '°C</span>';
      html += ' <span class="mdi mdi-walk" style="color:orange;font-weight:bold;font-size:smaller;">' + weather.tempfeels.toFixed(1) + '</span>';
      html += '</div>';

   }
   else {
      let wIconRef = 'img/weather/' + weather.icon + '.png';
      if (!images.find(img => img == wIconRef))
         wIconRef = 'http://openweathermap.org/img/wn/' + weather.icon + '.png';
      html += '<div style="display:inline-flex;align-items:center;font-weight:bold;"><span><img src="' + wIconRef + '"></img></span><span>' + weather.detail + '</span></div>';
      html += '<div><span style="color:orange;font-weight:bold;">' + weather.temp.toFixed(1) + '°C</span>';
      html += '<span style="font-size:smaller;"> (' + weather.tempmin.toFixed(1) + ' / ' + weather.tempmax.toFixed(1) + ')' + '</span></div>';
      html += '<div>Luftdruck: <span style="color:orange;font-weight:bold;">' + weather.pressure + ' hPa' + '</span></div>';
      html += '<div>Luftfeuchte: <span style="color:orange;font-weight:bold;">' + weather.humidity + ' %' + '</span></div>';
      html += '<div>Wind: <span style="color:orange;font-weight:bold;">' + weather.windspeed + ' m/s' + '</span></div>';
   }

   return html;
}

function weatherForecast()
{
   let lastDay = '';
   let form = document.createElement("div");

   $(form).addClass('rounded-border weatherForecast');
   $(form).attr('tabindex', 0);
   $(form).click(function() { $(form).remove(); });
   $('#container').append(form);

   let showExtras = $(form).outerWidth() > 580;
   let html = '<div class="rounded-border" style="justify-content:center;font-weight:bold;background-color:#2f2f2fd1;">' + weatherData.city + '</div>';

   for (let i = 0; i < weatherData.forecasts.length; i++) {
      let weather = weatherData.forecasts[i];
      let day = moment(weather.time*1000).format('dddd Do MMMM');
      let time = moment(weather.time*1000).format('HH:00');
      let wIconRef = 'img/weather/' + weather.icon + '.png';

      if (!images.find(img => img == wIconRef))
         wIconRef = 'http://openweathermap.org/img/wn/' + weather.icon + '.png';

      if (day != lastDay) {
         lastDay = day;
         html += '<div class="rounded-border" style="background-color:#2f2f2fd1;">' + day + '</div>';
      }

      let tempColor = weather.temp < 0 ? '#2c99eb' : (weather.temp > 20 ? 'red' : 'white');

      html += '<div class="rounded-border">';
      html += '<span>' + time + '</span>';
      html += '<span><img src="' + wIconRef + '"></img></span>';
      html += '<span class="mdi mdi-thermometer" style="color:' + tempColor + ';"> ' + weather.temp + ' °C</span>';
      if (showExtras)
         html += '<span class="mdi mdi-walk">' + weather.tempfeels + ' °C</span>';
      html += '<span class="mdi mdi-water-percent"> ' + weather.humidity + ' %</span>';
      html += '<span>' + weather.pressure + ' hPa</span>';
      html += '<span class="mdi mdi-weather-windy"> ' + weather.windspeed + ' m/s</span>';

      html += '</div>'
   }

   html += '</div>'

   $(form).html(html);
   $(form).focus();
   $(form).keydown(function(event) {
      if (event.which == 27 || event.which == 13) {
         event.preventDefault();
         $(form).remove();
      }
   });
}

function titleClick(ctrlKey, key)
{
   let fact = valueFacts[key];
   let widget = dashboards[actDashboard].widgets[key];
   let hasMode = fact && fact.outputModes == 3 //  -> ooUser and ooAuto are set

   // console.log("titleClick: ", ctrlKey, key, "fact.outputModes", fact.outputModes);

   if (setupMode) {
      widgetSetup(key);
   }
   else if (ctrlKey && hasMode) {
      if (fact && localStorage.getItem(storagePrefix + 'Rights') & fact.rights)
         // console.log("toggleMode(", fact.address, fact.type, ')');
         toggleMode(fact.address, fact.type);
   }
   else {
      let sensor = allSensors[key];
      let now = new Date();
      let last = new Date(sensor.last * 1000);
		let before = Math.round((now - daemonState.timeOffset -last)/1000)
      let peakMaxTime = new Date(sensor.peakmaxtime * 1000);
      let peakMinTime = new Date(sensor.peakmintime * 1000);
      let form = document.createElement("div");

      // console.log(sensor);
      // console.log(fact);

      $(form).append($('<div></div>')
                     .css('z-index', '9999')
                     .css('minWidth', '40vh')

                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('ID'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .css('font-family', 'monospace')
                                             .css('font-size', 'larger')
                                             .html(key + ' (' + parseInt(key.split(":")[1]) + ')')
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Title'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .css('font-style', 'italic')
                                             .addClass('rounded-border')
                                             .html(fact.title)
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('User Title'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .css('font-style', 'italic')
                                             .addClass('rounded-border')
                                             .html(fact.usrtitle)
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Value'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(sensor.value + ' ' + widget.unit)
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Peak Max'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .attr('id', 'dlgPeakMax')
                                             .addClass('rounded-border')
                                             .html(sensor.peakmax != null ? (sensor.peakmax + ' ' + widget.unit + ' / ' + peakMaxTime.toLocaleString('de-DE')) : '-')
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Peak Min'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .attr('id', 'dlgPeakMin')
                                             .addClass('rounded-border')
                                             .html(sensor.peakmin != null ? (sensor.peakmin + ' ' + widget.unit + ' / ' + peakMinTime.toLocaleString('de-DE')) : '-')
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Last Update'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(last.toLocaleString('de-DE') + ' (' + before + 's)'))
                                    ))
                     .append($('<div></div>')
                             .css('display', sensor.battery ? 'flex' : 'none')
                             .css('margin-bottom', '4px')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Battery'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(sensor.battery ? sensor.battery + ' %' :  '-')
                                            )))
                    );


      let btns = {};

      if (sensor.peakmax) {
         btns['Reset Peak'] = function() {
            $('#dlgPeakMax').html('-');
            $('#dlgPeakMin').html('-');
            socket.send({ "event" : "command", "object" : { "what" : 'peak', "type": fact.type, "address": fact.address } });
            let sensor = allSensors[key];
            sensor.peakmax = null;
            sensor.peakmin = null;
         };
      }

      if (hasMode) {
         btns['Manuel/Auto'] = function () {
            if (fact && localStorage.getItem(storagePrefix + 'Rights') & fact.rights)
               toggleMode(fact.address, fact.type);
            $(this).dialog('close');
         };
      }

      btns['Ok'] = function () { $(this).dialog('close'); };

      $(form).dialog({
         modal: true,
         closeOnEscape: true,
         hide: "fade",
         width: "auto",
         title: "Info",
         buttons: btns,
         close: function() { $(this).dialog('destroy').remove();}
      });
   }
}

function updateDashboard(widgets, refresh)
{
   if (widgets != null && dashboards[actDashboard] != null) {
      for (let key in widgets) {
         if (dashboards[actDashboard].widgets[key] == null)
            continue;

         updateWidget(widgets[key], refresh, dashboards[actDashboard].widgets[key]);
      }
   }
}

function updateWidget(sensor, refresh, widget)
{
   if (sensor == null)
      return ;

   let key = toKey(sensor.type, sensor.address);
   fact = valueFacts[key];

   // console.log("updateWidget " + fact.name + " of type " + widget.widgettype);
   // console.log("updateWidget" + JSON.stringify(sensor, undefined, 4));

   if (fact == null) {
      console.log("Fact for widget '" + key + "' not found, ignoring");
      return ;
   }
   if (widget == null) {
      console.log("Widget '" + key + "' not found, ignoring");
      return;
   }

   if (widget.unit == '')
      widget.unit = fact.unit;

   let widgetDiv = $('#div_' + key.replace(':', '\\:'));

   if (widget.range > 2)
   {
      console.log("widget.range", key, widget.range);
      sensor.valid = true;
   }

   if (sensor.type == 'WEA' && (sensor.address == 2 || sensor.address == 3)) // Windy App
      sensor.valid = true;

   widgetDiv.css('opacity', sensor.valid ? '100%' : '45%');
   // $('#div_' + key.replace(':', '\\:')).css('opacity', sensor.valid ? '100%' : '25%');

   if (widget.widgettype == 0 || widget.widgettype == 9 || widget.widgettype == 12)         // Symbol, Symbol-Value, Symbol-Text
   {
      // console.log("sensor: ", JSON.stringify(sensor));
      let state = fact.type != 'HMB' ? sensor.value != 0 : sensor.value == 100;
      let image = '';
      let classes = '';

      if (sensor.image != null)                                      // own image got by data
         image = sensor.image;
      else if (state && widget.symbolOn && widget.symbolOn != '') {  // MDI Icon ON
         classes = widget.symbolOn.replace(':', ' ');
         $("#widget" + fact.type + fact.address).remove();
      }
      else if (!state && widget.symbol && widget.symbol != '') {     // MDI Icon OFF
         classes = widget.symbol.replace(':', ' ');
         $("#widget" + fact.type + fact.address).remove();
      }
      else if (widget.symbol && widget.symbol != '') {               // MDI Icon
         classes = widget.symbol.replace(':', ' ');
         $("#widget" + fact.type + fact.address).remove();
      }
      else                                                           // image by patch as <img> element
         image = state ? widget.imgon : widget.imgoff;

      if (image != '')
         $("#widget" + fact.type + fact.address).attr("src", image);
      else {
         $("#button" + fact.type + fact.address).removeClass();
         $("#button" + fact.type + fact.address).addClass('widget-main');
         $("#button" + fact.type + fact.address).addClass(classes);

         let fontSize = Math.min(widgetDiv.height(), widgetDiv.width()) * 0.6;
         $("#button" + fact.type + fact.address).css('font-size', fontSize + 'px');
      }

      $("#button" + fact.type + fact.address).css('background-color', sensor.working ? '#7878787878' : 'transparent');

      // $('#div_'+key.replace(':', '\\:')).css('background-color', (fact.outputModes == 3 && sensor.mode == 'manual') ? '#a27373' : '');
      widgetDiv.css('background-color', (fact.outputModes == 3 && sensor.mode == 'manual') ? '#a27373' : '');
      widget.colorOn = widget.colorOn == null ? symbolOnColorDefault : widget.colorOn;

		let color = state ? widget.colorOn : widget.color;

		color = getWidgetColor(widget, widget.colorCondition, color, sensor.value);

      if (sensor.hue)
         color = tinycolor({ 'h': sensor.hue, 's': sensor.sat, 'v': sensor.score }).toHslString();
		else if (sensor.color)
			color = sensor.color;

      // widget.color = widget.color == null ? symbolColorDefault : widget.color;

      $("#button" + fact.type + fact.address).css('color', color);

      if (widget.widgettype == 9)
         $("#value" + fact.type + fact.address).text(sensor.value.toFixed(widget.unit == '%' || widget.unit == '' ? 0 : 2) + (widget.unit != '' ? ' ' : '') + widget.unit);
      else if (widget.widgettype == 12 && sensor.text != null)
         $("#value" + fact.type + fact.address).text(sensor.text.replace(/(?:\r\n|\r|\n)/g, '<br>'));

//      let prs = $('#progressBar' + fact.type + fact.address);
//      $('#progress' + fact.type + fact.address).css('display', fact.options & 0x02 && sensor.value ? 'block' : 'none');
//
//      if (sensor.score && prs != null)
//         $(prs).css('width', sensor.score + '%');
//
//      if (sensor.mode == "auto" && sensor.next > 0) {
//         let pWidth = 100;
//         let s = sensor;
//         let id = fact.type + fact.address;
//         let iid = setInterval(progress, 200);
//
//         function progress() {
//            if (pWidth <= 0) {
//               clearInterval(iid);
//            } else {
//               let d = new Date();
//               pWidth = 100 - ((d/1000 - s.last) / ((s.next - s.last) / 100));
//               document.getElementById("progressBar" + id).style.width = pWidth + "%";
//            }
//         }
//      }
   }
   else if (widget.widgettype == 1 || widget.widgettype == 13)    // Chart
   {
      if (widget.showpeak != null && widget.showpeak)
         $("#peak" + fact.type + fact.address).text(sensor.peakmax != null ? sensor.peakmax.toFixed(2) + " " + widget.unit : "");

      $("#value" + fact.type + fact.address).text(sensor.value.toFixed(2) + " " + widget.unit);

      if (refresh) {
         let jsonRequest = {};
         if (!widget.range)
            widget.range = 1;
         prepareChartRequest(jsonRequest, toKey(fact.type, fact.address), 0, widget.range, widget.widgettype == 13 ? "chartwidgetbar" : "chartwidget");
         socket.send({ "event" : "chartdata", "object" : jsonRequest });
      }
   }
   else if (widget.widgettype == 8)    // Choice
   {
      // let text = sensor.text.replace(/(?:\r\n|\r|\n)/g, '<br>');

      let choices = fact.choices.split(",");
      for (c = 0; c < choices.length; ++c) {
         $("#widget" + fact.type + fact.address + '_' + c).css('background-color', sensor.text == choices[c] ? 'gray' : "");
      }
   }
   else if (widget.widgettype == 2 || widget.widgettype == 7)    // Text, PlainText
   {
      if (sensor.type == 'WEA' && sensor.address == 1 && sensor.text != null) { // Open Weather Map
         let bigView = false;
         let wfact = fact;
         weatherData = JSON.parse(sensor.text);
         $("#widget" + fact.type + fact.address).html(getWeatherHtml(bigView, wfact, weatherData));
         if (weatherInterval)
            clearInterval(weatherInterval);
         if (weatherData && config.toggleWeatherView != 0) {
            weatherInterval = setInterval(function() {
               bigView = !bigView;
               $("#widget" + fact.type + fact.address).html(getWeatherHtml(bigView, wfact, weatherData));
            }, 5*1000);
         }
      }
      else if (sensor.type == 'WEA' && (sensor.address == 2 || sensor.address == 3)) { // Windy App

      }
      else if (sensor.text != null) {
         $("#widget" + fact.type + fact.address).css('color', getWidgetColor(widget, widget.colorCondition, widget.color, sensor.text));
         if (widget.widgettype == 7 && sensor.text.indexOf('\n') != -1) {
            // show plain with tabs text as table

            let lines = sensor.text.split(/(?:\r\n|\r|\n)/);
            let html = '';

            for (l = 0; l < lines.length; ++l) {
               let tabs = lines[l].split(/(?:\t)/);
               let tabHtml = '';
               for (t = 0; t < tabs.length; ++t)
                  tabHtml += '<td>' + tabs[t] + '</td>';
               html += '<tr>' + tabHtml + '</tr>\n';
            }

            $("#widget" + fact.type + fact.address).html('<table>' + html + '</table>');
         }
         else {
            let text = sensor.text.replace(/(?:\r\n|\r|\n)/g, '<br>');
            $("#widget" + fact.type + fact.address).html(text);
         }
      }
   }
   else if (widget.widgettype == 3)      // Value
   {
      $("#widget" + fact.type + fact.address).css('color', getWidgetColor(widget, widget.colorCondition, widget.color, sensor.value));
      $("#widget" + fact.type + fact.address).html(sensor.value + " " + widget.unit);
      if (widget.showpeak != null && widget.showpeak)
         $("#peak" + fact.type + fact.address).text(sensor.peakmax != null ? sensor.peakmax.toFixed(2) + " " + widget.unit : "");
   }
   else if (widget.widgettype == 4)      // Gauge
   {
      let value = sensor.value.toFixed(2);
      let scaleMax = !widget.scalemax || widget.unit == '%' ? 100 : widget.scalemax.toFixed(0);
      let scaleMin = value >= 0 ? "0" : Math.ceil(value / 5) * 5 - 5;
      let _peak = sensor.peakmax != null ? sensor.peakmax : 0;
      if (scaleMax < Math.ceil(value))
         scaleMax = value;
      if (widget.showpeak != null && widget.showpeak && scaleMax < Math.ceil(_peak))
         scaleMax = _peak.toFixed(0);
      $("#sMin" + fact.type + fact.address).text(scaleMin);
      $("#sMax" + fact.type + fact.address).text(scaleMax);
      $("#value" + fact.type + fact.address).text(value + " " + widget.unit);
      let ratio = (value - scaleMin) / (scaleMax - scaleMin);
      let peak = (_peak.toFixed(2) - scaleMin) / (scaleMax - scaleMin);

      $("#pb" + fact.type + fact.address).attr("d", "M 950 500 A 450 450 0 0 0 50 500");
      $("#pv" + fact.type + fact.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, -90, ratio * 180.0 - 90));
      if (widget.showpeak != null && widget.showpeak)
         $("#pp" + fact.type + fact.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, peak * 180.0 - 91, peak * 180.0 - 90));
   }
   else if (widget.widgettype == 5 || widget.widgettype == 6)    // Meter
   {
      if (sensor.value != null) {
         let value = sensor.value;
			let reInitMeter = false;

			if (widgetDiv.data('scalemax') != widget.scalemax)
				widgetDiv.data('scalemax', widget.scalemax);

         if (widget.rescale) {
            let neededScaleMax = widget.scalemax;

            if (value <= 0)
               ;
            else if (value < parseInt(widget.scalemax / 100))
               neededScaleMax = parseInt(widget.scalemax / 100);
            else if (value < parseInt(widget.scalemax / 80))
               neededScaleMax = parseInt(widget.scalemax / 80);
            else if (value < parseInt(widget.scalemax / 40))
               neededScaleMax = parseInt(widget.scalemax / 40);
            else if (value < parseInt(widget.scalemax / 20))
               neededScaleMax = parseInt(widget.scalemax / 20);
            else if (value < parseInt(widget.scalemax / 10))
               neededScaleMax = parseInt(widget.scalemax / 10);
            else if (value < parseInt(widget.scalemax / 4))
               neededScaleMax = parseInt(widget.scalemax / 4);
            else if (value < parseInt(widget.scalemax / 2))
               neededScaleMax = parseInt(widget.scalemax / 2);
            else if (value < parseInt(widget.scalemax / 1.5))
               neededScaleMax = parseInt(widget.scalemax / 1.5);

            if (widgetDiv.data('scalemax') != neededScaleMax) {
               widgetDiv.data('scalemax', neededScaleMax);
					reInitMeter = true;
               // initMeter(key, widget, fact, neededScaleMax, sensor.value);
            }
         }

			let progressColor = getWidgetColor(widget, widget.colorConditionBar, widget.barcolor, value);

			if (widgetDiv.data('progressColor') != progressColor) {
            widgetDiv.data('progressColor', progressColor);
				reInitMeter = true;
			}

			if (reInitMeter)
            initMeter(key, widget, fact, widgetDiv.data('scalemax'), sensor.value);

         if (widget.widgettype == 6) {
            $("#widgetValue" + fact.type + fact.address).css('color', getWidgetColor(widget, widget.colorCondition, widget.color, value));
            $('#widgetValue' + fact.type + fact.address).html(value.toFixed(widget.unit == '%' ? 0 : 1) + ' ' + widget.unit);
         }

			let gauge = dashboardGauges[fact.type + fact.address];

         if (gauge != null)
            gauge.value = value;
         else
            console.log("Missing gauge instance for " + '#widget' + fact.type + fact.address);

         if (widget.showpeak != null && widget.showpeak)
            $("#peak" + fact.type + fact.address).text(sensor.peakmax != null ? sensor.peakmax.toFixed(2) + " " + widget.unit : "");
      }
      else
         console.log("Missing value for " + '#widget' + fact.type + fact.address);
   }
   else if (widget.widgettype == 14) {  // Special Symbol
      let div = document.getElementById('widget' + key);
      let l = document.getElementById('svg' + key);
      let v = document.getElementById('value' + key);
      let y = parseFloat($(div).data('yStart'));
      let fullPx = parseFloat($(div).data('heightStart'));
      let percent = sensor.value;
      if (widget.unit != '%')
         percent = sensor.value / (widget.scalemax / 100);
      let levelPx = Math.round((fullPx / 100) * percent);

      v.innerHTML = sensor.value + ' ' + widget.unit;
      l.setAttribute('y', fullPx - levelPx + y);
      l.setAttribute('height', levelPx);

      let ellipse = document.getElementById('svgEllipse' + key);

      if (ellipse != null)
         ellipse.setAttribute('cy', fullPx - levelPx + y);

      // console.log("value:", sensor.value, "levelPx:", levelPx);
   }
}

var filterExpression = null;
var addFilterString = '';

function addWidget()
{
   // console.log("add widget ..");

   let form = document.createElement("div");

   $(form)
      .append($('<div></div>')
              .append($('<span></span>')
                      .addClass('labelB2')
                      .html('Filter'))
              .append($('<span></span>')
                      .append($('<input></input>')
                              .attr('id', 'incFilter')
                              .attr('type', 'search')
                              .attr('placeholder', 'expression...')
                              .addClass('rounded-border input')
                              .val(addFilterString)
                                  .on('input', function() {updateSelection();})
                             )))
      .append($('<div></div>')
              .append($('<span></span>')
                      .addClass('labelB2')
                      .html('Widget'))
              .append($('<span></span>')
                      .append($('<select></select>')
                              .addClass('rounded-border input')
                              .attr('id', 'widgetKey')
                             )));

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Add Widget",
      open: function() {
         updateSelection();
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Ok': function () {
            let json = {};
            console.log("store widget");
            $('#widgetContainer > div').each(function () {
               let key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               json[key] = dashboards[actDashboard].widgets[key];
            });

            if ($("#widgetKey").val() == 'ALL') {
               for (let key in valueFacts) {
                  if (!valueFacts[key].state)   // use only active facts
                     continue;
                  if (filterExpression && !filterExpression.test(valueFacts[key].title) &&
                      !filterExpression.test(valueFacts[key].usrtitle) &&
                      !filterExpression.test(valueFacts[key].type))
                     continue;
                  if (dashboards[actDashboard].widgets[key] != null)
                     continue;
                  json[key] = "";
               }
            }
            else
               json[$("#widgetKey").val()] = "";

            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });

   function updateSelection() {
      let addrSpacer = -1;
      let addrTime = -1;
      $('#widgetKey').empty();
      // console.log("update selection: " + $('#incFilter').val());
      for (let key in dashboards[actDashboard].widgets) {
         n = parseInt(key.split(":")[1]);
         if (key.split(":")[0] == 'SPACER' && n > addrSpacer)
            addrSpacer = n;
         if (key.split(":")[0] == 'TIME' && n > addrTime)
            addrTime = n;
      }
      $('#widgetKey').append($('<option></option>')
                             .val('SPACER:0x' + (addrSpacer + 1).toString(16))
                             .html('Spacer'));
      $('#widgetKey').append($('<option></option>')
                             .val('TIME:0x' + (addrTime + 1).toString(16))
                             .html('Time'));

      let jArray = [];
      filterExpression = null;
      addFilterString = $('#incFilter').val();

      if ($('#incFilter').val() != '')
         filterExpression = new RegExp(addFilterString);

      for (let key in valueFacts) {
         if (!valueFacts[key].state)   // use only active facts here
            continue;
         // if (dashboards[actDashboard].widgets[key] != null)
         //    continue;
         if (filterExpression && !filterExpression.test(valueFacts[key].title) &&
             !filterExpression.test(valueFacts[key].usrtitle) &&
             !filterExpression.test(valueFacts[key].type))
            continue;

         jArray.push([key, valueFacts[key]]);
      }
      jArray.push(['ALL', { 'title': '- ALLE -'}]);
      jArray.sort(function(a, b) {
         let A = (a[1].usrtitle ? a[1].usrtitle : a[1].title).toLowerCase();
         let B = (b[1].usrtitle ? b[1].usrtitle : b[1].title).toLowerCase();
         if (B > A) return -1;
         if (A > B) return  1;
         return 0;

      });
      for (let i = 0; i < jArray.length; i++) {
         let key = jArray[i][0];
         let title = jArray[i][1].usrtitle ? jArray[i][1].usrtitle : jArray[i][1].title;

         if (valueFacts[key] != null)
            title += ' / ' + valueFacts[key].type;
         if (jArray[i][1].unit != null && jArray[i][1].unit != '')
            title += ' [' + jArray[i][1].unit + ']';

         $('#widgetKey').append($('<option></option>')
                                .val(jArray[i][0])
                                .html(title));
      }
   }
}

function toggleChartDialog(type, address)
{
   let dialog = document.querySelector('dialog');
   dialog.style.position = 'fixed';
   console.log("chart for " + type + address);

   if (type != "" && !dialog.hasAttribute('open')) {
      let canvas = document.querySelector("#chartDialog");
      canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
      chartDialogSensor = type + address;

      // show the dialog

      dialog.setAttribute('open', 'open');

      let jsonRequest = {};
      let key = toKey(type, address);
      let widget = dashboards[actDashboard].widgets[key];

      if (!widget.range)
         widget.range = 1;

      prepareChartRequest(jsonRequest, toKey(type, address), 0, widget.range, "chartdialog");
      socket.send({ "event" : "chartdata", "object" : jsonRequest });

      // EventListener für ESC-Taste

      document.addEventListener('keydown', closeChartDialog);

      // only hide the background *after* you've moved focus out of
      //   the content that will be "hidden"

      let div = document.createElement('div');
      div.id = 'backdrop';
      document.body.appendChild(div);
   }
   else {
      chartDialogSensor = "";
      document.removeEventListener('keydown', closeChartDialog);
      dialog.removeAttribute('open');
      let div = document.querySelector('#backdrop');
      div.parentNode.removeChild(div);
   }
}

function closeChartDialog(event)
{
   if (event.keyCode == 27)
      toggleChartDialog("", 0);
}

// ---------------------------------
// gauge stuff ...

function polar_to_cartesian(cx, cy, radius, angle)
{
   let radians = (angle - 90) * Math.PI / 180.0;
   return [Math.round((cx + (radius * Math.cos(radians))) * 100) / 100, Math.round((cy + (radius * Math.sin(radians))) * 100) / 100];
};

function svg_circle_arc_path(x, y, radius, start_angle, end_angle)
{
   let start_xy = polar_to_cartesian(x, y, radius, end_angle);
   let end_xy = polar_to_cartesian(x, y, radius, start_angle);
   return "M " + start_xy[0] + " " + start_xy[1] + " A " + radius + " " + radius + " 0 0 0 " + end_xy[0] + " " + end_xy[1];
};

// ---------------------------------
// drag&drop stuff ...


function dragDashboard(ev)
{
   // console.log("drag: " + ev.target.getAttribute('id'));
   ev.originalEvent.dataTransfer.setData("source", ev.target.getAttribute('id'));
}

function dragOverDashboard(ev)
{
   event.preventDefault();
   let target = ev.target;
   target.setAttribute('drop-active', true);
}

function dragLeaveDashboard(ev)
{
   event.preventDefault();
   let target = ev.target;
   target.removeAttribute('drop-active', true);
}

function dropDashboard(ev)
{
   ev.preventDefault();
   let target = ev.target;
   target.removeAttribute('drop-active', true);
   // console.log("drop: " + ev.target.getAttribute('id'));

   let source = document.getElementById(ev.originalEvent.dataTransfer.getData("source"));

   if (source.dataset.dragtype == 'widget') {
      let key = source.getAttribute('id').substring(source.getAttribute('id').indexOf("_") + 1);
      // console.log("drag widget " + key + " from dashboard " + parseInt(actDashboard) + " to " + parseInt(target.getAttribute('id')));
      source.remove();
      socket.send({ "event" : "storedashboards", "object" :
                    { 'action' : 'move',
                      'key' : key,
                      'from' : parseInt(actDashboard),
                      'to': parseInt(target.getAttribute('id').substring(4)) } });

      return;
   }

   if (source.dataset.dragtype != 'dashboard') {
      console.log("drag source not a dashboard");
      return;
   }

   target.after(source);

   // -> The order for json objects follows a certain set of rules since ES2015,
   //    but it does not (always) follow the insertion order. Simply,
   //    the iteration order is a combination of the insertion order for strings keys,
   //    and ascending order for number keys
   // terefore we use a array instead

   let jOrder = [];
   let i = 0;

   $('#dashboardMenu > button').each(function () {
      if ($(this).attr('data-dragtype') == 'dashboard') {
         let did = $(this).attr('id');
         console.log("add: " + did);
         jOrder[i++] = parseInt(did.substring(4));
      }
   });

   socket.send({ "event" : "storedashboards", "object" : { 'action' : 'order', 'order' : jOrder } });
   socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
}

// widgets

function dragWidget(ev)
{
   console.log("drag: " + ev.target.getAttribute('id'));
   ev.dataTransfer.setData("source", ev.target.getAttribute('id'));
}

function dragOver(ev)
{
   event.preventDefault();
   let target = ev.target;
   while (target) {
      if (target.dataset.droppoint)
         break;
      target = target.parentElement;
   }
   target.setAttribute('drop-active', true);
}

function dragLeave(ev)
{
   event.preventDefault();
   let target = ev.target;
   while (target) {
      if (target.dataset.droppoint)
         break;
      target = target.parentElement;
   }
   target.removeAttribute('drop-active', true);
}

function dropWidget(ev)
{
   ev.preventDefault();
   let target = ev.target;
   while (target) {
      if (target.dataset.droppoint)
         break;
      target = target.parentElement;
   }
   target.removeAttribute('drop-active', true);

   let source = document.getElementById(ev.dataTransfer.getData("source"));

   if (source.dataset.dragtype != 'widget') {
      console.log("drag source not a widget");
      return;
   }

   console.log("drop element: " + source.getAttribute('id') + ' on ' + target.getAttribute('id'));
   target.after(source);

   let json = {};
   $('#widgetContainer > div').each(function () {
      let key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
      json[key] = dashboards[actDashboard].widgets[key];
   });

   socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
   socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
}
