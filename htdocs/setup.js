/*
 *  setup.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var configCategories = {};
var ioSections = {};
var theConfigdetails = {}

function initConfig(configdetails)
{
   theConfigdetails = configdetails;

   $('#container').removeClass('hidden');

   document.getElementById("container").innerHTML =
      '<div id="setupContainer" class="rounded-border inputTableConfig">';

   var root = document.getElementById("setupContainer");
   var lastCat = "";
   root.innerHTML = "";

   $('#btnInitMenu').bind('click', function(event) {
      if (event.ctrlKey)
         initTables('menu-force');
      else
         initTables('menu');
   });

   // console.log(JSON.stringify(configdetails, undefined, 4));

   for (var i = 0; i < configdetails.length; i++) {
      var item = configdetails[i];
      var html = "";

      if (lastCat != item.category) {
         if (!configCategories.hasOwnProperty(item.category))
            configCategories[item.category] = true;

         var sign = configCategories[item.category] ? '&#11013;' : '&#11015;';
         html += '<div class="rounded-border seperatorFold" onclick="foldCategory(\'' + item.category + '\')">' + sign + ' ' + item.category + "</div>";
         var elem = document.createElement("div");
         elem.innerHTML = html;
         root.appendChild(elem);
         html = "";
         lastCat = item.category;
      }

      if (!configCategories[item.category]) {
         console.log("!!!! skip category: " + item.category)
         continue;
      }

      html += "    <span>" + item.title + ":</span>\n";

      if (item.description == "")
         item.description = "&nbsp;";  // on totally empty the line height not fit :(

      switch (item.type) {
      case 0:     // integer
         html += "    <span><input id=\"input_" + item.name + "\" class=\"rounded-border inputNum\" type=\"number\" value=\"" + item.value + "\"/></span>\n";
         html += "    <span class=\"inputComment\">" + item.description + "</span>\n";
         break;

      case 1:     // number (float)
         html += "    <span><input id=\"input_" + item.name + "\" class=\"rounded-border inputFloat\" type=\"number\" step=\"0.1\" value=\"" + parseFloat(item.value.replace(',', '.')) + "\"/></span>\n";
         html += "    <span class=\"inputComment\">" + item.description + "</span>\n";
         break;

      case 2:     // string

         html += "    <span><input id=\"input_" + item.name + "\" class=\"rounded-border input\" type=\"search\" value=\"" + item.value + "\"/></span>\n";
         html += "    <span class=\"inputComment\">" + item.description + "</span>\n";
         break;

      case 3:     // boolean
         html += "    <span><input id=\"checkbox_" + item.name + "\" class=\"rounded-border input\" style=\"width:auto;\" type=\"checkbox\" " + (item.value == 1 ? "checked" : "") + "/>" +
            '<label for="checkbox_' + item.name + '"></label></span></span>\n';
         html += "    <span class=\"inputComment\">" + item.description + "</span>\n";
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
               html += "     <input id=\"range_" + nameFrom + "\" type=\"search\" class=\"rounded-border inputTime\" value=\"" + range[0] + "\"/> -";
               html += "     <input id=\"range_" + nameTo + "\" type=\"search\" class=\"rounded-border inputTime\" value=\"" + range[1] + "\"/>\n";
               html += "   </span>\n";
               html += "   <span></span>\n";
            }
         }

         var nameFrom = item.name + n + "From";
         var nameTo = item.name + n + "To";

         if (n > 0) html += "  <span/>  </span>\n";
         html += "  <span>\n";
         html += "    <input id=\"range_" + nameFrom + "\" value=\"\" type=\"search\" class=\"rounded-border inputTime\" /> -";
         html += "    <input id=\"range_" + nameTo + "\" value=\"\" type=\"search\" class=\"rounded-border inputTime\" />\n";
         html += "  </span>\n";
         html += "  <span class=\"inputComment\">" + item.description + "</span>\n";

         break;

      case 5:    // choice
         html += '<span>\n';
         html += '  <select id="input_' + item.name + '" class="rounded-border input" name="style">\n';

         if (item.options != null) {
            for (var o = 0; o < item.options.length; o++) {
               var option = item.options[o];
               var sel = item.value == option ? 'SELECTED' : '';
               html += '    <option value="' + option + '" ' + sel + '>' + option + '</option>\n';
            }
         }

         html += '  </select>\n';
         html += '</span>\n';
         break;

      case 6:    // MultiSelect
         html += '<span style="width:75%;">\n';
         html += '  <input style="width:inherit;" class="rounded-border input" ' +
            ' id="mselect_' + item.name + '" data-index="' + i +
            '" data-value="' + item.value + '" type="text" value=""/>\n';
         html += '</span>\n';
         break;

      case 7:    // BitSelect
         html += '<span id="bmaskgroup_' + item.name + '" style="width:75%;">';

         var array = item.value.split(',');

         for (var o = 0; o < item.options.length; o++) {
            var checked = false;
            for (var n = 0; n < array.length; n++) {
               if (array[n] == item.options[o])
                  checked = true;
            }
            html += '<input class="rounded-border input" id="bmask' + item.name + '_' + item.options[o] + '"' +
               ' type="checkbox" ' + (checked ? 'checked' : '') + '/>' +
               '<label for="bmask' + item.name + '_' + item.options[o] + '">' + item.options[o] + '</label>';
         }
         html += '</span>';

         break;
      }

      var elem = document.createElement("div");
      elem.innerHTML = html;
      root.appendChild(elem);
   }

   $('input[id^="mselect_"]').each(function () {
      var item = configdetails[$(this).data("index")];
      $(this).autocomplete({
         source: item.options,
         multiselect: true});
      if ($(this).data("value").trim() != "") {
         // console.log("set", $(this).data("value"));
         setAutoCompleteValues($(this), $(this).data("value").trim().split(","));
      }
   });

   $("#container").height($(window).height() - $("#menu").height() - 8);
   window.onresize = function() {
      $("#container").height($(window).height() - $("#menu").height() - 8);
   };
}

function initTables(what)
{
   showProgressDialog();
   socket.send({ "event" : "inittables", "object" : { "action" : what } });
}

function storeConfig()
{
   var jsonObj = {};
   var rootConfig = document.getElementById("container");

   console.log("storeSettings");

   var elements = rootConfig.querySelectorAll("[id^='input_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1); {
         if (elements[i].getAttribute('type') == 'number' && elements[i].getAttribute('step') != undefined &&
             elements[i].value != '')
            jsonObj[name] = parseFloat(elements[i].value).toLocaleString("de-DE");
         else
            jsonObj[name] = elements[i].value;
      }
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
         // console.log("value: " + jsonObj[name]);
      }
   }

   // data type 6 - 'MultiSelect' -> as string

   var elements = rootConfig.querySelectorAll("[id^='mselect_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      var value = getAutoCompleteValues($("#mselect_" + name));
      value = value.replace(/ /g, ',');
      jsonObj[name] = value;
   }

   // data type 7 - 'BitSelect' -> as string

   var elements = rootConfig.querySelectorAll("[id^='bmaskgroup_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      var bits = rootConfig.querySelectorAll("[id^='bmask" + name + "_']");
      var value = '';

      for (var i = 0; i < bits.length; i++) {
         var o = bits[i].id.substring(bits[i].id.indexOf("_") + 1);
         if (bits[i].checked)
            value += o + ','
      }
      console.log("store bitmak: " + value);
      jsonObj[name] = value;
   }

   // console.log(JSON.stringify(jsonObj, undefined, 4));

   socket.send({ "event" : "storeconfig", "object" : jsonObj });
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

window.resetPeaks = function()
{
   if (confirm("Peaks zurücksetzen?"))
      socket.send({ "event" : "reset", "object" : { "what" : "peaks" } });
}

var filterActive = false;

function filterIoSetup()
{
   filterActive = !filterActive;
   console.log("filterIoSetup: " + filterActive);

   $("#filterIoSetup").html(filterActive ? "[aktive]" : "[alle]");
   initIoSetup(valueFacts);
}

function doIncrementalFilterIoSetup()
{
   initIoSetup(valueFacts);
}

function foldCategory(category)
{
   configCategories[category] = !configCategories[category];
   console.log(category + ' : ' + configCategories[category]);
   initConfig(theConfigdetails);
}

function foldSection(sectionId)
{
   ioSections[sectionId].visible = !ioSections[sectionId].visible;
   console.log(sectionId + ' : ' + ioSections[sectionId].visible);
   initIoSetup(valueFacts);
}

function tableHeadline(title, sectionId)
{
   if (!ioSections[sectionId].visible)
      return '  <div id="fold_' + sectionId + '" class="rounded-border seperatorFold" onclick="foldSection(\'' + sectionId + '\')">' + '&#11015; ' + title + '</div>';

   return '  <div id="fold_' + sectionId + '" class="rounded-border seperatorFold" onclick="foldSection(\'' + sectionId + '\')">' + '&#11013; ' + title + '</div>' +
      '  <table class="tableMultiCol">' +
      '    <thead>' +
      '      <tr>' +
      '        <td style="width:20%;">Name</td>' +
      '        <td style="width:25%;">Titel</td>' +
      '        <td style="width:4%;">Einheit</td>' +
      '        <td style="width:3%;">Aktiv</td>' +
      '        <td style="width:3%;">Aufzeichnen</td>' +
      '        <td style="width:6%;">ID</td>' +
      '        <td style="width:10%;">Gruppe</td>' +
      '      </tr>' +
      '    </thead>' +
      '    <tbody id="' + sectionId + '">' +
      '    </tbody>' +
      '  </table>';
}

function initIoSetup(valueFacts)
{
   // console.log(JSON.stringify(valueFacts, undefined, 4));

   $('#container').removeClass('hidden');

   for (var key in ioSections)
      ioSections[key].exist = false;

   var html = '<div id="ioSetupContainer">';

   for (var i = 0; i < valueTypes.length; i++) {
      var section = 'io' + valueTypes[i].title.replace(' ', '');
      if (!ioSections.hasOwnProperty(section)) {
         ioSections[section] = {};
         ioSections[section].visible = true;
      }
      if (!ioSections[section].exist) {
         html += tableHeadline(valueTypes[i].title, section);
         ioSections[section].exist = true;
      }
   }

   html += '</div>';
   document.getElementById("container").innerHTML = html;

   for (var key in ioSections) {
      if (ioSections[key].visible && document.getElementById(key))
         document.getElementById(key).innerHTML = "";
   }

   var filterExpression = null;

   if ($("#incSearchName").val() != "")
      filterExpression = new RegExp($("#incSearchName").val());

   for (var key in valueFacts) {
      var sectionId = "";
      var item = valueFacts[key];
      var usrtitle = item.usrtitle != null ? item.usrtitle : "";

      if (!item.state && filterActive)
         continue;

      if (filterExpression && !filterExpression.test(item.title) && !filterExpression.test(usrtitle))
         continue;

      // console.log("item.type: " + item.type);

      for (var i = 0; i < valueTypes.length; i++) {
         if (valueTypes[i].type == item.type)
            sectionId = 'io' + valueTypes[i].title.replace(' ', '');
      }

      if (sectionId == '' || !ioSections[sectionId]) {
         console.log("Ignoring unexpected sensor type  " + item.type);
         continue;
      }

      if (!ioSections[sectionId].visible)
         continue;

      var html = '<td id="row_' + item.type + item.address + '" data-address="' + item.address + '" data-type="' + item.type + '" >' + item.title + '</td>';
      html += '<td class="tableMultiColCell"><input id="usrtitle_' + item.type + item.address + '" class="rounded-border inputSetting" type="search" value="' + usrtitle + '"/></td>';
      html += '<td class="tableMultiColCell"><input id="unit_' + item.type + item.address + '" class="rounded-border inputSetting" type="search" value="' + item.unit + '"/></td>';
      html += '<td><input id="state_' + item.type + item.address + '" class="rounded-border inputSetting" type="checkbox" ' + (item.state ? 'checked' : '') + ' /><label for="state_' + item.type + item.address + '"></label></td>';
      html += '<td><input id="record_' + item.type + item.address + '" class="rounded-border inputSetting" type="checkbox" ' + (item.record ? 'checked' : '') + ' /><label for="record_' + item.type + item.address + '"></label></td>';
      html += '<td>' + key + '</td>';

      html += '<td><select id="group_' + item.type + item.address + '" class="rounded-border inputSetting" name="group">';
      if (grouplist != null) {
         for (var g = 0; g < grouplist.length; g++) {
            var group = grouplist[g];
            var sel = item.groupid == group.id ? 'SELECTED' : '';
            html += '    <option value="' + group.id + '" ' + sel + '>' + group.name + '</option>';
         }
      }
      html += '  </select></td>';

      var root = document.getElementById(sectionId);

      if (root != null)
      {
         var elem = document.createElement("tr");
         elem.innerHTML = html;
         root.appendChild(elem);
      }
   }
}

function storeIoSetup()
{
   var jsonArray = [];
   var rootSetup = document.getElementById("ioSetupContainer");
   var elements = rootSetup.querySelectorAll("[id^='row_']");

   console.log("storeIoSetup");

   for (var i = 0; i < elements.length; i++) {
      var jsonObj = {};
      var type = $(elements[i]).data("type");
      var address = $(elements[i]).data("address");

      jsonObj["type"] = type;
      jsonObj["address"] = address;
      jsonObj["usrtitle"] = $("#usrtitle_" + type + address).val();
      jsonObj["unit"] = $("#unit_" + type + address).val();
      jsonObj["state"] = $("#state_" + type + address).is(":checked");
      jsonObj["record"] = $("#record_" + type + address).is(":checked");
      jsonObj["groupid"] = parseInt($("#group_" + type + address).val());

      jsonArray[i] = jsonObj;
   }

   socket.send({ "event" : "storeiosetup", "object" : jsonArray });
}
