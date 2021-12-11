/*
 *  dashboard.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var widgetWidthBase = null;

var actDashboard = -1;
var gauge = null;

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
      $('#dashboardMenu').html('');
   }

   $("#container").height($(window).height() - $("#menu").height() - 8);

   window.onresize = function() {
      $("#container").height($(window).height() - $("#menu").height() - 8);
   };

   // setupMode = true;  // to debug

   if (setupMode) {
      $('#dashboardMenu').append($('<button></button>')
                                 .addClass('rounded-border buttonDashboardTool')
                                 .addClass('mdi mdi-close-outline')
                                 .attr('title', 'Setup beenden')
                                 .click(function() { setupDashboard(); })
                                );
   }

   for (var did in dashboards) {
      if (actDashboard < 0)
         actDashboard = did;

      var classes = dashboards[did].symbol != '' ? dashboards[did].symbol.replace(':', ' ') : '';

      if (dashboards[actDashboard] == dashboards[did])
         classes += ' buttonDashboardActive';

      $('#dashboardMenu').append($('<button></button>')
                                 .addClass('rounded-border buttonDashboard widgetDropZone')
                                 .addClass(classes)
                                 .attr('id', did)
                                 .attr('draggable', setupMode)
                                 .data('droppoint', setupMode)
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

      if (setupMode && did == actDashboard)
         $('#dashboardMenu').append($('<button></button>')
                                    .addClass('rounded-border buttonDashboardTool')
                                    .addClass('mdi mdi-lead-pencil')  //  mdi-draw-pen
                                    .click({"id" : did}, function(event) { dashboardSetup(event.data.id); })
                                   );
   }

   document.getElementById("container").innerHTML = '<div id="widgetContainer" class="widgetContainer"></div>';

   for (var key in dashboards[actDashboard].widgets) {
      initWidget(allSensors[key], dashboards[actDashboard].widgets[key]);
      updateWidget(allSensors[key], true, dashboards[actDashboard].widgets[key]);
   }

   // additional setup elements

   if (setupMode) {
      $('#dashboardMenu')
         .append($('<div></div>')
                 .css('float', 'right')
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
                        ));

      var factAdd = {
        "address": 998,
         "type": "SP",
         "state": true,
         "name": "add",
         "title": "add"
      }
      initWidget({"address": 998, "type": "SP"}, { "widgettype": 998 }, factAdd);

      var factDel = {
         "address": 999,
         "type": "SP",
         "state": true,
         "name": "del",
         "title": "del"
      }
      initWidget({"address": 999, "type": "SP"}, { "widgettype": 999 }, factDel);
   }
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

function initWidget(sensor, widget, fact)
{
   if (sensor == null)
      return ;

   var key = toKey(sensor.type, sensor.address);
   var root = document.getElementById("widgetContainer");

   if (fact == null)
      fact = valueFacts[key];

   if (fact == null) {
      console.log("Fact '" + key + "' not found, ignoring");
      return;
   }

   if (widget == null) {
      console.log("Missing widget for '" + sensor.type + ":" + sensor.address + "'  ignoring");
      return;
   }

   // console.log("initWidget " + key + " : " + sensor.name);
   // console.log("widget: " + JSON.stringify(widget, undefined, 4));

   var editButton = '';
   var id = 'div_' + key;
   var elem = document.getElementById(id);
   if (elem == null) {
      // console.log("element '" + id + "' not found, creating");
      elem = document.createElement("div");
      root.appendChild(elem);
      elem.setAttribute('id', id);
   }

   elem.innerHTML = "";
   var marginPadding = 8;

   if (widgetWidthBase == null)
      widgetWidthBase = elem.clientWidth;

   // console.log("clientWidth: " + elem.clientWidth + ' : ' + widgetWidthBase);
   elem.style.width = widgetWidthBase * widget.widthfactor + ((widget.widthfactor-1) * marginPadding) + 'px';

   if (setupMode && (widget.widgettype < 900 || widget.widgettype == null)) {
      elem.setAttribute('draggable', true);
      elem.dataset.droppoint = true;
      elem.addEventListener('dragstart', function(event) {dragWidget(event)}, false);
      elem.addEventListener('dragover', function(event) {dragOver(event)}, false);
      elem.addEventListener('dragleave', function(event) {dragLeave(event)}, false);
      elem.addEventListener('drop', function(event) {dropWidget(event)}, false);

      editButton += '  <button class="rounded-border widget-edit mdi mdi-lead-pencil" onclick="widgetSetup(\'' + key + '\')"></button>';
   }

   var title = fact.usrtitle != '' && fact.usrtitle != null ? fact.usrtitle : fact.title;

   switch (widget.widgettype) {
      case 0:           // Symbol
         var html = editButton;
         if (localStorage.getItem(storagePrefix + 'Rights') & fact.rights) {
            html += '  <button class="widget-title" type="button" onclick="toggleMode(' + fact.address + ", '" + fact.type + '\')">' + title + '</button>';
            html += '  <button class="widget-main" type="button" onclick="toggleIo(' + fact.address + ",'" + fact.type + '\')" >';
         }
         else {
            html += '  <button class="widget-title" type="button">' + title + '</button>';
            html += '  <button class="widget-main" type="button">';
         }

         html += '    <img id="widget' + fact.type + fact.address + '" draggable="false")/>';
         html += "   </button>\n";
         html += "<div id=\"progress" + fact.type + fact.address + "\" class=\"widget-progress\">";
         html += "   <div id=\"progressBar" + fact.type + fact.address + "\" class=\"progress-bar\" style=\"visible\"></div>";
         html += "</div>";

         elem.className = "widget rounded-border widgetDropZone";
         elem.innerHTML = html;
         document.getElementById("progress" + fact.type + fact.address).style.visibility = "hidden";
         break;

      case 1:          // Chart
         elem.className = "widgetChart rounded-border widgetDropZone";
         var eTitle = document.createElement("div");
         eTitle.className = "widget-title";
         eTitle.innerHTML = title + editButton;
         elem.appendChild(eTitle);

         var ePeak = document.createElement("div");
         ePeak.setAttribute('id', 'peak' + fact.type + fact.address);
         ePeak.className = "chart-peak";
         elem.appendChild(ePeak);

         var eValue = document.createElement("div");
         eValue.setAttribute('id', 'value' + fact.type + fact.address);
         eValue.className = "chart-value";
         elem.appendChild(eValue);

         var eChart = document.createElement("div");
         eChart.className = "chart-canvas-container";
         if (!setupMode)
            eChart.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");
         elem.appendChild(eChart);

         var eCanvas = document.createElement("canvas");
         eCanvas.setAttribute('id', 'widget' + fact.type + fact.address);
         eCanvas.className = "chart-canvas";
         eChart.appendChild(eCanvas);
         break;

      case 3:   // type 3 (Value)
         var html = '<div class="widget-title">' + title + editButton + '</div>';
         html += '<div id="widget' + fact.type + fact.address + '" class="widget-value"></div>\n';
         elem.className = "widgetValue rounded-border widgetDropZone";
         if (!setupMode)
            elem.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");
         elem.innerHTML = html;
         var ePeak = document.createElement("div");
         ePeak.setAttribute('id', 'peak' + fact.type + fact.address);
         ePeak.className = "chart-peak";
         elem.appendChild(ePeak);
         break;

      case 4:           // Gauge
         var html = "  <div class=\"widget-title\">" + title + editButton + "</div>";

         html += "  <svg class=\"widget-main-gauge\" viewBox=\"0 0 1000 600\" preserveAspectRatio=\"xMidYMin slice\">\n";
         html += "    <path id=\"pb" + fact.type + fact.address + "\"/>\n";
         html += "    <path class=\"data-arc\" id=\"pv" + fact.type + fact.address + "\"/>\n";
         html += "    <path class=\"data-peak\" id=\"pp" + fact.type + fact.address + "\"/>\n";
         html += "    <text id=\"value" + fact.type + fact.address + "\" class=\"gauge-value\" text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"500\" y=\"450\" font-size=\"140\" font-weight=\"bold\"></text>\n";
         html += "    <text id=\"sMin" + fact.type + fact.address + "\" class=\"scale-text\" text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"50\" y=\"550\"></text>\n";
         html += "    <text id=\"sMax" + fact.type + fact.address + "\" class=\"scale-text\" text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"950\" y=\"550\"></text>\n";
         html += "  </svg>\n";

         elem.className = "widgetGauge rounded-border participation widgetDropZone";
         if (!setupMode)
            elem.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");
         elem.innerHTML = html;
         break;

      case 5:          // Meter
      case 6:          // MeterLevel
         var radial = widget.widgettype == 5;
         elem.className = radial ? "widgetMeter rounded-border" : "widgetMeterLinear rounded-border";
         elem.className += " widgetDropZone";
         var eTitle = document.createElement("div");
         eTitle.className = "widget-title";
         eTitle.innerHTML = title + editButton;
         elem.appendChild(eTitle);

         var main = document.createElement("div");
         main.className = radial ? "widget-main-meter" : "widget-main-meter-lin";
         elem.appendChild(main);
         var canvas = document.createElement('canvas');
         main.appendChild(canvas);
         canvas.setAttribute('id', 'widget' + fact.type + fact.address);
         if (!setupMode)
            canvas.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");

         if (!radial) {
            var value = document.createElement("div");
            value.setAttribute('id', 'widgetValue' + fact.type + fact.address);
            value.className = "widget-main-value-lin";
            elem.appendChild(value);
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
         highlights = [
            { from: widget.scalemin, to: critmin,  color: 'rgba(255,0,0,.6)' },
            { from: critmin,              to: critmax,  color: 'rgba(0,255,0,.6)' },
            { from: critmax,              to: scalemax, color: 'rgba(255,0,0,.6)' }
         ];

         // console.log("widget: " + JSON.stringify(widget, undefined, 4));
         // console.log("ticks: " + ticks + " range; " + scaleRange);
         // console.log("highlights: " + JSON.stringify(highlights, undefined, 4));

         options = {
            renderTo: 'widget' + fact.type + fact.address,
            units: radial ? widget.unit : '',
            // title: radial ? false : widget.unit
            colorTitle: 'white',
            minValue: widget.scalemin,
            maxValue: scalemax,
            majorTicks: ticks,
            minorTicks: 5,
            strokeTicks: false,
            highlights: highlights,
            highlightsWidth: radial ? 8 : 6,
            colorPlate: radial ? '#2177AD' : 'rgba(0,0,0,0)',
            colorBar: 'gray',
            colorBarProgress: widget.unit == '%' ? 'blue' : 'red',
            colorBarStroke: 'red',
            colorMajorTicks: '#f5f5f5',
            colorMinorTicks: '#ddd',
            colorTitle: '#fff',
            colorUnits: '#ccc',
            colorNumbers: '#eee',
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
            colorValueText: 'white',
            colorValueBoxBackground: 'transparent',
            // colorValueBoxBackground: '#2177AD',
            valueBoxStroke: 0,
            fontValueSize: 45,
            valueTextShadow: false,
            animationRule: 'bounce',
            animationDuration: 500,
            barWidth: radial ? 0 : 7,
            numbersMargin: 0,

            // linear gauge specials

            barBeginCircle: widget.unit == '°C' ? 15 : 0,
            tickSide: 'left',
            needleSide: 'right',
            numberSide: 'left',
            colorPlate: 'transparent'
         };

         if (radial)
            gauge = new RadialGauge(options);
         else
            gauge = new LinearGauge(options);

         gauge.draw();
         $('#widget' + fact.type + fact.address).data('gauge', gauge);

         break;

      case 7:     // 7 (PlainText)
         var html = '<div class="widget-title">' + editButton + '</div>';
         html += '<div id="widget' + fact.type + fact.address + '" class="widget-value" style="height:inherit;"></div>';
         elem.className = "widgetPlain rounded-border widgetDropZone";
         elem.innerHTML = html;
         break;

      case 8:     // 8 (Choice)
         var html = '<div class="widget-title">' + title + editButton + '</div>';
         html += '<div id="widget' + fact.type + fact.address + '" class="widget-value"></div>\n';
         elem.className = "widget rounded-border widgetDropZone";
         elem.style.cursor = 'pointer';
         elem.addEventListener('click', function(event) {
             socket.send({ "event": "toggleio", "object":
                           { "address": fact.address,
                             "type": fact.type }
                         });
            ;}, false);
         elem.innerHTML = html;
         break;

      case 9:           // Symbol-Value
         var html = editButton;
         if (localStorage.getItem(storagePrefix + 'Rights') & fact.rights) {
            html += '  <button class="widget-title" type="button" onclick="toggleMode(' + fact.address + ", '" + fact.type + '\')">' + title + '</button>';
            html += '  <button class="widget-main" style="color:white;font-size:6em;" type="button" onclick="toggleIo(' + fact.address + ",'" + fact.type + '\')" >';
         }
         else {
            html += '  <button class="widget-title" type="button">' + title + '</button>';
            html += '  <button id="button' + fact.type + fact.address + '" class="widget-main" style="color:white;font-size:6em;" type="button">';
         }

         html += '    <img id="widget' + fact.type + fact.address + '" draggable="false"/>';
         html += "   </button>\n";

         html += "<div id=\"progress" + fact.type + fact.address + "\" class=\"widget-progress\">";
         html += "   <div id=\"progressBar" + fact.type + fact.address + "\" class=\"progress-bar\" style=\"visible\"></div>";
         html += "</div>";

         elem.className = "widgetSymbolValue rounded-border widgetDropZone";
         elem.innerHTML = html;
         document.getElementById("progress" + fact.type + fact.address).style.visibility = "hidden";

         var eValue = document.createElement("div");
         eValue.setAttribute('id', 'value' + fact.type + fact.address);
         eValue.className = "symbol-value";
         elem.appendChild(eValue);

         break;

      case 998:     // 998 (Add Widget)
         elem.innerHTML = '<div class="widget-title"></div>' +
                          '<div id="widget' + fact.type + fact.address + '" class="widget-value" style="height:inherit;font-size:6em;">+</div>';
         elem.style.backgroundColor = "var(--light3)";
         elem.className = "widgetPlain rounded-border widgetDropZone";
         elem.addEventListener('click', function(event) {addWidget();}, false);
         break;

      case 999:     // 999 (Del Widget)
         elem.innerHTML = '<div class="widget-title"></div>' +
                          '<div id="widget' + fact.type + fact.address + '" class="widget-value" style="height:inherit;font-size:6em;">&#128465;</div>';
         elem.className = "widgetPlain rounded-border widgetDropZone";
         elem.style.backgroundColor = "var(--light3)";
         elem.title = 'zum Löschen hier ablegen';
         elem.addEventListener('dragover', function(event) {event.preventDefault()}, false);
         elem.addEventListener('drop', function(event) {deleteWidget(event)}, false);
         break;

      default:   // type 2 (Text)
         var html = '<div class="widget-title">' + title + editButton + '</div>';
         html += '<div id="widget' + fact.type + fact.address + '" class="widget-value"></div>\n';
         elem.className = "widget rounded-border widgetDropZone";
         if (!setupMode)
            elem.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");
         elem.innerHTML = html;
         break;
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

   // console.log("updateWidget " + sensor.name + " of type " + widget.widgettype);
   // console.log("updateWidget" + JSON.stringify(sensor, undefined, 4));

   if (fact == null) {
      console.log("Fact for widget '" + key + "' not found, ignoring");
      return ;
   }
   if (widget == null) {
      console.log("Widget '" + key + "' not found, ignoring");
      return;
   }

   if (widget.widgettype == 0 || widget.widgettype == 9)         // Symbol, Symbol-Value
   {
      // console.log("updateWidget" + JSON.stringify(sensor, undefined, 4));
      var modeStyle = sensor.options == 3 && sensor.mode == 'manual' ? "background-color: #a27373;" : "";
      var image = 'img/icon/unknown.png';
      var classes = '';
      if (sensor.image != null)
         image = sensor.image;
      else if (widget.symbol != null && widget.symbol != '') {
         classes = widget.symbol.replace(':', ' ');
         image = '';
         $("#widget" + fact.type + fact.address).remove();
      }
      else
         image = sensor.value != 0 ? widget.imgon : widget.imgoff;

      if (image != '')
         $("#widget" + fact.type + fact.address).attr("src", image);
      else {
         $("#button" + fact.type + fact.address).addClass(classes);
         // $("#widget" + fact.type + fact.address).addClass('hidden');
      }

      var e = document.getElementById("div_" + key);
      e.setAttribute("style", modeStyle);

      if (widget.widgettype == 9) {
         $("#value" + fact.type + fact.address).text(sensor.value.toFixed(2) + (widget.unit!="" ? " " : "") + widget.unit);
      }

      var e = document.getElementById("progress" + fact.type + fact.address);
      if (e != null)
         e.style.visibility = (sensor.next == null || sensor.next == 0) ? "hidden" : "visible";
      // else console.log("Element '" + "progress" + sensor.type + sensor.address + "' not found");

      if (sensor.mode == "auto" && sensor.next > 0) {
         var pWidth = 100;
         var s = sensor;
         var id = fact.type + fact.address;
         var iid = setInterval(progress, 200);

         function progress() {
            if (pWidth <= 0) {
               clearInterval(iid);
            } else {
               var d = new Date();
               pWidth = 100 - ((d/1000 - s.last) / ((s.next - s.last) / 100));
               document.getElementById("progressBar" + id).style.width = pWidth + "%";
            }
         }
      }
   }
   else if (widget.widgettype == 1)    // Chart
   {
      // var elem = $("#widget" + fact.type + fact.address);

      if (widget.showpeak != null && widget.showpeak)
         $("#peak" + fact.type + fact.address).text(sensor.peak != null ? sensor.peak.toFixed(2) + " " + widget.unit : "");

      if (!sensor.disabled) {
         $("#value" + fact.type + fact.address).text(sensor.value.toFixed(2) + " " + widget.unit);
         $("#value" + fact.type + fact.address).css('color', "var(--buttonFont)")
      }
      else {
         $("#value" + fact.type + fact.address).css('color', "var(--caption2)")
         $("#value" + fact.type + fact.address).text("(" + sensor.value.toFixed(2) + (widget.unit!="" ? " " : "") + widget.unit + ")");
      }

      if (refresh) {
         var jsonRequest = {};
         prepareChartRequest(jsonRequest, fact.type + ":0x" + fact.address.toString(16), 0, 1, "chartwidget");
         socket.send({ "event" : "chartdata", "object" : jsonRequest });
      }
   }
   else if (widget.widgettype == 2 || widget.widgettype == 7 || widget.widgettype == 8)      // Text, PlainText
   {
      if (sensor.text != null) {
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
      if (scaleMax < Math.ceil(value)) scaleMax = value;

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

function addWidget()
{
   // console.log("add widget ..");

   var form = document.createElement("div");

   $(form).append($('<div></div>')
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Widget'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widgetKey')
                                         )))
                 );


   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Add Widget",
      open: function() {
         for (var key in valueFacts) {
            if (!valueFacts[key].state)   // use only active facts here
               continue;
            if (dashboards[actDashboard].widgets[key] == null)
               $('#widgetKey').append($('<option></option>')
                                      .val(key)
                                      .html(valueFacts[key].usrtitle ? valueFacts[key].usrtitle : valueFacts[key].title));
         }
      },
      buttons: {
         'Cancel': function () {
            $(this).dialog('close');
         },
         'Ok': function () {
            var json = {};
            console.log("store widget");
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               if (key != 'SP:0x3e6' && key != 'SP:0x3e7') {
                  // console.log(" add " + sensor + " for sensor " + $(this).attr('id'));
                  json[key] = dashboards[actDashboard].widgets[key];
               }
            });

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
}

function toggleChartDialog(type, address)
{
   var dialog = document.querySelector('dialog');
   dialog.style.position = 'fixed';

   if (type != "" && !dialog.hasAttribute('open')) {
      var canvas = document.querySelector("#chartDialog");
      canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
      chartDialogSensor = type + address;

      // show the dialog

      dialog.setAttribute('open', 'open');

      var jsonRequest = {};

      prepareChartRequest(jsonRequest, type + ":0x" + address.toString(16), 0, 1, "chartdialog");
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
      var did = $(this).attr('id');
      console.log("add: " + did);
      jOrder[i++] = parseInt(did);
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
   console.log("drop element: " + source.getAttribute('id') + ' on ' + target.getAttribute('id'));
   target.after(source);

   var json = {};
   $('#widgetContainer > div').each(function () {
      var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
      if (key != 'SP:0x3e6' && key != 'SP:0x3e7') {
         // console.log(" add " + key + " for " + $(this).attr('id'));
         json[key] = dashboards[actDashboard].widgets[key];
      }
   });

   // console.log("dashboards " + JSON.stringify(json));
   socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
   socket.send({ "event" : "forcerefresh", "object" : {} });
}

function deleteWidget(ev)
{
   ev.preventDefault();
   var target = ev.target;

   var sourceId = document.getElementById(ev.dataTransfer.getData("source")).getAttribute('id');
   sourceId = sourceId.substring(sourceId.indexOf("_") + 1);
   console.log("delete widget " + sourceId);

   document.getElementById(ev.dataTransfer.getData("source")).remove();

   var json = {};
   $('#widgetContainer > div').each(function () {
      var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
      if (key != 'SP:0x3e6' && key != 'SP:0x3e7') {
         json[key] = dashboards[actDashboard].widgets[key];
      }
   });

   socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
   socket.send({ "event" : "forcerefresh", "object" : {} });
}

function dashboardSetup(dashboardId)
{
   var form = document.createElement("div");

   $(form).append($('<div></div>')
                  .css('z-index', '9999')
                  .css('minWidth', '40vh')

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Titel'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'dashTitle')
                                          .val(dashboards[dashboardId].title)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Symbol'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'dashSymbol')
                                          .val(dashboards[dashboardId].symbol)
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
      },
      buttons: {
         'Dashboard löschen': function () {
            console.log("delete dashboard: " + dashboards[dashboardId].title);

            socket.send({ "event" : "storedashboards", "object" : { [dashboardId] : { 'action' : 'delete' } } });
            socket.send({ "event" : "forcerefresh", "object" : {} });

            $(this).dialog('close');
         },
         'Ok': function () {
            if (dashboards[dashboardId].title != $("#dashTitle").val() || dashboards[dashboardId].symbol != $("#dashSymbol").val()) {
               console.log("change title from: " + dashboards[dashboardId].title + " to " + $("#dashTitle").val());
               dashboards[dashboardId].title = $("#dashTitle").val();
               dashboards[dashboardId].symbol = $("#dashSymbol").val();

               socket.send({ "event" : "storedashboards", "object" : { [dashboardId] : { 'title' : dashboards[dashboardId].title,
                                                                                         'symbol' : dashboards[dashboardId].symbol } } });
               socket.send({ "event" : "forcerefresh", "object" : {} });
            }
            $(this).dialog('close');
         },
         'Cancel': function () {
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
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('ID'))
                          .append($('<span></span>')
                                  .append($('<div></div>')
                                          .addClass('rounded-border')
                                          .html(key)
                                         )))
                  .append($('<br></br>'))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Widget'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widgettype')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Einheit'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'unit')
                                          .val(widget.unit)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
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
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
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
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
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
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
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
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Skala Crit Min'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'critmin')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.critmin)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Skala Crit Max'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'critmax')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.critmax)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Symbol'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'symbol')
                                          .val(widget.symbol)
                                          .change(function() {
                                             $('#imgon').prop( "disabled", $(this).val() != '');
                                             $('#imgoff').prop( "disabled", $(this).val() != '');
                                          })
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Image On'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgon')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Image Off'))
                          .append($('<span></span>')
                                  .css('width', '300px')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgoff')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Peak'))
                          .append($('<span></span>')
                                  .append($('<input></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'peak')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.showpeak))
                                  .append($('<label></label>')
                                          .prop('for', 'peak')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '30%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Breite'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widthfactor')
                                          .val(widget.unit)
                                         )))

                 );

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Widget - " + (item.usrtitle ? item.usrtitle : item.title),
      open: function() {
         for (var wdKey in widgetTypes) {
            $('#widgettype').append($('<option></option>')
                                    .val(widgetTypes[wdKey])
                                    .html(wdKey)
                                    .attr('selected', widgetTypes[wdKey] == widget.widgettype));
         }

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

         for (var w = 1.0; w <= 2.0; w += 0.5)
            $('#widthfactor').append($('<option></option>')
                                     .val(w)
                                     .html(w)
                                     .attr('selected', widget.widthfactor == w));

         if (allSensors[key] != null) {
            initWidget(allSensors[key], widget);
            updateWidget(allSensors[key], false, widget);
         }
         else
            console.log("No widget for " + item.type + " - " + item.address + " found");

         $('#symbol').trigger('change');
      },
      buttons: {
         'Cancel': function () {
            // socket.send({ "event" : "forcerefresh", "object" : {} });
            if (allSensors[key] == null) {
               console.log("missing sensor!!");
            }
            initWidget(allSensors[key], dashboards[actDashboard].widgets[key]);
            updateWidget(allSensors[key], true, dashboards[actDashboard].widgets[key]);

            $(this).dialog('close');
         },
         'Preview': function () {
            if (allSensors[key] != null) {
               widget = Object.create(valueFacts[key]);

               widget.unit = $("#unit").val();
               widget.scalemax = parseFloat($("#factor").val()) || 1.0;
               widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
               widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
               widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
               widget.critmin = parseFloat($("#critmin").val()) || -1;
               widget.critmax = parseFloat($("#critmax").val()) || -1;
               widget.symbol = $("#symbol").val();
               widget.imgon = $("#imgon").val();
               widget.imgoff = $("#imgoff").val();
               widget.widgettype = parseInt($("#widgettype").val());
               widget.showpeak = $("#peak").is(':checked');
               widget.widthfactor = $("#widthfactor").val();

               initWidget(allSensors[key], widget);
               updateWidget(allSensors[key], true, widget);
            }
         },
         'Ok': function () {
            if (allSensors[key] != null) {
               widget.unit = $("#unit").val();
               widget.scalemax = parseFloat($("#factor").val()) || 1.0;
               widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
               widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
               widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
               widget.critmin = parseFloat($("#critmin").val()) || -1;
               widget.critmax = parseFloat($("#critmax").val()) || -1;
               widget.symbol = $("#symbol").val();
               widget.imgon = $("#imgon").val();
               widget.imgoff = $("#imgoff").val();
               widget.widgettype = parseInt($("#widgettype").val());
               widget.showpeak = $("#peak").is(':checked');
               widget.widthfactor = $("#widthfactor").val();

               initWidget(allSensors[key], widget);
               updateWidget(allSensors[key], true, widget);
            }

            var json = {};
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               if (key != 'SP:0x3e6' && key != 'SP:0x3e7') {
                  json[key] = dashboards[actDashboard].widgets[key];
               }
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
            if ($("#imgon").length)
               json[key]["imgon"] = $("#imgon").val();
            if ($("#imgoff").length)
               json[key]["imgoff"] = $("#imgoff").val();

            json[key]["widthfactor"] = parseFloat($("#widthfactor").val());
            json[key]["widgettype"] = parseInt($("#widgettype").val());
            json[key]["showpeak"] = $("#peak").is(':checked');

            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : {} });

            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });
}
