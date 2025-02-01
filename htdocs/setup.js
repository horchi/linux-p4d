/*
 *  setup.js
 *
 *  (c) 2020-2024 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var theConfigdetails = {};
var setupCategory = '';

// ----------------------------------------------------------------
// Base Setup
// ----------------------------------------------------------------

function initConfig(configdetails)
{
   theConfigdetails = configdetails;

   $('#container').removeClass('hidden');
   $('#container').empty();

   dlgTabs = ($('<div></div>')
              .addClass('tabButtons')
              .attr('id', 'setupTabs'));

   $('#container').append(dlgTabs);

   $(dlgTabs).append($('<span></span>')
                     .append($('<button></button>')
                             .addClass('rounded-border buttonOptions')
                             .html('Speichern')
                             .click(function() { storeConfig(); })));

   $(dlgTabs).append($('<span></span>')
                     .attr('id', 'btnTabs'));

   dlgContent = ($('<div></div>')
                 .attr('id', 'setupContainer')
                 .addClass('inputTableConfig'));

   $('#container').append(dlgContent);

   let lastCat = '';

   for (var i = 0; i < configdetails.length; i++) {
      let item = configdetails[i];
      let choiceSel = null;

      if (lastCat != item.category) {

         if (setupCategory == '')
            setupCategory = item.category;

         $('#btnTabs').append($('<button></button>')
                           .html(item.category)
                           .addClass(setupCategory == item.category ? 'active' : '')
                           .click(function() {
                              setupCategory = $(this).html();
                              initConfig(theConfigdetails);
                           }));

         lastCat = item.category;
      }

      if (item.category != setupCategory) {
         // console.log("Skipping category", item.category)
         continue;
      }

      let itemsDiv = null;
      $(dlgContent).append(itemsDiv = $('<div></div>')
                           .attr('title', item.description)
                           .append($('<span></span>')
                                   .addClass('labelB1')
                                   .html(item.title)));

      switch (item.type) {
      case 0:     // ctInteger
         $(itemsDiv)
            .append($('<span></span>')
                    .append($('<input></input>')
                            .attr('id', 'input_' + item.name)
                            .attr('type', 'number')
                            .addClass('rounded-border inputNum')
                            .val(item.value)));
         break;

      case 1:     // ctNumber (float)
         $(itemsDiv)
            .append($('<span></span>')
                    .append($('<input></input>')
                            .attr('id', 'input_' + item.name)
                            .attr('type', 'number')
                            .attr('step', '0.1')
                            .addClass('rounded-border inputFloat')
                            .val(parseFloat(item.value.replace(',', '.')))));
         break;

      case 2:     // ctString
         $(itemsDiv)
            .append($('<span></span>')
                    .append($('<input></input>')
                            .attr('id', 'input_' + item.name)
                            .attr('type', 'search')
                            .addClass('rounded-border input')
                            .val(item.value)));
         break;

      case 3:     // ctBool
         $(itemsDiv)
            .append($('<span></span>')
                    .append($('<input></input>')
                            .attr('id', 'checkbox_' + item.name)
                            .attr('type', 'checkbox')
                            .addClass('rounded-border input')
                            .prop('checked', item.value == 1))
                    .append($('<label></label>')
                            .prop('for', 'checkbox_' + item.name)));
         break;

      case 4:     // ctRange
      {
         $(itemsDiv).append(rangeDiv = $('<span></span>')
                       .css('display', 'inline-grid'));
         function appendRangeTuple(index, name, range, description = '')
         {
            var aRange = range.split("-");

            $(rangeDiv)
               .append($('<span></span>')
                       .css('display', 'inline-flex')
                       .append($('<input></input>')
                               .attr('id', 'range_' + name + index + 'From')
                               .attr('type', 'search')
                               .addClass('rounded-border inputTime')
                               .val(aRange[0] ? aRange[0] : ''))
                       .append($('<span></span>')
                               .html(' bis '))
                       .append($('<input></input>')
                               .attr('id', 'range_' + name + index + 'To')
                               .attr('type', 'search')
                               .addClass('rounded-border inputTime')
                               .val(aRange[1] ? aRange[1] : '')));
         }

         let n = 0;

         if (item.value != '') {
            var ranges = item.value.split(",");
            for (n = 0; n < ranges.length; ++n)
               appendRangeTuple(n, item.name, ranges[n]);
         }
         appendRangeTuple(n, item.name, '', item.description);

         break;
      }
      case 5:    // ctChoice
         $(itemsDiv)
            .append($('<span></span>')
                    .append(choiceSel = $('<select></select>')
                            .attr('id', 'input_' + item.name)
                            .addClass('rounded-border input')
                            .prop('name', 'style')));

         if (item.options != null) {
            for (var o = 0; o < item.options.length; o++) {
               $(choiceSel).append($('<option></option>')
                                   .val(item.options[o])
                                   .html(item.options[o])
                                   .attr('selected', item.options[o] == item.value));
            }
         }
         break;

      case 6:    // ctMultiSelect
         $(itemsDiv)
            .append(choiceSel = $('<span></span>')
                    .append($('<input></input>')
                            .attr('id', 'mselect_' + item.name)
                            .attr('type', 'text')
                            .addClass('rounded-border input')
                            .data('index', i)
                            .data('value', item.value)
                            .prop('name', 'style')
                            .val('')
                           ));
         break;

      case 7:    // ctBitSelect
         let bmGroup = null;
         $(itemsDiv)
            .append(bmGroup = $('<span></span>')
                    .attr('id', 'bmaskgroup_' + item.name));

         var array = item.value.split(',');

         for (var o = 0; o < item.options.length; o++) {
            var checked = false;
            for (var n = 0; n < array.length; n++) {
               if (array[n] == item.options[o])
                  checked = true;
            }
            $(bmGroup)
               .append($('<input></input>')
                       .attr('id', 'bmask' + item.name + '_' + item.options[o])
                       .attr('type', 'checkbox')
                       .prop('checked', checked)
                       .addClass('rounded-border input'))
               .append($('<label></label>')
                       .prop('for', 'bmask' + item.name + '_' + item.options[o])
                       .html(item.options[o]));
         }
         break;

      case 8:     // ctText
         $(itemsDiv)
            .append($('<span></span>')
                    .append($('<textarea></textarea>')
                            .attr('id', 'input_' + item.name)
                            .attr('type', 'number')
                            .addClass('rounded-border input')
                            .val(item.value)));
         break;
      }
   }

   $('input[id^="mselect_"]').each(function () {
      var item = configdetails[$(this).data('index')];
      $(this).autocomplete({
         source: item.options,
         multiselect: true});
      if ($(this).data('value').trim() != "") {
         setAutoCompleteValues($(this), $(this).data('value').trim().split(','));
      }
   });

   $("#container").height($(window).height() - $("#menu").height() - getTotalHeightOf('footer') - sab - 8);
   window.onresize = function() {
      $("#container").height($(window).height() - $("#menu").height() - getTotalHeightOf('footer') - sab - 8);
   };
}

function storeConfig()
{
   var jsonObj = {};
   var rootConfig = document.getElementById("container");

   console.log("storeSettings");

   // ctString, ctNumber, ctInteger, ctChoice, ctText

   var elements = rootConfig.querySelectorAll("[id^='input_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1); {
         if (elements[i].getAttribute('type') == 'number' && elements[i].getAttribute('step') != undefined &&
             elements[i].value != '')
            jsonObj[name] = parseFloat(elements[i].value).toLocaleString("de-DE");
         else
            jsonObj[name] = elements[i].value;
      }
   }

   // ctBool

   var elements = rootConfig.querySelectorAll("[id^='checkbox_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      jsonObj[name] = (elements[i].checked ? "1" : "0");
   }

   // ctRange

   var elements = rootConfig.querySelectorAll("[id^='range_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      if (name.match("0From$")) {
         name = name.substring(0, name.indexOf("0From"));
         jsonObj[name] = toTimeRangesString("range_" + name);
         // console.log("value: " + jsonObj[name]);
      }
   }

   // ctMultiSelect -> as string

   var elements = rootConfig.querySelectorAll("[id^='mselect_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      var value = getAutoCompleteValues($("#mselect_" + name));
      value = value.replace(/ /g, ',');
      jsonObj[name] = value;
   }

   // ctBitSelect -> as string

   var elements = rootConfig.querySelectorAll("[id^='bmaskgroup_']");

   for (var i = 0; i < elements.length; i++) {
      var name = elements[i].id.substring(elements[i].id.indexOf("_") + 1);
      var bits = rootConfig.querySelectorAll("[id^='bmask" + name + "_']");
      var value = '';

      for (var i = 0; i < bits.length; i++) {
         var o = bits[i].id.substring(bits[i].id.indexOf("_") + 1);
         if (bits[i].checked)
            value += o + ','
      }
      console.log("store bitmak: " + value);
      jsonObj[name] = value;
   }

   // console.log(JSON.stringify(jsonObj, undefined, 4));

   socket.send({ "event" : "storeconfig", "object" : jsonObj });
}

function toTimeRangesString(base)
{
   var times = '';

   for (var i = 0; i < 10; i++) {
      var id = "#" + base + i;

      if ($(id + "From") && $(id + "From").val() && $(id + "To") && $(id + "To").val()) {
         if (times != "") times += ",";
         times += $(id + "From").val() + "-" + $(id + "To").val();
      }
      else {
         break;
      }
   }

   return times;
}
