/*
 *  pellets.js
 *
 *  (c) 2021 Jörg Wendel
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 */

function initPellets(pellets)
{
   // console.log("pellets: ", JSON.stringify(pellets, undefined, 4));

   $('#container').removeClass('hidden');
   $('#container').html('');

   $('#container').append($('<div></div>')
                          .addClass('tableRow')
                          .append($('<div></<div>')
                                  .css('min-width', '155px')
                                  .html('Datum'))
                          .append($('<div></<div>')
                                  .css('min-width', '115px')
                                  .html('Menge [kg]'))
                          .append($('<div></<div>')
                                  .css('min-width', '115px')
                                  .html('Preis'))
                          .append($('<div></<div>')
                                  .css('min-width', '350px')
                                  .html('Notiz'))
                          .append($('<div></<div>')
                                  .css('min-width', '50px')
                                  .html('Tage'))
                          .append($('<div></<div>')
                                  .css('min-width', '70px')
                                  .html('€ / t'))
                          .append($('<div></<div>')
                                  .css('min-width', '50px')
                                  .html('Stoker [h]'))
                          .append($('<div></<div>')
                                  .css('min-width', '50px')
                                  .html('kg / h'))
                         );

   for (var i = 0; i < pellets.length; i++) {
      var date = new Date(pellets[i].time * 1000);
      addRow(pellets[i].id, date, pellets[i].amount, pellets[i].price,
             pellets[i].comment, pellets[i].stokerHours, pellets[i].consumptionH, pellets[i].duration, pellets[i].sum, pellets[i]);
   }

   addRow(-1, new Date(), 0, 0, '', 0, 0, 0, false);

   // calc container size

   $("#container").height($(window).height() - getTotalHeightOf('menu') - 15);
   window.onresize = function() {
      $("#container").height($(window).height() - getTotalHeightOf('menu') - 15);
   };
}

function addRow(id, date, amount, price, comment, stokerHours, consumptionH, durationDays, sum, pData)
{
   var bgColor = sum ? 'var(--blue)' : id == -1 ? 'var(--editadd)' : 'var(--edit)';

   if (id == -1)
      $('#container').append($('<br/>'));

   var row = $('<div></div>')
   $('#container').append($(row).addClass('tableRow'))

   $(row)
      .append($('<input></input>')
              .attr('id', 'inpTime')
              .attr('type', 'date')
              .attr('disabled', sum)
              .addClass('rounded-border input')
              .css('min-width', '155px')
              .css('max-width', '155px')
              .css('background-color', bgColor)
              .val(date.toDateLocal()))
      .append($('<input></input>')
              .attr('id', 'inpAmount')
              .attr('type', 'number')
              .attr('disabled', sum)
              .attr('step', '1')
              .addClass('rounded-border input')
              .css('min-width', '115px')
              .css('max-width', '115px')
              .css('background-color', bgColor)
              .val(amount))
      .append($('<input></input>')
              .attr('id', 'inpPrice')
              .attr('type', 'number')
              .attr('step', '0.1')
              .attr('disabled', sum)
              .addClass('rounded-border input')
              .css('min-width', '115px')
              .css('max-width', '115px')
              .css('background-color', bgColor)
              .val(price))
      .append($('<input></input>')
              .attr('id', 'inpComment')
              .attr('type', 'search')
              .attr('disabled', sum)
              .addClass('rounded-border input')
              .css('min-width', '350px')
              .css('max-width', '350px')
              .css('background-color', bgColor)
              .val(comment))
      .append($('<div></div>')
              .css('min-width', '50px')
              .css('max-width', '50px')
              .html(durationDays))
      .append($('<div></div>')
              .css('min-width', '70px')
              .css('max-width', '70px')
              .html(price && amount ? (price / (amount/1000)).toFixed(2) + ' €' : ''))
      .append($('<div></div>')
              .css('min-width', '50px')
              .css('max-width', '50px')
              .html(stokerHours))
      .append($('<div></div>')
              .addClass('rounded-border')
              .css('min-width', sum ? '' : '50px')
              .css('max-width', sum ? '' : '50px')
              .css('padding', sum ? '4px' : '')
              .css('background-color', sum ? bgColor : '')
              .attr('title', pData != null ? pData.consumptionHint : '')
              .html(sum ? pData.consumptionDelta + ' kg' : (consumptionH ? (consumptionH).toFixed(2) : '')));

   if (!sum)
      $(row).append($('<button></button>')
                    .addClass('rounded-border buttonOptions')
                    .html(id == -1 ? 'hinzufügen' : 'ändern')
                    .css('margin-left', '15px')
                    .click( function() {
                       var amount = parseInt($(this).parent().find('#inpAmount').val());
                       var price = parseFloat($(this).parent().find('#inpPrice').val().replace(',', '.'));
                       var comment = $(this).parent().find('#inpComment').val();
                       if (amount == 0 || price == 0) {
                          showInfoDialog({'message' : 'Missing input'});
                       }
                       else {
                          var unixDate = (new Date($(this).parent().find('#inpTime').val())).getTime() / 1000
                          socket.send({ "event": "pelletsadd", "object":
                                        { "id":      id,
                                          "time":    unixDate,
                                          "amount":  amount,
                                          "price":   price,
                                          "comment": comment
                                        }});
                       }
                    }));

   if (id != -1)
      $(row).append($('<button></button>')
                    .addClass('rounded-border buttonOptions')
                    .html('löschen')
                    .css('margin-left', '15px')
                    .click( function() {
                       socket.send({ "event": "pelletsadd", "object": { "id": id, "delete": true }});
                    }));
}
