/*
 *  list.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initList(update = false)
{
   if (!allWidgets)
   {
      console.log("Fatal: Missing payload!");
      return;
   }

   if (!update) {
      $('#stateContainer').removeClass('hidden');
      $('#container').removeClass('hidden');

      // ...
   }

   document.getElementById("container").innerHTML = '<div id="listContainer" class="rounded-border listContainer"</div>';
   var root = document.getElementById("listContainer");

   // deamon state

   var rootState = document.getElementById("stateContainer");
   var html = "";

   if (daemonState.state != null && daemonState.state == 0)
   {
      html +=  '<div id="aStateOk"><span style="text-align: center;">' + pageTitle + ' ONLINE</span></div><br/>';
      html +=  "<div><span>Läuft seit:</span><span>" + daemonState.runningsince + "</span>       </div>\n";
      html +=  "<div><span>Version:</span> <span>" + daemonState.version + "</span></div>\n";
      html +=  "<div><span>CPU-Last:</span><span>" + daemonState.average0 + " " + daemonState.average1 + " "  + daemonState.average2 + " " + "</span>           </div>\n";
   }
   else
   {
      html += '<div id="aStateFail">ACHTUNG:<br/>' + pageTitle + ' OFFLINE</div>';
   }

   rootState.innerHTML = "";
   var elem = document.createElement("div");
   elem.className = "rounded-border daemonState";
   elem.innerHTML = html;
   rootState.appendChild(elem);

   // clean page content

   root.innerHTML = "";

   var elem = document.createElement("div");
   elem.className = "chartTitle rounded-border";
   elem.innerHTML = "<center id=\"refreshTime\" \>";
   root.appendChild(elem);

   // build page content

   for (var i = 0; i < allWidgets.length; i++)
   {
      var html = "";
      var widget = allWidgets[i];
      var id = "id=\"widget" + widget.type + widget.address + "\"";
      var fact = valueFacts[widget.type + ":" + widget.address];

      if (fact == null || fact == undefined) {
         console.log("Fact for widget '" + widget.type + ":" + widget.address + "' not found, ignoring");
         continue;
      }

      var title = fact.usrtitle != '' && fact.usrtitle != undefined ? fact.usrtitle : fact.title;

      if (fact.widget.widgettype == 1 || fact.widget.widgettype == 3) {      // 1 Gauge or 3 Value
         html += "<span class=\"listFirstCol\"" + id + ">" + widget.value.toFixed(2) + "&nbsp;" + fact.widget.unit;
         html += "&nbsp; <p style=\"display:inline;font-size:12px;font-style:italic;\">(" + (widget.peak != null ? widget.peak.toFixed(2) : "  ") + ")</p>";
         html += "</span>";
      }
      else if (fact.widget.widgettype == 0) {   // 0 Symbol
         html += "   <div class=\"listFirstCol\" onclick=\"toggleIo(" + fact.address + ",'" + fact.type + "')\"><img " + id + "/></div>\n";
      }
      else {   // 2 Text
         html += "<div class=\"listFirstCol\"" + id + "></div>";
      }

      html += "<span class=\"listSecondCol listText\" >" + title + "</span>";

      var elem = document.createElement("div");
      elem.className = "listRow";
      elem.innerHTML = html;
      root.appendChild(elem);
   }

   updateList(allWidgets);
}

function updateList(widgets)
{
   var d = new Date();     // #TODO use SP:0x4 instead of Date()
   document.getElementById("refreshTime").innerHTML = "Messwerte von " + d.toLocaleTimeString();

   for (var i = 0; i < widgets.length; i++)
   {
      var widget = widgets[i];
      var fact = valueFacts[widget.type + ":" + widget.address];

      if (fact == null || fact == undefined) {
         console.log("Fact for widget '" + widget.type + ":" + widget.address + "' not found, ignoring");
         continue;
      }

      var id = "#widget" + fact.type + fact.address;

      if (fact.widget.widgettype == 1 || fact.widget.widgettype == 3) {
         var peak = widget.peak != null ? widget.peak.toFixed(2) : "  ";
         $(id).html(widget.value.toFixed(2) + "&nbsp;" + fact.widget.unit +
                    "&nbsp; <p style=\"display:inline;font-size:12px;font-style:italic;\">(" + peak + ")</p>");
      }
      else if (fact.widget.widgettype == 0) {   // Symbol
         var image = widget.value != 0 ? fact.widget.imgon : fact.widget.imgoff;
         $(id).attr("src", image);
      }
      else if (fact.widget.widgettype == 2 || fact.widget.widgettype == 7) {   // Text, PlainText
         $(id).html(widget.text);
      }
      else {
         if (widget.value == undefined)
            console.log("Missing value for " + widget.type + ":" + widget.address);
         else
            $(id).html(widget.value.toFixed(0));
      }

      // console.log(i + ") " + fact.widget.widgettype + " : " + title + " / " + widget.value + "(" + id + ")");
   }
}

//function updateList(sensors)
//{
//   var refreshTime = document.getElementById("refreshTime");
//
//   if (refreshTime != null)
//      refreshTime.innerHTML = "Messwerte von " + lastUpdate;
//
//   // heating state
//
//   var rootStateS3200 = document.getElementById("stateContainerS3200");
//
//   switch (s3200State.state) {
//      case 0:  style = "aStateFail";     break;
//      case 3:  style = "aStateHeating";  break;
//      case 19: style = "aStateOk";       break;
//      default: style = "aStateOther"
//   }
//
//   d = new Date(s3200State.time * 1000);
//
//   html = '<div><span id="' + style + '">' + s3200State.stateinfo + '</span></div>\n';
//   html += '<br/>\n';
//   html += '<div style="display:flex;"><span style="width:30%;display:inline-block;">Uhrzeit:</span><span>' + d.toLocaleTimeString() + '</span></div>\n';
//   html += '<div style="display:flex;"><span style="width:30%;display:inline-block;">Betriebsmodus: </span><span>' + s3200State.modeinfo + '</span></div>\n';
//
//   rootStateS3200.innerHTML = html;
//
//   // state image
//   var rootStateImage = document.getElementById("stateContainerImage");
//
//   html = '<img src="' + s3200State.image + '">\n';
//
//   rootStateImage.innerHTML = html;
//
//   // daemon state
//   var rootStateP4 = document.getElementById("stateContainerP4");
//
//   if (daemonState.state != null && daemonState.state == 0)
//   {
//      html =  '<div id="aStateOk"><span style="text-align: center;">Heating Control ONLINE</span></div>';
//      html +=  '<br/>\n';
//      html +=  '<div style="display:flex;"><span style="width:30%;display:inline-block;">Läuft seit:</span><span display="inline-block">' + daemonState.runningsince + '</span></div>\n';
//      html +=  '<div style="display:flex;"><span style="width:30%;display:inline-block;">Version:</span> <span display="inline-block">' + daemonState.version + '</span></div>\n';
//      html +=  '<div style="display:flex;"><span style="width:30%;display:inline-block;">CPU-Last:</span><span display="inline-block">' + daemonState.average0 + " " + daemonState.average1 + ' '  + daemonState.average2 + ' ' + '</span></div>\n';
//   }
//   else
//   {
//      html = '<div id="aStateFail">ACHTUNG:<br/>Heating Control OFFLINE</div>\n';
//   }
//
//   rootStateP4.innerHTML = html;
//
//
//   for (var i = 0; i < sensors.length; i++)
//   {
//      var sensor = sensors[i];
//      var id = "#widget" + sensor.type + sensor.address;
//
//      if (sensor.widgettype == 1 || sensor.widgettype == 3) {
//         $(id).html(sensor.value.toFixed(2) + "&nbsp;" + sensor.unit +
//                    "&nbsp; <p style=\"display:inline;font-size:12px;font-style:italic;\">(" + sensor.peakmin.toFixed(2) + "/" + sensor.peak.toFixed(2) + ")</p>");
//      }
//      else if (sensor.widgettype == 0)    // Symbol
//         $(id).attr("src", sensor.image);
//      else if (sensor.widgettype == 2)    // Text
//         $(id).html(sensor.text);
//      else
//         $(id).html(sensor.value.toFixed(0));
//
//      // console.log(i + ") " + sensor.widgettype + " : " + sensor.title + " / " + sensor.value + "(" + id + ")");
//   }
//}
