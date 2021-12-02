/*
 *  dashboard.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var gauge = null;

function initDashboard(update = false)
{
   // console.log("initDashboard " + JSON.stringify(allWidgets, undefined, 4));

   if (!allWidgets || !allWidgets.length) {
      console.log("Fatal: Missing widgets!");
      return;
   }

   if (!update)
      $('#container').removeClass('hidden');

   $("#container").height($(window).height() - $("#menu").height() - 8);

   window.onresize = function() {
      $("#container").height($(window).height() - $("#menu").height() - 8);
   };

   document.getElementById("container").innerHTML = '<div id="widgetContainer" class="widgetContainer"></div>';

   for (var i = 0; i < allWidgets.length; i++)
      initWidget(allWidgets[i], null);

   // additional setup elements

   if (setupMode) {
      var factAdd = {
         "address": 998,
         "type": "SP",
         "state": true,
         "name": "add",
         "title": "add",
         "widget": {
            "imgon": "",
            "imgoff": "",
            "widgettype": 998
         }
      }

      initWidget({"address": 998, "type": "SP"}, factAdd);

      var factDel = {
         "address": 999,
         "type": "SP",
         "state": true,
         "name": "del",
         "title": "del",
         "widget": {
            "imgon": "",
            "imgoff": "",
            "widgettype": 999
         }
      }

      initWidget({"address": 999, "type": "SP"}, factDel);

   }

   updateDashboard(allWidgets, true);
}

function initWidget(widget, fact)
{
   var root = document.getElementById("widgetContainer");

   if (fact == null)
      fact = valueFacts[widget.type + ":" + widget.address];

   // console.log("initWidget " + widget.name);

   if (fact == null || fact == undefined) {
      console.log("Fact for widget '" + widget.type + ":" + widget.address + "' not found, ignoring");
      return;
   }

   if (fact.widget == null || fact.widget == undefined) {
      console.log("fact.widget '" + widget.type + ":" + widget.address + "' not found, ignoring");
      return;
   }

   // console.log("fact: " + JSON.stringify(fact, undefined, 4));

   var editButton = '';
   var id = 'div_' + fact.type + ":0x" + fact.address.toString(16).padStart(2, '0');
   var elem = document.getElementById(id);
   if (elem == null) {
      // console.log("element '" + id + "' not found, creating");
      elem = document.createElement("div");
      root.appendChild(elem);
      elem.setAttribute('id', 'div_' + fact.type + ":0x" + fact.address.toString(16).padStart(2, '0'));
   }

   elem.innerHTML = "";
   var marginPadding = 8;

   if (fact.widget.baseWidth == null)
      fact.widget.baseWidth = elem.clientWidth;

   // console.log("clientWidth: " + elem.clientWidth + ' : ' + fact.widget.baseWidth);
   elem.style.width = fact.widget.baseWidth * fact.widget.widthfactor + ((fact.widget.widthfactor-1) * marginPadding) + 'px';

   if (setupMode && fact.widget.widgettype < 900) {
      elem.setAttribute('draggable', true);
      elem.dataset.droppoint = true;
      elem.addEventListener('dragstart', function(event) {dragWidget(event)}, false);
      elem.addEventListener('dragover', function(event) {event.preventDefault()}, false);
      elem.addEventListener('drop', function(event) {dropWidget(event)}, false);

      editButton += '  <button class="widget-edit widget-title" onclick="widgetSetup(\'' + fact.type + ':' + fact.address + '\')">' + '&#128393' + '</button>';
   }

   var title = fact.usrtitle != '' && fact.usrtitle != undefined ? fact.usrtitle : fact.title;

   switch (fact.widget.widgettype) {
      case 0:           // Symbol
         var html = editButton;
         html += "  <button class=\"widget-title\" type=\"button\" onclick=\"toggleMode(" + fact.address + ", '" + fact.type + "')\">" + title + "</button>\n";
         html += "  <button class=\"widget-main\" type=\"button\" onclick=\"toggleIo(" + fact.address + ",'" + fact.type + "')\" >\n";
         html += '    <img id="widget' + fact.type + fact.address + '" draggable="false")/>';
         html += "   </button>\n";
         html += "<div id=\"progress" + fact.type + fact.address + "\" class=\"widget-progress\">";
         html += "   <div id=\"progressBar" + fact.type + fact.address + "\" class=\"progress-bar\" style=\"visible\"></div>";
         html += "</div>";

         elem.className = "widget rounded-border";
         elem.innerHTML = html;
         document.getElementById("progress" + fact.type + fact.address).style.visibility = "hidden";
         break;

      case 1:          // Chart
         elem.className = "widgetChart rounded-border";
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
         eChart.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");
         elem.appendChild(eChart);

         var eCanvas = document.createElement("canvas");
         eCanvas.setAttribute('id', 'widget' + fact.type + fact.address);
         eCanvas.className = "chart-canvas";
         eChart.appendChild(eCanvas);

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

         elem.className = "widgetGauge rounded-border participation";
         elem.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");
         elem.innerHTML = html;
         break;

      case 5:          // Meter
      case 6:          // MeterLevel
         var radial = fact.widget.widgettype == 5;
         elem.className = radial ? "widgetMeter rounded-border" : "widgetMeterLinear rounded-border";
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
         canvas.setAttribute("onclick", "toggleChartDialog('" + fact.type + "'," + fact.address + ")");

         if (!radial) {
            var value = document.createElement("div");
            value.setAttribute('id', 'widgetValue' + fact.type + fact.address);
            value.className = "widget-main-value-lin";
            elem.appendChild(value);
         }

         if (fact.widget.scalemin == undefined)
            fact.widget.scalemin = 0;

         var ticks = [];
         var scaleRange = fact.widget.scalemax - fact.widget.scalemin;
         var stepWidth = fact.widget.scalestep != undefined ? fact.widget.scalestep : 0;

         if (!stepWidth) {
            var steps = 10;
            if (scaleRange <= 100)
               steps = scaleRange % 10 == 0 ? 10 : scaleRange % 5 == 0 ? 5 : scaleRange;
            if (steps < 10)
               steps = 10;
            if (!radial && fact.widget.unit == '%')
               steps = 4;
            stepWidth = scaleRange / steps;
         }

         if (stepWidth <= 0) stepWidth = 1;
         stepWidth = Math.round(stepWidth*10) / 10;
         var steps = -1;
         for (var step = fact.widget.scalemin; step.toFixed(2) <= fact.widget.scalemax; step += stepWidth) {
            ticks.push(step % 1 ? parseFloat(step).toFixed(1) : parseInt(step));
            steps++;
         }

         var scalemax = fact.widget.scalemin + stepWidth * steps; // cals real scale max based on stepWidth and step count!
         var highlights = {};
         var critmin = (fact.widget.critmin == undefined || fact.widget.critmin == -1) ? fact.widget.scalemin : fact.widget.critmin;
         var critmax = (fact.widget.critmax == undefined || fact.widget.critmax == -1) ? scalemax : fact.widget.critmax;
         highlights = [
            { from: fact.widget.scalemin, to: critmin,  color: 'rgba(255,0,0,.6)' },
            { from: critmin,              to: critmax,  color: 'rgba(0,255,0,.6)' },
            { from: critmax,              to: scalemax, color: 'rgba(255,0,0,.6)' }
         ];

         // console.log("widget: " + JSON.stringify(widget, undefined, 4));
         // console.log("ticks: " + ticks + " range; " + scaleRange);
         // console.log("highlights: " + JSON.stringify(highlights, undefined, 4));

         options = {
            renderTo: 'widget' + fact.type + fact.address,
            units: radial ? fact.widget.unit : '',
            // title: radial ? false : fact.widget.unit
            colorTitle: 'white',
            minValue: fact.widget.scalemin,
            maxValue: scalemax,
            majorTicks: ticks,
            minorTicks: 5,
            strokeTicks: false,
            highlights: highlights,
            highlightsWidth: radial ? 8 : 6,
            colorPlate: radial ? '#2177AD' : 'rgba(0,0,0,0)',
            colorBar: 'gray',
            colorBarProgress: fact.widget.unit == '%' ? 'blue' : 'red',
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

            barBeginCircle: fact.widget.unit == '°C' ? 15 : 0,
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
         elem.className = "widgetPlain rounded-border";
         elem.innerHTML = html;
         break;

      case 8:     // 8 (Choice)
         var html = '<div class="widget-title">' + title + editButton + '</div>';
         html += '<div id="widget' + fact.type + fact.address + '" class="widget-value"></div>\n';
         elem.className = "widget rounded-border";
         elem.style.cursor = 'pointer';
         elem.addEventListener('click', function(event) {
             socket.send({ "event": "toggleio", "object":
                           { "address": fact.address,
                             "type": fact.type }
                         });
            ;}, false);
         elem.innerHTML = html;
         break;

      case 998:     // 998 (Add Widget)
         elem.innerHTML = '<div class="widget-title"></div>' +
                          '<div id="widget' + fact.type + fact.address + '" class="widget-value" style="height:inherit;font-size:6em;">+</div>';
         elem.style.backgroundColor = "var(--light3)";
         elem.className = "widgetPlain rounded-border";
         elem.addEventListener('click', function(event) {addWidget();}, false);
         break;

      case 999:     // 999 (Del Widget)
         elem.innerHTML = '<div class="widget-title"></div>' +
                          '<div id="widget' + fact.type + fact.address + '" class="widget-value" style="height:inherit;font-size:6em;">&#128465;</div>';
         elem.className = "widgetPlain rounded-border";
         elem.style.backgroundColor = "var(--light3)";
         elem.title = 'zum Löschen hier ablegen';
         elem.addEventListener('dragover', function(event) {event.preventDefault()}, false);
         elem.addEventListener('drop', function(event) {deleteWidget(event)}, false);
         break;

      default:   // type 2(Text), 3(Value)
         var html = '<div class="widget-title">' + title + editButton + '</div>';
         html += '<div id="widget' + fact.type + fact.address + '" class="widget-value"></div>\n';
         elem.className = "widget rounded-border";
         elem.innerHTML = html;
         break;
   }
}

function updateDashboard(widgets, refresh)
{
   // console.log("updateDashboard");

   if (widgets)
   {
      for (var i = 0; i < widgets.length; i++)
         updateWidget(widgets[i], refresh)
   }
}

function updateWidget(sensor, refresh, fact)
{
   if (fact == null)
      fact = valueFacts[sensor.type + ":" + sensor.address];

   // console.log("updateWidget " + sensor.name + " of type " + fact.widget.widgettype);
   // console.log("updateWidget" + JSON.stringify(sensor, undefined, 4));

   if (fact == null || fact == undefined) {
      console.log("Fact for widget '" + sensor.type + ":" + sensor.address + "' not found, ignoring");
      return ;
   }
   if (fact.widget == null || fact.widget == undefined) {
      console.log("fact.widget '" + fact.type + ":" + fact.address + "' not found, ignoring");
      return;
   }

   if (fact.widget.widgettype == 0)         // Symbol
   {
      // console.log("updateWidget" + JSON.stringify(sensor, undefined, 4));
      var modeStyle = sensor.options == 3 && sensor.mode == 'manual' ? "background-color: #a27373;" : "";
      var image = 'img/icon/unknown.png';
      if (sensor.image != null)
         image = sensor.image;
      else
         image = sensor.value != 0 ? fact.widget.imgon : fact.widget.imgoff;
      $("#widget" + fact.type + fact.address).attr("src", image);

      var ddd = "div_" + fact.type + ":0x" + fact.address.toString(16).padStart(2, '0');
      var e = document.getElementById(ddd);
      e.setAttribute("style", modeStyle);
      // $('#' + ddd).attr("style", modeStyle);  // #TODO geht nicht??

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
   else if (fact.widget.widgettype == 1)    // Chart
   {
      // var elem = $("#widget" + fact.type + fact.address);

      if (fact.widget.showpeak != undefined && fact.widget.showpeak)
         $("#peak" + fact.type + fact.address).text(sensor.peak != null ? sensor.peak.toFixed(2) + " " + fact.widget.unit : "");

      if (!sensor.disabled) {
         $("#value" + fact.type + fact.address).text(sensor.value.toFixed(2) + " " + fact.widget.unit);
         $("#value" + fact.type + fact.address).css('color', "var(--buttonFont)")
      }
      else {
         $("#value" + fact.type + fact.address).css('color', "var(--caption2)")
         $("#value" + fact.type + fact.address).text("(" + sensor.value.toFixed(2) + (fact.widget.unit!="" ? " " : "") + fact.widget.unit + ")");
      }

      if (refresh) {
         var jsonRequest = {};
         prepareChartRequest(jsonRequest, fact.type + ":0x" + fact.address.toString(16) , 0, 1, "chartwidget");
         socket.send({ "event" : "chartdata", "object" : jsonRequest });
      }
   }
   else if (fact.widget.widgettype == 2 || fact.widget.widgettype == 7 || fact.widget.widgettype == 8)      // Text, PlainText
   {
      if (sensor.text != undefined) {
         var text = sensor.text.replace(/(?:\r\n|\r|\n)/g, '<br>');
         $("#widget" + fact.type + fact.address).html(text);
      }
   }
   else if (fact.widget.widgettype == 3)      // plain value
   {
      $("#widget" + fact.type + fact.address).html(sensor.value + " " + fact.widget.unit);
   }
   else if (fact.widget.widgettype == 4)      // Gauge
   {
      var value = sensor.value.toFixed(2);
      var scaleMax = !fact.widget.scalemax || fact.widget.unit == '%' ? 100 : fact.widget.scalemax.toFixed(0);
      var scaleMin = value >= 0 ? "0" : Math.ceil(value / 5) * 5 - 5;
      var _peak = sensor.peak != null ? sensor.peak : 0;
      if (scaleMax < Math.ceil(value)) scaleMax = value;

      if (fact.widget.showpeak != undefined && fact.widget.showpeak && scaleMax < Math.ceil(_peak))
         scaleMax = _peak.toFixed(0);

      $("#sMin" + fact.type + fact.address).text(scaleMin);
      $("#sMax" + fact.type + fact.address).text(scaleMax);
      $("#value" + fact.type + fact.address).text(value + " " + fact.widget.unit);

      var ratio = (value - scaleMin) / (scaleMax - scaleMin);
      var peak = (_peak.toFixed(2) - scaleMin) / (scaleMax - scaleMin);

      $("#pb" + fact.type + fact.address).attr("d", "M 950 500 A 450 450 0 0 0 50 500");
      $("#pv" + fact.type + fact.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, -90, ratio * 180.0 - 90));
      if (fact.widget.showpeak != undefined && fact.widget.showpeak)
         $("#pp" + fact.type + fact.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, peak * 180.0 - 91, peak * 180.0 - 90));
   }
   else if (fact.widget.widgettype == 5 || fact.widget.widgettype == 6)    // Meter
   {
      if (sensor.value != undefined) {
         var value = (fact.widget.factor != undefined && fact.widget.factor) ? fact.widget.factor*sensor.value : sensor.value;
         // console.log("DEBUG: Update " + '#widget' + fact.type + fact.address + " to: " + value + " (" + sensor.value +")");
         $('#widgetValue' + fact.type + fact.address).html(value.toFixed(fact.widget.unit == '%' ? 0 : 1) + ' ' + fact.widget.unit);
         var gauge = $('#widget' + fact.type + fact.address).data('gauge');
         if (gauge != null)
            gauge.value = value;
         else
            console.log("Missing gauge instance for " + '#widget' + fact.type + fact.address);
      }
      else
         console.log("Missing value for " + '#widget' + fact.type + fact.address);
      //  = fact.address != 4 ? value = sensor.value : 45;  // for test
   }
}

function addWidget()
{
   //console.log("add widget ..");

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
                                          .attr('id', 'widgetId')
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
         for (var fKey in valueFacts) {
            if (!valueFacts[fKey].state)   // use only active facts here
               continue;
            // console.log(fKey);
            var found = false;
            for (var i = 0; i < allWidgets.length; i++) {
               if (allWidgets[i].type + ':' + allWidgets[i].address == fKey) {
                  found = true;
                  break;
               }
            }
            if (!found)
               $('#widgetId').append($('<option></option>')
                                     .val(valueFacts[fKey].type + ":0x" + valueFacts[fKey].address.toString(16).padStart(2, '0'))
                                     .html(valueFacts[fKey].usrtitle ? valueFacts[fKey].usrtitle : valueFacts[fKey].title));
         }
      },
      buttons: {
         'Cancel': function () {
            $(this).dialog('close');
         },
         'Ok': function () {
            var list = '';
            $('#widgetContainer > div').each(function () {
               var sensor = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               list += sensor + ',';
            });
            list += $("#widgetId").val();
            socket.send({ "event" : "storeconfig", "object" : { "addrsDashboard" : list } });
            console.log(" - " + list);
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
      prepareChartRequest(jsonRequest, type + ":0x" + ((address)>>>0).toString(16) , 0, 1, "chartdialog");
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

function dragWidget(ev)
{
   console.log("drag: " + ev.target.getAttribute('id'));
   ev.dataTransfer.setData("source", ev.target.getAttribute('id'));
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

   var source = document.getElementById(ev.dataTransfer.getData("source"));
   console.log("drop element: " + source.getAttribute('id') + ' on ' + target.getAttribute('id'));
   target.after(source);

   var list = '';
   $('#widgetContainer > div').each(function () {
      var sensor = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
      if (sensor != 'SP:0x3e6' && sensor != 'SP:0x3e7') {
         // console.log(" add " + sensor + " for sensor " + $(this).attr('id'));
         list += sensor + ',';
      }
   });

   socket.send({ "event" : "storeconfig", "object" : { "addrsDashboard" : list } });
   console.log(" - " + list);
}

function deleteWidget(ev)
{
   ev.preventDefault();
   var target = ev.target;

   var sourceId = document.getElementById(ev.dataTransfer.getData("source")).getAttribute('id');
   sourceId = sourceId.substring(sourceId.indexOf("_") + 1);
   console.log("delete widget " + sourceId);

   document.getElementById(ev.dataTransfer.getData("source")).remove();

   var list = '';
   $('#widgetContainer > div').each(function () {
      var sensor = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
      if (sensor != 'SP:0x3e6' && sensor != 'SP:0x3e7' && sensor != sourceId)
         list += sensor + ',';
   });

   socket.send({ "event" : "storeconfig", "object" : { "addrsDashboard" : list } });
   console.log(" - " + list);
}

function widgetSetup(key)
{
   var item = valueFacts[key];
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
                                  .html('Widget'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widgettype')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Einheit'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'unit')
                                          .val(item.widget.unit)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
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
                                          .val(item.widget.factor)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
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
                                          .val(item.widget.scalemin)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
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
                                          .val(item.widget.scalemax)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
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
                                          .val(item.widget.scalestep)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
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
                                          .val(item.widget.critmin)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
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
                                          .val(item.widget.critmax)
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Image On'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgon')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Image Off'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgoff')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Peak'))
                          .append($('<span></span>')
                                  .append($('<input></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'peak')
                                          .attr('type', 'checkbox')
                                          .prop('checked', item.widget.showpeak))
                                  .append($('<label></label>')
                                          .prop('for', 'peak')
                                         )))

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '25%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Breite'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widthfactor')
                                          .val(item.widget.unit)
                                         )))

                 );

   var widget = null;

   for (var i = 0; i < allWidgets.length; i++)
   {
      console.log("check " + allWidgets[i].type + " - " + allWidgets[i].address);
      if (allWidgets[i].type == item.type && allWidgets[i].address == item.address) {
         widget = allWidgets[i];
         break;
      }
   }

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Widget Konfiguration - " + (item.usrtitle ? item.usrtitle : item.title),
      open: function() {
         for (var wdKey in widgetTypes) {
            $('#widgettype').append($('<option></option>')
                                    .val(widgetTypes[wdKey])
                                    .html(wdKey)
                                    .attr('selected', widgetTypes[wdKey] == item.widget.widgettype));
         }

         for (var img in images) {
            $('#imgoff').append($('<option></option>')
                                .val(images[img])
                                .html(images[img])
                                .attr('selected', item.widget.imgoff == images[img]));
            $('#imgon').append($('<option></option>')
                               .val(images[img])
                               .html(images[img])
                               .attr('selected', item.widget.imgon == images[img]));
         }

         for (var w = 1.0; w <= 2.0; w += 0.5)
            $('#widthfactor').append($('<option></option>')
                                     .val(w)
                                     .html(w)
                                     .attr('selected', item.widget.widthfactor == w));

         if (widget != null) {
            initWidget(widget, null);
            updateWidget(widget, false);
         }
         else
            console.log("No widget for " + item.type + " - " + item.address + " found");
      },
      buttons: {
         'Cancel': function () {
            $(this).dialog('close');
         },
         'Preview': function () {
            if (widget != null) {
               fact = Object.create(valueFacts[item.type + ":" + item.address]);

               fact.widget.unit = $("#unit").val();
               fact.widget.scalemax = parseFloat($("#factor").val()) || 1.0;
               fact.widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
               fact.widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
               fact.widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
               fact.widget.critmin = parseFloat($("#critmin").val()) || -1;
               fact.widget.critmax = parseFloat($("#critmax").val()) || -1;
               fact.widget.imgon = $("#imgon").val();
               fact.widget.imgoff = $("#imgoff").val();
               fact.widget.widgettype = parseInt($("#widgettype").val());
               fact.widget.showpeak = $("#peak").is(':checked');
               fact.widget.widthfactor = $("#widthfactor").val();
               initWidget(widget, fact);
               updateWidget(widget, true, fact);
            }
         },
         'Ok': function () {
            if (widget != null) {
               fact = Object.create(valueFacts[item.type + ":" + item.address]);

               fact.widget.unit = $("#unit").val();
               fact.widget.scalemax = parseFloat($("#factor").val()) || 1.0;
               fact.widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
               fact.widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
               fact.widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
               fact.widget.critmin = parseFloat($("#critmin").val()) || -1;
               fact.widget.critmax = parseFloat($("#critmax").val()) || -1;
               fact.widget.imgon = $("#imgon").val();
               fact.widget.imgoff = $("#imgoff").val();
               fact.widget.widgettype = parseInt($("#widgettype").val());
               fact.widget.showpeak = $("#peak").is(':checked');
               fact.widget.widthfactor = $("#widthfactor").val();

               initWidget(widget, fact);
               updateWidget(widget, true, fact);
            }

            var jsonObj = {};
            jsonObj["widget"] = {};
            jsonObj["type"] = item.type;
            jsonObj["address"] = item.address;
            if ($("#unit").length)
               jsonObj["widget"]["unit"] = $("#unit").val();
            if ($("#factor").length)
               jsonObj["widget"]["factor"] = parseFloat($("#factor").val());
            if ($("#scalemax").length)
               jsonObj["widget"]["scalemax"] = parseFloat($("#scalemax").val());
            if ($("#scalemin").length)
               jsonObj["widget"]["scalemin"] = parseFloat($("#scalemin").val());
            if ($("#scalestep").length)
               jsonObj["widget"]["scalestep"] = parseFloat($("#scalestep").val());
            if ($("#critmin").length)
               jsonObj["widget"]["critmin"] = parseFloat($("#critmin").val());
            if ($("#critmax").length)
               jsonObj["widget"]["critmax"] = parseFloat($("#critmax").val());
            if ($("#imgon").length)
               jsonObj["widget"]["imgon"] = $("#imgon").val();
            if ($("#imgoff").length)
               jsonObj["widget"]["imgoff"] = $("#imgoff").val();

            jsonObj["widget"]["widthfactor"] = parseFloat($("#widthfactor").val());
            jsonObj["widget"]["widgettype"] = parseInt($("#widgettype").val());
            jsonObj["widget"]["showpeak"] = $("#peak").is(':checked');

            var jsonArray = [];
            jsonArray[0] = jsonObj;
            socket.send({ "event" : "storeiosetup", "object" : jsonArray });

            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });
}
