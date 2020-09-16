/*
 *  dashboard.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initDashboard(widgets, root)
{
   if (!widgets) {
      console.log("Fatal: Missing payload!");
      return;
   }

   // clean page content

   root.innerHTML = "";
   var elem = document.createElement("div");
   elem.className = "widget rounded-border";
   elem.innerHTML = "<div class=\"widget-title\">Aktualisiert</div>\n<div id=\"refreshTime\" class=\"widget-value\"></div>";
   root.appendChild(elem);

   // build page content

   for (var i = 0; i < widgets.length; i++)
   {
      var widget = widgets[i];

      switch(widget.widgettype) {
      case 0:           // Symbol
         var html = "";
         var ctrlButtons = widget.name == "Pool Light" && config.poolLightColorToggle == 1;

         html += "  <button class=\"widget-title\" type=\"button\" onclick=\"toggleMode(" + widget.address + ", '" + widget.type + "')\">" + widget.title + "</button>\n";
         html += "  <button class=\"widget-main\" type=\"button\" onclick=\"toggleIo(" + widget.address + ",'" + widget.type + "')\" >\n";
         html += "    <img id=\"widget" + widget.type + widget.address + "\" />\n";
         html += "   </button>\n";

         if (ctrlButtons)
         {
            html += "  <div class=\"widget-ctrl2\">\n";
            html += "    <button type=\"button\" onclick=\"toggleIoNext(" + widget.address + ",'" + widget.type + "')\">\n";
            html += "      <img src=\"img/icon/right.png\" />\n";
            html += "    </button>\n";
            html += "  </div>\n";
         }

         html += "<div id=\"progress" + widget.type + widget.address + "\" class=\"widget-progress\">";
         html += "   <div id=\"progressBar" + widget.type + widget.address + "\" class=\"progress-bar\" style=\"visible\"></div>";
         html += "</div>";

         var elem = document.createElement("div");

         if (ctrlButtons)
            elem.className = "widgetCtrl rounded-border";
         else
            elem.className = "widget rounded-border";

         elem.setAttribute("id", "div" + widget.type + widget.address);
         elem.innerHTML = html;
         root.appendChild(elem);

         document.getElementById("progress" + widget.type + widget.address).style.visibility = "hidden";

         break;
      case 1:           // Gauge
         var html = "  <div class=\"widget-title\">" + widget.title + "</div>";
         var elem = document.createElement("div");

         if (false && widget.address != 90680919) {
            html += "  <svg class=\"widget-svg\" viewBox=\"0 0 1000 600\" preserveAspectRatio=\"xMidYMin slice\">\n";
            html += "    <path id=\"pb" + widget.type + widget.address + "\"/>\n";
            html += "    <path class=\"data-arc\" id=\"pv" + widget.type + widget.address + "\"/>\n";
            html += "    <path class=\"data-peak\" id=\"pp" + widget.type + widget.address + "\"/>\n";
            html += "    <text id=\"value" + widget.type + widget.address + "\" class=\"gauge-value\" text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"500\" y=\"450\" font-size=\"140\" font-weight=\"bold\"></text>\n";
            html += "    <text id=\"sMin" + widget.type + widget.address + "\" class=\"scale-text\" text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"50\" y=\"550\"></text>\n";
            html += "    <text id=\"sMax" + widget.type + widget.address + "\" class=\"scale-text\" text-anchor=\"middle\" alignment-baseline=\"middle\" x=\"950\" y=\"550\"></text>\n";
            html += "  </svg>\n";

            elem.className = "widgetGauge rounded-border participation";
            elem.setAttribute("id", "widget" + widget.type + widget.address);
            elem.setAttribute("onclick", "toggleChartDialog('" + widget.type + "'," + widget.address + ")");
         }
         else {
            html += "<div id=\"peak" + widget.type + widget.address + "\" class=\"chart-peak\"></div>";
            html += "<div id=\"value" + widget.type + widget.address + "\" class=\"chart-value\"></div>";
            html += "<div class=\"chart-canvas-container\"><canvas id=\"widget" + widget.type + widget.address + "\" class=\"chart-canvas\"></canvas></div>";

            elem.className = "widgetChart rounded-border";
            elem.setAttribute("onclick", "toggleChartDialog('" + widget.type + "'," + widget.address + ")");
         }

         elem.innerHTML = html;
         root.appendChild(elem);

         break;

      default:   // type 2(Text), 3(value), ...
         var html = "";
         html += "<div class=\"widget-title\">" + widget.title + "</div>\n";
         html += "<div id=\"widget" + widget.type + widget.address + "\" class=\"widget-value\"></div>\n";

         var elem = document.createElement("div");
         elem.className = "widget rounded-border";
         elem.innerHTML = html;
         root.appendChild(elem);

         break;
      }
   }
}

function updateDashboard(sensors)
{
   // console.log("updateDashboard");

   document.getElementById("refreshTime").innerHTML = lastUpdate;

   if (sensors)
   {
      for (var i = 0; i < sensors.length; i++)
      {
         var sensor = sensors[i];

         if (sensor.widgettype == 0)         // Symbol
         {
            var modeStyle = sensor.options == 3 && sensor.mode == 'manual' ? "background-color: #a27373;" : "";
            $("#widget" + sensor.type + sensor.address).attr("src", sensor.image);
            $("#div" + sensor.type + sensor.address).attr("style", modeStyle);

            var e = document.getElementById("progress" + sensor.type + sensor.address);
            if (e != null)
               e.style.visibility = (sensor.next == null || sensor.next == 0) ? "hidden" : "visible";
            // else console.log("Element '" + "progress" + sensor.type + sensor.address + "' not found");

            if (sensor.mode == "auto" && sensor.next > 0) {
               var pWidth = 100;
               var s = sensor;
               var id = sensor.type + sensor.address;
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
         else if (sensor.widgettype == 1)    // Gauge
         {
            var elem = $("#widget" + sensor.type + sensor.address);

            if (false && sensor.address != 90680919) {
               var value = sensor.value.toFixed(2);
               var scaleMax = !sensor.scalemax || sensor.unit == '%' ? 100 : sensor.scalemax.toFixed(0);
               var scaleMin = value >= 0 ? "0" : Math.ceil(value / 5) * 5 - 5;

               if (scaleMax < Math.ceil(value))       scaleMax = value;
               if (scaleMax < Math.ceil(sensor.peak)) scaleMax = sensor.peak.toFixed(0);

               $("#sMin" + sensor.type + sensor.address).text(scaleMin);
               $("#sMax" + sensor.type + sensor.address).text(scaleMax);
               $("#value" + sensor.type + sensor.address).text(value + " " + sensor.unit);

               var ratio = (value - scaleMin) / (scaleMax - scaleMin);
               var peak = (sensor.peak.toFixed(2) - scaleMin) / (scaleMax - scaleMin);

               $("#pb" + sensor.type + sensor.address).attr("d", "M 950 500 A 450 450 0 0 0 50 500");
               $("#pv" + sensor.type + sensor.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, -90, ratio * 180.0 - 90));
               $("#pp" + sensor.type + sensor.address).attr("d", svg_circle_arc_path(500, 500, 450 /*radius*/, peak * 180.0 - 91, peak * 180.0 - 90));
            }
            else {
               var jsonRequest = {};
               $("#peak" + sensor.type + sensor.address).text(sensor.peak.toFixed(2) + " " + sensor.unit);
               $("#value" + sensor.type + sensor.address).text(sensor.value.toFixed(2) + " " + sensor.unit);
               prepareChartRequest(jsonRequest, sensor.type + ":0x" + sensor.address.toString(16) , 0, 1, "chartwidget");
               socket.send({ "event" : "chartdata", "object" : jsonRequest });
            }
         }
         else if (sensor.widgettype == 2)      // Text
         {
            $("#widget" + sensor.type + sensor.address).html(sensor.text);
         }
         else if (sensor.widgettype == 3)      // plain value
         {
            $("#widget" + sensor.type + sensor.address).html(sensor.value + " " + sensor.unit);
         }

         // console.log(i + ": " + sensor.name + " / " + sensor.title);
      }
   }
}

function toggleChartDialog(type, address)
{
   var dialog = document.querySelector('dialog');

   if (type != "" && !dialog.hasAttribute('open')) {
      var canvas = document.querySelector("#chartDialog");
      canvas.getContext('2d').clearRect(0, 0, canvas.width, canvas.height);
      chartDialogSensor = type + address;

      // show the dialog

      dialog.setAttribute('open', 'open');

      var jsonRequest = {};
      prepareChartRequest(jsonRequest, type + ":0x" + address.toString(16) , 0, 1, "chartdialog");
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
