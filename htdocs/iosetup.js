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

function initIoSetup()
{
   // console.log(JSON.stringify(valueFacts, undefined, 4));

   hideProgressDialog();

   $('#controlContainer').removeClass('hidden');
   $('#container').removeClass('hidden');

   prepareSetupMenu();

   if (activeSection == '')
      activeSection = 'io' + valueTypes[0].title.replace(' ', '');

   $("#controlContainer")
      .empty()
      .append($('<div></div>')
              .append($('<button></button>')
                      .addClass('rounded-border tool-button')
                      .css('background-color', 'slategray')
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
              .attr('type', 'search')
              .addClass('input rounded-border clearableOD')
              .css('width', '-webkit-fill-available')
              .css('width', '-moz-available')
              .css('margin-bottom', '8px')
              .on('input', function() { showTable(activeSection); }))
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

   $("#content").height($(window).height() - getTotalHeightOf('menu')- getTotalHeightOf('footer') - sab);
   $("#container").css('height', '');  // reset until all pages using the height of 'content'
   window.onresize = function() {
      $("#content").height($(window).height() - getTotalHeightOf('menu')- getTotalHeightOf('footer') - sab);
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

   $('button[id^="btn_io"]').each(function () {
      $(this).css('background-color', '');
   });

   $('#btn_' + activeSection).css('background-color', 'slategray');

   let html =
       '  <table class="tableMultiCol">' +
       '    <thead>' +
       '      <tr>' +
       '        <td style="width:22%;">Name</td>' +
       '        <td style="min-width:25vw;">Titel</td>' +
       '        <td style="width:7%;min-width:70px;">Einheit</td>' +
       '        <td style="width:5%;">Aktiv</td>' +
       '        <td style="width:5%;">Aufz.</td>' +
       '        <td style="width:12%;">ID</td>' +
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
         '        <td colspan="8" style="background-color:#333333;">' +
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

   let valueFactsSorted = Object.keys(valueFacts).sort(function(a, b) {
      return valueFacts[a].name.localeCompare(valueFacts[b].name);
   });

   for (let k = 0; k < valueFactsSorted.length; ++k) {
      let key = valueFactsSorted[k];
      let item = valueFacts[key];
      let usrtitle = item.usrtitle != null ? item.usrtitle : "";

      if (!item.state && filterActive)
         continue;

      if (filterIoExpression && !filterIoExpression.test(item.title) && !filterIoExpression.test(usrtitle))
         continue;

      // console.log("item.type: " + item.type);

      let sectionId = '';

      for (var i = 0; i < valueTypes.length; i++) {
         if (valueTypes[i].type == item.type)
            sectionId = 'io' + valueTypes[i].title.replace(' ', '');
      }

      let title = item.title.split("\n");

      let html = '<td id="row_' + item.type + item.address + '" data-address="' + item.address + '" data-type="' + item.type + '" >'
          + '<div>' + title[0] + '</div>'
          + '<div style="font-size:smaller;color:darkgray;">' + (title.length > 1 ? title[1] : '') + '</div>'
          + '</td>';

      html += '<td class="tableMultiColCell"><input id="usrtitle_' + item.type + item.address + '" class="inputSetting rounded-border" type="search" value="' + usrtitle + '"/></td>';
      html += '<td class="tableMultiColCell"><input id="unit_' + item.type + item.address + '" class="inputSetting rounded-border" type="search" value="' + item.unit + '"/></td>';
      html += '<td><input id="state_' + item.type + item.address + '" class="rounded-border input" type="checkbox" ' + (item.state ? 'checked' : '') + ' /><label for="state_' + item.type + item.address + '"></label></td>';
      html += '<td><input id="record_' + item.type + item.address + '" class="rounded-border input" type="checkbox" ' + (item.record ? 'checked' : '') + ' /><label for="record_' + item.type + item.address + '"></label></td>';
      html += '<td>' + key + '</td>';

      if (item.type == 'AI' || item.type.startsWith('ADS'))
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorAiSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'CV')
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorCvSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'DO' || item.type.startsWith('MCPO'))
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorDoSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'DI' || item.type.startsWith('MCPI'))
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorDiSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
      else if (item.type == 'SC') {
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorScSetup(\'' + item.type + '\', \'' + item.address + '\')">Setup</button>' + '</td>';
         if (item.parameter != null && item.parameter.cloneable) {
            html += '<td>' + '<button class="buttonOptions rounded-border" onclick="sensorScClone(\'' + item.type + '\', \'' + item.address + '\')">Clone</button>' + '</td>';
         }
      }
      else if (item.type == 'W1' || item.type == 'RTL433')
         html += '<td>' + '<button class="buttonOptions rounded-border" onclick="deleteValueFact(\'' + item.type + '\', \'' + item.address + '\')">Löschen</button>' + '</td>';
      else
         html += '<td></td>';

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
      $('#actuaCalValue').val(allSensors[key].value + ' ' + valueFacts[key].unit + ' (' + allSensors[key].plain + ')');
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
                  .addClass('settingsDialogContent')
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Kalibrieren'))
                          .append($('<span></span>')
                          .append($('<select></select>')
                                  .attr('id', 'calPointSelect')
                                          .addClass('rounded-border inputSetting')
                                  .change(function() {
                                     if ($(this).val() == 'pointA') {
                                        $('#calPoint').val(valueFacts[key].calPointA);
                                        $('#calPointValue').val(valueFacts[key].calPointValueA)
                                     }
                                     else {
                                        $('#calPoint').val(valueFacts[key].calPointB);
                                        $('#calPointValue').val(valueFacts[key].calPointValueB)
                                     }
                                          }))
                                 ))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Aktuell'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                  .attr('id', 'actuaCalValue')
                                          .addClass('rounded-border inputSetting')
                                          .css('pointer-events', 'none')
                                          .css('background-color', 'var(--yellow2)')
                                          .css('color', 'var(--neutral0)')
                                          .css('width', '60%')
                                          .val('-'))
                          .append($('<button></button>')
                                  .addClass('buttonOptions rounded-border')
                                  .css('width', 'auto')
                                          .css('float', 'right')
                                  .html('>')
                                          .click(function() { $('#calPointValue').val(allSensors[toKey(calSensorType, calSensorAddress)].plain); }))
                                 ))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('bei Wert'))
                          .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'calPoint')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                          .addClass('rounded-border inputSetting')
                                  .val(valueFacts[key].calPointA)
                                         )))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('-> Sensor Wert'))
                          .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'calPointValue')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                          .addClass('rounded-border inputSetting')
                                  .val(valueFacts[key].calPointValueA)
                                         )))
                  .append($('<br></br>'))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Runden'))
                          .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'calRound')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                          .addClass('rounded-border inputSetting')
                                  .val(valueFacts[key].calRound)
                                         )))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Abschneiden unter')
                                 )
                          .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'calCutBelow')
                                  .attr('type', 'number')
                                  .attr('step', 0.1)
                                          .addClass('rounded-border inputSetting')
                                  .val(valueFacts[key].calCutBelow)
                                         )))
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
            $('#actuaCalValue').val(allSensors[key].value + ' ' + valueFacts[key].unit + ' (' + (allSensors[key].plain != null ? allSensors[key].plain : '-') + ')');
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Speichern': function () {
            console.log("store sensor settings", $('#calPoint').val(), '/', $('#calPointValue').val(), 'for', $('#calPointSelect').val());

            socket.send({ "event" : "storesensorsetup", "object" : {
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

   socket.send({ "event" : "storesensorsetup", "object" : {
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
                  .addClass('settingsDialogContent')
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Invertieren'))
                          .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'invertDo')
                                  .attr('type', 'checkbox')
                                          .addClass('rounded-border inputSetting')
                                          .prop('checked', valueFacts[key].settings ? valueFacts[key].settings.invert : true))
                                  .append($('<label></label>')
                                          .prop('for', 'invertDo'))))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Impuls (50ms)'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('id', 'impulseDo')
                                          .attr('type', 'checkbox')
                                          .addClass('rounded-border inputSetting')
                                          .prop('checked', valueFacts[key].settings ? valueFacts[key].settings.impulse : false))
                                  .append($('<label></label>')
                                          .prop('for', 'impulseDo'))))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Feedback Input'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('id', 'feedbackIo')
                                          .attr('type', 'search')
                                          .addClass('rounded-border inputSetting')
                                          .val(valueFacts[key].settings && valueFacts[key].settings.feedbackInType ?
                                               valueFacts[key].settings.feedbackInType
                                               + ':0x'
                                               + valueFacts[key].settings.feedbackInAddress.toString(16) : ''))))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .css('width', 'auto')
                                  .html('Skript'))
                          .append($('<span></span>')
                                  .append($('<textarea></textarea>')
                                          .attr('id', 'scriptDo')
                                          .addClass('rounded-border inputSetting inputSettingScript')
                                  .css('height', '100px')
                                          .val(valueFacts[key].settings ? valueFacts[key].settings.script : '')
                                         )))
                         );

   var title = valueFacts[key].usrtitle != '' ? valueFacts[key].usrtitle : valueFacts[key].title;

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      width: "500px",
      hide: "fade",
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

            let addr = parseInt($('#feedbackIo').val().split(":")[1]);
            let type = $('#feedbackIo').val().split(":")[0];

            socket.send({ "event" : "storesensorsetup", "object" : {
               'type' : calSensorType,
               'address' : parseInt(calSensorAddress),
               'settings' : JSON.stringify({
                  'invert' : $('#invertDo').is(':checked'),
                  'impulse' : $('#impulseDo').is(':checked'),
                  'script' : $('#scriptDo').val(),
                  'feedbackInType' : type,
                  'feedbackInAddress' : addr
               })
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

function sensorScClone(type, address)
{
   socket.send({ "event" : "storesensorsetup", "object" : {
      'type' : type,
      'address' : parseInt(address),
      'action' : 'clone'
   }});
}

function sensorScSetup(type, address)
{
   calSensorType = type;
   calSensorAddress = parseInt(address);

   var key = toKey(calSensorType, calSensorAddress);
   var form = document.createElement("div");

   if (valueFacts[key] == null) {
      console.log("Sensor ", key, "undefined");
      return;
   }

   $(form).append($('<div></div>')
                  .addClass('settingsDialogContent')
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .css('width', 'auto')
                                  .html('Argumente (JSON)'))
                          .append($('<span></span>')
                                  .append($('<textarea></textarea>')
                                          .attr('id', 'arguments')
                                          .addClass('rounded-border inputSetting inputSettingScript')
                                          .css('height', '100px')
                                          .val(JSON.stringify(valueFacts[key].settings, undefined, 3))
                                         )))
                 );

   var title = valueFacts[key].usrtitle != '' ? valueFacts[key].usrtitle : valueFacts[key].title;

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "80%",
      title: "Skript Argumente für '" + title + "'",
      open: function() {
         calSensorType = type;
         calSensorAddress = parseInt(address);
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Speichern': function () {
            console.log("store script arguments", $('#arguments').val());

            socket.send({ "event" : "storesensorsetup", "object" : {
               'type' : calSensorType,
               'address' : calSensorAddress,
               'settings' : $('#arguments').val().replace(/\s\s+/g, ' ')
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

function sensorDiSetup(type, address)
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

   dlgContent = ($('<div></div>'));

   $(form).append(dlgContent)
      .addClass('settingsDialogContent')
      .append($('<div></div>')
              .append($('<span></span>')
                      .html('Invertieren'))
              .append($('<span></span>')
                      .append($('<input></input>')
                              .attr('id', 'invertDi')
                              .attr('type', 'checkbox')
                              .addClass('rounded-border inputSetting')
                              .prop('checked', valueFacts[key].settings ? valueFacts[key].settings.invert : true))
                      .append($('<label></label>')
                              .prop('for', 'invertDi'))))
      .append($('<div></div>')
              .append($('<span></span>')
                      .html('Pull Up/Down'))
              .append($('<span></span>')
                      .append($('<select></select>')
                              .attr('id', 'pullDi')
                              .addClass('rounded-border inputSetting'))));


   if (type == 'DI') {
      $(dlgContent)
         .append($('<span></span>')
                 .html('Interrupt'))
         .append($('<span></span>')
                 .append($('<input></input>')
                         .attr('id', 'interruptDi')
                         .attr('type', 'checkbox')
                         .addClass('rounded-border inputSetting')
                         .prop('checked', valueFacts[key].settings ? valueFacts[key].settings.interrupt : false))
                 .append($('<label></label>')
                         .attr('title', 'Neustart zum aktivieren')
                         .prop('for', 'interruptDi')));
   }

   var title = valueFacts[key].usrtitle != '' ? valueFacts[key].usrtitle : valueFacts[key].title;

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "500px",
      title: "DI settings '" + title + '"',
      open: function() {
         calSensorType = type;
         calSensorAddress = address;
      },
      buttons: {
         'Abbrechen': function () {
            $(this).dialog('close');
         },
         'Speichern': function () {
            socket.send({ "event" : "storesensorsetup", "object" : {
               'type' : calSensorType,
               'address' : parseInt(calSensorAddress),
               'settings' : JSON.stringify({
                  'invert' : $('#invertDi').is(':checked'),
                  'interrupt' : $('#interruptDi').is(':checked'),
                  'pull' : parseInt($('#pullDi').val())
               })
            }});

            $(this).dialog('close');
         }
      },
      open: function(){
         let pull = valueFacts[key].settings ? valueFacts[key].settings.pull : 0;
         $('#pullDi')
            .append($('<option></option>')
                    .val(0)
                    .attr('selected', pull == 0)
                    .html('None'))
            .append($('<option></option>')
                    .val(1)
                    .attr('selected', pull == 1)
                    .html('Up'))
            .append($('<option></option>')
                    .val(2)
                    .attr('selected', pull == 2)
                    .html('Down'));
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
                  .addClass('settingsDialogContent')
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .css('width', 'auto')
                                  .html('Skript'))
                          .append($('<span></span>')
                          .append($('<textarea></textarea>')
                                  .attr('id', 'luaScript')
                                          .addClass('rounded-border inputSetting inputSettingScript')
                                  .css('height', '100px')
                                  .val(valueFacts[key].luaScript)
                                         )))
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

            socket.send({ "event" : "storesensorsetup", "object" : {
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
   socket.send({ "event" : "storesensorsetup", "object" : {
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
