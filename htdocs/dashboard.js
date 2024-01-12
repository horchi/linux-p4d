/*
 *  dashboard.js
 *
 *  (c) 2020-2021 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var widgetWidthBase = null;
var widgetHeightBase = null;
var weatherData = null;
var weatherInterval = null;
var moseDownOn = { 'object' : null };
var lightClickTimeout = null;
var lightClickPosX = null;
var lightClickPosY = null;

const symbolColorDefault = '#ffffff';
const symbolOnColorDefault = '#059eeb';

function initDashboard(update = false)
{
   // console.log("initDashboard " + JSON.stringify(allSensors, undefined, 4));

   if (allSensors == null) {
      console.log("Fatal: Missing widgets!");
      return;
   }

   if (!update) {
      $('#container').removeClass('hidden');
      $('#dashboardMenu').removeClass('hidden');
      $('#dashboardMenu').empty();
   }

   if (!Object.keys(dashboards).length)
      setupMode = true;

   // setupMode = true;  // to debug

   if (setupMode) {
      $('#dashboardMenu').append($('<button></button>')
                                 .addClass('rounded-border buttonDashboardTool')
                                 .addClass('mdi mdi-close-outline')
                                 .attr('title', 'Setup beenden')
                                 .click(function() { setupDashboard(); })
                                );
   }

   var jDashboards = [];
   for (var did in dashboards) {
      if (!dashboardGroup || dashboards[did].group == dashboardGroup)
         jDashboards.push([dashboards[did].order, did]);
   }
   jDashboards.sort();

   for (var i = 0; i < jDashboards.length; i++) {
      var did = jDashboards[i][1];
      if (actDashboard < 0)
         actDashboard = did;

      if (kioskBackTime > 0 && actDashboard != jDashboards[0][1]) {
         setTimeout(function() {
            actDashboard = jDashboards[0][1];
            initDashboard(false);
         }, kioskBackTime * 1000);
      }

      var classes = dashboards[did].symbol != '' ? dashboards[did].symbol.replace(':', ' ') : '';

      if (dashboards[actDashboard] == dashboards[did])
         classes += ' buttonDashboardActive';

      $('#dashboardMenu').append($('<button></button>')
                                 .addClass('rounded-border buttonDashboard widgetDropZone')
                                 .addClass(classes)
                                 .attr('id', did)
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

      if (kioskMode)
         $('#'+did).css('font-size', '-webkit-xxx-large');

      if (setupMode && did == actDashboard)
         $('#dashboardMenu').append($('<button></button>')
                                    .addClass('rounded-border buttonDashboardTool')
                                    .addClass('mdi mdi-lead-pencil')  //  mdi-draw-pen
                                    .click({"id" : did}, function(event) { dashboardSetup(event.data.id); })
                                   );
   }

   // additional elements in setup mode

   if (setupMode) {
      $('#dashboardMenu')
         .append($('<div></div>')
                 .css('float', 'right')
                 .append($('<button></button>')
                         .addClass('rounded-border buttonDashboardTool')
                         .addClass('mdi mdi-shape-plus')
                         .attr('title', 'widget zum dashboard hinzufügen')
                         .css('font-size', 'x-large')
                         .click(function() { addWidget(); })
                        )
                 .append($('<input></input>')
                         .addClass('rounded-border input')
                         .css('margin', '15px,15px,15px,0px')
                         .attr('id', 'newDashboard')
                         .attr('title', 'Neues Dashboard')
                        )
                 .append($('<button></button>')
                         .addClass('rounded-border buttonDashboardTool')
                         .addClass('mdi mdi-file-plus-outline')
                         .attr('title', 'dashboard hinzufügen')
                         .click(function() { newDashboard(); })
                        )
                );
   }

   // widgets

   document.getElementById("container").innerHTML = '<div id="widgetContainer" class="widgetContainer"></div>';

   if (dashboards[actDashboard] != null) {
      for (var key in dashboards[actDashboard].widgets) {
         initWidget(key, dashboards[actDashboard].widgets[key]);
         updateWidget(allSensors[key], true, dashboards[actDashboard].widgets[key]);
      }
   }

   initLightColorDialog();

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - 15);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - 15);
   };
}

function newDashboard()
{
   if (!$('#newDashboard').length)
      return;

   var name = $('#newDashboard').val();
   $('#newDashboard').val('');

   if (name != '') {
      console.log("new dashboard: " + name);
      socket.send({ "event" : "storedashboards", "object" : { [-1] : { 'title' : name, 'widgets' : {} } } });
      socket.send({ "event" : "forcerefresh", "object" : {} });
   }
}

var keyTimeout = null;

function initWidget(key, widget, fact)
{
   // console.log("Widget " + key + ': '+ JSON.stringify(widget));

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

   // console.log("initWidget " + key + " : " + (fact ? fact.name : ''));
   // console.log("fact: " + JSON.stringify(fact, undefined, 4));
   // console.log("widget: " + JSON.stringify(widget, undefined, 4));

   if (fact && widget.unit == '')
      widget.unit = fact.unit;

   const marginPadding = 8;
   var root = document.getElementById("widgetContainer");
   var id = 'div_' + key;
   var elem = document.getElementById(id);
   if (elem == null) {
      // console.log("element '" + id + "' not found, creating");
      elem = document.createElement("div");
      root.appendChild(elem);
      elem.setAttribute('id', id);
      if (!widgetHeightBase)
         widgetHeightBase = elem.clientHeight;
      var eHeight = parseInt(elem.style.height);
      var useKioskHeight = kioskMode == 1 || kioskMode == 2;
      if (!useKioskHeight && dashboards[actDashboard].options && dashboards[actDashboard].options.heightfactor)
         eHeight = widgetHeightBase * dashboards[actDashboard].options.heightfactor;
      if (useKioskHeight && dashboards[actDashboard].options && dashboards[actDashboard].options.heightfactorKiosk)
         eHeight = widgetHeightBase * dashboards[actDashboard].options.heightfactorKiosk;

      // widget.heightfactor -> to be implementen (nur vorbereitet)

      if (widget.heightfactor != null && widget.heightfactor != 1)
         eHeight = eHeight * widget.heightfactor + ((widget.heightfactor-1) * marginPadding-1);

      elem.style.height = eHeight + 'px';
   }

   elem.innerHTML = "";

   if (widgetWidthBase == null)
      widgetWidthBase = elem.clientWidth;

   // console.log("clientWidth: " + elem.clientWidth + ' : ' + widgetWidthBase);

   elem.style.width = widgetWidthBase * widget.widthfactor + ((widget.widthfactor-1) * marginPadding-1) + 'px';

   if (setupMode && (widget.widgettype < 900 || widget.widgettype == null)) {
      elem.setAttribute('draggable', true);
      elem.dataset.droppoint = true;
      elem.dataset.dragtype = 'widget';
      elem.addEventListener('dragstart', function(event) {dragWidget(event)}, false);
      elem.addEventListener('dragover', function(event) {dragOver(event)}, false);
      elem.addEventListener('dragleave', function(event) {dragLeave(event)}, false);
      elem.addEventListener('drop', function(event) {dropWidget(event)}, false);
   }

   var titleClass = '';
   var title = setupMode ? ' ' : ' ';
   if (fact != null)
      title += fact.usrtitle && fact.usrtitle != '' ? fact.usrtitle : fact.title;

   if (!setupMode && widget.unit == '°C')
      titleClass = 'mdi mdi-thermometer';
   else if (!setupMode && (widget.unit == 'hPa' || widget.unit == 'A' || widget.unit == 'mA' ||
                           widget.unit == 'W' || widget.unit == 'V' || widget.unit == 'Ah'))
      titleClass = 'mdi mdi-gauge';
   else if (!setupMode && widget.unit == '%')
      titleClass = 'mdi mdi-label-percent-outline';
   else if (!setupMode && widget.unit == 'xxxx')  // can't detect if '%' is humidity
      titleClass = 'mdi mdi-water-percent';
   if (!setupMode && widget.unit == 'l')
      titleClass = 'mdi mdi-water';

   if (!widget.color)
      widget.color = 'white';

   $(document).on({'mouseup touchend' : function(e){
      if (!moseDownOn.object)
         return;
      e.preventDefault();
      var scale = parseInt((e.pageX - $(moseDownOn.div).position().left) / ($(moseDownOn.div).innerWidth() / 100));
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
      var scale = parseInt((e.pageX - $(moseDownOn.div).position().left) / ($(moseDownOn.div).innerWidth() / 100));
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
                    .addClass('rounded-border')
                    .addClass('widget-main')
                    .append($('<img></img>')
                            .attr('id', 'widget' + fact.type + fact.address)
                            .attr('draggable', false)
                            .css('user-select', 'none')));
//            .append($('<div></div>')
//                    .attr('id', 'progress' + fact.type + fact.address)
//                    .addClass('widget-progress')
//                    .css('user-select', 'none')
//                    .on({'mousedown touchstart' : function(e){
//                       e.preventDefault();
//                       moseDownOn.object = $('#progressBar' + fact.type + fact.address);
//                       moseDownOn.fact = fact;
//                       moseDownOn.div = $(this);
//                       console.log("mousedown on " + moseDownOn.object.attr('id'));
//                    }})
//                    .append($('<div></div>')
//                            .attr('id', 'progressBar' + fact.type + fact.address)
//                            .css('user-select', 'none')
//                            .addClass('progress-bar')));

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
         var eTitle = document.createElement("div");
         var cls = setupMode ? 'mdi mdi-lead-pencil widget-edit' : '';
         eTitle.className = "widget-title " + cls + ' ' + titleClass;
         eTitle.innerHTML = title;
         eTitle.addEventListener("click", function(event) {titleClick(event.ctrlKey, key)}, false);
         elem.appendChild(eTitle);

         var ePeak = document.createElement("div");
         ePeak.setAttribute('id', 'peak' + fact.type + fact.address);
         ePeak.className = "chart-peak";
         elem.appendChild(ePeak);

         var eValue = document.createElement("div");
         eValue.setAttribute('id', 'value' + fact.type + fact.address);
         eValue.className = "chart-value";
         eValue.style.color = widget.color;
         elem.appendChild(eValue);

         var eChart = document.createElement("div");
         eChart.className = "chart-canvas-container";
         var cFact = fact;
         if (!setupMode && fact.record)
            eChart.setAttribute("onclick", "toggleChartDialog('" + cFact.type + "'," + cFact.address + ")");
         elem.appendChild(eChart);

         var eCanvas = document.createElement("canvas");
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
                    .click(function() {
                       var cFact = fact;
                       if (!setupMode && fact.record)
                          toggleChartDialog(cFact.type, cFact.address, key);}))
            .append($('<div></div>')
                    .attr('id', 'peak' + fact.type + fact.address)
                    .css('user-select', 'none')
                    .addClass('chart-peak'));

//         var cFact = fact;
//         if (!setupMode && fact.record)
//            $(elem).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});

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
         var cFact = fact;
         if (!setupMode && fact.record)
            $(elem).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});

         var divId = '#svgDiv' + fact.type + fact.address;
         $(divId).html($(divId).html());  // redraw to activate the SVG !!
         break;
      }

      case 5:            // Meter
      case 6: {          // MeterLevel
         var radial = widget.widgettype == 5;
         elem.className = radial ? "widgetMeter" : "widgetMeterLinear";
         elem.className += " widgetDropZone";
         var eTitle = document.createElement("div");
         eTitle.className = "widget-title " + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : titleClass);
         eTitle.addEventListener("click", function(event) {titleClick(event.ctrlKey, key)}, false);
         eTitle.innerHTML = title;
         elem.appendChild(eTitle);

         var main = document.createElement("div");
         main.className = radial ? "widget-main-meter" : "widget-main-meter-lin";
         elem.appendChild(main);
         var canvas = document.createElement('canvas');
         main.appendChild(canvas);
         canvas.setAttribute('id', 'widget' + fact.type + fact.address);
         var cFact = fact;
         if (!setupMode && fact.record)
            $(canvas).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});

         if (!radial) {
            var value = document.createElement("div");
            value.setAttribute('id', 'widgetValue' + fact.type + fact.address);
            value.className = "widget-main-value-lin";
            elem.appendChild(value);
            $("#widgetValue" + fact.type + fact.address).css('color', widget.color);
            var ePeak = document.createElement("div");
            ePeak.setAttribute('id', 'peak' + fact.type + fact.address);
            ePeak.className = "widget-main-peak-lin";
            elem.appendChild(ePeak);
         }

         if (widget.scalemin == null)
            widget.scalemin = 0;

         var ticks = [];
         var scaleRange = widget.scalemax - widget.scalemin;
         var stepWidth = widget.scalestep != null ? widget.scalestep : 0;

         if (!stepWidth) {
            var steps = 10;
            if (scaleRange <= 100)
               steps = scaleRange % 10 == 0 ? 10 : scaleRange % 5 == 0 ? 5 : scaleRange;
            if (steps < 10)
               steps = 10;
            if (!radial && widget.unit == '%')
               steps = 4;
            stepWidth = scaleRange / steps;
         }

         if (stepWidth <= 0) stepWidth = 1;
         stepWidth = Math.round(stepWidth*10) / 10;
         var steps = -1;
         for (var step = widget.scalemin; step.toFixed(2) <= widget.scalemax; step += stepWidth) {
            ticks.push(step % 1 ? parseFloat(step).toFixed(1) : parseInt(step));
            steps++;
         }

         var scalemax = widget.scalemin + stepWidth * steps; // cals real scale max based on stepWidth and step count!
         var highlights = {};
         var critmin = (widget.critmin == null || widget.critmin == -1) ? widget.scalemin : widget.critmin;
         var critmax = (widget.critmax == null || widget.critmax == -1) ? scalemax : widget.critmax;
         var minColor = widget.unit == '°C' ? 'blue' : 'rgba(255,0,0,.6)';

         highlights = [
            { from: widget.scalemin, to: critmin,  color: minColor },
            { from: critmin,         to: critmax,  color: 'rgba(0,255,0,.6)' },
            { from: critmax,         to: scalemax, color: 'rgba(255,0,0,.6)' }
         ];

         // console.log("widget: " + JSON.stringify(widget, undefined, 4));
         // console.log("ticks: " + ticks + " range; " + scaleRange);
         // console.log("highlights: " + JSON.stringify(highlights, undefined, 4));

         options = {
            renderTo: 'widget' + fact.type + fact.address,
            units: radial ? widget.unit : '',
            // title: radial ? false : widget.unit
            minValue: widget.scalemin,
            maxValue: scalemax,
            majorTicks: ticks,
            minorTicks: 5,
            strokeTicks: false,
            highlights: highlights,
            highlightsWidth: radial ? 8 : 6,

            colorPlate: radial ? '#2177AD' : 'rgba(0,0,0,0)',
            colorBar: colorStyle.getPropertyValue('--scale'),        // 'gray',
            colorBarProgress: widget.unit == '%' ? 'blue' : 'red',
            colorBarStroke: 'red',
            colorMajorTicks: colorStyle.getPropertyValue('--scale'), // '#f5f5f5',
            colorMinorTicks: colorStyle.getPropertyValue('--scale'), // '#ddd',
            colorTitle: colorStyle.getPropertyValue('--scaleText'),
            colorUnits: colorStyle.getPropertyValue('--scaleText'),  // '#ccc',
            colorNumbers: colorStyle.getPropertyValue('--scaleText'),
            colorNeedle: 'rgba(240, 128, 128, 1)',
            colorNeedleEnd: 'rgba(255, 160, 122, .9)',

            fontNumbersSize: radial ? 34 : (onSmalDevice ? 45 : 45),
            fontUnitsSize: 45,
            fontUnitsWeight: 'bold',
            borderOuterWidth: 0,
            borderMiddleWidth: 0,
            borderInnerWidth: 0,
            borderShadowWidth: 0,
            needle: radial,
            fontValueWeight: 'bold',
            valueBox: radial,
            valueInt: 0,
            valueDec: 2,
            colorValueText: colorStyle.getPropertyValue('--scale'),
            colorValueBoxBackground: 'transparent',
            // colorValueBoxBackground: '#2177AD',
            valueBoxStroke: 0,
            fontValueSize: 45,
            valueTextShadow: false,
            animationRule: 'bounce',
            animationDuration: 500,
            barWidth: radial ? 0 : (widget.unit == '%' ? 10 : 7),
            numbersMargin: 0,

            // linear gauge specials

            barBeginCircle: widget.unit == '°C' ? 15 : 0,
            tickSide: 'left',
            needleSide: 'right',
            numberSide: 'left',
            colorPlate: 'transparent'
         };

         var gauge = null;

         if (radial)
            gauge = new RadialGauge(options);
         else
            gauge = new LinearGauge(options);

         gauge.draw();
         $('#widget' + fact.type + fact.address).data('gauge', gauge);

         break;
      }

      case 7: {    // 7 (PlainText)
         $(elem).addClass('widgetPlain widgetDropZone');
         if (setupMode)
            $(elem).append($('<div></div>')
                           .addClass('widget-title ' + (setupMode ? 'mdi mdi-lead-pencil widget-edit' : ''))
                           .addClass(titleClass)
                           .css('user-select', 'none')
                           .click(function(event) {titleClick(event.ctrlKey, key);})
                           .html(title));
         var wFact = fact;
         $(elem).append($('<div></div>')
                        .attr('id', 'widget' + fact.type + fact.address)
                        .addClass(fact.type == 'WEA' ? 'widget-weather' : 'widget-value')
                        .css('user-select', 'none')
                        .css('color', widget.color)
                        .css('height', 'inherit')
                        .click(function() { if (wFact.type == 'WEA') weatherForecast(); }));
         break;
      }

      case 8: {     // 8 (Choice)
         $(elem)
            .addClass("widget widgetDropZone")
            .css('cursor', 'pointer')
            .click(function() {
               socket.send({"event": "toggleio", "object": {"address": fact.address, "type": fact.type, "action": "toggle"}});
            })
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
                    .css('color', widget.color));
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
//            .append($('<div></div>')
//                    .attr('id', 'progress' + fact.type + fact.address)
//                    .addClass('widget-progress')
//                    .css('user-select', 'none')
//                    .on({'mousedown touchstart' : function(e){
//                       e.preventDefault();
//                       moseDownOn.object = $('#progressBar' + fact.type + fact.address);
//                       moseDownOn.fact = fact;
//                       moseDownOn.div = $(this);
//                       console.log("mousedown on " + moseDownOn.object.attr('id'));
//                    }})
//                    .append($('<div></div>')
//                            .attr('id', 'progressBar' + fact.type + fact.address)
//                            .css('user-select', 'none')
//                            .addClass('progress-bar')))
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
                        .css('height', 'inherit')
                        .css('color', widget.color)
                        .css('user-select', 'none')
                        .html(getTimeHtml()));
         setInterval(function() {
            var timeId = '#widget'+key.replace(':', '\\:');
            $(timeId).html(getTimeHtml());
         }, 1*1000);

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

         var cFact = fact;
         if (!setupMode && fact.record)
            $(elem).click(function() {toggleChartDialog(cFact.type, cFact.address, key);});
         break;
      }
   }
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

   var options = {
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

      var fact = valueFacts[$(this).data('key')];
      var hue = parseInt($(this).wheelColorPicker('getColor').h * 360);
      var sat = parseInt($(this).wheelColorPicker('getColor').s * 100);
      var bri = parseInt($(this).wheelColorPicker('getColor').v * 100);

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
   var posX = ($('#container').innerWidth() - $('#lightColorDiv').outerWidth()) / 2;
   var posY = ($('#container').innerHeight() - $('#lightColorDiv').outerHeight()) / 2;
   var sensor = allSensors[key];

   $('#lightColor').data('key', key);
   $('#lightColorDiv').css('left', posX + 'px');
   $('#lightColorDiv').css('top', posY + 'px');
   $('#lightColorDiv').css('display', 'block');
   $('#lightColor').wheelColorPicker('setColor', { 'h': sensor.hue/360.0, 's': sensor.sat/100.0, 'v': sensor.score/100.0 });
}

function getTimeHtml()
{
   var now = new Date();

   // calc daytime every 60 seconds

   if (!daytimeCalcAt || now.getTime() >= daytimeCalcAt.getTime()+60000){
      daytimeCalcAt = now;
      var sunset = new Date().sunset(config.latitude.replace(',', '.'), config.longitude.replace(',', '.'));
      var sunrise = new Date().sunrise(config.latitude.replace(',', '.'), config.longitude.replace(',', '.'));
      isDaytime = daytimeCalcAt > sunrise && daytimeCalcAt < sunset;
      // console.log("isDaytime: " + isDaytime + ' : ' + sunrise + ' : '+ sunset);
   }

   var cls = isDaytime ? 'mdi mdi-weather-sunny' : 'mdi mdi-weather-night';

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

   var html = '';

   if (symbolView) {
      var wIconRef = 'img/weather/' + weather.icon + '@4x.png';
      if (!images.find(img => img == wIconRef))
         wIconRef = 'http://openweathermap.org/img/wn/' + weather.icon + '@4x.png';
      html += '<div style="display:block;font-weight:bold;height:75%;padding-left:5px;"><span>' + weather.detail + '</span><img style="height:100%" src="' + wIconRef + '"></img></div>';
      html += '<div style="display:flex;justify-content:space-between;padding-top:3px;">';
      html += ' <span style="color:orange;font-weight:bold;padding-left:5px;">' + weather.temp.toFixed(1) + '°C</span>';
      html += ' <span class="mdi mdi-walk" style="color:orange;font-weight:bold;font-size:smaller;">' + weather.tempfeels.toFixed(1) + '</span>';
      html += '</div>';

   }
   else {
      var wIconRef = 'img/weather/' + weather.icon + '.png';
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
   var lastDay = '';
   var form = document.createElement("div");

   $(form).addClass('rounded-border weatherForecast');
   $(form).attr('tabindex', 0);
   $(form).click(function() { $(form).remove(); });
   $('#container').append(form);

   var showExtras = $(form).outerWidth() > 580;
   var html = '<div class="rounded-border" style="justify-content:center;font-weight:bold;background-color:#2f2f2fd1;">' + weatherData.city + '</div>';

   for (var i = 0; i < weatherData.forecasts.length; i++) {
      var weather = weatherData.forecasts[i];
      var day = moment(weather.time*1000).format('dddd Do MMMM');
      var time = moment(weather.time*1000).format('HH:00');
      var wIconRef = 'img/weather/' + weather.icon + '.png';

      if (!images.find(img => img == wIconRef))
         wIconRef = 'http://openweathermap.org/img/wn/' + weather.icon + '.png';

      if (day != lastDay) {
         lastDay = day;
         html += '<div class="rounded-border" style="background-color:#2f2f2fd1;">' + day + '</div>';
      }

      var tempColor = weather.temp < 0 ? '#2c99eb' : (weather.temp > 20 ? 'red' : 'white');

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
   let hasMode = fact && (fact.type == 'DO' || fact.type == 'SC');

   // console.log("titleClick: ", ctrlKey, key);

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
      let widget = dashboards[actDashboard].widgets[key];
      let now = new Date();
      var last = new Date(sensor.last * 1000);
      var form = document.createElement("div");

      $(form).append($('<div></div>')
                     .css('z-index', '9999')
                     .css('minWidth', '40vh')

                     .append($('<div></div>')
                             .css('display', 'flex')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('ID'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(key + ' (' + parseInt(key.split(":")[1]) + ')')
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Title'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(fact.title)
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('User Title'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(fact.usrtitle)
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
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
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Peak'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .attr('id', 'dlgPeak')
                                             .addClass('rounded-border')
                                             .html(sensor.peak + ' ' + widget.unit)
                                            )))
                     .append($('<div></div>')
                             .css('display', 'flex')
                             .append($('<span></span>')
                                     .css('width', '30%')
                                     .css('text-align', 'end')
                                     .css('margin-right', '10px')
                                     .html('Last Update'))
                             .append($('<span></span>')
                                     .append($('<div></div>')
                                             .addClass('rounded-border')
                                             .html(last.toLocaleString('de-DE') + ' (' + Math.round((now-last)/1000) + 's)'))
                                    ))
                    );


      let btns = {};

      if (sensor.peak) {
         btns['Reset Peak'] = function() {
            $('#dlgPeak').html('-');
            socket.send({ "event" : "command", "object" : { "what" : 'peak', "type": fact.type, "address": fact.address } });
            let sensor = allSensors[key];
            sensor.peak = null;
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
      for (var key in widgets) {
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

   var key = toKey(sensor.type, sensor.address);
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

   var widgetDiv = $('#div_' + key.replace(':', '\\:'));

   if (widget.range > 2)
   {
      console.log("widget.range", key, widget.range);
      sensor.valid = true;
   }

   widgetDiv.css('opacity', sensor.valid ? '100%' : '45%');
   // $('#div_' + key.replace(':', '\\:')).css('opacity', sensor.valid ? '100%' : '25%');

   if (widget.widgettype == 0 || widget.widgettype == 9 || widget.widgettype == 12)         // Symbol, Symbol-Value, Symbol-Text
   {
      // console.log("sensor: ", JSON.stringify(sensor));
      var state = fact.type != 'HMB' ? sensor.value != 0 : sensor.value == 100;
      var image = '';
      var classes = '';

      if (sensor.image != null)
         image = sensor.image;
      else if (state && widget.symbolOn && widget.symbolOn != '') {
         classes = widget.symbolOn.replace(':', ' ');
         image = '';
         $("#widget" + fact.type + fact.address).remove();
      }
      else if (!state && widget.symbol && widget.symbol != '') {
         classes = widget.symbol.replace(':', ' ');
         image = '';
         $("#widget" + fact.type + fact.address).remove();
      }
      else if (widget.symbol && widget.symbol != '') {
         classes = widget.symbol.replace(':', ' ');
         image = '';
         $("#widget" + fact.type + fact.address).remove();
      }
      else
         image = state ? widget.imgon : widget.imgoff;

      if (image != '')
         $("#widget" + fact.type + fact.address).attr("src", image);
      else {
         $("#button" + fact.type + fact.address).removeClass();
         $("#button" + fact.type + fact.address).addClass('widget-main');
         $("#button" + fact.type + fact.address).addClass(classes);

         var fontSize = Math.min(widgetDiv.height(), widgetDiv.width()) * 0.6;
         $("#button" + fact.type + fact.address).css('font-size', fontSize + 'px');
      }

      $("#button" + fact.type + fact.address).css('background-color', sensor.working ? '#7878787878' : 'transparent');

      // $('#div_'+key.replace(':', '\\:')).css('background-color', (sensor.options == 3 && sensor.mode == 'manual') ? '#a27373' : '');
      widgetDiv.css('background-color', (sensor.options == 3 && sensor.mode == 'manual') ? '#a27373' : '');
      widget.colorOn = widget.colorOn == null ? symbolOnColorDefault : widget.colorOn;

      if (sensor.hue)
         widget.colorOn = tinycolor({ 'h': sensor.hue, 's': sensor.sat, 'v': sensor.score }).toHslString();

      widget.color = widget.color == null ? symbolColorDefault : widget.color;

      // console.log("set color to: : ", widget.colorOn);
      $("#button" + fact.type + fact.address).css('color', state ? widget.colorOn : widget.color);

      if (widget.widgettype == 9)
         $("#value" + fact.type + fact.address).text(sensor.value.toFixed(widget.unit=="%" ? 0 : 2) + (widget.unit!="" ? " " : "") + widget.unit);
      else if (widget.widgettype == 12 && sensor.text != null)
         $("#value" + fact.type + fact.address).text(sensor.text.replace(/(?:\r\n|\r|\n)/g, '<br>'));

//      var prs = $('#progressBar' + fact.type + fact.address);
//      $('#progress' + fact.type + fact.address).css('display', fact.options & 0x02 && sensor.value ? 'block' : 'none');
//
//      if (sensor.score && prs != null)
//         $(prs).css('width', sensor.score + '%');
//
//      if (sensor.mode == "auto" && sensor.next > 0) {
//         var pWidth = 100;
//         var s = sensor;
//         var id = fact.type + fact.address;
//         var iid = setInterval(progress, 200);
//
//         function progress() {
//            if (pWidth <= 0) {
//               clearInterval(iid);
//            } else {
//               var d = new Date();
//               pWidth = 100 - ((d/1000 - s.last) / ((s.next - s.last) / 100));
//               document.getElementById("progressBar" + id).style.width = pWidth + "%";
//            }
//         }
//      }
   }
   else if (widget.widgettype == 1 || widget.widgettype == 13)    // Chart
   {
      if (widget.showpeak != null && widget.showpeak)
         $("#peak" + fact.type + fact.address).text(sensor.peak != null ? sensor.peak.toFixed(2) + " " + widget.unit : "");

      $("#value" + fact.type + fact.address).text(sensor.value.toFixed(2) + " " + widget.unit);

      if (refresh) {
         var jsonRequest = {};
         if (!widget.range)
            widget.range = 1;
         prepareChartRequest(jsonRequest, toKey(fact.type, fact.address), 0, widget.range, widget.widgettype == 13 ? "chartwidgetbar" : "chartwidget");
         socket.send({ "event" : "chartdata", "object" : jsonRequest });
      }
   }
   else if (widget.widgettype == 2 || widget.widgettype == 7 || widget.widgettype == 8)    // Text, PlainText, Choice
   {
      if (sensor.type == 'WEA' && sensor.text != null) {
         var bigView = false;
         var wfact = fact;
         weatherData = JSON.parse(sensor.text);
         // console.log("weather" + JSON.stringify(weatherData, undefined, 4));
         $("#widget" + wfact.type + wfact.address).html(getWeatherHtml(bigView, wfact, weatherData));
         if (weatherInterval)
            clearInterval(weatherInterval);
         if (weatherData && config.toggleWeatherView != 0) {
            weatherInterval = setInterval(function() {
               bigView = !bigView;
               $("#widget" + wfact.type + wfact.address).html(getWeatherHtml(bigView, wfact, weatherData));
            }, 5*1000);
         }
      }
      else if (sensor.text != null) {
         var text = sensor.text.replace(/(?:\r\n|\r|\n)/g, '<br>');
         $("#widget" + fact.type + fact.address).html(text);
      }
   }
   else if (widget.widgettype == 3)      // plain value
   {
      $("#widget" + fact.type + fact.address).html(sensor.value + " " + widget.unit);
      if (widget.showpeak != null && widget.showpeak)
         $("#peak" + fact.type + fact.address).text(sensor.peak != null ? sensor.peak.toFixed(2) + " " + widget.unit : "");
   }
   else if (widget.widgettype == 4)      // Gauge
   {
      var value = sensor.value.toFixed(2);
      var scaleMax = !widget.scalemax || widget.unit == '%' ? 100 : widget.scalemax.toFixed(0);
      var scaleMin = value >= 0 ? "0" : Math.ceil(value / 5) * 5 - 5;
      var _peak = sensor.peak != null ? sensor.peak : 0;
      if (scaleMax < Math.ceil(value))
         scaleMax = value;
      if (widget.showpeak != null && widget.showpeak && scaleMax < Math.ceil(_peak))
         scaleMax = _peak.toFixed(0);
      $("#sMin" + fact.type + fact.address).text(scaleMin);
      $("#sMax" + fact.type + fact.address).text(scaleMax);
      $("#value" + fact.type + fact.address).text(value + " " + widget.unit);
      var ratio = (value - scaleMin) / (scaleMax - scaleMin);
      var peak = (_peak.toFixed(2) - scaleMin) / (scaleMax - scaleMin);

      $("#pb" + fact.type + fact.address).attr("d", "M 950 500 A 450 450 0 0 0 50 500");
      $("#pv" + fact.type + fact.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, -90, ratio * 180.0 - 90));
      if (widget.showpeak != null && widget.showpeak)
         $("#pp" + fact.type + fact.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, peak * 180.0 - 91, peak * 180.0 - 90));
   }
   else if (widget.widgettype == 5 || widget.widgettype == 6)    // Meter
   {
      if (sensor.value != null) {
         var value = (widget.factor != null && widget.factor) ? widget.factor*sensor.value : sensor.value;
         // console.log("DEBUG: Update " + '#widget' + fact.type + fact.address + " to: " + value + " (" + sensor.value +")");
         $('#widgetValue' + fact.type + fact.address).html(value.toFixed(widget.unit == '%' ? 0 : 1) + ' ' + widget.unit);
         var gauge = $('#widget' + fact.type + fact.address).data('gauge');
         if (gauge != null)
            gauge.value = value;
         else
            console.log("Missing gauge instance for " + '#widget' + fact.type + fact.address);
         if (widget.showpeak != null && widget.showpeak)
            $("#peak" + fact.type + fact.address).text(sensor.peak != null ? sensor.peak.toFixed(2) + " " + widget.unit : "");
      }
      else
         console.log("Missing value for " + '#widget' + fact.type + fact.address);
   }
}

var filterExpression = null;
var addFilterString = '';

function addWidget()
{
   // console.log("add widget ..");

   var form = document.createElement("div");

   $(form).append($('<div></div>')
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Filter'))
                          .append($('<input></input>')
                                  .attr('id', 'incFilter')
                                  .attr('type', 'search')
                                  .attr('placeholder', 'expression...')
                                  .addClass('rounded-border inputSetting')
                                  .val(addFilterString)
                                  .on('input', function() {updateSelection();})
                                 ))
                  .append($('<br></br>'))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Widget'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widgetKey')
                                         ))));

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
            var json = {};
            console.log("store widget");
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               // console.log(" add " + sensor + " for sensor " + $(this).attr('id'));
               json[key] = dashboards[actDashboard].widgets[key];
            });

            if ($("#widgetKey").val() == 'ALL') {
               for (var key in valueFacts) {
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

            // console.log("storedashboards " + JSON.stringify(json, undefined, 4));
            // console.log("storedashboards key " + $("#widgetKey").val());

            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : {} });
            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });

   function updateSelection() {
      var addrSpacer = -1;
      var addrTime = -1;
      $('#widgetKey').empty();
      // console.log("update selection: " + $('#incFilter').val());
      for (var key in dashboards[actDashboard].widgets) {
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

      var jArray = [];
      filterExpression = null;
      addFilterString = $('#incFilter').val();

      if ($('#incFilter').val() != '')
         filterExpression = new RegExp(addFilterString);

      for (var key in valueFacts) {
         if (!valueFacts[key].state)   // use only active facts here
            continue;
//         if (dashboards[actDashboard].widgets[key] != null)
//            continue;
         if (filterExpression && !filterExpression.test(valueFacts[key].title) &&
             !filterExpression.test(valueFacts[key].usrtitle) &&
             !filterExpression.test(valueFacts[key].type))
            continue;

         jArray.push([key, valueFacts[key]]);
      }
      jArray.push(['ALL', { 'title': '- ALLE -'}]);
      jArray.sort(function(a, b) {
         var A = (a[1].usrtitle ? a[1].usrtitle : a[1].title).toLowerCase();
         var B = (b[1].usrtitle ? b[1].usrtitle : b[1].title).toLowerCase();
         if (B > A) return -1;
         if (A > B) return  1;
         return 0;

      });
      for (var i = 0; i < jArray.length; i++) {
         var key = jArray[i][0];
         var title = jArray[i][1].usrtitle ? jArray[i][1].usrtitle : jArray[i][1].title;

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
   var dialog = document.querySelector('dialog');
   dialog.style.position = 'fixed';
   console.log("chart for " + type + address);

   if (type != "" && !dialog.hasAttribute('open')) {
      var canvas = document.querySelector("#chartDialog");
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

      var div = document.createElement('div');
      div.id = 'backdrop';
      document.body.appendChild(div);
   }
   else {
      chartDialogSensor = "";
      document.removeEventListener('keydown', closeChartDialog);
      dialog.removeAttribute('open');
      var div = document.querySelector('#backdrop');
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
   var radians = (angle - 90) * Math.PI / 180.0;
   return [Math.round((cx + (radius * Math.cos(radians))) * 100) / 100, Math.round((cy + (radius * Math.sin(radians))) * 100) / 100];
};

function svg_circle_arc_path(x, y, radius, start_angle, end_angle)
{
   var start_xy = polar_to_cartesian(x, y, radius, end_angle);
   var end_xy = polar_to_cartesian(x, y, radius, start_angle);
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
   var target = ev.target;
   target.setAttribute('drop-active', true);
}

function dragLeaveDashboard(ev)
{
   event.preventDefault();
   var target = ev.target;
   target.removeAttribute('drop-active', true);
}

function dropDashboard(ev)
{
   ev.preventDefault();
   var target = ev.target;
   target.removeAttribute('drop-active', true);
   // console.log("drop: " + ev.target.getAttribute('id'));

   var source = document.getElementById(ev.originalEvent.dataTransfer.getData("source"));

   if (source.dataset.dragtype == 'widget') {
      var key = source.getAttribute('id').substring(source.getAttribute('id').indexOf("_") + 1);
      // console.log("drag widget " + key + " from dashboard " + parseInt(actDashboard) + " to " + parseInt(target.getAttribute('id')));
      source.remove();
      socket.send({ "event" : "storedashboards", "object" :
                    { 'action' : 'move',
                      'key' : key,
                      'from' : parseInt(actDashboard),
                      'to': parseInt(target.getAttribute('id')) } });

      return;
   }

   if (source.dataset.dragtype != 'dashboard') {
      console.log("drag source not a dashboard");
      return;
   }

   // console.log("drop element: " + source.getAttribute('id') + ' on ' + target.getAttribute('id'));
   target.after(source);

   // -> The order for json objects follows a certain set of rules since ES2015,
   //    but it does not (always) follow the insertion order. Simply,
   //    the iteration order is a combination of the insertion order for strings keys,
   //    and ascending order for number keys
   // terefore we use a array instead

   var jOrder = [];
   var i = 0;

   $('#dashboardMenu > button').each(function () {
      if ($(this).attr('data-dragtype') == 'dashboard') {
         var did = $(this).attr('id');
         console.log("add: " + did);
         jOrder[i++] = parseInt(did);
      }
   });

   // console.log("store:  " + JSON.stringify( { 'action' : 'order', 'order' : jOrder }, undefined, 4));
   socket.send({ "event" : "storedashboards", "object" : { 'action' : 'order', 'order' : jOrder } });
   socket.send({ "event" : "forcerefresh", "object" : {} });
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
   var target = ev.target;
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
   var target = ev.target;
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
   var target = ev.target;
   while (target) {
      if (target.dataset.droppoint)
         break;
      target = target.parentElement;
   }
   target.removeAttribute('drop-active', true);

   var source = document.getElementById(ev.dataTransfer.getData("source"));

   if (source.dataset.dragtype != 'widget') {
      console.log("drag source not a widget");
      return;
   }

   console.log("drop element: " + source.getAttribute('id') + ' on ' + target.getAttribute('id'));
   target.after(source);

   var json = {};
   $('#widgetContainer > div').each(function () {
      var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
      // console.log(" add " + key + " for " + $(this).attr('id'));
      json[key] = dashboards[actDashboard].widgets[key];
   });

   // console.log("dashboards " + JSON.stringify(json));
   socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
   socket.send({ "event" : "forcerefresh", "object" : {} });
}

function dashboardSetup(dashboardId)
{
   var form = document.createElement("div");

   if (dashboards[dashboardId].options == null)
      dashboards[dashboardId].options = {};

   $(form).append($('<div></div>')
                  .css('z-index', '9999')
                  .css('minWidth', '40vh')

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Titel'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'dashTitle')
                                          .attr('type', 'search')
                                          .val(dashboards[dashboardId].title)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Symbol'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'dashSymbol')
                                          .attr('type', 'search')
                                          .val(dashboards[dashboardId].symbol)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Zeilenhöhe'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'heightfactor')
                                          .val(dashboards[dashboardId].options.heightfactor)
                                         ))
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Kiosk'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'heightfactorKiosk')
                                          .val(dashboards[dashboardId].options.heightfactorKiosk)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Gruppe'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('type', 'number')
                                          .attr('id', 'group')
                                          .val(dashboards[dashboardId].group)
                                         )))
                 );

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Dashbord - " + dashboards[dashboardId].title,
      open: function() {
         $(".ui-dialog-buttonpane button:contains('Dashboard löschen')").attr('style','color:red');

         if (!dashboards[dashboardId].options.heightfactor)
            dashboards[dashboardId].options.heightfactor = 1;
         if (!dashboards[dashboardId].options.heightfactorKiosk)
            dashboards[dashboardId].options.heightfactorKiosk = 1;

         for (var w = 0.5; w <= 2.0; w += 0.5) {
            $('#heightfactor').append($('<option></option>')
                                      .val(w).html(w).attr('selected', dashboards[dashboardId].options.heightfactor == w));
            $('#heightfactorKiosk').append($('<option></option>')
                                      .val(w).html(w).attr('selected', dashboards[dashboardId].options.heightfactorKiosk == w));
         }
      },
      buttons: {
         'Dashboard löschen': function () {
            console.log("delete dashboard: " + dashboards[dashboardId].title);
            socket.send({ "event" : "storedashboards", "object" : { [dashboardId] : { 'action' : 'delete' } } });
            socket.send({ "event" : "forcerefresh", "object" : {} });

            $(this).dialog('close');
         },
         'Ok': function () {
            dashboards[dashboardId].options = {};
            dashboards[dashboardId].options.heightfactor = $("#heightfactor").val();
            dashboards[dashboardId].options.heightfactorKiosk = $("#heightfactorKiosk").val();
            console.log("change title from: " + dashboards[dashboardId].title + " to " + $("#dashTitle").val());
            dashboards[dashboardId].title = $("#dashTitle").val();
            dashboards[dashboardId].symbol = $("#dashSymbol").val();
            dashboards[dashboardId].group = parseInt($("#group").val());

            socket.send({ "event" : "storedashboards", "object" : { [dashboardId] : { 'title' : dashboards[dashboardId].title,
                                                                                      'group' : dashboards[dashboardId].group,
                                                                                      'symbol' : dashboards[dashboardId].symbol,
                                                                                      'options' : dashboards[dashboardId].options} } });
            socket.send({ "event" : "forcerefresh", "object" : {} });
            $(this).dialog('close');
         },
         'Abbrechen': function () {
            $(this).dialog('close');
         }
      },

      close: function() { $(this).dialog('destroy').remove(); }
   });
}

function widgetSetup(key)
{
   var item = valueFacts[key];
   var widget = dashboards[actDashboard].widgets[key];
   var battery = null;

   if (allSensors[key] != null)
   {
      // console.log("sensor " + JSON.stringify(allSensors[key], undefined, 4));
      // console.log("sensor found, batt is : " + allSensors[key].battery);
      battery = allSensors[key].battery;
   }
   else
      console.log("sensor not found: " + key);

   if (widget == null)
      console.log("widget not found: " + key);

   var form = document.createElement("div");

   $(form).append($('<div></div>')
                  .css('z-index', '9999')
                  .css('minWidth', '40vh')

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('ID'))
                          .append($('<span></span>')
                                  .append($('<div></div>')
                                          .addClass('rounded-border')
                                          .html(key + ' (' + parseInt(key.split(":")[1]) + ')')
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Battery'))
                          .append($('<span></span>')
                                  .append($('<div></div>')
                                          .addClass('rounded-border')
                                          .html(battery ? battery + ' %' :  '-')
                                         )))
                  .append($('<br></br>'))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Widget'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .change(function() {widgetTypeChanged();})
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widgettype')
                                         )))
                  .append($('<div></div>')
                          .attr('id', 'divUnit')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Einheit'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('type', 'search')
                                          .attr('id', 'unit')
                                          .val(widget.unit)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divFactor')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Faktor'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'factor')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.factor)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divScalemin')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Skala Min'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'scalemin')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.scalemin)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divScalemax')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Skala Max'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'scalemax')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.scalemax)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divScalestep')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Skala Step'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'scalestep')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.scalestep)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divCritmin')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Skala Crit Min'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'critmin')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.critmin)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divCritmax')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Skala Crit Max'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'critmax')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.critmax)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divSymbol')
                          .css('display', 'flex')
                          .append($('<a></a>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('color', 'blue')
                                  .css('align-self', 'center')
                                  .html('Icon')
                                  .attr('target','_blank')
                                  .attr('href', 'https://pictogrammers.github.io/@mdi/font/6.5.95'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'symbol')
                                          .attr('type', 'search')
                                          .val(widget.symbol)
                                          .change(function() {widgetTypeChanged();})
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divSymbolOn')
                          .css('display', 'flex')
                          .append($('<a></a>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('color', 'blue')
                                  .css('align-self', 'center')
                                  .html('Icon an')
                                  .attr('target','_blank')
                                  .attr('href', 'https://pictogrammers.github.io/@mdi/font/6.5.95'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'symbolOn')
                                          .attr('type', 'search')
                                          .val(widget.symbolOn)
                                          .change(function() {widgetTypeChanged();})
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divImgoff')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Image'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgoff')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divImgon')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Image An'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgon')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divColor')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .attr('id', 'spanColor')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Farbe aus / an'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .css('width', '80px')
                                          .attr('id', 'color')
                                          .attr('type', 'text')
                                          .val(widget.color)
                                         ))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .css('width', '80px')
                                          .attr('id', 'colorOn')
                                          .attr('type', 'text')
                                          .val(widget.colorOn)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divPeak')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Peak anzeigen'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'peak')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.showpeak))
                                  .append($('<label></label>')
                                          .prop('for', 'peak')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divLinefeed')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Zeilenumbruch'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'linefeed')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.linefeed))
                                  .append($('<label></label>')
                                          .prop('for', 'linefeed')
                                         )))
                  .append($('<div></div>')
                          .attr('id', 'divRange')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Chart Bereich'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('title', 'Tage')
                                          .attr('id', 'range')
                                          .attr('type', 'number')
                                          .attr('step', '0.5')
                                          .val(widget.range)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .css('align-self', 'center')
                                  .html('Breite'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widthfactor')
                                         )))
/*                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('margin-right', '10px')
                                  .html('Höhe'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'heightfactor')
                                         )))*/
                 );

   function widgetTypeChanged()
   {
      var wType = parseInt($('#widgettype').val());

      $("#divUnit").css("display", [1,3,4,5,6,9,13].includes(wType) ? 'flex' : 'none');
      $("#divFactor").css("display", [1,3,4,6,9,13].includes(wType) ? 'flex' : 'none');
      $("#divScalemax").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divScalemin").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divScalestep").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divCritmin").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divCritmax").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divSymbol").css("display", [0,9].includes(wType) ? 'flex' : 'none');
      $("#divSymbolOn").css("display", [0,9].includes(wType) ? 'flex' : 'none');
      $("#divImgon").css("display", ([0,9].includes(wType) && $('#symbol').val() == '') ? 'flex' : 'none');
      $("#divImgoff").css("display", ([0,9].includes(wType) && $('#symbol').val() == '') ? 'flex' : 'none');
      $("#divPeak").css("display", [1,3,6,9,13].includes(wType) ? 'flex' : 'none');
      $("#divColor").css("display", [0,1,3,4,6,7,9,10,11,13].includes(wType) ? 'flex' : 'none');
      $("#divLinefeed").css("display", [10].includes(wType) ? 'flex' : 'none');
      $("#divRange").css("display", [1,5,6,13].includes(wType) ? 'flex' : 'none');

      if ([0].includes(wType) && $('#symbol').val() == '' && $('#symbolOn').val() == '')
         $("#divColor").css("display", 'none');

      if (![0,9].includes(wType) || $('#symbolOn').val() == '') {
         $('.spColorOn').css('display', 'none');
         $('#spanColor').html("Farbe");
      }
      else {
         $('.spColorOn').css('display', 'flex');
         $('#spanColor').html("Farbe aus / an");
      }
   }

   var title = key.split(":")[0];

   if (item != null)
      title = (item.usrtitle ? item.usrtitle : item.title);

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Widget - " + title,
      open: function() {
         for (var wdKey in widgetTypes) {
            $('#widgettype').append($('<option></option>')
                                    .val(widgetTypes[wdKey])
                                    .html(wdKey)
                                    .attr('selected', widgetTypes[wdKey] == widget.widgettype));
         }

         images.sort();

         for (var img in images) {
            $('#imgoff').append($('<option></option>')
                                .val(images[img])
                                .html(images[img])
                                .attr('selected', widget.imgoff == images[img]));
            $('#imgon').append($('<option></option>')
                               .val(images[img])
                               .html(images[img])
                               .attr('selected', widget.imgon == images[img]));
         }

         if (widget.widthfactor == null)
            widget.widthfactor = 1;

         for (var w = 0.5; w <= 4.0; w += 0.5)
            $('#widthfactor').append($('<option></option>')
                                     .val(w)
                                     .html(w)
                                     .attr('selected', widget.widthfactor == w));

/*         if (widget.heightfactor == null)
            widget.heightfactor = 1;

         for (var h = 0.5; h <= 1.0; h += 0.5)
            $('#heightfactor').append($('<option></option>')
                                     .val(h)
                                     .html(h)
                                     .attr('selected', widget.heightfactor == h)); */

         $('#color').spectrum({
            type: "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [ ],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            showInitial: true,
            showAlpha: true,
            allowEmpty: false,
            replacerClassName: 'spColor'
         });

         $('#colorOn').spectrum({
            type : "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [ ],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            showInitial: true,
            showAlpha: true,
            allowEmpty: false,
            replacerClassName: 'spColorOn'
         });

         var wType = parseInt($('#widgettype').val());

         if (![0,9].includes(wType) || $('#symbolOn').val() == '') {
            $('.spColorOn').css('display', 'none');
            $('#spanColor').html("Farbe");
         }

         widgetTypeChanged();
         $(".ui-dialog-buttonpane button:contains('Widget löschen')").attr('style','color:red');
      },
      buttons: {
         'Widget löschen': function () {
            console.log("delete widget: " + key);
            document.getElementById('div_' + key).remove();

            var json = {};
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               json[key] = dashboards[actDashboard].widgets[key];
            });

            // console.log("delete widget " + JSON.stringify(json, undefined, 4));
            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : {} });
            $(this).dialog('close');
         },

         'Abbrechen': function () {
            // socket.send({ "event" : "forcerefresh", "object" : {} });
            if (allSensors[key] == null) {
               console.log("missing sensor!!");
            }
            initWidget(key, dashboards[actDashboard].widgets[key]);
            updateWidget(allSensors[key], true, dashboards[actDashboard].widgets[key]);

            $(this).dialog('close');
         },
         'Vorschau': function () {
            widget = Object.create(dashboards[actDashboard].widgets[key]); // valueFacts[key]);
            widget.unit = $("#unit").val();
            widget.scalemax = parseFloat($("#factor").val()) || 1.0;
            widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
            widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
            widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
            widget.critmin = parseFloat($("#critmin").val()) || -1;
            widget.critmax = parseFloat($("#critmax").val()) || -1;
            widget.symbol = $("#symbol").val();
            widget.symbolOn = $("#symbolOn").val();
            widget.imgon = $("#imgon").val();
            widget.imgoff = $("#imgoff").val();
            widget.widgettype = parseInt($("#widgettype").val());
            widget.color = $("#color").spectrum('get').toRgbString();
            widget.colorOn = $("#colorOn").spectrum('get').toRgbString();
            widget.showpeak = $("#peak").is(':checked');
            widget.linefeed = $("#linefeed").is(':checked');
            widget.widthfactor = $("#widthfactor").val();
            widget.heightfactor = $("#heightfactor").val();
            widget.range = $("#range").val();

            initWidget(key, widget);
            if (allSensors[key] != null)
               updateWidget(allSensors[key], true, widget);
         },
         'Ok': function () {
            widget.unit = $("#unit").val();
            widget.scalemax = parseFloat($("#factor").val()) || 1.0;
            widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
            widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
            widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
            widget.critmin = parseFloat($("#critmin").val()) || -1;
            widget.critmax = parseFloat($("#critmax").val()) || -1;
            widget.symbol = $("#symbol").val();
            widget.symbolOn = $("#symbolOn").val();
            widget.imgon = $("#imgon").val();
            widget.imgoff = $("#imgoff").val();
            widget.widgettype = parseInt($("#widgettype").val());
            widget.color = $("#color").spectrum("get").toRgbString();
            widget.colorOn = $("#colorOn").spectrum("get").toRgbString();
            widget.showpeak = $("#peak").is(':checked');
            widget.linefeed = $("#linefeed").is(':checked');
            widget.widthfactor = $("#widthfactor").val();
            widget.heightfactor = $("#heightfactor").val();
            widget.range = $("#range").val();

            initWidget(key, widget);
            if (allSensors[key] != null)
               updateWidget(allSensors[key], true, widget);

            var json = {};
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               json[key] = dashboards[actDashboard].widgets[key];
            });

            if ($("#unit").length)
               json[key]["unit"] = $("#unit").val();
            if ($("#factor").length)
               json[key]["factor"] = parseFloat($("#factor").val());
            if ($("#scalemax").length)
               json[key]["scalemax"] = parseFloat($("#scalemax").val());
            if ($("#scalemin").length)
               json[key]["scalemin"] = parseFloat($("#scalemin").val());
            if ($("#scalestep").length)
               json[key]["scalestep"] = parseFloat($("#scalestep").val());
            if ($("#critmin").length)
               json[key]["critmin"] = parseFloat($("#critmin").val());
            if ($("#critmax").length)
               json[key]["critmax"] = parseFloat($("#critmax").val());
            if ($("#symbol").length)
               json[key]["symbol"] = $("#symbol").val();
            if ($("#symbolOn").length)
               json[key]["symbolOn"] = $("#symbolOn").val();
            if ($("#imgon").length)
               json[key]["imgon"] = $("#imgon").val();
            if ($("#imgoff").length)
               json[key]["imgoff"] = $("#imgoff").val();

            json[key]["widthfactor"] = parseFloat($("#widthfactor").val());
            json[key]["heightfactor"] = parseFloat($("#heightfactor").val());
            json[key]["range"] = parseFloat($("#range").val());
            json[key]["widgettype"] = parseInt($("#widgettype").val());
            json[key]["showpeak"] = $("#peak").is(':checked');
            json[key]["linefeed"] = $("#linefeed").is(':checked');
            json[key]["color"] = $("#color").spectrum("get").toRgbString();
            json[key]["colorOn"] = $("#colorOn").spectrum("get").toRgbString();

            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : {} });

            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });
}
