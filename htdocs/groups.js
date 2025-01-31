/*
 *  groups.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initGroupSetup(groups)
{
   // console.log(JSON.stringify(groups, undefined, 4));

   $('#container').removeClass('hidden');
   document.getElementById("container").innerHTML =
      '<div class="rounded-border seperatorFold">Baugruppen</div>' +
      '  <table class="setupContainer tableMultiCol">' +
      '    <thead>' +
      '      <tr>' +
      '        <td style="width:50%;">Baugruppe</td>' +
      '        <td style="width:30%;"></td>' +
      '      </tr>' +
      '    </thead>' +
      '    <tbody id="groups">' +
      '    </tbody>' +
      '  </table>' +
      '</div><br/>' +
      '<div id="addGroupDiv" class="rounded-border inputTableConfig"/>';

   tableRoot = document.getElementById("groups");
   tableRoot.innerHTML = "";

   for (var i = 0; i < groups.length; i++) {
      var item = groups[i];
      // var html = "<td id=\"row_" + item.id + "\" data-id=\"" + item.id + "\" >" + item.id + "</td>";
      var html = '<td id="row_' + item.id + '" data-id="' + item.id + '" class="tableMultiColCell">';
      html += "  <input id=\"name_" + item.id + "\" class=\"rounded-border inputSetting\" type=\"text\" value=\"" + item.name + "\"/>";
      html += "</td>";
      html += "<td><button class=\"buttonOptions rounded-border\" style=\"margin-right:10px;\" onclick=\"groupConfig(" + item.id + ", 'delete')\">Löschen</button></td>";

      if (tableRoot != null) {
         var elem = document.createElement("tr");
         elem.innerHTML = html;
         tableRoot.appendChild(elem);
      }
   }

   html =  "  <span>Gruppe: </span><input id=\"input_group\" class=\"rounded-border input\"/>";
   html += "  <button class=\"buttonOptions rounded-border\" onclick=\"groupConfig(0, 'add')\">+</button>";

   document.getElementById("addGroupDiv").innerHTML = html;

      // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('footer') - sab - 10);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('footer') - sab - 10);
   };
}

window.storeGroups = function()
{
   var jsonArray = [];
   var rootSetup = document.getElementById("container");
   var elements = rootSetup.querySelectorAll("[id^='row_']");

   // console.log("storeGroups");

   for (var i = 0; i < elements.length; i++) {
      var jsonObj = {};
      var id = $(elements[i]).data("id");
      jsonObj["id"] = id;
      jsonObj["name"] = $("#name_" + id).val();
      jsonArray[i] = jsonObj;
   }

   socket.send({ "event": "groupconfig", "object":
                 { "groups": jsonArray,
                   "action": "store" }
               });
}

window.groupConfig = function(id, action)
{
   // console.log("groupConfig(" + action + ", " + id + ")");

   if (action == "delete") {
      if (confirm("Gruppe löschen?"))
         socket.send({ "event": "groupconfig", "object":
                       { "id": id,
                         "action": "del" }});
   }
   else if (action == "add") {
      socket.send({ "event": "groupconfig", "object":
                    { "group": $("#input_group").val(),
                      "action": "add" }
                  });
      $("#input_group").val("");
   }
}
