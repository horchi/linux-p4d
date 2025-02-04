/*
 *  commands.js
 *
 *  (c) 2020-2021 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initCommands()
{
   $('#container').removeClass('hidden');
   $('#container').html('');

   prepareSetupMenu();

   $('#container').append($('<div></div>')
                          .attr('id', 'inputTableConfig')
                          .addClass('rounded-border inputTableConfig'));

   for (var i = 0; i < commands.length; i++) {
      var command = commands[i];
      $('#inputTableConfig').append($('<div></div>')
                                    .css('display', 'block')
                                    .css('margin', '4px')
                                    .append($('<span></span>')
                                            .append($('<button></button>')
                                                    .css('width', '20%')
                                                    .addClass('rounded-border buttonOptions')
                                                    .html(command.title)
                                                    .click({'cmd' : command.cmd}, function(event) {
                                                       console.log("send command with " + event.data.cmd);
                                                       socket.send({ "event" : "command", "object" : { "what" : event.data.cmd } });
                                                    })
                                                   ))
                                    .append($('<span></span>')
                                            .css('margin-left', '20px')
                                            .html(' -> ' + command.description))
                                   );
   }

   $("#container").height($(window).height() - $("#menu").height() - 8);
   window.onresize = function() {
      $("#container").height($(window).height() - $("#menu").height() - 8);
   };
}
