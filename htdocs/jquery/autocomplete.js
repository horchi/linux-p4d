
(function($, undefined) {
   if (typeof $.uix !== "object") {
      $.uix = {};
   }
   var ac = $.ui.autocomplete.prototype;
   var _super = {
      _create: ac._create,
      _destroy: ac._destroy,
      _resizeMenu: ac._resizeMenu,
      _suggest: ac._suggest,
      search: ac.search,
      open: ac.open,
      close: ac.close
   };
   ac = $.extend({}, ac, {
      options: $.extend({}, ac.options, {
         multiselect: false,
         triggerChar: false,
         classes: ''
      }),
      _create: function() {
         var self = this;
         var o = self.options;
         _super._create.apply(self);

         if (o.multiselect) {
            self.selectedItems = {};
            self.multiselect = $('<div class="sortable"></div>')
               .addClass("ui-autocomplete-multiselect ui-state-default ui-widget")
               .addClass(o.classes)
               // .css("width", self.element.width())
               .insertBefore(self.element)
               .append(self.element)
               .bind("click.autocomplete", function() {
                  self.element.focus();
               })
               .sortable({
                  placeholder: 'placeholder',
                  cursor: "move",
                  cursorAt: { right: 5 },
                  opacity: 0.8,revert: true,
                  // tolerance: "pointer",
                  update: function(event, ui) {
                     var arr = []; var i = 0;
                     self.multiselect.children('div').each(function () {
                        if ($(this).attr('id') != undefined)
                           arr[i++] = $(this).attr('id');
                     });
                     setAutoCompleteValuesDirect(self, arr);
                  }
               });

            var fontSize = parseInt(self.element.css("fontSize"), 10);

            function autoSize(e) {
               // Hackish autosizing
               var $this = $(this);
               $this.width(1).width(this.scrollWidth + fontSize - 1);
            };

            var kc = $.ui.keyCode;
            self.element.bind({
               "keydown.autocomplete": function(e) {
                  if ((this.value === "") && (e.keyCode == kc.BACKSPACE)) {
                     var prev = self.element.prev();
                     delete self.selectedItems[prev.text()];
                     prev.remove();
                  }
               },
               // TODO: Implement outline of container
               "focus.autocomplete blur.autocomplete": function() {
                  // self.multiselect.toggleClass("ui-state-active");
               },
               "keypress.autocomplete change.autocomplete focus.autocomplete blur.autocomplete": autoSize
            }).trigger("change");
            o.select = o.select || function(e, ui) {
               $('<div></div>')
                  .attr('title', ui.item.value)
                  .attr('id', ui.item.value)
                  .data('label', ui.item.label)
                  .addClass("ui-autocomplete-multiselect-item")
                  .text(ui.item.label)
                  .append(
                     $('<span></span>')
                        .addClass("ui-icon ui-icon-close")
                        .click(function() {
                           var item = $(this).parent();
                           delete self.selectedItems[item.text()];
                           item.remove();
                        })
                  )
                  .insertBefore(self.element);

               self.selectedItems[ui.item.label] = ui.item;
               self._value("");
               return false;
            }
         }
         return this;
      },
      _resizeMenu: function() {
         if (this.options.multiselect) {
            var ul = this.menu.element;
            ul.outerWidth(Math.max(
               ul.width("").outerWidth(),
               this.multiselect.outerWidth()
            ));
         } else {
            _super._resizeMenu.apply(this);
         }
      },
      _suggest: function(items) {
         var elm = this.element;
         // Override this.element with our multiselect for proper positioning
         this.element = this.options.multiselect ? this.multiselect : this.element;
         _super._suggest.apply(this, [items]);
         this.element = elm;
      },
      search: function(value, event) {
         value = value != null ? value : this._value();
         if (this.options.triggerChar) {
            if (value.substring(0, 1) != this.options.triggerChar) {
               return;
            } else {
               value = value.substring(1);
            }
         }
         return _super.search.apply(this, [value, event]);
      },
      // Borrowed from 1.9
      _value: function(value) {
         return this.valueMethod.apply(this.element, arguments);
      },
      // Borrowed from 1.9
      valueMethod: function() {
         var result = this[this.is("input") ? "val" : "text"].apply(this, arguments);
         this.trigger("change");
         return result;
      }
   });

   $.uix.autocomplete = ac;
   $.widget("uix.autocomplete", $.uix.autocomplete);
})(jQuery);

function setAutoCompleteValuesDirect(ac, values)
{
   // console.log("set value", values);
   var item, i, matcher;
   ac.multiselect && ac.multiselect.find(">div").remove();
   ac.selectedItems = {};
   if (values) {
      for (i = 0; i < values.length; i++) {
         item = null;
         matcher = new RegExp($.ui.autocomplete.escapeRegex(values[i]), "i");
         $.each(ac.options.source, function () {
            if (matcher.test(this.value || this.label || this))
               return !(item = this);
         });
         if (item)
            ac._trigger("select", null, { item: item.label ? item : {label:item, value:item} });
      }
   }
}

function setAutoCompleteValues(element, values)
{
   setAutoCompleteValuesDirect(element.autocomplete("instance"), values);
}

function getAutoCompleteValues(element)
{
   var values = '';
   var ac, key;
   try {
      ac = element.autocomplete("instance").selectedItems;
      for (key in ac)
         values += ' ' + ac[key].value;
   } catch (e) {}

   return values.slice(1);
}
