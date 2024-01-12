/*
 *  iosetup.js
 *
 *  (c) 2020-2023 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var activeSection = '';
var ioSections = {};

// ----------------------------------------------------------------
// IO Setup
// ----------------------------------------------------------------

function initIoSetup(valueFacts)
{
   // console.log(JSON.stringify(valueFacts, undefined, 4));

   $('#controlContainer').removeClass('hidden');
   $('#container').removeClass('hidden');

   activeSection = 'io' + valueTypes[0].title.replace(' ', '');

   $("#controlContainer")
      .empty()
      .append($('<div></div>')
              .append($('<button></button>')
                      .addClass('rounded-border tool-button')
                      .html('Speichern')
                      .click(function() { storeIoSetup(); })))
      .append($('<div></div>')
              .addClass('button-group-spacing'))
      .append($('<div></div>')
              .addClass('labelB1')
              .html('Filter'))
      .append($('<input></input>')
              .attr('id', 'incSearchName')
              .attr('placeholder', 'expression...')
              .attr('title', 'Type title, path, state, GOOD or BAD')
              .attr('type', 'search')
              .addClass('input rounded-border clearableOD')
              .css('width', '-webkit-fill-available')
              .css('width', '-moz-available')
              .on('input', function() { showTable(activeSection); }))
      .append($('<div></div>')
              .addClass('button-group-spacing'))
      .append($('<div></div>')
              .append($('<button></button>')
                      .attr('id', 'filterIoSetup')
                      .addClass('rounded-border tool-button')
                      .html('[alle]')
                      .click(function() { filterIoSetup(); })))
      .append($('<div></div>')
              .addClass('button-group-spacing'))
      .append($('<div></div>')
              .addClass('button-group-spacing'));

   $('#container').html('<div id="ioSetupContainer" class="setupContainer"></div>');

   for (var i = 0; i < valueTypes.length; i++) {
      var section = 'io' + valueTypes[i].title.replace(' ', '');

      if (!$("#"+'btn_' + section).length) {
         $("#controlContainer").append($('<div></div>')
                                       .append($('<button></button>')
                                               .attr('id', 'btn_' + section)
                                               .addClass('rounded-border tool-button')
                                               .click({ "section" : section }, function(event) { showTable(event.data.section); })
                                               .html(valueTypes[i].title)));

         ioSections[section] = {};
         ioSections[section].title = valueTypes[i].title;
         ioSections[section].type = valueTypes[i].type;
      }
   }

   $("#content").height($(window).height() - getTotalHeightOf('menu'));
   $("#container").css('height', '');  // reset until all pages using the height of 'content'
   window.onresize = function() {
      $("#content").height($(window).height() - getTotalHeightOf('menu'));
   };

   showTable(activeSection);
}

var filterActive = false;

function filterIoSetup()
{
   filterActive = !filterActive;
   console.log("filterIoSetup: " + filterActive);

   $("#filterIoSetup").html(filterActive ? "[aktive]" : "[alle]");
   showTable(activeSection);
}

function showTable(section)
{
   activeSection = section;

   console.log("showTable(", activeSection, ")");

   $('button[id^="btn_io"]').each(function () {
      $(this).css('background-color', '');
   });

   $('#btn_' + activeSection).css('background-color', 'slategray');

   let html =
       '  <table class="tableMultiCol">' +
       '    <thead>' +
       '      <tr>' +
       '        <td style="width:20%;">Name</td>' +
       '        <td style="min-width:20vw;">Titel</td>' +
       '        <td style="width:4%;">Einheit</td>' +
       '        <td style="width:3%;">Aktiv</td>' +
       '        <td style="width:3%;">Aufz.</td>' +
       '        <td style="width:6%;">ID</td>' +
       '        <td style="width:6%;"></td>' +
       // '        <td style="width:10%;">Gruppe</td>' +
       '      </tr>' +
       '    </thead>' +
       '    <tbody id="' + section + '">' +
       '    </tbody>';

   if (ioSections[section].type == 'CV') {
      html +=
         '    <tfoot>' +
         '      <tr>' +
         '        <td id="footer_' + sectionId + '" colspan="8" style="background-color:#333333;">' +
         '          <button class="buttonOptions rounded-border" onclick="sensorCvAdd()">+</button>' +
         '        </td>' +
         '      </tr>' +
         '    </tfoot>';
   }

   html += '  </table>';

   $('#ioSetupContainer').html(html);

   let filterIoExpression = null;

   if ($("#incSearchName").val() != "")
      filterIoExpression = new RegExp($("#incSearchName").val());

   for (var key in valueFacts) {
      var sectionId = '';
      var item = valueFacts[key];
      var usrtitle = item.usrtitle != null ? item.usrtitle : "";

      if (!item.state && filterActive)
         continue;

      if (filterIoExpression && !filterIoExpression.test(item.title) && !filterIoExpression.test(usrtitle))
         continue;

      // console.log("item.type: " + item.type);

      for (var i = 0; i < valueTypes.length; i++) {
         if (valueTypes[i].type == item.type)
            sectionId = 'io' + valueTypes[i].title.replace(' ', '');
      }

      let html = '<td id="row_' + item.type + item.address + '" data-address="' + item.address + '" data-type="' + item.type + '" >' + item.title + '</td>';
      html += '<td class="tableMultiColCell"><input id="usrtitle_' + item.type + item.address + '" class="inputSetting rounded-border" type="search" value="' + usrtitle + '"/></td>';
      html += '<td class="tableMultiColCell"><input id="unit_' + item.type + item.address + '" class="inputSetting rounded-border" type="search" value="' + item.unit + '"/></td>';
      html += '<td><input id="state_' + item.type + item.address + '" class="rounded-border input" type="checkbox" ' + (item.state ? 'checked' : '') + ' /><label for="state_' + item.type + item.address + '"></label></td>';
      html += '<td><input id="record_' + item.type + item.address + '" class="rounded-border input" type="checkbox" ' + (item.record ? 'checked' : '') + ' /><label for="record_' + item.type + item.address + '"></label></td>';
      html += '<td>' + key + '</td>';

      if (item.type == 'AI')
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorAiSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'CV')
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorCvSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'DO')
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorDoSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'W1' || item.type == 'RTL433' || item.type == 'SC')
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="deleteValueFact(\'' + item.type + '\', \'' + item.address + '\')">Löschen</button>' + '</td>';
      else
         html += '<td></td>';

//      html += '<td><select id="group_' + item.type + item.address + '" class="rounded-border input" name="group">';
//      if (grouplist != null) {
//         for (var g = 0; g < grouplist.length; g++) {
//            var group = grouplist[g];
//            var sel = item.groupid == group.id ? 'SELECTED' : '';
//            html += '    <option value="' + group.id + '" ' + sel + '>' + group.name + '</option>';
//         }
//      }
//      html += '  </select></td>';

      var root = document.getElementById(sectionId);

      if (root) {
         let elem = document.createElement("tr");
         elem.innerHTML = html;
         root.appendChild(elem);
      }
   }
}

var calSensorType = '';
var calSensorAddress = -1;

function updateIoSetupValue()
{
   var key = toKey(calSensorType, calSensorAddress);

   if (calSensorType != '' && allSensors[key] != null && allSensors[key].plain != null) {
      $('#actuaCalValue').html(allSensors[key].value + ' ' + valueFacts[key].unit + ' (' + allSensors[key].plain + ')');
   }
}

function sensorAiSetup(type, address)
{
   console.log("sensorSetup ", type, address);

   calSensorType = type;
   calSensorAddress = parseInt(address);

   var key = toKey(calSensorType, calSensorAddress);
   var form = document.createElement("div");

   if (valueFacts[key] == null) {
      console.log("Sensor ", key, "undefined");
      return;
   }

   $(form).append($('<div></div>')
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '50%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Kalibrieren'))
                          .append($('<select></select>')
                                  .attr('id', 'calPointSelect')
                                  .addClass('rounded-border input')
                                  .change(function() {
                                     if ($(this).val() == 'pointA') {
                                        $('#calPoint').val(valueFacts[key].calPointA);
                                        $('#calPointValue').val(valueFacts[key].calPointValueA)
                                     }
                                     else {
                                        $('#calPoint').val(valueFacts[key].calPointB);
                                        $('#calPointValue').val(valueFacts[key].calPointValueB)
                                     }
                                  })
                                 ))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '50%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Aktuell'))
                          .append($('<span></span>')
                                  .attr('id', 'actuaCalValue')
                                  .css('background-color', 'var(--dialogBackground)')
                                  .css('text-align', 'start')
                                  .css('align-self', 'center')
                                  .css('width', '345px')
                                  .addClass('rounded-border input')
                                  .html('-')
                                 )
                          .append($('<button></button>')
                                  .addClass('buttonOptions rounded-border')
                                  .css('width', 'auto')
                                  .html('>')
                                  .click(function() { $('#calPointValue').val(allSensors[toKey(calSensorType, calSensorAddress)].plain); })
                                 ))
                  .append($('<br></br>'))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '50%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('bei Wert'))
                          .append($('<input></input>')
                                  .attr('id', 'calPoint')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                  .addClass('rounded-border input')
                                  .val(valueFacts[key].calPointA)
                                 ))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '50%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('-> Sensor Wert'))
                          .append($('<input></input>')
                                  .attr('id', 'calPointValue')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                  .addClass('rounded-border input')
                                  .val(valueFacts[key].calPointValueA)
                                 ))
                  .append($('<br></br>'))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '50%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Runden')
                                 )
                          .append($('<input></input>')
                                  .attr('id', 'calRound')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                  .addClass('rounded-border input')
                                  .val(valueFacts[key].calRound)
                                 ))
                  .append($('<br></br>'))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('width', '50%')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Abschneiden unter')
                                 )
                          .append($('<input></input>')
                                  .attr('id', 'calCutBelow')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                  .addClass('rounded-border input')
                                  .val(valueFacts[key].calCutBelow)
                                 ))
                         );

   var title = valueFacts[key].usrtitle != '' ? valueFacts[key].usrtitle : valueFacts[key].title;

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "500px",
      title: "Sensor '" + title + "' kalibrieren",
      open: function() {
         calSensorType = type;
         calSensorAddress = parseInt(address);

         $('#calPointSelect').append($('<option></option>')
                                     .val('pointA')
                                     .html('Punkt 1'));
         $('#calPointSelect').append($('<option></option>')
                                     .val('pointB')
                                     .html('Punkt 2'));

         if (allSensors[key] != null)
            $('#actuaCalValue').html(allSensors[key].value + ' ' + valueFacts[key].unit + ' (' + (allSensors[key].plain != null ? allSensors[key].plain : '-') + ')');
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Speichern': function () {
            console.log("store calibration value", $('#calPoint').val(), '/', $('#calPointValue').val(), 'for', $('#calPointSelect').val());

            socket.send({ "event" : "storecalibration", "object" : {
               'type' : calSensorType,
               'address' : calSensorAddress,
               'calPoint' : parseFloat($('#calPoint').val()),
               'calPointValue' : parseFloat($('#calPointValue').val()),
               'calRound' : parseInt($('#calRound').val()),
               'calCutBelow' : parseFloat($('#calCutBelow').val()),
               'calPointSelect' : $('#calPointSelect').val()
            }});

            $(this).dialog('close');
         }
      },
      close: function() {
         calSensorType = '';
         calSensorAddress = -1;
         $(this).dialog('destroy').remove();
      }
   });

}

function deleteValueFact(type, address)
{
   console.log("sensor delete ", type, address);

   socket.send({ "event" : "storecalibration", "object" : {
      'type' : type,
      'address' : parseInt(address),
      'action' : 'delete'
   }});
}

function sensorDoSetup(type, address)
{
   console.log("sensorSetup ", type, address);

   calSensorType = type;
   calSensorAddress = address;

   var key = toKey(calSensorType, parseInt(calSensorAddress));
   var form = document.createElement("div");

   if (valueFacts[key] == null) {
      console.log("Sensor ", key, "undefined");
      return;
   }

   $(form).append($('<div></div>')
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<input></input>')
                                  .attr('id', 'invertDo')
                                  .attr('type', 'checkbox')
                                  .addClass('rounded-border input')
                                  .css('height', '100px')
                                  .prop('checked', valueFacts[key].invertDo))
                          .append($('<label></label>')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .prop('for', 'invertDo')
                                  .html('Digitalaugang invertieren'))
                         ));

   var title = valueFacts[key].usrtitle != '' ? valueFacts[key].usrtitle : valueFacts[key].title;

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "80%",
      title: "DO settings '" + title + '"',
      open: function() {
         calSensorType = type;
         calSensorAddress = address;
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Speichern': function () {
            console.log("store 'DO' setting invert = ", $('#invertDo').is(':checked'));

            socket.send({ "event" : "storecalibration", "object" : {
               'type' : calSensorType,
               'address' : parseInt(calSensorAddress),
               'invertDo' : $('#invertDo').is(':checked')
            }});

            $(this).dialog('close');
         }
      },
      close: function() {
         calSensorType = '';
         calSensorAddress = -1;
         $(this).dialog('destroy').remove();
      }
   });
}

function sensorCvSetup(type, address)
{
   console.log("sensorSetup ", type, address);

   calSensorType = type;
   calSensorAddress = parseInt(address);

   var key = toKey(calSensorType, calSensorAddress);
   var form = document.createElement("div");

   if (valueFacts[key] == null) {
      console.log("Sensor ", key, "undefined");
      return;
   }

   $(form).append($('<div></div>')
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .css('text-align', 'end')
                                  .css('align-self', 'center')
                                  .css('margin-right', '10px')
                                  .html('Skript'))
                          .append($('<textarea></textarea>')
                                  .attr('id', 'luaScript')
                                  .addClass('rounded-border input inputSettingScript')
                                  .css('height', '100px')
                                  .val(valueFacts[key].luaScript)
                                 ))
                 );

   var title = valueFacts[key].usrtitle != '' ? valueFacts[key].usrtitle : valueFacts[key].title;

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "80%",
      title: "LUA Skript für '" + title + "'",
      open: function() {
         calSensorType = type;
         calSensorAddress = parseInt(address);
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Speichern': function () {
            console.log("store lua script", $('#luaScript').val());

            socket.send({ "event" : "storecalibration", "object" : {
               'type' : calSensorType,
               'address' : calSensorAddress,
               'luaScript' : $('#luaScript').val()
            }});

            $(this).dialog('close');
         }
      },
      close: function() {
         calSensorType = '';
         calSensorAddress = -1;
         $(this).dialog('destroy').remove();
      }
   });
}

function sensorCvAdd()
{
   socket.send({ "event" : "storecalibration", "object" : {
      'type' : 'CV',
      'action' : 'add'
   }});
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
