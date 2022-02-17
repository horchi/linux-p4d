/*
 *  chart.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var currentRequest = null;

function drawCharts(dataObject)
{
   var root = document.getElementById("chartContainer");

   if (!root) {
      $('#container').removeClass('hidden');
      $('#dashboardMenu').removeClass('hidden');
      $('#dashboardMenu').html('');

      document.getElementById("dashboardMenu").innerHTML =
         '<div id="chartTitle" class="rounded-border chartTitle"></div>' +
         '<div id="chartBookmarks" class="rounded-border chartBookmarks"></div>';

      //

      document.getElementById("container").innerHTML =
         '<canvas id="chartContainer" class="chartCanvas" width="1600" height="600"></canvas>' +
         '<div class="rounded-border chartButtons">' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'prevmonth\')">&lt; Monat</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'prev\')">&lt; Tag</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'now\')">Jetzt</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'next\')">Tag &gt;</button>' +
         '  <button class="rounded-border chartButton" onclick="chartSelect(\'nextmonth\')">Monat &gt;</button>' +
         '  <div>Tage </div><input class="rounded-border chartButton" style="width:90px;" onchange="chartSelect(\'range\')" id="chartRange" type="number" step="0.25" min="0.25" value="' + theChartRange + '"></input>' +
         '</div>' +
         '<div id="chartSelector" class="rounded-border chartSelectors"></div>';

      root = document.getElementById("chartContainer");
   }

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
         }
      }
   };

   // console.log("dataObject: " + JSON.stringify(dataObject, undefined, 4));

   var colors = ['gray','yellow','white','red','lightblue','lightgreen','purple','blue','green','pink','#E69138'];

   var yAxes = [];
   var knownUnits = {};

   for (var i = 0; i < dataObject.rows.length; i++) {
      var unit = valueFacts[dataObject.rows[i].key].unit;
      var dataset = {};

      if (knownUnits[unit] == null) {
         // console.log("Adding yAxis for " + unit);
         knownUnits[unit] = true;
         yAxes.push({
            id: unit,
            display: true,
            ticks: { padding: 10, maxTicksLimit: 20, fontColor: colors[i] },
            gridLines: { lineWidth: 1, color: colors[i], zeroLineColor: 'gray', borderDash: [5,5] },
            scaleLabel: { display: true, fontColor: colors[i], labelString: '[' + unit + ']' }
         });
      }

      dataset["data"] = dataObject.rows[i].data;
      dataset["backgroundColor"] = colors[i];
      dataset["borderColor"] = colors[i];
      dataset["label"] = dataObject.rows[i].title;
      dataset["borderWidth"] = 1.2;
      dataset["fill"] = false;
      dataset["pointRadius"] = 0;
      dataset["yAxisID"] = unit;

      data.data.datasets.push(dataset);
   }

   data.options.scales.yAxes = yAxes;
   var end = new Date();
   end.setFullYear(theChartStart.getFullYear(), theChartStart.getMonth(), theChartStart.getDate()+theChartRange);

   $("#chartTitle").html(theChartStart.toLocaleString('de-DE') + "  -  " + end.toLocaleString('de-DE'));
   $("#chartSelector").html("");

   updateChartBookmarks();

   for (var i = 0; i < dataObject.sensors.length; i++) {
      console.log("sensor " + dataObject.sensors[i].id);
      var fact = valueFacts[dataObject.sensors[i].id];

      var html = '<div class="chartSel"><input id="checkChartSel_' + dataObject.sensors[i].id +
          '" type="checkbox" onclick="chartSelect(\'choice\')" ' + (dataObject.sensors[i].active ? 'checked' : '') +
          '/><label for="checkChartSel_' + dataObject.sensors[i].id + '">' + dataObject.sensors[i].title + ' [' + fact.unit + ']</label></div>';
      $("#chartSelector").append(html);
   }

   theChart = new Chart(root.getContext("2d"), data);

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - 15);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - 15);
   };
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

var refreshTimer = null;

function setRefreshTimer()
{
   if (refreshTimer)
      clearTimeout(refreshTimer);

   if ($('#refresh').is(":checked")) {
      refreshTimer = setTimeout(function() {
         refresh();
         console.log("refresh");
      }, 120000);
   }
}

function refresh()
{
   console.log("do refresh");
   if (currentRequest != null) {
      socket.send({ "event" : "chartdata", "object" : currentRequest });
      showProgressDialog();
   }
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

   var html = ' <div style="display:flex;margin-left:60px;text-align:right;align-items: center;"><span style="align-self:center;width:120px;">Refresh:</span><span><input id="refresh" style="width:auto;" type="checkbox"' + (refreshTimer != null ? ' checked' : '') + '/><label for="refresh"></label></span></div>';
   $("#chartBookmarks").append(html);
   $("#refresh").click(function() {setRefreshTimer()});

   setRefreshTimer();
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
   else
      theChartStart = new Date().subHours(theChartRange * 24);

   // console.log("sensors:  '" + sensors + "'" + ' Range:' + theChartRange);

   var jsonRequest = {};
   prepareChartRequest(jsonRequest, sensors, theChartStart, theChartRange, "chart");
   currentRequest = jsonRequest;
   socket.send({ "event" : "chartdata", "object" : jsonRequest });
   showProgressDialog();
}

function chartSelectBookmark(sensors)
{
   console.log("bookmark: " + sensors);

   var jsonRequest = {};
   prepareChartRequest(jsonRequest, sensors, theChartStart, theChartRange, "chart");
   currentRequest = jsonRequest;
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
