/*
 *  images.js
 *
 *  (c) 2020 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initImages()
{
   // console.log("Images: " + JSON.stringify(images, undefined, 4));

   $('#container').removeClass('hidden');
   document.getElementById("container").innerHTML =
      '<div class="rounded-border seperatorFold">Images</div>' +
      '  <table class="setupContainer tableMultiCol">' +
      '    <thead>' +
      '      <tr>' +
      '        <td style="width:90px;">Image</td>' +
      '        <td>Path</td>' +
      '        <td></td>' +
      '      </tr>' +
      '    </thead>' +
      '    <tbody id="images">' +
      '    </tbody>' +
      '  </table>' +
      '</div><br/>' +
      '<div id="addImageDiv" class="rounded-border inputTableConfig">' +
      '  <button class="buttonOptions rounded-border" onclick="selectImage()">Bild auwählen</button>' +
      '<br/><br/>' +
      '  <img id="previewImg" style="height:100px;width:100px;margin-right:30px;"\>' +
      '  <br/><br/><span">Upload Name:   (ohne Endung!)</span>' +
      '  <br/><input id="uploadName" class="rounded-border input" title="Namen ohne Endung!"\>' +
      '  <button class="buttonOptions rounded-border" onclick="uploadImage()">-&gt; hochladen</button>' +
      '</div>';

   tableRoot = document.getElementById("images");
   tableRoot.innerHTML = "";

   for (var img in images) {
      if (images[img] == '')
         continue;
      if (images[img].indexOf('img/user/') != 0)
         continue;

      var html = '<td><img class="tableImage" src="' + images[img] + '"></img></td>';
      html += '<td class="tableMultiColCell">' + images[img] + '</td>';
      html += '<td><button class="rounded-border" style="margin-right:10px;" onclick="delImage(\'' + images[img] + '\')">Löschen</button></td>';

      var elem = document.createElement("tr");
      elem.innerHTML = html;
      tableRoot.appendChild(elem);
   }

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - 10);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - 10);
   };
}

function delImage(path, action)
{
   // console.log("delImage(" + id + ")");

   // #TODO to be implemented

   if (confirm("Image löschen?"))
      socket.send({ "event": "imageconfig", "object":
                    { "path": path,
                      "action": "delete" }});
}

function selectImage()
{
   var input = document.createElement('input');
	input.type = 'file';
   input.accept = 'image/*';

   input.onchange = e => {
      var reader = new FileReader();
      reader.onload = function(){
         var output = document.getElementById('previewImg');
         output.src = reader.result;
      };
      reader.readAsDataURL(e.target.files[0]);
   }

   input.click();
}

function uploadImage()
{
   if ($('#uploadName').val() == '')
      return ;

   var output = document.getElementById('previewImg');

   console.log("upload file: ", $('#uploadName').val());
   socket.send({ 'event': 'imageconfig', 'object':
                 { 'action': 'upload',
                   'name': $('#uploadName').val(),
                   'data' : output.src }});
}
