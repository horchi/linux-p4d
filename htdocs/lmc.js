/*
 *  lmc.js
 *
 *  (c) 2020-2022 Jörg Wendel
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
                              .attr('id', 'lmcCover')
                              .addClass('lmcCover lmcWidget')
                              .append($('<img></img>')
                                      .attr('id', 'lmcCoverImage'))
                             ))
              .append($('<div></div>')
                      .addClass('lmcRightColumn')
                      .attr('id', 'lmcRightColumn')
                      .append($('<div></div>')
                              .addClass('lmcControl')
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
                                      .css('width', '30px')
                                      .addClass('rounded-border lmcButton')
                                     )
                              .append($('<button></button>')
                                      .addClass('mdi mdi-volume-medium rounded-border lmcButton')
                                      .click(function() { lmcAction('volumeDown'); })
                                     )
                              .append($('<button></button>')
                                      .addClass('mdi mdi-volume-high rounded-border lmcButton')
                                      .click(function() { lmcAction('volumeUp'); })
                                     )
                              .append($('<button></button>')
                                      .attr('id', 'btnLmcMute')
                                      .addClass('mdi mdi-volume-off rounded-border lmcButton')
                                      .click(function() {  lmcAction('muteToggle'); })
                                     )
                              .append($('<div></div>')
                                      .css('flex-grow', '1'))
                              .append($('<button></button>')
                                      .attr('id', 'lmcBtnBurgerMenu')
                                      .addClass('rounded-border burgerMenu lmcButton')
                                      .click(function() { onMenu(); })
                                      .append($('<div></div>'))
                                      .append($('<div></div>'))
                                      .append($('<div></div>'))
                                     )
                             )
                      .append($('<div></div>')
                              .attr('id', 'lmcPlaylist')
                              .addClass('lmcPlaylist')))
             );

   updateLmc();

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - 5);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - 5);
   };
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

   if (lmcData.state.mode != "play")
      clearProgressTrigger();

   $('#lmcCurrentArtist').html(lmcData.current.artist);
   $('#lmcCurrentTitle').html(lmcData.current.title);
   $('#lmcCurrentAlbum').html(lmcData.current.album);
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

   console.log("staring playlist display at", startWith, "of", lmcData.playlist.length, " : ", lmcData.playlist[startWith].title, " visibleCount:", visibleCount);

   for (let i = startWith; i < lmcData.playlist.length; i++) {
      // console.log("track", i, lmcData.playlist[i].title);
      $('#lmcPlaylist')
         .append($('<div></div>')
                 .attr('id', 'plItem_' + i)
                 .css('color', i != lmcData.state.plIndex ? '' : 'var(--yellow2)')
                 .addClass('rounded-border lmcWidget')
                 .click(function() {
                    let index = parseInt($(this).attr('id').substring($(this).attr('id').indexOf("_") + 1));
                    socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : 'play', 'index' : index } });
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
   $('#lmcTrackProgressBar').css('width', percent+'%');
   // console.log("setting progress to", paserInt(percent)+'%');

   $('#lmcTrackTime').html(parseInt(time/60) + ':' + (time%60).toString().padStart(2, '0'));
   $('#lmcTrackDuration').html(parseInt(lmcData.current.duration/60) + ':' + (lmcData.current.duration%60).toString().padStart(2, '0'));
}

function onMenu()
{
   if ($('#lmcLeftColumnMenu').hasClass('hidden')) {
      $('#lmcLeftColumnMenu').removeClass('hidden');
      $('#lmcLeftColumn').addClass('hidden');
      showMenu();
   }
   else {
      $('#lmcLeftColumnMenu').addClass('hidden');
      $('#lmcLeftColumn').removeClass('hidden');
   }
}

function showMenu(menuData)
{
   let menu = menuData ? menuData : lmcData.menu;

   console.log("menu " + JSON.stringify(menu, undefined, 4));

   $('#lmcLeftColumnMenu').empty();

   for (let i = 0; i < menu.length; i++) {
      $('#lmcLeftColumnMenu')
         .append($('<div></div>')
                 .data('id', menu[i].id)
                 .addClass('rounded-border lmcWidget')
                 .click(function() {
                    // console.log("send menu request", { 'event' : 'lmcaction', 'object' : { 'action' : 'menu', 'queryType' :  $(this).data('id')} });
                    socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : 'menu', 'query' :  $(this).data('id')} });
                 })
                 .html(menu[i].name)
                );
   }
}

function lmcMenu(menu)
{
   showMenu(menu);
}

function lmcAction(action)
{
   socket.send({ 'event' : 'lmcaction', 'object' : { 'action' : action} });
}
