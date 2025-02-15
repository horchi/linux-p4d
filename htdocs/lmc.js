/*
 *  lmc.js
 *
 *  (c) 2020-2024 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initLmc()
{
   $('#container').removeClass('hidden');
   $('#container').empty();

   // console.log("initLmc " + JSON.stringify(lmcData, undefined, 4));

   $('#container')
      .append($('<div></div>')
              .attr('id', 'lmcContainer')
              .addClass('lmcContainer')
              .append($('<div></div>')
                      .addClass('lmcLeftColumn lmcMenu hidden')
                      .attr('id', 'lmcLeftColumnMenu')
                     )
              .append($('<div></div>')
                      .addClass('lmcLeftColumn')
                      .attr('id', 'lmcLeftColumn')
                      .append($('<div></div>')
                              .addClass('lmcCurrentTrack')
                              .append($('<div></div>')
                                      .attr('id', 'lmcCurrentTitle')
                                      .addClass('lmcCurrentTitle'))
                              .append($('<div></div>')
                                      .attr('id', 'lmcCurrentArtist')
                                      .addClass('lmcCurrentArtist'))
                              .append($('<div></div>')
                                      .css('margin-top', '8px')
                                      .append($('<span></span>')
                                              .html('Album: '))
                                      .append($('<span></span>')
                                              .attr('id', 'lmcCurrentAlbum')))
                              .append($('<div></div>')
                                      .append($('<span></span>')
                                              .attr('id', 'lmcCurrentGenreYear')))
                              .append($('<div></div>')
                                      .append($('<span></span>')
                                              .html('Stream: '))
                                      .append($('<span></span>')
                                              .attr('id', 'lmcCurrenStream')))
                              .append($('<div></div>')
                                      .addClass('lmcTrackProgress')
                                      .attr('id', 'lmcTrackProgress')
                                      .click(function(event) {
                                         let relX = event.pageX - $(this).offset().left;
                                         let percent = parseInt(relX / ($(this).width()/100));
                                         // console.log("click position", percent, '%');
                                         socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : 'time', 'percent' : percent} });
                                      })
                                      .append($('<div></div>')
                                              .attr('id', 'lmcTrackProgressBar')
                                              .addClass('lmcTrackProgressBar')))
                              .append($('<div></div>')
                                      .append($('<span></span>')
                                              .css('float', 'left')
                                              .attr('id', 'lmcTrackTime'))
                                      .append($('<span></span>')
                                              .css('float', 'right')
                                              .attr('id', 'lmcTrackDuration')))
                             )
                      .append($('<div></div>')
                              .addClass('lmcWidget')
                              .css('display', 'flex')
                              .css('justify-content', 'center')
                              .append($('<div></div>')
                                      .attr('id', 'lmcCover')
                                      .addClass('lmcCover ')
                                      .append($('<img></img>')
                                              .attr('id', 'lmcCoverImage'))
                                     )))
              .append($('<div></div>')
                      .addClass('lmcRightColumn')
                      .attr('id', 'lmcRightColumn')
                      .append($('<div></div>')
                              .addClass('lmcControl')
                              .append($('<button></button>')
                                      .attr('id', 'lmcBtnBurgerMenu')
                                      .addClass('mdi mdi-menu rounded-border lmcButton')
                                      .click(function() { onMenu(); }))
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcPlay')
                                      .addClass('mdi mdi-play-circle rounded-border lmcButton')
                                      .click(function() { lmcAction('play'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcPause')
                                      .addClass('mdi mdi-pause-circle rounded-border lmcButton')
                                      .click(function() { lmcAction('pause'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcStop')
                                      .addClass('mdi mdi-stop-circle rounded-border lmcButton')
                                      .click(function() { lmcAction('stop'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcPrev')
                                      .addClass('mdi mdi-arrow-left-circle rounded-border lmcButton')
                                      .click(function() {  lmcAction('prevTrack'); })
                                     )
                              .append($('<button></button>')
                                      .addClass('mdi mdi-arrow-right-circle rounded-border lmcButton')
                                      .click(function() {  lmcAction('nextTrack'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcRepeat')
                                      .addClass('mdi mdi-repeat rounded-border lmcButton')
                                      .click(function() {  lmcAction('repeat'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcShuffle')
                                      .addClass('mdi mdi-shuffle-variant rounded-border lmcButton')
                                      .click(function() {  lmcAction('shuffle'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcRandonTracks')
                                      .addClass('mdi mdi-dice-multiple rounded-border lmcButton')
                                      .click(function() {  lmcAction('randomTracks'); })
                                     )
                              .append($('<button></button>')
                                      .css('width', '30px')
                                      .addClass('rounded-border lmcButton')
                                     )
                              .append($('<div></div>')
                                      .css('display', 'flex')
                                      .append($('<button></button>')
                                              .addClass('mdi mdi-volume-medium rounded-border lmcButton')
                                              .click(function() { lmcAction('volumeDown'); })
                                             )
                                      .append($('<span></span>')
                                              .attr('id', 'infoVolume')
                                             )
                                      .append($('<button></button>')
                                              .addClass('mdi mdi-volume-high rounded-border lmcButton')
                                              .click(function() { lmcAction('volumeUp'); })
                                             )
                                      .append($('<button></button>')
                                              .attr('id', 'btnLmcMute')
                                              .addClass('mdi mdi-volume-off rounded-border lmcButton')
                                              .click(function() {  lmcAction('muteToggle'); })
                                             ))
                              .append($('<div></div>')
                                      .css('flex-grow', '1'))
                              .append($('<select></select>')
                                      .attr('id', 'playerSelect')
                                      .css('height', '30px')
                                      .css('align-self', 'center')
                                      .css('background-color', 'transparent')
                                      .css('color', 'white')
                                      .css('border-radius', '5px')
                                      .on('change', function() {
                                         socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : 'changePlayer', 'mac' : $(this).val()} });
                                      }))
                             )
                      .append($('<div></div>')
                              .attr('id', 'lmcPlaylist')
                              .addClass('lmcPlaylist')))
             );

   updateLmc();

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('footer') - sab - 5);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - getTotalHeightOf('footer') - sab - 5);
   };
}

function lmcAction(action)
{
   socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : action } });
}

var lastTrackTime = 0;
var lastTrackTimeAt = 0;
var progressTrigger = 0;

function updateLmc()
{
   console.log("updateLmc lmcData.state =", JSON.stringify(lmcData.state, undefined, 4));

   if (!lmcData || !lmcData.current || !lmcData.state) {
      clearProgressTrigger();
      return;
   }

   $('#playerSelect').empty();
   let playerFound = false;

   for (var i = 0; i < lmcData.players.length; i++) {
      $('#playerSelect').append($('<option></option>')
                                .html(lmcData.players[i].name)
                                .val(lmcData.players[i].mac)
                                .attr('selected', lmcData.players[i].iscurrent)
                               );
      if (lmcData.players[i].iscurrent)
         playerFound = true;
   }

   // $('#playerSelect').append($('<option>xx</option>')
   //                           .attr('selected', !playerFound));

   if (lmcData.state.mode != "play")
      clearProgressTrigger();

   $('#lmcCurrentArtist').html(lmcData.current.artist);
   $('#lmcCurrentTitle').html(lmcData.current.title);
   $('#lmcCurrentAlbum').html(lmcData.current.album);
   $('#infoVolume').html(lmcData.state.volume + '%');

   $('#lmcCurrentGenreYear').html(lmcData.current.genre + ' / ' + lmcData.current.year);
   // $('#lmcCurrentYear').html(lmcData.current.year);
   $('#lmcCurrenStream').html(lmcData.current.bitrate + ' kb/s, ' + (lmcData.current.contentType == 'flc'? 'flac' : lmcData.current.contentType));

   lastTrackTime = lmcData.state.trackTime;
   lastTrackTimeAt = lNow();
   calcProgress();

   $('#lmcCoverImage').attr('src', lmcData.current.cover);

   if (lmcData.state.mode == 'play' && !progressTrigger) {
      // console.log("starting progress timer!");
      progressTrigger = setInterval(function() {
         calcProgress();
      }, 1000);
   }

   updateButtons();
   updatePlaylist();
}

function updateButtons()
{
   $("[id^='btnLmc']").css('color', 'white');

   if (lmcData.state.mode == 'play')
      $('#btnLmcPlay').css('color', 'var(--yellow2)');
   else if (lmcData.state.mode == 'pause')
      $('#btnLmcPause').css('color', 'var(--yellow2)');
   else if (lmcData.state.mode == 'stop')
      $('#btnLmcStop').css('color', 'var(--yellow2)');

   if (lmcData.state.plRepeat)
      $('#btnLmcRepeat').css('color', 'var(--yellow2)');
   if (lmcData.state.plShuffle)
      $('#btnLmcShuffle').css('color', 'var(--yellow2)');
   if (lmcData.state.muted)
      $('#btnLmcMute').css('color', 'var(--yellow2)');
}

function updatePlaylist()
{
   $('#lmcPlaylist').empty();

   if (!lmcData.playlist || !lmcData.playlist.length)
      return;

   let startWith = lmcData.state.plIndex < 3 ? 0 : lmcData.state.plIndex-2;
   let visibleCount = parseInt($('#lmcPlaylist').innerHeight() / 60);

   if (lmcData.playlist.length - startWith < visibleCount)
      startWith = lmcData.playlist.length - visibleCount;

   if (startWith < 0)
      startWith = 0;

   // console.log("staring playlist display at", startWith, "of", lmcData.playlist.length, " : ", lmcData.playlist[startWith].title, " visibleCount:", visibleCount);

   for (let i = startWith; i < lmcData.playlist.length; i++) {
      $('#lmcPlaylist')
         .append($('<div></div>')
                 .attr('id', 'plItem_' + i)
                 .css('color', i != lmcData.state.plIndex ? '' : 'var(--yellow2)')
                 .addClass('rounded-border lmcWidget')
                 .click(function() {
                    let index = $(this).attr('id').substring($(this).attr('id').indexOf("_") + 1);
                    socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : 'play', 'id' : index } });
                 })
                 .append($('<span></span>')
                         .append($('<img></img>')
                                 .attr('src', lmcData.playlist[i].cover)))
                 .append($('<span></span>')
                         .append($('<div></div>')
                                 .html(lmcData.playlist[i].title))
                         .append($('<div></div>')
                                 .html(lmcData.playlist[i].artist))
                         .append($('<div></div>')
                                 .html(lmcData.playlist[i].album))
                        )
                 .append($('<span></span>')
                         .addClass(i != lmcData.state.plIndex ? '' : 'mdi mdi-music-circle-outline')
                         .css('color', lmcData.state.mode == 'play' ? 'var(--yellow2)' : 'var(--light3)'))
                );
   }
}

function clearProgressTrigger()
{
   if (progressTrigger) {
      // console.log("stopping progress timer!");
      clearInterval(progressTrigger);
      progressTrigger = 0;
   }
}

function calcProgress()
{
   let time = lastTrackTime;

   if (lmcData.state.mode == 'play' && lastTrackTimeAt) {
      time = parseInt(lastTrackTime + (lNow() - lastTrackTimeAt));
   }

   let percent = time / (lmcData.current.duration / 100.0);
   $('#lmcTrackProgressBar').css('width', percent + '%');

   // console.log("setting progress to", paserInt(percent)+'%');

   $('#lmcTrackTime').html(parseInt(time/60) + ':' + (time%60).toString().padStart(2, '0'));
   $('#lmcTrackDuration').html(parseInt(lmcData.current.duration/60) + ':' + (lmcData.current.duration%60).toString().padStart(2, '0'));
}

function onMenu()
{
   if ($('#lmcLeftColumnMenu').hasClass('hidden')) {
      $('#lmcLeftColumnMenu').removeClass('hidden');
      $('#lmcLeftColumn').addClass('hidden');
      lmcMenu();
   }
   else {
      $('#lmcLeftColumnMenu').addClass('hidden');
      $('#lmcLeftColumn').removeClass('hidden');
   }
}

function lmcMenu(menuData)
{
   let menu = menuData ? menuData : lmcData.menu;

   console.log("menu " + JSON.stringify(menu, undefined, 4));

   $('#lmcLeftColumnMenu').empty()
      .append($('<div></div>')
              .append($('<span></span>')
                      .addClass('rounded-border lmcWidget')
                      .css('font-weight', 'bold')
                      .css('font-size', 'x-large')
                      .html(menu.title)
                     ));

   for (let i = 0; i < menu.items.length; i++) {
      $('#lmcLeftColumnMenu')
         .append($('<div></div>')
                 .append($('<span></span>')
                         .append($('<img></img>')
                                 .attr('src', menu.items[i].icon)))
                 .append($('<span></span>')
                         .html(menu.items[i].tracknum ? menu.items[i].tracknum + ' - ' : ''))
                 .append($('<span></span>')
                         .attr('title', menu.items[i].id)
                         .addClass('rounded-border lmcWidget')
                         .html(menu.items[i].name)
                         )
                 .data('id', menu.items[i].id)
                 .click({'menu' : menu, 'item' : menu.items[i] }, function(event) {
                    console.log("menu", event.data.menu.type, "clicked");
                    if (event.data.item.contextMenu)
                       return lmcContextMenu(event, event.data.menu, $(this).data('id'));
                    socket.send({ 'event' : 'lmcaction', 'object' : {
                       'action' : 'menu',
                       'text' : event.data.item.name,
                       'cmd' : event.data.item.cmd,
                       'cmdsp' : event.data.item.cmdsp,
                       'typeid' : event.data.menu.typeid,
                       'type' : event.data.menu.type,
                       'id' : $(this).data('id').toString()
                    }});
                 }));
   }
}

function lmcContextMenu(event, menu, id)
{
   var form = $('<div></div>')
       .attr('id', 'lmcContextDlg')
       .addClass('dialog-content');

   form.append($('<div></div>')
               .addClass('lmcContextMenu')
               .append($('<button></button>')
                       .addClass('rounded-border button1')
                       .html('Play now')
                       .click(function() {
                          $('#lmcContextDlg').dialog('destroy').remove();
                          socket.send({ 'event': 'lmcaction', 'object': {
                             'action': 'menuaction',
                             'what': 'playnow',
                             'typeid': menu.typeid,
                             'type' : menu.type,
                             'id': id.toString()
                          }});
                       }))
               .append($('<button></button>')
                       .addClass('rounded-border button1')
                       .html('Append to waitlist')
                       .click(function() {
                          $('#lmcContextDlg').dialog('destroy').remove();
                          socket.send({ 'event': 'lmcaction', 'object': {
                             'action': 'menuaction',
                             'what': 'append',
                             'typeid': menu.typeid,
                             'type': menu.type,
                             'id': id
                          }});
                       }))
              );

   form.dialog({
      modal: true,
      position: { my: "left top", at: "center", of: event },
      minHeight: "0px",
      width: "200px",
      closeOnEscape: true,
      dialogClass: "no-titlebar ui-widget-content-dark",
      resizable: false,
		closeOnEscape: true,
      hide: "fade",
      open: function(event, ui) {
         $('.ui-widget-overlay').bind('click', function() { $("#lmcContextDlg").dialog('close'); });
      },
      close: function() {
         $(this).dialog('destroy').remove();
      }
   });
}
