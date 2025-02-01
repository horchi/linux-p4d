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
// var schemaRoot = null;

function initSchema(schemaData)
{
   // console.log(JSON.stringify(schemaData, undefined, 4));

   lastSchemadata = schemaData;

   $('#container').removeClass('hidden');

   $('#container').empty()
      .append($('<div></div>')
              .attr('id', 'schemaContainer')
              .addClass('rounded-border schemaBox')
              .append($('<img></img>')
                      .attr('src', 'img/schema/schema-' + config.schema + '.png')
                      .attr('draggable', false)
                      .on('drop', function(event) {dropSValue(event);})
                      .on('dragover', function(event) {allowDropSValue(event);}))
             );


//   schemaRoot = document.getElementById("schemaContainer");
//    var image = document.createElement("img");
//   image.setAttribute("src", "img/schema/schema-" + config.schema + ".png");
//   image.setAttribute("draggable", "false");

//   if (schemaEditActive) {
//      image.setAttribute("ondrop", "dropSValue(event)");
//      image.setAttribute("ondragover", "allowDropSValue(event)");
//   }
//   schemaRoot.appendChild(image);

   for (var i = 0; i < schemaData.length; i++) {
      if (!lastSchemadata[i].deleted)
         initSchemaElement(schemaData[i], i);
   }

   updateSchema();

   $("#container").height($(window).height() - getTotalHeightOf('menu') - 8);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - 8);
   };
}

function getSchemDef(id)
{
   for (var i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type + ((lastSchemadata[i].address)>>>0).toString(10) == id)
         return lastSchemadata[i];
   }
}

// get schema definition by '<Type>:0x<HexAddr>' notation

function getItemDef(key)
{
   for (var i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type + ':0x' + ((lastSchemadata[i].address)>>>0).toString(16).toUpperCase() == key)
         return lastSchemadata[i];
   }

   return { 'value' : 'unkown id', 'title' : 'unkown', 'name' : 'unkown', type : 'NN', address : -1 };
}

function getItemById(id)
{
   for (var key in allSensors) {
      if (allSensors[key].type + ((allSensors[key].address)>>>0).toString(10) == id)
         return allSensors[key];
   }
}

function getItem(id)
{
   var idPar = id.split(':');

   if (idPar.length == 2) {
      for (var key in allSensors) {
         if (idPar[0] == allSensors[key].type && parseInt(idPar[1]) == allSensors[key].address) {

            if (parseInt(idPar[1]) == 0x1f)
               console.log("Debug key", key, 'value', allSensors[key]);

            return allSensors[key];
         }
      }
   }

   return { 'value' : 'unkown id', 'title' : 'unkown', 'name' : 'unkown', type : 'NN', address : -1 };
}

function initSchemaElement(item, tabindex)
{
   var id = item.type + ((item.address)>>>0).toString(10);
   var div = document.createElement("div");

   div.classList.add("schemaDiv");
   div.setAttribute("id", id);
   div.setAttribute('data-type', item.type);
   div.setAttribute('data-address', ((item.address)>>>0).toString(10));
   setAttributeStyleFrom(div, item.properties);
   div.style.visibility = (schemaEditActive || item.state == 'A') ? "visible" : "hidden";
   div.style.cursor = schemaEditActive ? "move" : "default";

   if (schemaEditActive) {
      div.setAttribute("onclick", 'editSchemaValue("' + item.type + '", ' + item.address + ')');
      div.setAttribute('tabindex', tabindex);
   }

   if (schemaEditActive) {
      div.setAttribute("draggable", true);
      div.setAttribute("ondragstart", 'dragSValue(event,"' + item.type + ((item.address)>>>0).toString(10) + '")');

      div.addEventListener('mousedown', event => {
         oTop = event.offsetY;
         oLeft = event.offsetX;
      });

      div.addEventListener("keydown", event => {
         // console.log(" keyCode: " + event.keyCode + " ctrlKey: " + event.ctrlKey);
         var ele = event.currentTarget;
         var top = parseInt(ele.style.top);
         var left = parseInt(ele.style.left);
         var id = ele.id; //.substr(3);
         var schemaDef = getSchemDef(id);
         var pixel = event.ctrlKey ? 10 : 1;

         switch (event.keyCode)
         {
            case 37: left -= pixel; break;
            case 38: top  -= pixel; break;
            case 39: left += pixel; break;
            case 40: top  += pixel; break;
         }

         schemaDef.properties["top"] = top + 'px';
         schemaDef.properties["left"] = left + 'px';
         ele.style.top = top + 'px';
         ele.style.left = left + 'px';
      });
   }

   $('#schemaContainer').append($(div));
}

function updateSchema()
{
   for (var i = 0; i < lastSchemadata.length; i++) {
      var schemaDef = lastSchemadata[i];
      var id = schemaDef.type + ((schemaDef.address)>>>0).toString(10);
      var item = getItemById(id);
      var key = toKey(schemaDef.type, schemaDef.address);
      var fact = valueFacts[key];

      if (schemaDef == null)
         continue;

      var theText = "";

      if (schemaDef.fct != null && schemaDef.fct != "") {
         try {
            theText += eval(schemaDef.fct);
         }
         catch (err) {
            showInfoDialog({'message' : "Fehler in JS Funktion: '" + err.message + ", " + schemaDef.fct + "'", 'status': -1});
            schemaDef.fct = "";  // delete the buggy function
         }
         // console.log("result of '" + schemaDef.fct + "' is " + result);
      }
      else if (schemaDef.usrtext != null)
         theText += schemaDef.usrtext;

      var html = "";

      if (schemaDef.showtitle && item != null && item.title != null)
         html += item.title + ":&nbsp;";

      if (schemaDef.kind == 2) {
         if (theText == "" && item != null)
            theText = item.image;
         else if (item == null)
            console.log("Missing item for " + JSON.stringify(schemaDef, undefined, 4));

         if (theText != "") {
            var style = 'width:' + (schemaDef.width ? schemaDef.width : 40) + 'px;';
            if (schemaDef.height)
               style = 'height:' + schemaDef.height + 'px';

            html += '<img style="' + style + '" src="' + theText + '"></img>';
         }
      }
      else {
         html += theText;
      }

      if ((schemaDef.fct == null || schemaDef.fct == "") && item != null) {
         if (schemaDef.kind == 0)
            html += item.value;
         else if (schemaDef.kind == 1)
            html += item.text || "";

         html += schemaDef.showunit ? fact.unit || "" : "";
      }
      else {
         if(schemaDef.fct == null && schemaDef.fct == "" && item == null)
             html += " item: '" + id + "' is null";
      }

      $("#"+id).html(html == "" ? "&nbsp;" : html);

      var title = "";
      if (schemaDef.type == "UC")
         title = "User Constant";
      if (schemaDef.type == "CN")
         title = "Leitung";
      else if (item != null)
         title = item.title;

      $("#"+id).attr("title", title + " (" + schemaDef.type + ":" + ((schemaDef.address)>>>0).toString(10) + ")");
   }
}

function schemaEditModeToggle()
{
   schemaEditActive = !schemaEditActive;
   initSchema(lastSchemadata);
   updateSchema();

   document.getElementById("buttonSchemaStore").style.visibility = schemaEditActive ? 'visible' : 'hidden';
   document.getElementById("buttonSchemaAddItem").style.visibility = schemaEditActive ? 'visible' : 'hidden';
   document.getElementById("makeResizableDiv").style.visibility = schemaEditActive ? 'visible' : 'hidden';

   if (!schemaEditActive)
      document.getElementById('resizers').display == "none"; //hide Resizer

   activateResizer();
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

// makeResizableDiv('.resizable')

function schemaAddItem()
{
   var addr = 0;

   for (var i = 0; i < lastSchemadata.length; i++) {
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
   var addr = 0;

   if (lastSchemadata) {
      for (var i = 0; i < lastSchemadata.length; i++) {
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
   //editSchemaValue(schemaDef.type, schemaDef.address, true);
}

function setAttributeStyleFrom(element, json)
{
   var styles = "";

   for (var key in json)
      styles += key + ":" + json[key] + ";";

   element.setAttribute("style", styles);
   element.style.position = "absolute";
}

function editSchemaValue(type, address, newUC)
{
   // console.log("editSchemaValue of id: " + id);

   var key = toKey(type, address);
   var id = type + ((address)>>>0).toString(10);

   var schemaDef = getSchemDef(id);
   var isUC = schemaDef.type == "UC";
   var isCN = schemaDef.type == "CN";
   var item = allSensors[key];

   var form =
       '<form><div id="settingsForm" style="display:grid;min-width:650px;max-width:750px;">' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="align-self:center;width:120px;">Einblenden:</span><span><input id="showIt" style="width:auto;" type="checkbox"' + (schemaDef.state == "A" ? "checked" : "") + '/><label for="showIt"></label></span></div>' +
       ' <div style="display:flex;margin:4px;text-align:left;">' +
	   '   <span style="width:120px;">Farbe:</span><span><input id="colorFg" type="text" value="' + (schemaDef.properties["color"] || "white") + '"/></span>' +
       '   <span style="align-self:center;width:120px;text-align:right;">Hintergrund: </span><span><input id="colorBg" type="text" value="' + (schemaDef.properties["background-color"] || "transparent") + '"/></span>' +
       ' </div>' +
       ' <div style="display:flex;margin:4px;text-align:left;">' +
       '   <span style="align-self:center;width:120px;">Rahmen:</span><span><input id="showBorder" style="width:auto;" type="checkbox"' + (schemaDef.properties["border"] != "none" ? "checked" : "") + '/><label for="showBorder"></label></span></span>' +
       '   <span style="align-self:center;width:120px;text-align:right;">Radius: </span><span><input id="borderRadius" style="width:60px;" type="text" value="' + (schemaDef.properties["border-radius"] || "3px") + '"/></span>' +
       ' </div>';
   if (!isCN){
      form +=
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="align-self:center;width:120px;">Schriftgröße:</span><span><input id="fontSize" style="width:inherit;" type="text" value="' + (schemaDef.properties["font-size"] || "16px") + '"/></span></div>' ;
   }
      form +=
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="align-self:center;width:120px;">Layer:</span><span><input id="zIndex" style="width:inherit;" type="number" step="1" value="' + (schemaDef.properties["z-index"] || "100") + '"/></span></div>';
   if (!isUC && !isCN) {
      form +=
         ' <div style="display:flex;margin:4px;text-align:left;"><span style="align-self:center;width:120px;">Bezeichnung:</span><span><input id="showTitle" style="width:auto;" type="checkbox"' + (schemaDef.showtitle ? "checked" : "") + '/></span><label for="showTitle"></label></span></div>' +
         ' <div style="display:flex;margin:4px;text-align:left;"><span style="align-self:center;width:120px;">Einheit:</span><span><input id="showUnit" style="width:auto;" type="checkbox"' + (schemaDef.showunit ? "checked" : "") + '/><label for="showUnit"></label></span></span></div>';
   }
   if (isCN){
   form +=
      ' <div style="display:flex;margin:4px;text-align:left;">' +
      '  <span style="width:120px;">Type:</span>' +
      '  <span style="">links->rechts</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(3)" value="3" type="radio" name="kind" ' + (schemaDef.kind == 3 ? "checked" : "") + '/></span>' +
      '  <span style="">rechts->links</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(4)" value="4" type="radio" name="kind" ' + (schemaDef.kind == 4 ? "checked" : "") + '/></span>' +
      '  <span style="">oben->unten</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(5)" value="5" type="radio" name="kind" ' + (schemaDef.kind == 5 ? "checked" : "") + '/></span>' +
      '  <span style="">unten->oben</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(6)" value="6" type="radio" name="kind" ' + (schemaDef.kind == 6 ? "checked" : "") + '/></span>' +
      ' </div>';
 }
   else{
   form +=
      ' <div style="display:flex;margin:4px;text-align:left;">' +
      '  <span style="width:120px;">Type:</span>' +
      '  <span style="">Wert:</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(0)" value="0" type="radio" name="kind" ' + (schemaDef.kind == 0 ? "checked" : "") + '/></span>' +
      '  <span style="">Text:</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(1)" value="1" type="radio" name="kind" ' + (schemaDef.kind == 1 ? "checked" : "") + '/></span>' +
      '  <span style="">Bild:</span> <span><input style="margin-right:20px;margin-left:7px;width:auto;" onclick="selectKindClick(2)" value="2" type="radio" name="kind" ' + (schemaDef.kind == 2 ? "checked" : "") + '/></span>' +
      ' </div>';
      }
   if (!isCN){
      form +=
         ' <div id="imgSize" style="display:flex;margin:4px;text-align:left;">' +
         '  <span style="align-self:center;width:120px;">Bild Breite:</span><span><input id="imgWidth" style="width:60px;" type="number" value="' + schemaDef.width + '"/></span>' +
         '  <span style="align-self:center;width:120px;text-align:right;">Bild Höhe: </span><span><input id="imgHeight" style="width:60px;" type="number" value="' + schemaDef.height + '"/></span>' +
         ' </div>';

      form += ' <div id="imgInfo" style="display:flex;margin:4px;text-align:left;"></span><span style="font-size: smaller;">Um das vom p4d gelieferte Bild zu verwenden (' + (item != null ? item.image : "") + ') Funktion und URL leer lassen!</span></div>';

      form += ' <div style="display:flex;margin:4px;text-align:left;">' +
         '  <span id="labelUsrText" style="width:120px;">xxx:</span>' +
         '  <span style="width:-webkit-fill-available;width:-moz-available;">' +
         '    <textarea id="usrText" style="width:inherit;height:22px;resize:vertical;">' + (schemaDef.usrtext || "") + '    </textarea>' +
         '  </span>' +
         '</div>';

      form += ' <div style="display:flex;margin:4px;text-align:left;">' +
         '  <span id="labelFunction" style="width:120px;">Funktion:</span>' +
         '  <span style="width:-webkit-fill-available;width:-moz-available;">' +
         '    <textarea id="function" style="width:inherit;height:120px;resize:vertical;font-family:monospace;">' + (schemaDef.fct != null ? schemaDef.fct : "") + '</textarea>' +
         '  </span>' +
         '</div>';
   }
   if (!isCN)
   {
   if (!isUC)
      form += ' <div style="display:flex;margin:4px;text-align:left;"></span><span style="font-size: smaller;">JavaScript Funktion zum berechnen des angezeigten Wertes. Auf die Daten des Elements kann über item.value, item.unit und item.text zugegriffen werden. Andere Elemente erhält man mit getItem(id)</span></div>';
   else
      form += ' <div style="display:flex;margin:4px;text-align:left;"></span><span style="font-size: smaller;">JavaScript Funktion zum berechnen des angezeigten Wertes. Elemente erhält man mit getItem(id)</span></div>';
   }
   form += '</div></form>';

   var title = isUC ? "User Constant" : ((item != null && item.title) || "");
   var title = isCN ? "Connection" : ((item != null && item.title) || "");

   var buttons = {
      'Ok': function () {
         if (apply("#"+id) == 0)
            $(this).dialog('close');
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
      title: title + " (" + schemaDef.type + ":0x" + ((schemaDef.address)>>>0).toString(16) + ")",
      width: "auto",
      buttons: buttons,
      open: function() {
         $('#colorFg').spectrum({
            type : "color",
            showPalette: true,
            showSelectionPalette: true,
            palette: [ ],
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
            palette: [ ],
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

   window.selectKindClick = function(kind) { selectKind(kind); }

   function selectKind(kind) {
      console.log("kind: " + kind);
      switch (kind) {
        case 0:
           $('#labelUsrText').html(isUC ? "Text:" : "Zusatz Text:");
           $('#labelFunction').html("Funktion:");
           $('#imgSize').css({'visibility' : "hidden"});
           $('#imgInfo').css({'visibility' : "hidden"});
           break;
        case 1:
           $('#labelUsrText').html(isUC ? "Text:" : "Zusatz Text:");
           $('#labelFunction').html("Funktion:");
           $('#imgSize').css({'visibility' : "hidden"});
           $('#imgInfo').css({'visibility' : "hidden"});
           break;
        case 2:
           $('#labelUsrText').html("Image URL:");
           $('#labelFunction').html("Funktion Image URL:");
           $('#imgSize').css({'visibility' : "visible"});
           $('#imgInfo').css({'visibility' : "visible"});
           break;
        case 3:
          // $('#labelUsrText').html("Image URL:");
          // $('#labelFunction').html("Funktion Image URL:");
          // $('#imgSize').css({'visibility' : "visible"});
          // $('#imgInfo').css({'visibility' : "visible"});
           break;
        case 4:
           break;
        case 5:
           break;
        case 6:
           break;

      }
   }

   function deleteUC(id) {
      for (var i = 0; i < lastSchemadata.length; i++) {
         if (lastSchemadata[i].type + ((lastSchemadata[i].address)>>>0).toString(10) == id) {
            lastSchemadata[i].deleted = true;
            $("#"+id).remove();
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
         var result = eval($('#function').val());
      }
      catch (err) {
         showInfoDialog({'message' : "Fehler in JS Funktion: '" + err.message + ", " + $('#function').val() + "'", 'status': -1});
         return -1;  // fail
      }

      schemaDef.fct = $('#function').val();

      setAttributeStyleFrom($('#schemaContainer')[0], schemaDef.properties);
      updateSchema()

      return 0;  // success
   }
}

function dragSValue(ev, id)
{
   ev.dataTransfer.setData("id", id);
}

function allowDropSValue(ev)
{
   ev.preventDefault();
}

function dropSValue(ev)
{
   ev.preventDefault();
   var id = "#" + ev.dataTransfer.getData("id");
   var schemaDef = getSchemDef(ev.dataTransfer.getData("id"));

   schemaDef.properties["top"] = (ev.offsetY - oTop)  + 'px';
   schemaDef.properties["left"] = (ev.offsetX - oLeft)  + 'px';
   //schemaDef.properties['width'] = element.style.width + 'px';
   //schemaDef.properties['height'] = element.style.height + 'px';
   $(id).css({'top'  : (ev.offsetY - oTop)  + 'px'});
   $(id).css({'left' : (ev.offsetX - oLeft) + 'px'});
}
