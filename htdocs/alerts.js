/*
 *  alerts.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initAlerts(alerts)
{
   $('#container').removeClass('hidden');
   var root = document.getElementById("container");
   root.innerHTML = '<div class="rounded-border seperatorFold">Sensor Alerts</div>';

   for (var i = 0; i < alerts.length; i++)
   {
      var html = printAlert(alerts[i]);
      var elem = document.createElement("div");
      elem.innerHTML = html;
      root.appendChild(elem);
   }

   var html = printAlert({"id": -1,
                          "type": "VA",
                          "address": "",
                          "state": "D",
                          "min": "",
                          "max": "",
                          "rangem": "",
                          "delta": "",
                          "maddress": "",
                          "msubject": "",
                          "mbody": "",
                          "maxrepeat": ""});

   var elem = document.createElement("div");
   elem.innerHTML = html;
   root.appendChild(elem);
}

function printAlert(item)
{
   var html = "";
   html += '<div id="row_' + item.id + '" data-id="' + item.id + '" class="rounded-border inputTableAlert">\n';
   html += ' <div>\n';
   html += '   <span>Aktiv</span>\n';
   html += '   <span><input id="state" type="checkbox" name="state" ' + (item.state == 'A' ? "checked" : "") + '></input><label for="state"></label></span>\n';
   html += '   <span><button class="rounded-border button2" type="button" onclick="sendAlertMail(' + item.id + ')">Test Mail</button></span>\n';
   html += '   <span><button class="rounded-border button2" type="button" onclick="deleteAlert(' + item.id +  ')">Löschen</button></span>\n';
   html += ' </div>\n';

   html += ' <div>\n';
   html += '   <span>Intervall:</span>\n';
   html += '   <span><input class="rounded-border inputNum" type="number" name="maxrepeat" value="' + item.maxrepeat + '"></input> Minuten</span>\n';
   html += ' </div>\n';

   html += ' <div>\n';
   html += '   <span>Adresse:</span>\n';
   html += '   <span><input class="rounded-border inputNum" type="number" name="address" value="' + item.address + '"></input></span>\n';
   html += '   <span>Typ:</span>\n';
   html += '   <span>\n';
   html += '     <select class="rounded-border input" name="type">\n';
   html += '       <option value="VA"' + (item.type == 'VA' ? 'SELECTED' : '') + '>VA</option>\n';
   html += '       <option value="US"' + (item.type == 'US' ? 'SELECTED' : '') + '>UD</option>\n';
   html += '       <option value="DI"' + (item.type == 'DI' ? 'SELECTED' : '') + '>DI</option>\n';
   html += '       <option value="DO"' + (item.type == 'DO' ? 'SELECTED' : '') + '>DO</option>\n';
   html += '       <option value="W1"' + (item.type == 'W1' ? 'SELECTED' : '') + '>W1</option>\n';
   html += '     </select>\n';
   html += '   </span>\n';
   html += ' </div>\n';

   html += ' <div>\n';
   html += '   <span>Minimum:</span>\n';
   html += '   <span><input class="rounded-border inputNum" type="number" name="min" value="' + item.min + '"></input></span>\n';
   html += '   <span>Maximum:</span>\n';
   html += '   <span><input class="rounded-border inputNum" type="number" name="max" value="' + item.max + '"></input></span>\n';
   html += ' </div>\n';

   html += ' <div>\n';
   html += '   <span>Änderung:</span>\n';
   html += '   <span><input class="rounded-border inputNum" type="number" name="delta" " value="' + item.delta + '"></input> %</span>\n';
   html += '   <span>im Zeitraum:</span>\n';
   html += '   <span><input class="rounded-border inputNum" type="number" name="rangem" value="' + item.rangem + '"></input> Minuten</span>\n';
   html += ' </div>\n';

   html += ' <div>\n';
   html += '   <span>Empfänger:</span>\n';
   html += '   <span><input class="rounded-border input" style="width:805px" type="text" name="maddress" value="' + item.maddress + '"></input></span>\n';
   html += ' </div>\n';
   html += ' <div>\n';
   html += '   <span>Betreff:</span>\n';
   html += '   <span><input class="rounded-border input" style="width:805px" type="text" name="msubject" value="' + item.msubject + '"></input></span>\n';
   html += ' </div>\n';

   html += ' <div>\n';
   html += '   <span>Inhalt:</span>\n';
   html += '   <span>\n';
   html += '   <textarea class="rounded-border input" rows="5" style="width:805px" name="mbody">' + item.mbody + '</textarea>\n';
   html += '   </span>\n';
   html += ' </div>\n';
   html += '</div>\n';

   return html;
}

function storeAlerts()
{
   var jsonArray = [];
   var alertContainer = document.getElementById("alertContainer");

   console.log("storeAlerts");

   var elements = alertContainer.querySelectorAll("[id^='row_']");
   var n = 0;

   for (var e = 0; e < elements.length; e++) {
      var jsonObj = {};
      var id = $(elements[e]).data("id");
      var inputs = elements[e].querySelectorAll("[name]");

      jsonObj["id"] = id;

      for (var i = 0; i < inputs.length; i++) {
         var name = $(inputs[i]).attr('name');
         var value = $(inputs[i]).val();

         if (name == "state")
            jsonObj[name] = $(inputs[i]).is(":checked") ? "A" : "D";
         else
            jsonObj[name] = !isNaN(value) ? parseInt(value) : value;
      }

      var address = jsonObj["address"];

      if (id != -1 || !isNaN(address))
         jsonArray[n++] = jsonObj;
   }

   socket.send({ "event" : "storealerts", "object" : { "action" : "store", "alerts" : jsonArray } });
}

function deleteAlert(alertId)
{
   socket.send({ "event" : "storealerts", "object" : { "action" : "delete", "alertid" : alertId } });
}

function sendAlertMail(alertId)
{
   socket.send({ "event" : "sendmail", "object" : { "alertid" : alertId} });
}
