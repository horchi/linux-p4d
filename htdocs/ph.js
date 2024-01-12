/*
 *  ph.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function requestActualPh()
{
   socket.send({ "event" : "ph", "object" : {} });
}

function updatePhCal(data)
{
   $('#container').removeClass('hidden');

   if (document.getElementById("inputCalMeasureValue1").value == "") {
      document.getElementById("inputCalMeasureValue1").value = data.calValue;
      document.getElementById("currentCalMeasureText1").innerHTML = "&nbsp;&nbsp;&nbsp;(Durchschnitt über&nbsp;"
         + data.duration + "&nbsp;Sekunden)";
   }
   else {
      document.getElementById("inputCalMeasureValue2").value = data.calValue;
      document.getElementById("currentCalMeasureText2").innerHTML = "&nbsp;&nbsp;&nbsp;(Durchschnitt über&nbsp;"
         + data.duration + "&nbsp;Sekunden)";

      // document.getElementById("buttonStoreCal").style.visibility = 'visible';
   }
}

function updatePhActual(data)
{
   $('#container').removeClass('hidden');

   if (document.getElementById("phShowActual") == null) {
      document.getElementById("container").innerHTML =
         '<div id="phShowActual" class="rounded-border phSetup">' +
         '  <div class="configDescriptionCaption">Der akuell gemessene PH Wert und die eingestellen Kalibrierungspunkte</div>' +
         '  <br/>' +
         '  <div><span>Aktueller PH Wert</span><span id="currentPh" style="text-align:right;">--</span><span style="text-align:center;">&ndash;</span><span id="currentPhValue">--</span><span>(digits)</span></div>' +
         '  <br/>' +
         '  <div><span>Erster Punkt bei PH</span><span id="currentPhA" style="text-align:right;">--</span><span style="text-align:center;">&ndash;</span><span id="currentCalA">--</span><span>(digits)</span></div>' +
         '  <div><span>Zweiter Punkt bei PH</span><span id="currentPhB" style="text-align:right;">--</span><span style="text-align:center;">&ndash;</span><span id="currentCalB">--</span><span>(digits)</span></div>' +
         '  <br/>' +
         '  <div><span>Aktueller PH Minus Bedarf</span><span id="currentPhMinusDemand" style="text-align:right;">--</span></div>' +
         '  <br/>' +
         '  <input id="checkboxRefresh" class="rounded-border" type="checkbox" onclick="checkboxRefreshKlick()" checked \>' +
         '  <label for="checkboxRefresh">Refresh</label>' +
         '</div>' +
         '<div id="phShowCal" class="rounded-border phSetup">' +
         '  <div class="configDescriptionCaption">Kalibrieren</div>' +
         '  <div class="configDescription">' +
         '    Zum Kalibrieren der PH Probe werden zwei Messpunkte mit unterschiedlichen PH Werten benötigt.' +
         '    Für diese sind genaue Pufferlösungen erforderlich. Der PH Wert des ersten Messpunktes muss zwingend unter dem des Zweiten liegen!' +
         '    Es ist zu empfehlen den Eichwert über eine Dauer von mindestens 20 Sekunden zu ermitteln, je nachdem wie schnell sich die verwendete PH Probe der Lösung anpasst ist vorher zu warten bis sich die Werte stabilisiert haben. Sollen typisch Werte zwischen 7 und 8 gemessen werden sollten Pufferlösungen mit um PH 7 und PH 9 eingesetzt werden.' +
         '    Ist der erste der Kalibrierungspunkte ermittelt speichert man diesen mit \'Speichern\', mit dem Zweiten ist im Anschluss genauso zu verfahren.' +
         '  </div>' +
         '  <div>' +
         '    <div>' +
         '      <span>Messung über</span>' +
         '      <input id="inputCalDuration" class="rounded-border inputNum" type="number" max="30" step="1" value="30" \>' +
         '      <span>Sekunden</span>' +
         '      <button class="rounded-border buttonOptions" style="height:inherit;" onclick="doPhCal()">Start</button>' +
         '    </div>' +
         '  </br>' +
         '  <div>' +
         '    <span>Messung für PH</span>' +
         '    <input id="inputCalPh1" class="rounded-border inputFloat" type="number" step="0.1" max="1.0" max="10.0" \>' +
         '    <input id="inputCalMeasureValue1" class="rounded-border inputNum" type="number" min="1" max="2023" step="1" \>' +
         '    <span id="currentCalMeasureText1" style="width: 25%;"></span>' +
         '  </div>' +
         '  <div>' +
         '    <span>Messung für PH</span>' +
         '    <input id="inputCalPh2" class="rounded-border inputFloat" type="number" step="0.1" max="1.0" max="10.0" \>' +
         '    <input id="inputCalMeasureValue2" class="rounded-border inputNum" type="number" min="1" max="2023" step="1" \>' +
         '    <span id="currentCalMeasureText2" style="width: 25%;"></span>' +
         '  </div>' +
         '  <button id="buttonStoreCal" class="rounded-border buttonOptions" style="height:inherit;" onclick="storePhCal()">Speichern</button>' +
         '  </div>' +
         '</div>';
   }

   var rootPhActual = document.getElementById("phShowActual");

   var phShowCal = document.getElementById("phShowCal");
   var currentPh = rootPhActual.querySelector("#currentPh");
   var currentPhMinusDemand = rootPhActual.querySelector("#currentPhMinusDemand");

   // phShowCal.querySelector("#buttonStoreCal").style.visibility = 'hidden';

   if (data.currentPh == "--")
      currentPh.innerHTML = data.currentPh;
   else {
      currentPh.innerHTML = data.currentPh.toFixed(2);
      currentPh.style.color = "yellow";
      currentPhMinusDemand.style.color = "yellow";
      hh = setInterval(resetFont, 400);
      rootPhActual.querySelector("#checkboxRefresh").checked = true;

      function resetFont() {
         currentPh.style.color = "white";
         currentPhMinusDemand.style.color = "white";
         clearInterval(hh);
      }
   }

   rootPhActual.querySelector("#currentPhValue").innerHTML = data.currentPhValue;
   rootPhActual.querySelector("#currentPhMinusDemand").innerHTML = data.currentPhMinusDemand + "&nbsp;ml";

   if (data.currentPhA != null) {
      rootPhActual.querySelector("#currentPhA").innerHTML = data.currentPhA.toFixed(2);
      rootPhActual.querySelector("#currentPhB").innerHTML = data.currentPhB.toFixed(2);
      rootPhActual.querySelector("#currentCalA").innerHTML = data.currentCalA;
      rootPhActual.querySelector("#currentCalB").innerHTML = data.currentCalB;

      phShowCal.querySelector("#inputCalPh1").value = data.currentPhA.toFixed(2);
      phShowCal.querySelector("#inputCalPh2").value = data.currentPhB.toFixed(2);
      phShowCal.querySelector("#inputCalMeasureValue1").value = data.currentCalA;
      phShowCal.querySelector("#inputCalMeasureValue2").value = data.currentCalB;
   }

   if (phCalTimerhandle == null)
      phCalTimerhandle = setInterval(requestActualPh, 3000);
}

window.checkboxRefreshKlick = function()
{
   if (!phCalTimerhandle && document.getElementById("checkboxRefresh").checked) {
      phCalTimerhandle = setInterval(requestActualPh, 3000);
   }
   else if (phCalTimerhandle && !document.getElementById("checkboxRefresh").checked) {
      clearInterval(phCalTimerhandle);
      phCalTimerhandle = null;
   }
}

window.doPhCal = function()
{
   showProgressDialog();

   clearInterval(phCalTimerhandle);
   phCalTimerhandle = null;
   document.getElementById("checkboxRefresh").checked = false;
   var duration = document.getElementById("inputCalDuration").value;
   socket.send({ "event" : "phcal", "object" : { "duration" : parseInt(duration)} });

   if (document.getElementById("inputCalMeasureValue2").value != "") {
      document.getElementById("inputCalMeasureValue1").value = "";
      document.getElementById("currentCalMeasureText1").innerHTML = "";
      document.getElementById("inputCalMeasureValue2").value = "";
      document.getElementById("currentCalMeasureText2").innerHTML = "";
   }
}

window.storePhCal = function()
{
   var cal1 = parseInt(document.getElementById("inputCalMeasureValue1").value);
   var ph1 = parseFloat(document.getElementById("inputCalPh1").value);
   var cal2 = parseInt(document.getElementById("inputCalMeasureValue2").value);
   var ph2 = parseFloat(document.getElementById("inputCalPh2").value);

   if (!cal1 || !ph1 || !cal2 || !ph2)
      return ;

   var message = "Kalibrierwert '" + cal1 + "' für PH " + ph1 + "</br>" +
       "Kalibrierwert '" + cal2 + "' für PH " + ph2;

   var form = '<form><div>' + message + '</div><br/></form>';

   $(form).dialog({
      modal: true,
      width: "60%",
      title: "Kalibrierwerte speichern?",
      buttons: {
         'Speichern': function () {
            console.log("storing calibration values");
            socket.send({ "event" : "phsetcal", "object" :
                          { "currentPhA"  : ph1,
                            "currentCalA" : cal1,
                            "currentPhB"  : ph2,
                            "currentCalB" : cal2 }
                        });
            $(this).dialog('close');
         },
         'Abbrechen': function () {
            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
      });
}
