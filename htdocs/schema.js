/*
 *  schema.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

var oTop = 0;
var oLeft = 0;
var lastSchemadata = null;
var properties = [ "top", "left", "color", "font-size", "color", "background-color", "border", "border-radius", "z-index"];

function initSchema(schemaData)
{
   // console.log(JSON.stringify(schemaData, undefined, 4));
   lastSchemadata = schemaData;

   $('#container').empty()
      .removeClass('hidden')
      .append($('<div></div>')
              .attr('id', 'schemaContainer')
              .addClass('rounded-border schemaBox')
              .on('contextmenu', function(event) {
                 if (schemaEditActive) {
                    schemaContextMenu(event);
                    return false;
                 }
              })
              .append($('<img></img>')
                      .attr('src', 'img/schema/schema-' + config.schema + '.png')
                      .attr('draggable', false)
                      .on('drop', function(event) { dropSValue(event); })
                      .on('dragover', function(event) { return false; }))
             );

   for (let i = 0; i < schemaData.length; i++) {
      if (!lastSchemadata[i].deleted)
         initSchemaElement(schemaData[i], i);
   }

   updateSchema();

   $("#container").height($(window).height() - getTotalHeightOf('menu') - 8);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - 8);
   };
}

function dropSValue(event)
{
   event.preventDefault();
   let id = '#' + event.originalEvent.dataTransfer.getData('id');
   let schemaDef = getSchemDef(event.originalEvent.dataTransfer.getData('id'));

   console.log("drop", id, 'at', (event.offsetX - oLeft)  + 'px', (event.offsetY - oTop)  + 'px');

   schemaDef.properties["top"] = (event.offsetY - oTop)  + 'px';
   schemaDef.properties["left"] = (event.offsetX - oLeft)  + 'px';
   $(id).css({'top'  : (event.offsetY - oTop)  + 'px'});
   $(id).css({'left' : (event.offsetX - oLeft) + 'px'});
}

function schemaContextMenu(event)
{
   let form = $('<div></div>')
       .attr('id', 'schemaContextMenu')
       .append($('<div></div>')
               .addClass('dialog-content')
               .append($('<div></div>')
                       .css('display', 'grid')
                       .css('grid-gap', '4px')
                       .append($('<button></button>')
                               .addClass('rounded-border button1')
                               .html('&#10010')
                               .click(function() { $("#schemaContextMenu").dialog('close');
																	schemaAddItem(); }))
                       .append($('<button></button>')
                               .attr('title', 'add user defines value')
                               .addClass('rounded-border button1')
                               .html('Leitung hinzufügen')
                               .click(function() { $("#schemaContextMenu").dialog('close');
																	makeResizableDiv(); }))
                       .append($('<button></button>')
                               .addClass('rounded-border button1')
                               .html('Speichern')
                               .click(function() { $("#schemaContextMenu").dialog('close');
																	schemaStore(); })
                              )));

   form.dialog({
      modal: true,
      position: { my: "left top", at: "center", of: event },
      width: "250px",
      dialogClass: "no-titlebar",
		modal: true,
      minHeight: "0px",
      resizable: false,
		closeOnEscape: true,
      hide: "fade",
      open: function() {
			$('.ui-widget-overlay').bind('click', function()
                                      { $("#schemaContextMenu").dialog('close'); });
      },
      close: function() {
         $(this).dialog('destroy').remove();
      }
   });
}

function setupSchema()
{
   $("#burgerPopup").dialog("close");

   schemaEditActive = !schemaEditActive;
   initSchema(lastSchemadata);
   updateSchema();

   // if (!schemaEditActive)
   // ?? document.getElementById('resizers').display == "none"; // hide Resizer

   activateResizer();
}

function getSchemDef(id)
{
   for (let i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type + ((lastSchemadata[i].address)>>>0).toString(10) == id)
         return lastSchemadata[i];
   }
}

// get schema definition by '<Type>:0x<HexAddr>' notation

function getItemDef(key)
{
   for (let i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type + ':0x' + ((lastSchemadata[i].address)>>>0).toString(16).toUpperCase() == key)
         return lastSchemadata[i];
   }

   return { 'value' : 'unkown id', 'title' : 'unkown', 'name' : 'unkown', type : 'NN', address : -1 };
}

function getItemById(id)
{
   for (let key in allSensors) {
      if (allSensors[key].type + ((allSensors[key].address)>>>0).toString(10) == id)
         return allSensors[key];
   }
}

function getItem(id)
{
   let idPar = id.split(':');

   if (idPar.length == 2) {
      for (let key in allSensors) {
         if (idPar[0] == allSensors[key].type && parseInt(idPar[1]) == allSensors[key].address) {
            return allSensors[key];
         }
      }
   }

   return { 'value' : 'unkown id', 'title' : 'unkown', 'name' : 'unkown', type : 'NN', address : -1 };
}

function initSchemaElement(item, tabindex)
{
   let id = item.type + (item.address >>> 0).toString(10);

   $('#schemaContainer')
      .append($('<div></div>')
              .attr('id', id)
              .attr('tabindex', tabindex)
              .addClass('schemaDiv')
              .css('visibility', schemaEditActive || item.state == 'A' ? 'visible' : 'hidden')
              .css('cursor', schemaEditActive ? 'move' : 'default')
              .css('position', 'absolute')
              .data('type', item.type)
              .data('address', (item.address >>> 0).toString(10))
             );

   setAttributeStyleByProperties('#'+id, item.properties);

   if (schemaEditActive) {
      $('#' + id).click(function() { editSchemaValue(item.type, item.address); });
      $('#' + id).attr('draggable', true);
      $('#' + id).on('dragstart', function(event) {
         event.originalEvent.dataTransfer.setData('id', item.type + (item.address >>> 0).toString(10));
      });

      $('#' + id).on('mousedown', function(event) {
         oTop = event.offsetY;
         oLeft = event.offsetX;
      });

      $('#' + id).on('keydown', function(event) {
         let top = parseInt($(this).css('top'));
         let left = parseInt($(this).css('left'));
         let pixel = event.ctrlKey ? 10 : 1;

         switch (event.keyCode) {
            case 37: left -= pixel; break;
            case 38: top  -= pixel; break;
            case 39: left += pixel; break;
            case 40: top  += pixel; break;
         }

         let schemaDef = getSchemDef($(this).attr('id'));
         schemaDef.properties["top"] = top + 'px';
         schemaDef.properties["left"] = left + 'px';
         $(this).css('top', top + 'px');
         $(this).css('left', left + 'px');
      });
   }
}

function updateSchema()
{
   if (!lastSchemadata)
      return;

   for (let i = 0; i < lastSchemadata.length; i++) {
      let schemaDef = lastSchemadata[i];
      let id = schemaDef.type + (schemaDef.address >>> 0).toString(10);
      var item = getItemById(id);
      let key = toKey(schemaDef.type, schemaDef.address);
      let fact = valueFacts[key];

      if (!schemaDef)
         continue;

      if (item && $("#"+id).data('changedAt') == item.changedAt)
         continue;

      // if (!item)
      //   console.log("no item for", id, key);

      $("#"+id).data('changedAt', item ? item.changedAt : '');

      let theText = '';

      if (schemaDef.fct != null && schemaDef.fct != '') {
         try {
            theText += eval(schemaDef.fct);
         }
         catch (err) {
            showInfoDialog({'message' : "Fehler in JS Funktion: '" + err.message + ", " + schemaDef.fct + "'", 'status': -1});
            schemaDef.fct = "";  // delete buggy function
         }
         // console.log("result of '" + schemaDef.fct + "' is " + result);
      }
      else if (schemaDef.usrtext != null)
         theText += schemaDef.usrtext;

      let html = '';

      if (schemaDef.showtitle && item != null && item.title != null)
         html += item.title + ":&nbsp;";

      if (schemaDef.kind == 2) {
         if (theText == '' && item != null)
            theText = item.image;
         // else if (item == null)
         //    console.log("Missing item for " + JSON.stringify(schemaDef, undefined, 4));

         if (theText != '') {
            let style = 'width:' + (schemaDef.width ? schemaDef.width : 40) + 'px;';
            if (schemaDef.height)
               style = 'height:' + schemaDef.height + 'px';

            html += '<img style="' + style + '" src="' + theText + '"></img>';
         }
      }
      else {
         html += theText;
      }

      if ((schemaDef.fct == null || schemaDef.fct == '') && item != null) {
         if (schemaDef.kind == 0)
            html += item.value;
         else if (schemaDef.kind == 1)
            html += item.text || '';

         html += schemaDef.showunit ? fact.unit || "" : "";
      }
      else {
         if(schemaDef.fct == null && schemaDef.fct == "" && item == null)
             html += " item: '" + id + "' is null";
      }

      $("#"+id).html(html == "" ? "&nbsp;" : html);

      let title = "";

      if (schemaDef.type == "UC")
         title = "User Constant";
      else if (schemaDef.type == "CN")
         title = "Leitung";
      else if (item != null)
         title = item.title || '';

      $("#"+id).attr("title", title + " " + schemaDef.type + ":0x" + ((schemaDef.address)>>>0).toString(16));
   }
}

function schemaStore()
{
   socket.send({ "event" : "storeschema", "object" : lastSchemadata });
}

function activateResizer()
{
   $('#schemaContainer').append($('<div></div>')
                                .addClass('resizable')
                                .attr('id', 'newLine')
                                .append($('<div></div>')
                                        .addClass('resizers')
                                        .append($('<div></div>')
                                                .addClass('resizer top-left'))
                                        .append($('<div></div>')
                                                .addClass('resizer top-right'))
                                        .append($('<div></div>')
                                                .addClass('resizer bottom-left'))
                                        .append($('<div></div>')
                                                .addClass('resizer bottom-right'))
                                       ));

   const element = document.querySelector('#newLine');
   const resizers = element.querySelectorAll('.resizer')
   const minimum_size = 10;
   let original_width = 0;
   let original_height = 0;
   let original_x = 0;
   let original_y = 0;
   let original_mouse_x = 0;
   let original_mouse_y = 0;
   for (let i = 0;i < resizers.length; i++) {
      const currentResizer = resizers[i];
      currentResizer.addEventListener('mousedown', function(e) {
         e.preventDefault()
         original_width = parseFloat(getComputedStyle(element, null).getPropertyValue('width').replace('px', ''));
         original_height = parseFloat(getComputedStyle(element, null).getPropertyValue('height').replace('px', ''));
         original_x = element.getBoundingClientRect().left;
         original_y = element.getBoundingClientRect().top;
         original_mouse_x = e.pageX;
         original_mouse_y = e.pageY;
         window.addEventListener('mousemove', resize)
         window.addEventListener('mouseup', stopResize)
      })

      function resize(e) {
         if (currentResizer.classList.contains('bottom-right')) {
            const width = original_width + (e.pageX - original_mouse_x);
            const height = original_height + (e.pageY - original_mouse_y)
            if (width > minimum_size) {
               element.style.width = width + 'px'
            }
            if (height > minimum_size) {
               element.style.height = height + 'px'
            }
         }
         else if (currentResizer.classList.contains('bottom-left')) {
            const height = original_height + (e.pageY - original_mouse_y)
            const width = original_width - (e.pageX - original_mouse_x)
            if (height > minimum_size) {
               element.style.height = height + 'px'
            }
            if (width > minimum_size) {
               element.style.width = width + 'px'
               element.style.left = original_x + (e.pageX - original_mouse_x) + 'px'
            }
         }
         else if (currentResizer.classList.contains('top-right')) {
            const width = original_width + (e.pageX - original_mouse_x)
            const height = original_height - (e.pageY - original_mouse_y)
            if (width > minimum_size) {
               element.style.width = width + 'px'
            }
            if (height > minimum_size) {
               element.style.height = height + 'px'
               element.style.top = original_y + (e.pageY - original_mouse_y-83.39) + 'px'
            }
         }
         else if (currentResizer.classList.contains('top-left')){
            const width = original_width - (e.pageX - original_mouse_x)
            const height = original_height - (e.pageY - original_mouse_y)
            if (width > minimum_size) {
               element.style.width = width + 'px'
               element.style.left = original_x + (e.pageX - original_mouse_x) + 'px'
            }
            if (height > minimum_size) {
               element.style.height = height + 'px'
               element.style.top = original_y + (e.pageY - original_mouse_y-83.39) + 'px'
            }
         }
      }

      function stopResize() {
         window.removeEventListener('mousemove', resize)
      }
   }
}

function schemaAddItem()
{
   let addr = 0;

   for (let i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type == "UC" && ((lastSchemadata[i].address)>>>0).toString(10) == addr)
         addr++;
   }

   schemaDef = { type: "UC",      // User Constant
                 address: addr,
                 state: "A",
                 properties: {},
                 kind: 0,
                 showtitle: 1,
                 showunit: 0 };

   lastSchemadata[lastSchemadata.length] = schemaDef;
   initSchemaElement(schemaDef, lastSchemadata.length);
   editSchemaValue(schemaDef.type, schemaDef.address, true);
}

function makeResizableDiv(div)
{
   let addr = 0;

   if (lastSchemadata) {
      for (let i = 0; i < lastSchemadata.length; i++) {
         if (lastSchemadata[i].type == "CN" && ((lastSchemadata[i].address)>>>0).toString(10) == addr)
            addr++;
      }
   }

   schemaDef = { type: "CN",      //Connection
                 address: addr,
                 state: "A",
                 properties: {},
                 kind: 3,
                 showtitle: 1,
                 showunit: 0 };

   const props = document.getElementById('newLine');

   schemaDef.properties['top'] =  props.style.top;
   schemaDef.properties['left'] =  props.style.left;
   schemaDef.properties['width'] =  (parseInt(props.style.width.replace(/px/,""))-10)+"px";
   schemaDef.properties['height'] =  (parseInt(props.style.height.replace(/px/,""))-10)+"px";

   lastSchemadata[lastSchemadata.length] = schemaDef;
   initSchemaElement(schemaDef, lastSchemadata.length);
   // editSchemaValue(schemaDef.type, schemaDef.address, true);
}

function setAttributeStyleByProperties(id, json)
{
   for (let key in json)
      $(id).css(key, json[key]);
}

function editSchemaValue(type, address, newUC)
{
   // console.log("editSchemaValue of id: " + id);

   let key = toKey(type, address);
   let id = type + ((address)>>>0).toString(10);

   let schemaDef = getSchemDef(id);
   let isUC = schemaDef.type == "UC";
   let isCN = schemaDef.type == "CN";
   let item = allSensors[key];

   let form = $('<div></div>')
       .attr('id', 'settingsForm')
       .addClass('schemaSettingsForm')

       .append($('<div></div>')
               .append($('<span></span>')
                       .html('Einblenden'))
               .append($('<span></span>')
                       .append($('<input></input>')
                               .attr('id', 'showIt')
                               .attr('type', 'checkbox')
                               .attr('checked', schemaDef.state == 'A'))
                       .append($('<label></label>')
                               .attr('for', 'showIt'))))

       .append($('<div></div>')
               .append($('<span></span>')
                       .html('Farbe'))
               .append($('<span></span>')
                       .append($('<input></input>')
                               .attr('id', 'colorFg')
                               .attr('type', 'text')
                               .val(schemaDef.properties["color"] || "white"))))
       .append($('<div></div>')
               .append($('<span></span>')
                       .html('Hintergrund'))
               .append($('<span></span>')
                       .append($('<input></input>')
                               .attr('id', 'colorBg')
                               .attr('type', 'text')
                               .val(schemaDef.properties["background-color"] || "transparent"))))

       .append($('<div></div>')
               .append($('<span></span>')
                       .html('Rahmen'))
               .append($('<span></span>')
                       .append($('<input></input>')
                               .attr('id', 'showBorder')
                               .attr('type', 'checkbox')
                               .val(schemaDef.properties['border'] != 'none')
                               .append($('<label></label>')
                                       .attr('for', 'showBorder')))))

       .append($('<div></div>')
               .append($('<span></span>')
                       .html('Radius'))
               .append($('<span></span>')
                       .append($('<input></input>')
                               .attr('id', 'borderRadius')
                               .attr('type', 'text')
                               .val(schemaDef.properties["border-radius"] || "3px"))));


   if (!isCN) {
      form.append($('<div></div>')
                  .append($('<span></span>')
                          .html('Schriftgröße'))
                  .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'fontSize')
                                  .attr('type', 'text')
                                  .val(schemaDef.properties["font-size"] || "16px"))));
   }

   form.append($('<div></div>')
               .append($('<span></span>')
                       .html('Layer'))
               .append($('<span></span>')
                       .append($('<input></input>')
                               .attr('id', 'zIndex')
                               .attr('type', 'number')
                               .attr('step', 1)
                               .val(schemaDef.properties["z-index"] || "100"))));

   if (!isUC && !isCN) {
      form.append($('<div></div>')
                  .append($('<span></span>')
                          .html('Bezeichnung'))
                  .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'showTitle')
                                  .attr('type', 'checkbox')
                                  .attr('checked', schemaDef.showtitle))
                          .append($('<label></label>')
                                  .attr('for', 'showTitle'))))
      form.append($('<div></div>')
                  .append($('<span></span>')
                          .html('Einheit'))
                  .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'showUnit')
                                  .attr('type', 'checkbox')
                                  .attr('checked', schemaDef.showunit))
                          .append($('<label></label>')
                                  .attr('for', 'showUnit'))));
   }

   if (isCN) {
      form.append($('<div></div>')
                  .append($('<span></span>')
                          .html('Type'))
                  .append($('<span></span>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('links->rechts'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 3)
                                          .val(3)
                                          .click(function() { selectKind(3); })))

                          .append($('<span></span>')
                                  .html('Type'))
                          .append($('<span></span>')
                                  .html('rechts->links'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 4)
                                          .val(4)
                                          .click(function() { selectKind(4); })))

                          .append($('<span></span>')
                                  .html('Type'))
                          .append($('<span></span>')
                                  .html('oben-unten'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 5)
                                          .val(5)
                                          .click(function() { selectKind(5); })))

                          .append($('<span></span>')
                                  .html('Type'))
                          .append($('<span></span>')
                                  .html('unten->oben'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 6)
                                          .val(6)
                                          .click(function() { selectKind(6); }))))
                 );
   }
   else {
      form.append($('<div></div>')
                  .append($('<span></span>')
                          .html('Type'))
                  .append($('<span></span>')
                          .css('display', 'flex')
                          .append($('<span></span>')
                                  .html('Wert'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 0)
                                          .val(0)
                                          .click(function() { selectKind(0); })))
                          .append($('<span></span>')
                                  .html('Type'))
                          .append($('<span></span>')
                                  .html('Text'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 1)
                                          .val(1)
                                          .click(function() { selectKind(1); })))
                          .append($('<span></span>')
                                  .html('Bild'))
                          .append($('<span></span>')
                                  .html('Wert'))
                          .append($('<span></span>')
                                  .append($('<input></input>')
                                          .attr('type', 'radio')
                                          .attr('name', 'kind')
                                          .attr('checked', schemaDef.kind == 2)
                                          .val(2)
                                          .click(function() { selectKind(2); }))))
                 );
   }

   if (!isCN) {
      form.append($('<div></div>')
                  .attr('id', 'imgSize')
                  .append($('<span></span>')
                          .html('Bild Breite'))
                  .append($('<span></span>')
                          .append($('<input></input>')
                                  .attr('id', 'imgWidth')
                                  .attr('type', 'number')
                                  .val(schemaDef.width))))
         .append($('<div></div>')
                 .append($('<span></span>')
                         .html('Bild Höhe'))
                 .append($('<span></span>')
                         .append($('<input></input>')
                                 .attr('id', 'imgHeight')
                                 .attr('type', 'number')
                                 .val(schemaDef.height))))

         .append($('<div></div>')
                 .attr('id', 'imgInfo')
                 .append($('<span></span>')
                         .css('font-size', 'smaller')
                         .css('width', 'auto')
                         .html('Um das vom p4d gelieferte Bild zu verwenden (' + (item ? item.image : '') + ') Funktion und URL leer lassen!')))

         .append($('<div></div>')
                 .append($('<span></span>')
                         .attr('id', 'labelFunction'))
                 .append($('<span></span>')
                         .append($('<textarea></textarea>')
                                 .attr('id', 'usrText')
                                 .html(schemaDef.usrtext || ''))))

         .append($('<div></div>')
                 .append($('<span></span>')
                         .attr('id', 'labelUsrText'))
                 .append($('<span></span>')
                         .append($('<textarea></textarea>')
                                 .attr('id', 'function')
                                 .css('height', '120px')
                                 .html(schemaDef.fct || ''))));
   }
   if (!isCN)
   {
      form.append($('<div></div>')
                  .append($('<span></span>')
                          .css('font-size', 'smaller')
                          .css('width', 'auto')
                          .html('JavaScript Funktion zum berechnen des angezeigten Wertes. Auf die Daten des Elements kann über item.value, item.unit und item.text zugegriffen werden. Andere Elemente erhält man mit getItem(id)')));

      // JavaScript Funktion zum berechnen des angezeigten Wertes. Elemente erhält man mit getItem(id)
   }

   let title = isUC ? "User Constant" : ((item != null && item.title) || "");
   title = isCN ? "Connection" : title;

   let buttons = {
      'Ok': function () {
         if (apply("#"+id) == 0)
            $(this).dialog('close');
      },
      'Check': function () {
         try {
            eval($('#function').val());
            showInfoDialog({ 'message' : 'function valid', 'status': 0 });
         }
         catch (err) {
            showInfoDialog({ 'message' : "Fehler in JS Funktion: '" + err.message + ", " + $('#function').val() + "'", 'status': -1 });
         }
      },
      'Abbrechen': function () {
         if (newUC)
            deleteUC(id);
         $(this).dialog('close');
      }
   };

   if (isUC || isCN)  {
      buttons['Löschen'] = function () {
         if (deleteUC(id) == 0)
            $(this).dialog('close');
      };
   }

   $(form).dialog({
      modal: true,
      title: title + " " + schemaDef.type + ":0x" + ((schemaDef.address)>>>0).toString(16),
      width: "50%",
      buttons: buttons,
      open: function() {
         $('#colorFg').spectrum({
            type : "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            appendTo: '#settingsForm',
            hideAfterPaletteSelect : true,
            showInput : true,
            showButtons : false,
            allowEmpty : false
         });
         $('#colorBg').spectrum({
            type : "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [],
            localStorageKey: "homectrl",
            togglePaletteOnly: false,
            appendTo: '#settingsForm',
            hideAfterPaletteSelect : "true",
            showInput : true,
            showButtons : false,
            allowEmpty : false
         });

         selectKind(schemaDef.kind);
      },
      close: function() { $(this).dialog('destroy').remove(); }
   });

   function selectKind(kind) {
      // console.log("kind", kind);
      switch (kind) {
        case 0:
           $('#labelUsrText').html(isUC ? "Text:" : "Zusatz Text:");
           $('#labelFunction').html("Funktion:");
           $('#imgSize').css('display', 'none');
           $('#imgInfo').css('display', 'none');
           break;
        case 1:
           $('#labelUsrText').html(isUC ? "Text:" : "Zusatz Text:");
           $('#labelFunction').html("Funktion:");
           $('#imgSize').css('display', 'none');
           $('#imgInfo').css('diese', 'none');
           break;
        case 2:
           $('#labelUsrText').html("Image URL:");
           $('#labelFunction').html("Funktion Image URL:");
           $('#imgSize').css('display', 'flex');
           $('#imgInfo').css('display', 'flex');
           break;
      }
   }

   function deleteUC(id) {
      for (let i = 0; i < lastSchemadata.length; i++) {
         if (lastSchemadata[i].type + ((lastSchemadata[i].address)>>>0).toString(10) == id) {
            lastSchemadata[i].deleted = true;
            $("#" + id).remove();
            return 0;
         }
      }
      return -1;
   }

   function apply(id) {
      schemaDef.properties["color"] = $('#colorFg').spectrum("get").toRgbString();
      schemaDef.properties["background-color"] = $('#colorBg').spectrum("get").toRgbString();
      schemaDef.properties["border"] = $('#showBorder').is(":checked") ? "" : "none";
      schemaDef.properties["border-radius"] = $('#borderRadius').val();
      schemaDef.properties["font-size"] = $('#fontSize').val();
      schemaDef.properties["z-index"] = $('#zIndex').val();
      schemaDef.usrtext = $('#usrText').val();
      schemaDef.width = parseInt($('#imgWidth').val());
      schemaDef.height = parseInt($('#imgHeight').val());
      schemaDef.kind = parseInt(document.querySelector('input[name="kind"]:checked').value);
      schemaDef.showtitle = $('#showTitle').is(":checked") ? 1 : 0;
      schemaDef.showunit = $('#showUnit').is(":checked") ? 1 : 0;
      schemaDef.state = $('#showIt').is(":checked") ? "A": "D";

      try {
         eval($('#function').val());
      }
      catch (err) {
         showInfoDialog({'message' : "Fehler in JS Funktion: '" + err.message + ", " + $('#function').val() + "'", 'status': -1});
         return -1;
      }

      schemaDef.fct = $('#function').val();
      setAttributeStyleByProperties(id, schemaDef.properties);
      updateSchema()

      return 0;
   }
}
