/*
 *  vdr.js
 *
 *  (c) 2020-2021 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initVdr()
{
   $('#container').removeClass('hidden');

   document.getElementById("container").innerHTML =
      '<div class="vdrContent">' +
      '  <div id="vdrFbContainer" class="vdrFbContainer">' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'1\')">1</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'2\')">2</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'3\')">3</button></div>' +
      '    <br/>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'4\')">4</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'5\')">5</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'6\')">6</button></div>' +
      '    <br/>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'7\')">7</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'8\')">8</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'9\')">9</button></div>' +

      '    <br/>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound glyphicon glyphicon-volume-up" type="button" onclick="vdrKeyPress(\'volume-\')">🔉</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'\0\')">0</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'volume+\')">🔊</button></div>' +

      '    <br/>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'menu\')">&#9776;</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonUp" type="button" onclick="vdrKeyPress(\'up\')">&uparrow;</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'back\')">↵</button></div>' +
      '    <br/>' +

      '    <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonLeft" type="button" onclick="vdrKeyPress(\'left\')">&leftarrow;</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'ok\')">ok</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonRight" type="button" onclick="vdrKeyPress(\'right\')">&rightarrow;</button></div>' +
      '    <br/>' +

      '    <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'mute\')">🔇</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonDown" type="button" onclick="vdrKeyPress(\'down\')">&downarrow;</button></div>' +
      '    <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'info\')">ℹ</button></div>' +
      '    <br/>' +

      '    <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonRed" type="button" onclick="vdrKeyPress(\'red\')"></button></div>' +
      '    <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonGreen" type="button" onclick="vdrKeyPress(\'green\')"></button></div>' +
      '    <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonYellow" type="button" onclick="vdrKeyPress(\'yellow\')"></button></div>' +
      '    <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonBlue" type="button" onclick="vdrKeyPress(\'blue\')"></button></div>' +
      '    <br/>' +
      '  </div>' +
      '</div>';

   var root = document.getElementById("vdrFbContainer");
   var elements = root.getElementsByClassName("vdrButtonDiv");

   for (var i = 0; i < elements.length; i++) {
      elements[i].style.height = getComputedStyle(elements[i]).width;
      if (elements[i].children[0].innerHTML == '')
         elements[i].style.visibility = 'hidden';
   }
}

function vdrKeyPress(key)
{
   if (key == undefined || key == "")
      return;

   socket.send({ "event": "keypress", "object":
                 { "key": key,
                   "repeat": 1 }
               });
}
