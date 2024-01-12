/*
 *  vdr.js
 *
 *  (c) 2020-2023 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initVdr()
{
   $('#container').removeClass('hidden');

//   if ($(window).height() < $(window).width())
//      console.log("horizontally !!");

   document.getElementById("container").innerHTML =
      '<div class="vdrContent">' +
      '  <div id="vdrFbContainer" class="vdrFbContainer">' +
      '    <div>' +
      '       <div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'1\')">1</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'2\')">2</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'3\')">3</button></div>' +
      '       </div>' +
      '       <div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'4\')">4</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'5\')">5</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'6\')">6</button></div>' +
      '       </div>' +
      '       <div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'7\')">7</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'8\')">8</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton" type="button" onclick="vdrKeyPress(\'9\')">9</button></div>' +
      '       </div>' +
      '    </div>' +
      '    <div>' +
      '       <div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'menu\')">&#9776;</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonUp" type="button" onclick="vdrKeyPress(\'up\')">&uparrow;</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'back\')">↵</button></div>' +
      '       </div>' +
      '       <div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonLeft" type="button" onclick="vdrKeyPress(\'left\')">&leftarrow;</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'ok\')">ok</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonRight" type="button" onclick="vdrKeyPress(\'right\')">&rightarrow;</button></div>' +
      '       </div>' +
      '       <div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'mute\')">🔇</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrArrowButton vdrArrowButtonDown" type="button" onclick="vdrKeyPress(\'down\')">&downarrow;</button></div>' +
      '         <div class="vdrButtonDiv"><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'info\')">ℹ</button></div>' +
      '       </div>' +
      '    </div>' +
      '    <div>' +
      '       <div>' +
      '         <div><button class="vdrButton vdrButtonRound glyphicon glyphicon-volume-up" type="button" onclick="vdrKeyPress(\'volume-\')">🔉</button></div>' +
      '         <div><button class="vdrButton" type="button" onclick="vdrKeyPress(\'0\')">0</button></div>' +
      '         <div><button class="vdrButton vdrButtonRound" type="button" onclick="vdrKeyPress(\'volume+\')">🔊</button></div>' +
      '       </div>' +
      '       <div>' +
      '         <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonRed" type="button" onclick="vdrKeyPress(\'red\')"></button></div>' +
      '         <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonGreen" type="button" onclick="vdrKeyPress(\'green\')"></button></div>' +
      '         <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonYellow" type="button" onclick="vdrKeyPress(\'yellow\')"></button></div>' +
      '         <div class="vdrButtonColorDiv"><button class="vdrButton vdrColorButtonBlue" type="button" onclick="vdrKeyPress(\'blue\')"></button></div>' +
      '       </div>' +
      '    </div>' +
      '  </div>' +  // vdrFbContainer
      '</div>';

      // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - 15);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('dashboardMenu') - 15);
   };
}

function vdrKeyPress(key)
{
   if (key == undefined || key == "")
      return;

   socket.send({ "event": "keypress",
                 "object": { "key": key, "repeat": 1 }
               });
}
