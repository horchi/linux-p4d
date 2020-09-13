/*
 *  chart.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */


function drawCharts(dataObject, root)
{
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
               time: { displayFormats: {
                  millisecond: 'MMM DD - HH:MM',
                  second: 'MMM DD - HH:MM',
                  minute: 'HH:MM',
                  hour: 'MMM DD - HH:MM',
                  day: 'HH:MM',
                  week: 'MMM DD - HH:MM',
                  month: 'MMM DD - HH:MM',
                  quarter: 'MMM DD - HH:MM',
                  year: 'MMM DD - HH:MM' } },
               distribution: "linear",
               display: true,
               ticks: {
                  maxTicksLimit: 25,
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

   var colors = ['yellow','white','red','lightblue','lightgreen','purple','blue'];

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
   end.setDate(theChartStart.getDate()+theChartRange);

   $("#chartTitle").html(theChartStart.toLocaleString('de-DE') + "  -  " + end.toLocaleString('de-DE'));
   $("#chartSelector").html("");

   for (var i = 0; i < dataObject.sensors.length; i++)
   {
      var html = "<div class=\"chartSel\"><input id=\"checkChartSel_" + dataObject.sensors[i].id
          + "\"type=\"checkbox\" onclick=\"chartSelect('choice')\" " + (dataObject.sensors[i].active ? "checked" : "")
          + "/>" + dataObject.sensors[i].title + "</div>";
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

window.chartSelect = function(action)
{
   theChartRange = parseInt($("#chartRange").val());

   var sensors = getSensors();
   var now = new Date();

   if (action == "next")
      theChartStart.setDate(theChartStart.getDate()+1);
   else if (action == "prev")
      theChartStart.setDate(theChartStart.getDate()-1);
   else if (action == "now")
      theChartStart.setDate(now.getDate()-theChartRange);
   else if (action == "range")
      theChartStart.setDate(now.getDate()-theChartRange);

   // console.log("sensors:  '" + sensors + "'");

   var jsonRequest = {};
   prepareChartRequest(jsonRequest, sensors, theChartStart, theChartRange, "chart");
   socket.send({ "event" : "chartdata", "object" : jsonRequest });
}

window.addBookmark = function()
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

      console.log("storing " + name + " - " + sensors);
      socket.send({ "event" : "storechartbookmark", "object" : { "name"  : name, "sensors" : sensors }});
   }
}
