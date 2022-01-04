/*
 *  errors.js
 *
 *  (c) 2020-2021 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initErrors(errors, root)
{
   // console.log(JSON.stringify(errors, undefined, 4));

   $('#container').removeClass('hidden');

   document.getElementById("container").innerHTML =
      '<div class="rounded-border seperatorFold">Fehlerspeicher</div>' +
      ' <table class="tableMultiCol">' +
      '   <thead>' +
      '     <tr>' +
      '       <td style="width:30px;"></td>' +
      '       <td>Zeit</td>' +
      '       <td>Dauer [m]</td>' +
      '       <td>Fehler</td>' +
      '       <td>Status</td>' +
      '     </tr>' +
      '   </thead>' +
      '   <tbody id="errors">' +
      '   </tbody>' +
      ' </table>' +
      '</div>';

   tableRoot = document.getElementById("errors");
   tableRoot.innerHTML = "";

   for (var i = 0; i < errors.length; i++) {
      var item = errors[i];

      var style = item.state == "quittiert" || item.state == "gegangen" ? "greenCircle" : "redCircle";
      var html = "<td><div class=\"" + style + "\"></div></td>";
      html += "<td class=\"tableMultiColCell\">" + item.time + "</td>";
      html += "<td class=\"tableMultiColCell\">" + (item.duration / 60).toFixed(0) + "</td>";
      html += "<td class=\"tableMultiColCell\">" + item.text + "</td>";
      html += "<td class=\"tableMultiColCell\">" + item.state + "</td>";

      if (tableRoot != null) {
         var elem = document.createElement("tr");
         elem.innerHTML = html;
         tableRoot.appendChild(elem);
      }
   }

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - 10);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - 10);
   };
}
