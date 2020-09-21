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
var modeEdit = false;
var lastSchemadata = null;
var lastData = null;
var properties = [ "top", "left", "color", "font-size", "color", "background-color", "border", "border-radius"];
var schemaRoot = null;

function getSchemDef(id)
{
   for (var i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type + lastSchemadata[i].address == id)
         return lastSchemadata[i];
   }
}

function getItemById(id)
{
   for (var i = 0; i < lastData.length; i++) {
      if (lastData[i].type + lastData[i].address == id)
         return lastData[i];
   }
}

// get data item by '<Type>:0x<HexAddr>' notation !

function getItem(id)
{
   for (var i = 0; i < lastData.length; i++) {
      if (lastData[i].type + ':0x' + lastData[i].address.toString(16).toUpperCase() == id)
         return lastData[i];
   }
}

// get data item definition by '<Type>:0x<HexAddr>' notation !

function getItemDef(id)
{
   for (var i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type + ':0x' + lastSchemadata[i].address.toString(16).toUpperCase() == id)
         return lastSchemadata[i];
   }
}

function initSchema(schemaData, root)
{
   // console.log(JSON.stringify(schemaData, undefined, 4));

   schemaRoot = root;
   lastSchemadata = schemaData;
   root.innerHTML = "";

   var image = document.createElement("img");
   image.setAttribute("src", "img/schema/schema-" + config.schema + ".png");

   if (modeEdit) {
      image.setAttribute("ondrop", "dropSValue(event)");
      image.setAttribute("ondragover", "allowDropSValue(event)");
   }

   root.appendChild(image);

   for (var i = 0; i < schemaData.length; i++) {
      initSchemaElement(schemaData[i]);
   }
}
function initSchemaElement(item)
{
   var id = item.type + item.address;

   var button = document.createElement("button");
   button.classList.add("buttonSchemaValue");
   button.setAttribute("id", id);
   button.setAttribute("data-div", "div" + id);
   button.style.cursor = modeEdit ? "move" : "default";

   if (modeEdit)
      button.setAttribute("onclick", 'editSchemaValue("' + id + '")');

   var div = document.createElement("div");

   div.classList.add("schemaDiv");
   div.setAttribute("id", "div" + item.type + item.address);
   div.setAttribute('data-type', item.type);
   div.setAttribute('data-address', item.address);

   setAttributeStyleFrom(div, item.properties);
   div.style.visibility = (modeEdit || item.state == 'A') ? "visible" : "hidden";

   div.appendChild(button);

   if (modeEdit) {
      div.setAttribute("draggable", true);
      div.setAttribute("ondragstart", 'dragSValue(event,"' + item.type + item.address + '")');

      div.addEventListener('mousedown', event => {
         oTop = event.offsetY;
         oLeft = event.offsetX;
      });

      div.addEventListener("keydown", event => {
         var ele = event.currentTarget;
         var top = parseInt(ele.style.top);
         var left = parseInt(ele.style.left);
         // console.log(" id: " + ele.id.substr(3));
         var id = ele.id.substr(3);
         var schemaDef = getSchemDef(id);

         switch (event.keyCode)
         {
            case 37: left--; break;
            case 38: top--;  break;
            case 39: left++; break;
            case 40: top++;  break;
         }

         schemaDef.properties["top"] = top + 'px';
         schemaDef.properties["left"] = left + 'px';
         ele.style.top = top + 'px';
         ele.style.left = left + 'px';
      });
   }

   schemaRoot.appendChild(div);
}

function updateSchema(data, root)
{
   if (data.length == 0)
      return ;

   lastData = data;

   for (var i = 0; i < lastSchemadata.length; i++) {
      var schemaDef = lastSchemadata[i];
      // var item = data[i];
      var id = schemaDef.type + schemaDef.address;
      var item = getItemById(id);
      // var schemaDef = getSchemDef(id);

      if (schemaDef == null)
         continue;

      var html = "";

      if (schemaDef.showtitle && item != null && item.title != null)
         html += item.title + ":&nbsp;";

      if (schemaDef.usrtext != null)
         html += schemaDef.usrtext;

      if (schemaDef.fct != null && schemaDef.fct != "") {
         try {
            html += eval(schemaDef.fct);
         }
         catch (err) {
            showInfoDialog("Fehler in JS Funktion: '" + err.message + "'", "Error in '" + schemaDef.fct + "'");
            schemaDef.fct = "";  // delete buggy function
         }
         // console.log("result of '" + schemaDef.fct + "' is " + result);
      }
      else if (item != null) {
         html += schemaDef.showtext ? item.text || item.value : item.value;
         html += schemaDef.showunit ? item.unit || "" : "";
      }

      $("#"+id).html(html);

      var title = "";
      if (schemaDef.type == "UC")
         title = "User Constant";
      else (item = null)
         title = ((item != null && item.title) || "");

      $("#"+id).attr("title", title + " (" + schemaDef.type + ":" + schemaDef.address + ")");
   }
}

function schemaEditModeToggle()
{
   modeEdit = !modeEdit;
   initSchema(lastSchemadata, schemaRoot);
   updateSchema(lastData, schemaRoot);

   document.getElementById("buttonSchemaStore").style.visibility = modeEdit ? 'visible' : 'hidden';
   document.getElementById("buttonSchemaAddItem").style.visibility = modeEdit ? 'visible' : 'hidden';
}

function schemaStore()
{
   socket.send({ "event" : "storeschema", "object" : lastSchemadata });
}

function schemaAddItem()
{
   var addr = 0;
   for (var i = 0; i < lastSchemadata.length; i++) {
      if (lastSchemadata[i].type == "UC" && lastSchemadata[i].address == addr)
         addr++;
   }
   schemaDef = { type: "UC",  // User Defined
                 address: addr,
                 state: "A",
                 properties: {},
                 showtext: 1,
                 showtitle: 1,
                 showunit: 0 };
   var id = schemaDef.type + schemaDef.address;
   lastSchemadata[lastSchemadata.length] = schemaDef;
   initSchemaElement(schemaDef);

   editSchemaValue(id);
}

function setAttributeStyleFrom(element, json)
{
   var styles = "";

   for (var key in json)
      styles += key + ":" + json[key] + ";";

   element.setAttribute("style", styles);
   element.style.position = "absolute";
}

function editSchemaValue(id)
{
   // console.log("editSchemaValue of id: " + id);

   var item = null;
   var schemaDef = getSchemDef(id);

   for (var i = 0; i < lastData.length; i++) {
      if (lastData[i].type + lastData[i].address == id) {
         item = lastData[i];
         break;
      }
   }

   var form =
       '<form><div style="display:grid;">' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Einblenden:</span><span><input id="showIt" style="width:auto;" type="checkbox"' + (schemaDef.state == "A" ? "checked" : "") + '/></span></div>' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Farbe:</span><span><input id="colorFg" value="' + (schemaDef.properties["color"] || "white") + '"/></span></div>' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Hintergrund:</span><span><input id="colorBg" value="' + (schemaDef.properties["background-color"] || "transparent") + '"/></span></div>' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Rahmen:</span><span><input id="showBorder" style="width:auto;" type="checkbox"' + (schemaDef.properties["border"] != "none" ? "checked" : "") + '/></span></div>' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Rahmen Radius:</span><span><input id="borderRadius" style="width:inherit;" type="text" value="' + (schemaDef.properties["border-radius"] || "3px") + '"/></span></div>' +
       ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Schriftgröße:</span><span><input id="fontSize" style="width:inherit;" type="text" value="' + (schemaDef.properties["font-size"] || "16px") + '"/></span></div>';

   if (schemaDef.type != "UC") {
      form +=
         ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Bezeichnung:</span><span><input id="showTitle" style="width:auto;" type="checkbox"' + (schemaDef.showtitle ? "checked" : "") + '/></span></div>' +
         ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Zusatz Text:</span><span><input id="usrText" style="width:inherit;" type="text" value="' + (schemaDef.usrtext || "") + '"/></span><span style="font-size:small;align-self:center;">&nbsp;(html syntax)</span></div>' +
         ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Text:</span><span><input id="showText" style="width:auto;" type="checkbox"' + (schemaDef.showtext ? "checked" : "") + '/></span></div>' +
         ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Einheit:</span><span><input id="showUnit" style="width:auto;" type="checkbox"' + (schemaDef.showunit ? "checked" : "") + '/></span></div>';
   }
   else {
      form += ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Text:</span><span><textarea id="usrText" style="width:inherit;height:inherit;">' + (schemaDef.usrtext || "") + '</textarea></span><span style="font-size:small;align-self:center;">&nbsp;(html syntax)</span></div>';
   }

   form += ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;">Funktion:</span><span style="width:450px;height:95px;"><textarea id="function" style="width:inherit;height:inherit;">' + (schemaDef.fct != null ? schemaDef.fct : "") + '</textarea></span></div>';

   if (schemaDef.type != "UC")
      form += ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;"></span><span style="font-size: smaller;">JavaScript Funktion zum berechnen des angezeigten Wertes. Auf die aktuellen Daten kann über item.value, item.unit und item.text zugegriffen werden.</span></div>';
   else
      form += ' <div style="display:flex;margin:4px;text-align:left;"><span style="width:120px;"></span><span style="font-size: smaller;">JavaScript Funktion zum berechnen des angezeigten Wertes</span></div>';

   form += '</div></form>';

   var title = "";
   if (schemaDef.type == "UC")
      title = "User Constant";
   else
      title = ((item != null && item.title) || "");

   $(form).dialog({
      modal: true,
      title: title + " (" + schemaDef.type + ":0x" + schemaDef.address.toString(16) + ")",
      width: "40%",
      buttons: {
         'Speichern': function () {
            if (apply("#div"+id) == 0)
               $(this).dialog('close');
         },
         'Abbrechen': function () {
            $(this).dialog('close');
         }
      },
      open: function() {
         $('#colorFg').spectrum({
            type : "color",
            hideAfterPaletteSelect : "true",
            showInput : "true",
            showButtons : "false",
            allowEmpty : "false"
         });
         $('#colorBg').spectrum({
            type : "color",
            hideAfterPaletteSelect : "true",
            showInput : "true",
            showButtons : "false",
            allowEmpty : "false"
         });

      },
      close: function() { $(this).dialog('destroy').remove(); }
   });

   function apply(id) {
      schemaDef.properties["color"] = $('#colorFg').spectrum("get").toRgbString();
      schemaDef.properties["background-color"] = $('#colorBg').spectrum("get").toRgbString();
      schemaDef.properties["border"] = $('#showBorder').is(":checked") ? "" : "none";
      schemaDef.properties["border-radius"] = $('#borderRadius').val();
      schemaDef.properties["font-size"] = $('#fontSize').val();
      schemaDef.usrtext = $('#usrText').val();
      schemaDef.showtext = $('#showText').is(":checked") ? 1 : 0;
      schemaDef.showtitle = $('#showTitle').is(":checked") ? 1 : 0;
      schemaDef.showunit = $('#showUnit').is(":checked") ? 1 : 0;
      schemaDef.state = $('#showIt').is(":checked") ? "A": "D";


      try {
         var result = eval($('#function').val());
      }
      catch (err) {
         showInfoDialog("Fehler in JS Funktion: '" + err.message + "'", "Error in '" + $('#function').val() + "'");
         return -1;
      }

      schemaDef.fct = $('#function').val();

      setAttributeStyleFrom(schemaRoot.querySelector(id), schemaDef.properties);
      updateSchema(lastData, schemaRoot)

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
   var id = "#div" + ev.dataTransfer.getData("id");
   var schemaDef = getSchemDef(ev.dataTransfer.getData("id"));

   schemaDef.properties["top"] = (ev.offsetY - oTop)  + 'px';
   schemaDef.properties["left"] = (ev.offsetX - oLeft)  + 'px';
   $(id).css({'top'  : (ev.offsetY - oTop)  + 'px'});
   $(id).css({'left' : (ev.offsetX - oLeft) + 'px'});
}
