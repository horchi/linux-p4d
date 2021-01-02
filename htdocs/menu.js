/*
 *  menu.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var actualParent = 1;   // the current parent of the menu items (the level of the menu)

function initMenu(menu, root)
{
   // console.log(JSON.stringify(menu, undefined, 4));

   var menuButtons = root.querySelector("#menuButtons");

   menuButtons.innerHTML = "";

   if (menu.last > 0) {
      var elem = document.createElement("div");
      elem.innerHTML = menu.title;
      elem.className = "menuButtonCaption rounded-border";
      elem.setAttribute("onclick", "menuSelected(" + menu.last + ")");
      menuButtons.appendChild(elem);
   }

   for (var i = 0; i < menu.items.length; i++) {
      var item = menu.items[i];
      var elem = document.createElement("div");

      actualParent = item.parent;   // all items in the loop should have the same parent !!
      elem.innerHTML = item.title;

      if (item.child) {
         elem.setAttribute("onclick", "menuSelected(" + item.child + ")");
         elem.className = "menuButton rounded-border";
      }
      else if (item.value != null) {
         elem.innerHTML += ": " + item.value + " " + item.unit;
         elem.className = "menuButtonValue rounded-border";
      }
      else {
         elem.className = "menuButton rounded-border";
      }

      if (item.editable) {
         elem.setAttribute("onclick", "menuEditRequest(" + item.id + "," + item.address + "," + item.range + "," + actualParent + ")");
         elem.className = "menuButtonValue menuButtonValueEditable rounded-border";
      }

      menuButtons.appendChild(elem);
   }
}

function editMenuParameter(parameter, root)
{
   console.log(JSON.stringify(parameter, undefined, 4));

   var inpStep = 'step="0.1"';
   var inpType = "text";
   var info = "";

   if (parameter.min != null)
      info = 'Bereich: ' + parameter.min + '-' + parameter.max + parameter.unit + '<br/>'
      + ' Default: ' + parameter.def + parameter.unit;

   var timeRange = null;
   var form = '<form id="dlgForm"><div>' + info + '</div><br/>';

   if (parameter.type == 0x0a) {
      timeRange = parameter.value.split("-");
      form +=
         '<div id=timepicker class="timerangepicker">' +
         '  <div class="timepicker" id="timeFrom"></div>' +
         '  <div style="align-self:center;width:90px;">' + ' bis ' + '</div>' +
         '  <div class="timepicker" id="timeTo">' +
         '  </div><div style="align-self:center;width:50px;">' + parameter.unit + '</div>' +
         '</div>';
   }
   else {
      form += '<input class="input rounded-border" type="' + inpType + '" value="' + parameter.value + '" name="input"> ' + parameter.unit;
   }

   form += '<br></form>';

   $(form).dialog({
      modal: true,
      width: (parameter.type == 0x0a ? "40%" : "60%"),
      title: parameter.title,
      buttons: {
         'Speichern': function () {
            var value = '';
            if (parameter.type == 0x0a) {
               var from = $("#timeFrom").picktim('val');
               var to = $("#timeTo").picktim('val');
               if (from.length == 5 && to.length == 5)
                  value = from + " - " + to;
            }
            else
               value = $('input[name="input"]').val();
            storeParameter(parameter.id, value, parameter.address, parameter.range, parameter.parent);
            $(this).dialog('close');
         },
         'Abbrechen': function () {
            $(this).dialog('close');
         }
      },
      open: function() {
         if (parameter.type == 0x0a) {
            $("#timeFrom").picktim({
               mode: 'h24',
               width: '180px',
               position: 'fixed',
               appendTo: '#timepicker',
               orientation: 'topLeft',
               defaultValue: timeRange[0].trim()
            });
            $("#timeTo").picktim({
               mode: 'h24',
               width: '180px',
               position: 'fixed',
               appendTo: '#timepicker',
               orientation: 'topLeft',
               defaultValue: timeRange[1].trim()
            });
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });

   function storeParameter(id, value, address, range, parent) {
      console.log("storing " + id + " - '" + value + "' - 0x" + address  + " timeN: " + range);
      socket.send({ "event" : "parstore", "object" :
                    { "id"  : id,
                      "value" : value,
                      "address" : address,
                      "range" : range,
                      "parent" : parent
                    }});
   }
}

function updateTimeRanges()
{
   console.log("updateTimeRanges");
   showProgressDialog();
   socket.send({ "event" : "updatetimeranges", "object" : { "parent" : actualParent } });
}

window.menuSelected = function(child)
{
   socket.send({ "event" : "menu", "object" : { "parent"  : child }});
}

window.menuEditRequest = function(id, address, range, parent)
{
   console.log("menuEdit " + id);
   socket.send({ "event" : "pareditrequest", "object" :
                 { "id"  : id,
                   "address" : address,
                   "range" : range,
                   "parent" : parent
                 }});
}
