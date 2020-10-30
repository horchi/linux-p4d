/*
 *  setup.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initConfig(configuration, root)
{
   var lastCat = "";

   console.log(JSON.stringify(configuration, undefined, 4));

   configuration.sort(function(a, b) {
      return a.category.localeCompare(b.category);
   });

   for (var i = 0; i < configuration.length; i++)
   {
      var item = configuration[i];
      var html = "";

      if (lastCat != item.category)
      {
         html += "<div class=\"rounded-border seperatorTitle1\">" + item.category + "</div>";
         var elem = document.createElement("div");
         elem.innerHTML = html;
         root.appendChild(elem);
         html = "";
         lastCat = item.category;
      }

      html += "    <span>" + item.title + ":</span>\n";

      if (item.descrtiption == "")
         item.descrtiption = "&nbsp;";  // on totally empty the line height not fit :(

      switch (item.type) {
      case 0:     // integer
         html += "    <span><input id=\"input_" + item.name + "\" class=\"rounded-border inputNum\" type=\"number\" value=\"" + item.value + "\"/></span>\n";
         html += "    <span class=\"inputComment\">" + item.descrtiption + "</span>\n";
         break;

      case 1:     // number (float)
         html += "    <span><input id=\"input_" + item.name + "\" class=\"rounded-border inputFloat\" type=\"number\" step=\"0.1\" value=\"" + item.value + "\"/></span>\n";
         html += "    <span class=\"inputComment\">" + item.descrtiption + "</span>\n";
         break;

      case 2:     // string

         html += "    <span><input id=\"input_" + item.name + "\" class=\"rounded-border input\" type=\"text\" value=\"" + item.value + "\"/></span>\n";
         html += "    <span class=\"inputComment\">" + item.descrtiption + "</span>\n";
         break;

      case 3:     // boolean
         html += "    <span><input id=\"checkbox_" + item.name + "\" class=\"rounded-border input\" style=\"width:auto;\" type=\"checkbox\" " + (item.value == 1 ? "checked" : "") + "/></span>\n";
         html += "    <span class=\"inputComment\">" + item.descrtiption + "</span>\n";
         break;

      case 4:     // range
         var n = 0;

         if (item.value != "")
         {
            var ranges = item.value.split(",");

            for (n = 0; n < ranges.length; n++)
            {
               var range = ranges[n].split("-");
               var nameFrom = item.name + n + "From";
               var nameTo = item.name + n + "To";

               if (!range[1]) range[1] = "";
               if (n > 0) html += "  <span/>  </span>\n";
               html += "   <span>\n";
               html += "     <input id=\"range_" + nameFrom + "\" type=\"text\" class=\"rounded-border inputTime\" value=\"" + range[0] + "\"/> -";
               html += "     <input id=\"range_" + nameTo + "\" type=\"text\" class=\"rounded-border inputTime\" value=\"" + range[1] + "\"/>\n";
               html += "   </span>\n";
               html += "   <span></span>\n";
            }
         }

         var nameFrom = item.name + n + "From";
         var nameTo = item.name + n + "To";

         if (n > 0) html += "  <span/>  </span>\n";
         html += "  <span>\n";
         html += "    <input id=\"range_" + nameFrom + "\" value=\"\" type=\"text\" class=\"rounded-border inputTime\" /> -";
         html += "    <input id=\"range_" + nameTo + "\" value=\"\" type=\"text\" class=\"rounded-border inputTime\" />\n";
         html += "  </span>\n";
         html += "  <span class=\"inputComment\">" + item.descrtiption + "</span>\n";

         break;

      case 5:    // choice
         html += '<span>\n';
         html += '  <select id="input_' + item.name + '" class="rounded-border input" name="style">\n';

         for (var o = 0; o < item.options.length; o++) {
            var option = item.options[o];
            var sel = item.value == option ? 'SELECTED' : '';
            html += '    <option value="' + option + '" ' + sel + '>' + option + '</option>\n';
         }

         html += '  </select>\n';
         html += '</span>\n';
         break;
      }

      var elem = document.createElement("div");
      elem.innerHTML = html;
      root.appendChild(elem);
   }
}

function initTables(what)
{
   showInfoDialog("init gestartet (kann bis zu einer Minute dauern) ...");
   socket.send({ "event" : "inittables", "object" : { "action" : what } });
}

window.storeConfig = function()
{
   var jsonObj = {};
   var rootConfig = document.getElementById("configContainer");

   console.log("storeSettings");

   var elements = rootConfig.querySelectorAll("[id^='input_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      jsonObj[name] = elements[i].value;
   }

   var elements = rootConfig.querySelectorAll("[id^='checkbox_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      jsonObj[name] = (elements[i].checked ? "1" : "0");
   }

   var elements = rootConfig.querySelectorAll("[id^='range_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      if (name.match("0From$")) {
         name = name.substring(0, name.indexOf("0From"));
         jsonObj[name] = toTimeRangesString("range_" + name);
         console.log("value: " + jsonObj[name]);
      }
   }

   // console.log(JSON.stringify(jsonObj, undefined, 4));

   socket.send({ "event" : "storeconfig", "object" : jsonObj });

   // try force reload (not working)
   // location.reload(true);
   // window.location.href = window.location.href;
}

window.resetPeaks = function()
{
   socket.send({ "event" : "resetpeaks", "object" : { "what" : "all" } });
}

var filterActive = false;

function filterIoSetup()
{
   filterActive = !filterActive;
   console.log("filterIoSetup: " + filterActive);

   $("#filterIoSetup").html(filterActive ? "[aktive]" : "[alle]");
   socket.send({ "event" : "iosetup", "object" : { "filter" : filterActive } });
}

function initIoSetup(valueFacts, root)
{
   // console.log(JSON.stringify(valueFacts, undefined, 4));

   document.getElementById("ioValues").innerHTML = "";
   document.getElementById("ioDigitalOut").innerHTML = "";
   document.getElementById("ioDigitalIn").innerHTML = "";
   document.getElementById("ioAnalogOut").innerHTML = "";
   document.getElementById("ioOneWire").innerHTML = "";
   document.getElementById("ioOther").innerHTML = "";
   document.getElementById("ioScripts").innerHTML = "";

   for (var i = 0; i < valueFacts.length; i++)
   {
      var item = valueFacts[i];
      var root = null;

      var usrtitle = item.usrtitle != null ? item.usrtitle : "";
      var html = "<td id=\"row_" + item.type + item.address + "\" data-address=\"" + item.address + "\" data-type=\"" + item.type + "\" >" + item.title + "</td>";
      html += "<td class=\"tableMultiColCell\">";
      html += "  <input id=\"usrtitle_" + item.type + item.address + "\" class=\"rounded-border inputSetting\" type=\"text\" value=\"" + usrtitle + "\"/>";
      html += "</td>";
      if (item.type != "DO" && item.type != "SC")
         html += "<td class=\"tableMultiColCell\"><input id=\"scalemax_" + item.type + item.address + "\" class=\"rounded-border inputSetting\" type=\"number\" value=\"" + item.scalemax + "\"/></td>";
      else
         html += "<td class=\"tableMultiColCell\"></td>";
      html += "<td style=\"text-align:center;\">" + item.unit + "</td>";
      html += "<td><input id=\"state_" + item.type + item.address + "\" class=\"rounded-border inputSetting\" type=\"checkbox\" " + (item.state == 1 ? "checked" : "") + " /></td>";
      html += "<td>" + item.type + ":0x" + item.address.toString(16) + "</td>";

      if (item.type == "VA")
         html += "<td>" + item.value + item.unit + "</td>";

      switch (item.type) {
         case 'VA': root = document.getElementById("ioValues");     break
         case 'DI': root = document.getElementById("ioDigitalIn");  break
         case 'DO': root = document.getElementById("ioDigitalOut"); break
         case 'AO': root = document.getElementById("ioAnalogOut");  break
         case 'W1': root = document.getElementById("ioOneWire");    break
         case 'SP': root = document.getElementById("ioOther");      break
         case 'SC': root = document.getElementById("ioScripts");    break
      }

      if (root != null)
      {
         var elem = document.createElement("tr");
         elem.innerHTML = html;
         root.appendChild(elem);
      }
   }
}

function toTimeRangesString(base)
{
   var times = "";

   for (var i = 0; i < 10; i++) {
      var id = "#" + base + i;

      if ($(id + "From") && $(id + "From").val() && $(id + "To") && $(id + "To").val()) {
         if (times != "") times += ",";
         times += $(id + "From").val() + "-" + $(id + "To").val();
      }
      else {
         break;
      }
   }

   return times;
}

window.storeIoSetup = function()
{
   var jsonArray = [];
   var rootSetup = document.getElementById("ioSetupContainer");
   var elements = rootSetup.querySelectorAll("[id^='row_']");

   console.log("storeIoSetup");

   for (var i = 0; i < elements.length; i++) {
      var jsonObj = {};

      var type = $(elements[i]).data("type");
      var address = $(elements[i]).data("address");
      // console.log("  loop for: " + type + ":" + address);
      // console.log("   - usrtitle: " + $("#usrtitle_" + type + address).val());

      jsonObj["type"] = type;
      jsonObj["address"] = address;
      if ($("#scalemax_" + type + address).length)
         jsonObj["scalemax"] = parseInt($("#scalemax_" + type + address).val());
      jsonObj["usrtitle"] = $("#usrtitle_" + type + address).val();
      jsonObj["state"] = $("#state_" + type + address).is(":checked") ? 1 : 0;
      jsonArray[i] = jsonObj;
   }

   socket.send({ "event" : "storeiosetup", "object" : jsonArray });
}
