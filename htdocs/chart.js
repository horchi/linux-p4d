/*
 *  chart.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function drawCharts(dataObject)
{
   var update = document.getElementById("chartTitle") != null;

   if (!update) {
      $('#container').removeClass('hidden');

      document.getElementById("container").innerHTML =
         '<div id="chartTitle" class="rounded-border chartTitle"></div>' +
         '<div id="chartBookmarks" class="chartBookmarks"></div>' +
         '<canvas id="chartContainer" class="chartCanvas" width="1600" height="600"></canvas>' +
         '<div class="chartButtons">' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'prevmonth\')">&lt; Monat</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'prev\')">&lt; Tag</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'now\')">Jetzt</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'next\')">Tag &gt;</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'nextmonth\')">Monat &gt;</button>' +
         '  <div>Tage </div><input class="rounded-border chartButton" style="width:90px;" onchange="chartSelect(\'range\')" id="chartRange" type="number" step="0.25" min="0.25" value="1"></input>' +
         '</div>' +
         '<div id="chartSelector" class="chartSelectors"></div>';
   }

   var root = document.getElementById("chartContainer");

   if (theChart != null) {
      theChart.destroy();
      theChart = null;
   }

   var data = {
      type: "line",
      data: {
         labels: [],
         datasets: []
      },
      options: {
         responsive: false,
         tooltips: {
            mode: "index",
            intersect: false,
         },
         hover: {
            mode: "nearest",
            intersect: true
         },
         legend: {
            display: true,
            labels: {
               fontColor: "white"
            }
         },
         scales: {
            xAxes: [{
               type: "time",
               time: {
                  unit: 'hour',
                  unitStepSize: 1,
                  displayFormats: {
                  millisecond: 'MMM DD - HH:mm',
                  second: 'MMM DD - HH:mm',
                  minute: 'HH:mm',
                  hour: 'MMM DD - HH:mm',
                  day: 'HH:mm',
                  week: 'MMM DD - HH:mm',
                  month: 'MMM DD - HH:mm',
                  quarter: 'MMM DD - HH:mm',
                  year: 'MMM DD - HH:mm' } },
               distribution: "linear",
               display: true,
               ticks: {
                  maxTicksLimit: 24,
                  padding: 10,
                  fontColor: "white"
               },
               gridLines: {
                  color: "gray",
                  borderDash: [5,5]
               },
               scaleLabel: {
                  display: true,
                  fontColor: "white",
                  labelString: "Zeit"
               }
            }],
            yAxes: [{
               display: true,
               ticks: {
                  padding: 10,
                  maxTicksLimit: 20,
                  fontColor: "white"
               },
               gridLines: {
                  color: "gray",
                  zeroLineColor: 'gray',
                  borderDash: [5,5]
               },
               scaleLabel: {
                  display: true,
                  fontColor: "white",
                  labelString: "Temperatur [°C]"
               }
            }]
         }
      }
   };

   // console.log("dataObject: " + JSON.stringify(dataObject, undefined, 4));

   var colors = ['yellow','white','red','lightblue','lightgreen','purple','blue','green','pink','#E69138'];

   for (var i = 0; i < dataObject.rows.length; i++)
   {
      var dataset = {};

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = colors[i];
      dataset["borderColor"] = colors[i];
      dataset["label"] = dataObject.rows[i].title;
      dataset["borderWidth"] = 1.2;
      dataset["fill"] = false;
      dataset["pointRadius"] = 0;

      data.data.datasets.push(dataset);
   }

   var end = new Date();
   end.setFullYear(theChartStart.getFullYear(), theChartStart.getMonth(), theChartStart.getDate()+theChartRange);

   $("#chartTitle").html(theChartStart.toLocaleString('de-DE') + "  -  " + end.toLocaleString('de-DE'));
   $("#chartSelector").html("");

   updateChartBookmarks();

   for (var i = 0; i < dataObject.sensors.length; i++) {
      var html = '<div class="chartSel"><input id="checkChartSel_' + dataObject.sensors[i].id
          + '" type="checkbox" onclick="chartSelect(\'choice\')" ' + (dataObject.sensors[i].active ? 'checked' : '') + '/><label for="checkChartSel_' + dataObject.sensors[i].id + '">' + dataObject.sensors[i].title + '</label></div>';
      $("#chartSelector").append(html);
   }

   theChart = new Chart(root.getContext("2d"), data);
}

function getSensors()
{
   var sensors = "";
   var root = document.getElementById("chartSelector");
   var elements = root.querySelectorAll("[id^='checkChartSel_']");

   for (var i = 0; i < elements.length; i++) {
      if (elements[i].checked) {
         var id = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
         sensors += id + ",";
       }
   }

   return sensors;
}

function updateChartBookmarks()
{
   if (localStorage.getItem(storagePrefix + 'Rights') & 0x08 || localStorage.getItem(storagePrefix + 'Rights') & 0x10)
      $("#chartBookmarks").html('<div>'
                                + '<button title="Lesezeichen hinzufügen" class="rounded-border chartBookmarkButton" style="min-width:30px;" onclick="addBookmark()">&#128209;</button>'
                                + '<button title="zum Löschen hier ablegen" ondrop="dropBm(event)" ondragover="allowDropBm(event)" class="rounded-border chartBookmarkButton" style="min-width:30px;margin-right:50px;">&#128465;</button>'
                                + '</div>');
   else
      $("#chartBookmarks").html('');

   if (chartBookmarks != null) {
      var html = "<div>"

      for (var i = 0; i < chartBookmarks.length; i++)
         html += '<button class="rounded-border chartBookmarkButton" ondragstart="dragBm(event, \'' + chartBookmarks[i].name + '\')" draggable="true" onclick="chartSelectBookmark(\'' + chartBookmarks[i].sensors + '\')">' + chartBookmarks[i].name + '</button>';

      html += "</div>"
      $("#chartBookmarks").append(html);
   }
}

function dragBm(ev, name)
{
   console.log("drag: " + name);
   ev.dataTransfer.setData("name", name);
}

function allowDropBm(ev)
{
   console.log("allowDropBm");
   ev.preventDefault();
}

function dropBm(ev)
{
   ev.preventDefault();
   var name = ev.dataTransfer.getData("name");

   console.log("dropBm: " + name);
   for (var i = 0; i < chartBookmarks.length; i++) {
      if (chartBookmarks[i].name == name) {
         chartBookmarks.splice(i, 1);
         break;
      }
   }

   socket.send({ "event" : "storechartbookmarks", "object" : chartBookmarks});

   // console.log("bookmarks now " + JSON.stringify(chartBookmarks, undefined, 4));
}

Date.prototype.subHours = function(h)
{
  this.setTime(this.getTime() - (h*60*60*1000));
  return this;
}

function chartSelect(action)
{
   theChartRange = parseFloat($("#chartRange").val());
   console.log("theChartRange: " + theChartRange);
   var sensors = getSensors();
   var now = new Date();

   if (action == "next")
      theChartStart.setFullYear(theChartStart.getFullYear(), theChartStart.getMonth(), theChartStart.getDate()+1);
   else if (action == "nextmonth")
      theChartStart.setFullYear(theChartStart.getFullYear(), theChartStart.getMonth(), theChartStart.getDate()+30);
   else if (action == "prev")
      theChartStart.setFullYear(theChartStart.getFullYear(), theChartStart.getMonth(), theChartStart.getDate()-1);
   else if (action == "prevmonth")
      theChartStart.setFullYear(theChartStart.getFullYear(), theChartStart.getMonth(), theChartStart.getDate()-30);
   else if (action == "now")
      theChartStart.setFullYear(now.getFullYear(), now.getMonth(), now.getDate()-theChartRange);
   else if (action == "range"){
      theChartStart = new Date().subHours(theChartRange * 24); //  setFullYear(now.getFullYear(), now.getMonth(), now.getDate()-theChartRange);
   }

   // console.log("sensors:  '" + sensors + "'" + ' Range:' + theChartRange);

   var jsonRequest = {};
   prepareChartRequest(jsonRequest, sensors, theChartStart, theChartRange, "chart");
   socket.send({ "event" : "chartdata", "object" : jsonRequest });
   showProgressDialog();
}

function chartSelectBookmark(sensors)
{
   console.log("bookmark: " + sensors);

   var jsonRequest = {};
   prepareChartRequest(jsonRequest, sensors, theChartStart, theChartRange, "chart");
   socket.send({ "event" : "chartdata", "object" : jsonRequest });
   showProgressDialog();
}

function addBookmark()
{
   var sensors = getSensors();

   var form = '<form><div>Sensoren: ' + sensors + '</div><br/><input type="text" name="input"><br></form>';

   $(form).dialog({
      modal: true,
      width: "60%",
      title: "Lesezeichen",
      buttons: {
         'Speichern': function () {
            var name = $('input[name="input"]').val();
            if (name != "")
               storeBookmark(name, sensors);
            $(this).dialog('close');
         },
         'Abbrechen': function () {
            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });

   function storeBookmark(name, sensors) {
      if (chartBookmarks != null)
         chartBookmarks.push({ "name"  : name, "sensors" : sensors });
      else
         chartBookmarks = [{ "name"  : name, "sensors" : sensors }];

      console.log("storing " + name + " - " + sensors);
      socket.send({ "event" : "storechartbookmarks", "object" : chartBookmarks});
   }
}
