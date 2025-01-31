/*
 *  widgetsetup.js
 *
 *  (c) 2020-2024 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function widgetSetup(key)
{
   var item = valueFacts[key];
   var widget = dashboards[actDashboard].widgets[key];

   if (allSensors[key] != null)
   {
      // console.log("sensor " + JSON.stringify(allSensors[key], undefined, 4));
      // console.log("sensor found, batt is : " + allSensors[key].battery);
   }
   else
      console.log("sensor not found: " + key);

   if (widget == null)
      console.log("widget not found: " + key);

   var form = document.createElement("div");

   $(form).append($('<div></div>')
                  .addClass('settingsDialogContent')

                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('ID'))
                          .append($('<span></span>')
                                  .append($('<div></div>')
                                          .addClass('rounded-border')
                                          .html(key + ' (' + parseInt(key.split(":")[1]) + ')')
                                         )))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Widget'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .change(function() {widgetTypeChanged();})
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widgettype')
                                         )))

                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Titel'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('type', 'search')
                                          .attr('id', 'title')
                                          .val(widget.title)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divUnit')
                          .append($('<span></span>')
                                  .html('Einheit'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('type', 'search')
                                          .attr('id', 'unit')
                                          .val(widget.unit)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divScalemin')
                          .append($('<span></span>')
                                  .html('Skala Min'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'scalemin')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.scalemin)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divScalemax')
                          .append($('<span></span>')
                                  .html('Skala Max'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'scalemax')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.scalemax)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divRescale')
                          .append($('<span></span>')
                                  .html('Rescale'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'rescale')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.rescale))
                                  .append($('<label></label>')
                                          .prop('for', 'rescale')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divScalestep')
                          .append($('<span></span>')
                                  .html('Skala Step'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'scalestep')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.scalestep)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divCritmin')
                          .append($('<span></span>')
                                  .html('Skala Crit Min'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'critmin')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.critmin)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divCritmax')
                          .append($('<span></span>')
                                  .html('Skala Crit Max'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'critmax')
                                          .attr('type', 'number')
                                          .attr('step', '0.1')
                                          .val(widget.critmax)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divBarWidth')
                          .append($('<span></span>')
                                  .html('Bar Breite '))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'barwidth')
                                          .attr('type', 'number')
                                          .attr('step', '1')
                                          .val(widget.barwidth)
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divBarColor')
                          .append($('<span></span>')
                                  .html('Bar Farbe, condition'))
                          .append($('<span></span>')
                                  .css('display', 'flex')
                                  .append($('<span></span>')
                                          .css('margin-right', '5px')
                                          .append($('<input></input>')
                                                  .attr('id', 'barcolor')
                                                  .attr('type', 'text')
                                                  .addClass('rounded-border inputSetting')
                                                  .val(widget.barcolor)))
                                  .append($('<span></span>')
														.css('width', '-webkit-fill-available')
														.css('width', '-moz-available')
														.append($('<input></input>')
																  .attr('id', 'colorConditionBar')
																  .attr('title', 'Syntax:\nWert1=Farbe1,Wert2=Farbe2,...\n Operatoren =,<,>,>=,<=,<>,~\n ~ enthalten in\nFarbe: rgb(r g b), #xxxxxx, <color>')
																  .attr('type', 'search')
																  .addClass('rounded-border inputSetting')
																  .val(widget.colorConditionBar)
																 ))))

                  .append($('<div></div>')
                          .attr('id', 'divSymbol')
                          .append($('<span></span>')
                                  .append($('<a></a>')
                                          .html('Icon')
                                          .attr('target', '_blank')  // open in new tab
                                          .attr('href', 'https://pictogrammers.github.io/@mdi/font/6.5.95')))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'symbol')
                                          .attr('type', 'search')
                                          .val(widget.symbol)
                                          .change(function() {widgetTypeChanged();})
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divSymbolOn')
                          .append($('<span></span>')
                                  .append($('<a></a>')
                                          .html('Icon an')
                                          .attr('target', '_blank')  // open in new tab
                                          .attr('href', 'https://pictogrammers.github.io/@mdi/font/6.5.95')))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'symbolOn')
                                          .attr('type', 'search')
                                          .val(widget.symbolOn)
                                          .change(function() {widgetTypeChanged();})
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divImgoff')
                          .append($('<span></span>')
                                  .html('Image'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgoff')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divImgon')
                          .append($('<span></span>')
                                  .html('Image an'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'imgon')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divColor')
                          .append($('<span></span>')
                                  .attr('id', 'spanColor')
                                  .html('Farbe, condition'))
                          .append($('<span></span>')
                                  .css('display', 'flex')
                                  .append($('<span></span>')
														.attr('id', 'colorSpan')
                                          .append($('<input></input>')
                                                  .attr('id', 'color')
                                                  .attr('type', 'text')
                                                  .addClass('rounded-border inputSetting')
                                                  .val(widget.color)
                                                 ))
                                  .append($('<span></span>')
														.css('margin-right', '5px')
                                          .append($('<input></input>')
                                                  .attr('id', 'colorOn')
                                                  .attr('type', 'text')
                                                  .addClass('rounded-border inputSetting')
                                                  .val(widget.colorOn)
																 ))

											 .append($('<span></span>')
														.attr('id', 'divColorCondition')
														.css('width', '-webkit-fill-available')
														.css('width', '-moz-available')
														.append($('<input></input>')
																  .attr('id', 'colorCondition')
																  .attr('title', 'Syntax:\nWert1=Farbe1,Wert2=Farbe2,...\n Operatoren =,<,>,>=,<=,<>,~\n ~ enthalten in\nFarbe: rgb(r g b), #xxxxxx, <color>')
																  .attr('type', 'search')
																  .addClass('rounded-border inputSetting')
																  .val(widget.colorCondition)
																 ))
											))

                  .append($('<div></div>')
                          .attr('id', 'divPeak')
                          .append($('<span></span>')
                                  .html('Peak anzeigen'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'peak')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.showpeak))
                                  .append($('<label></label>')
                                          .prop('for', 'peak')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divShowValue')
                          .append($('<span></span>')
                                  .html('Value anzeigen'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'showvalue')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.showvalue != null ? widget.showvalue : true))
                                  .append($('<label></label>')
                                          .prop('for', 'showvalue')
                                         )))

                  .append($('<div></div>')
                          .attr('id', 'divLinefeed')
                          .append($('<span></span>')
                                  .html('Zeilenumbruch'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'linefeed')
                                          .attr('type', 'checkbox')
                                          .prop('checked', widget.linefeed))
                                  .append($('<label></label>')
                                          .prop('for', 'linefeed')
                                         )))
                  .append($('<div></div>')
                          .attr('id', 'divRange')
                          .append($('<span></span>')
                                  .html('Chart Bereich'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('title', 'Tage')
                                          .attr('id', 'range')
                                          .attr('type', 'number')
                                          .attr('step', '0.5')
                                          .val(widget.range)
                                         )))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Breite'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widthfactor')
                                         )))
                  .append($('<div></div>')
                          .append($('<span></span>')
                                  .html('Höhe'))
                          .append($('<span></span>')
                                  .append($('<select></select>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'heightfactor')
                                         )))
                 );

   function widgetTypeChanged()
   {
      var wType = parseInt($('#widgettype').val());

      $("#divUnit").css("display", [1,3,4,5,6,9,13,14].includes(wType) ? 'flex' : 'none');
      // $("#divFactor").css("display", [1,3,4,6,9,13].includes(wType) ? 'flex' : 'none');
      $("#divScalemax").css("display", [5,6,14].includes(wType) ? 'flex' : 'none');
      $("#divRescale").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divScalemin").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divScalestep").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divCritmin").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divCritmax").css("display", [5,6].includes(wType) ? 'flex' : 'none');
      $("#divBarWidth").css("display", [6,14].includes(wType) ? 'flex' : 'none');
      $("#divBarColor").css("display", [6].includes(wType) ? 'flex' : 'none');
      $("#divSymbolOn").css("display", [6].includes(wType) ? 'flex' : 'none');
      $("#divSymbol").css("display", [0,9,12].includes(wType) ? 'flex' : 'none');
      $("#divSymbolOn").css("display", [0,9,12].includes(wType) ? 'flex' : 'none');
      $("#divImgon").css("display", ([0,9].includes(wType) && $('#symbol').val() == '') ? 'flex' : 'none');
      $("#divImgoff").css("display", ([0,9].includes(wType) && $('#symbol').val() == '') ? 'flex' : 'none');
      $("#divPeak").css("display", [1,3,6,9,13].includes(wType) ? 'flex' : 'none');
      $("#divColor").css("display", [0,1,2,3,4,6,7,8,9,10,11,12,13,14].includes(wType) ? 'flex' : 'none');
      $("#divColorCondition").css("display", [0,2,3,6,7,8,9,10,11,12].includes(wType) ? 'flex' : 'none');
      $("#divLinefeed").css("display", [10].includes(wType) ? 'flex' : 'none');
      $("#divRange").css("display", [1,5,6,13].includes(wType) ? 'flex' : 'none');

      if ([0].includes(wType) && $('#symbol').val() == '' && $('#symbolOn').val() == '')
         $("#divColor").css("display", 'none');

      if (![0,9].includes(wType) || $('#symbolOn').val() == '') {
         $('.spColorOn').css('display', 'none');
         $('#spanColor').html("Farbe, condition");
      }
      else {
         $('.spColorOn').css('display', 'flex');
         $('#spanColor').html("Farbe aus, an, condition");
			$('#colorSpan').css("margin-right", '5px');
      }
   }

   var title = key.split(":")[0];

   if (item != null)
      title = (item.usrtitle ? item.usrtitle : item.title);

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: 'fade',
      width: 'auto',
      title: 'Widget - ' + title,
      open: function() {
         for (var wdKey in widgetTypes) {
            $('#widgettype').append($('<option></option>')
                                    .val(widgetTypes[wdKey])
                                    .html(wdKey)
                                    .attr('selected', widgetTypes[wdKey] == widget.widgettype));
         }

         images.sort();

         for (var img in images) {
            $('#imgoff').append($('<option></option>')
                                .val(images[img])
                                .html(images[img])
                                .attr('selected', widget.imgoff == images[img]));
            $('#imgon').append($('<option></option>')
                               .val(images[img])
                               .html(images[img])
                               .attr('selected', widget.imgon == images[img]));
         }

         if (widget.widthfactor == null)
            widget.widthfactor = 1;

         for (var w = 1; w <= 4.0; w += 0.5)
            $('#widthfactor').append($('<option></option>')
                                     .val(w)
                                     .html(w)
                                     .attr('selected', widget.widthfactor == w));

         if (widget.heightfactor == null)
            widget.heightfactor = 1;

         for (var h = 0.5; h <= 2.0; h += 0.5)
            $('#heightfactor').append($('<option></option>')
                                     .val(h)
                                      .html(h)
                                     .attr('selected', widget.heightfactor == h));

         $('#color').spectrum({
            type: "color",
            containerClassName: 'rounded-border',
            showPalette: true,
            showSelectionPalette: true,
            palette: [ ],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            showInitial: true,
            showAlpha: true,
            allowEmpty: false,
            replacerClassName: 'spColor'
         });

         $('#colorOn').spectrum({
            type : "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [ ],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            showInitial: true,
            showAlpha: true,
            allowEmpty: false,
            replacerClassName: 'spColorOn'
         });

         $('#barcolor').spectrum({
            type : "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [ ],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            showInitial: true,
            showAlpha: true,
            allowEmpty: false,
            replacerClassName: 'spColor'
         });

         var wType = parseInt($('#widgettype').val());

         if (![0,9].includes(wType) || $('#symbolOn').val() == '') {
            $('.spColorOn').css('display', 'none');
            $('#spanColor').html("Farbe");
         }

         widgetTypeChanged();
         $(".ui-dialog-buttonpane button:contains('Widget löschen')").attr('style','color:#ff5757');
      },
      buttons: {
         'Widget löschen': function () {
            console.log("delete widget: " + key);
            document.getElementById('div_' + key).remove();

            var json = {};
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               json[key] = dashboards[actDashboard].widgets[key];
            });

            // console.log("delete widget " + JSON.stringify(json, undefined, 4));
            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
            $(this).dialog('close');
         },

         'Abbrechen': function () {
            // socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
            if (allSensors[key] == null) {
               console.log("missing sensor!!");
            }
            initWidget(key, dashboards[actDashboard].widgets[key]);
            updateWidget(allSensors[key], true, dashboards[actDashboard].widgets[key]);

            $(this).dialog('close');
         },
         'Vorschau': function () {
            widget = Object.create(dashboards[actDashboard].widgets[key]); // valueFacts[key]);
            widget.title = $("#title").val();
            widget.unit = $("#unit").val();
            widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
            widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
            widget.rescale = $("#rescale").is(':checked');
            widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
            widget.critmin = parseFloat($("#critmin").val()) || -1;
            widget.critmax = parseFloat($("#critmax").val()) || -1;
            widget.barwidth = parseInt($("#barwidth").val()) || 7;
            widget.barcolor = $("#barcolor").spectrum('get').toRgbString();
            widget.symbol = $("#symbol").val();
            widget.symbolOn = $("#symbolOn").val();
            widget.imgon = $("#imgon").val();
            widget.imgoff = $("#imgoff").val();
            widget.widgettype = parseInt($("#widgettype").val());
            widget.color = $("#color").spectrum('get').toRgbString();
            widget.colorOn = $("#colorOn").spectrum('get').toRgbString();
            widget.colorCondition = $("#colorCondition").val();
				widget.colorConditionBar = $("#colorConditionBar").val();
            widget.showpeak = $("#peak").is(':checked');
            widget.showvalue = $("#showvalue").is(':checked');
            widget.linefeed = $("#linefeed").is(':checked');
            widget.widthfactor = $("#widthfactor").val();
            widget.heightfactor = $("#heightfactor").val();
            widget.range = $("#range").val();

            initWidget(key, widget);
            if (allSensors[key] != null)
               updateWidget(allSensors[key], true, widget);
         },
         'Ok': function () {
            widget.title = $("#title").val();
            widget.unit = $("#unit").val();
            widget.scalemax = parseFloat($("#scalemax").val()) || 0.0;
            widget.scalemin = parseFloat($("#scalemin").val()) || 0.0;
            widget.rescale = $("#rescale").is(':checked');
            widget.scalestep = parseFloat($("#scalestep").val()) || 0.0;
            widget.critmin = parseFloat($("#critmin").val()) || -1;
            widget.critmax = parseFloat($("#critmax").val()) || -1;
            widget.barwidth = parseInt($("#barwidth").val()) || 7;
            widget.barcolor = $("#barcolor").spectrum('get').toRgbString();
            widget.symbol = $("#symbol").val();
            widget.symbolOn = $("#symbolOn").val();
            widget.imgon = $("#imgon").val();
            widget.imgoff = $("#imgoff").val();
            widget.widgettype = parseInt($("#widgettype").val());
            widget.color = $("#color").spectrum("get").toRgbString();
            widget.colorOn = $("#colorOn").spectrum("get").toRgbString();
            widget.colorCondition = $("#colorCondition").val();
				widget.colorConditionBar = $("#colorConditionBar").val();
            widget.showpeak = $("#peak").is(':checked');
            widget.showvalue = $("#showvalue").is(':checked');
            widget.linefeed = $("#linefeed").is(':checked');
            widget.widthfactor = $("#widthfactor").val();
            widget.heightfactor = $("#heightfactor").val();
            widget.range = $("#range").val();

            initWidget(key, widget);
            if (allSensors[key] != null)
               updateWidget(allSensors[key], true, widget);

            var json = {};
            $('#widgetContainer > div').each(function () {
               var key = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
               json[key] = dashboards[actDashboard].widgets[key];
            });

            if ($("#title").length)
               json[key]["title"] = $("#title").val();
            if ($("#unit").length)
               json[key]["unit"] = $("#unit").val();
            // if ($("#factor").length)
            //    json[key]["factor"] = parseFloat($("#factor").val());
            if ($("#scalemax").length)
               json[key]["scalemax"] = parseFloat($("#scalemax").val());
            if ($("#scalemin").length)
               json[key]["scalemin"] = parseFloat($("#scalemin").val());
            if ($("#scalestep").length)
               json[key]["scalestep"] = parseFloat($("#scalestep").val());
            if ($("#critmin").length)
               json[key]["critmin"] = parseFloat($("#critmin").val());
            if ($("#critmax").length)
               json[key]["critmax"] = parseFloat($("#critmax").val());
            if ($("#barwidth").length)
               json[key]["barwidth"] = parseInt($("#barwidth").val());
            if ($("#symbol").length)
               json[key]["symbol"] = $("#symbol").val();
            if ($("#symbolOn").length)
               json[key]["symbolOn"] = $("#symbolOn").val();
            if ($("#imgon").length)
               json[key]["imgon"] = $("#imgon").val();
            if ($("#imgoff").length)
               json[key]["imgoff"] = $("#imgoff").val();

            json[key]["widthfactor"] = parseFloat($("#widthfactor").val());
            json[key]["heightfactor"] = parseFloat($("#heightfactor").val());
            json[key]["range"] = parseFloat($("#range").val());
            json[key]["widgettype"] = parseInt($("#widgettype").val());
            json[key]["rescale"] = $("#rescale").is(':checked');
            json[key]["showpeak"] = $("#peak").is(':checked');
            json[key]["showvalue"] = $("#showvalue").is(':checked');
            json[key]["linefeed"] = $("#linefeed").is(':checked');
            json[key]["color"] = $("#color").spectrum("get").toRgbString();
            json[key]["colorOn"] = $("#colorOn").spectrum("get").toRgbString();
            json[key]["colorCondition"] = $("#colorCondition").val();
				json[key]["colorConditionBar"] = $("#colorConditionBar").val();
            json[key]["barcolor"] = $("#barcolor").spectrum("get").toRgbString();

            socket.send({ "event" : "storedashboards", "object" : { [actDashboard] : { 'title' : dashboards[actDashboard].title, 'widgets' : json } } });
            socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });

            $(this).dialog('close');
         }
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });
}
function dashboardSetup(did)
{
   let form = document.createElement("div");
   let dashboardId = parseInt(did.substring(4));

   if (dashboards[dashboardId].options == null)
      dashboards[dashboardId].options = {};

   $(form).append($('<div></div>')
                  .addClass('settingsDialogContent')

                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Titel'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'dashTitle')
                                          .attr('type', 'search')
                                          .val(dashboards[dashboardId].title)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Symbol'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'dashSymbol')
                                          .attr('type', 'search')
                                          .val(dashboards[dashboardId].symbol)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Zeilenhöhe'))
                          .append($('<span></span>')
                                  // .append($('<select></select>')
											 .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'heightfactor')
                                          .val(dashboards[dashboardId].options.heightfactor)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Zeilenhöhe Kiosk'))
                          .append($('<span></span>')
                                  // .append($('<select></select>')
											 .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'heightfactorKiosk')
                                          .val(dashboards[dashboardId].options.heightfactorKiosk)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Widget Breite Kiosk'))
                          .append($('<span></span>')
                                  // .append($('<select></select>')
											 .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('id', 'widthfactorKiosk')
                                          .val(dashboards[dashboardId].options.widthfactorKiosk)
                                         )))
                  .append($('<div></div>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Gruppe'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .addClass('rounded-border inputSetting')
                                          .attr('type', 'number')
                                          .attr('id', 'group')
                                          .val(dashboards[dashboardId].group)
                                         )))
                 );

   $(form).dialog({
      modal: true,
      resizable: false,
      closeOnEscape: true,
      hide: "fade",
      width: "auto",
      title: "Dashbord - " + dashboards[dashboardId].title,
      open: function() {
         $(".ui-dialog-buttonpane button:contains('Dashboard löschen')").attr('style','color:#ff5757');

         if (!dashboards[dashboardId].options.heightfactor)
            dashboards[dashboardId].options.heightfactor = 1;
         if (!dashboards[dashboardId].options.heightfactorKiosk)
            dashboards[dashboardId].options.heightfactorKiosk = 1;
			if (!dashboards[dashboardId].options.widthfactorKiosk)
            dashboards[dashboardId].options.widthfactorKiosk = 1;

//         for (let w = 0.5; w <= 2.0; w += 0.5) {
//            $('#heightfactor').append($('<option></option>')
//                                      .val(w).html(w).attr('selected', dashboards[dashboardId].options.heightfactor == w));
//            $('#heightfactorKiosk').append($('<option></option>')
//                                      .val(w).html(w).attr('selected', dashboards[dashboardId].options.heightfactorKiosk == w));
//         }
      },
      buttons: {
         'Dashboard löschen': function () {
            console.log("delete dashboard: " + dashboards[dashboardId].title);
            socket.send({ "event" : "storedashboards", "object" : { [dashboardId] : { 'action' : 'delete' } } });
            socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });

            $(this).dialog('close');
         },
         'Ok': function () {
            dashboards[dashboardId].options = {};
            dashboards[dashboardId].options.heightfactor = $("#heightfactor").val();
            dashboards[dashboardId].options.heightfactorKiosk = $("#heightfactorKiosk").val();
            dashboards[dashboardId].options.widthfactorKiosk = $("#widthfactorKiosk").val();
            console.log("set options: " + JSON.stringify(dashboards[dashboardId].options, undefined, 4));
            dashboards[dashboardId].title = $("#dashTitle").val();
            dashboards[dashboardId].symbol = $("#dashSymbol").val();
            dashboards[dashboardId].group = parseInt($("#group").val());

            socket.send({ "event" : "storedashboards", "object" : { [dashboardId] : { 'title' : dashboards[dashboardId].title,
                                                                                      'group' : dashboards[dashboardId].group,
                                                                                      'symbol' : dashboards[dashboardId].symbol,
                                                                                      'options' : dashboards[dashboardId].options} } });
            socket.send({ "event" : "forcerefresh", "object" : { 'action' : 'dashboards' } });
            $(this).dialog('close');
         },
         'Abbrechen': function () {
            $(this).dialog('close');
         }
      },

      close: function() { $(this).dialog('destroy').remove(); }
   });
}
