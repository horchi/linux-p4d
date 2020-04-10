/*
  JavaScript-File for the p4d Web-InterFace
  Copyright by Stefan Doering and Joerg Wendel
  used for different functions in all pages
  please do not modify unless you know what you are doing!
*/

function confirmSubmit(msg)
{
    if (confirm(msg))
        return true;

    return false;
}

function showContent(elm)
{
	 if (document.getElementById(elm).style.display == "block")
    {
        document.getElementById(elm).style.display = "none"
    }
    else
    {
        document.getElementById(elm).style.display = "block"
    }
}

function readonlyContent(elm, chk)
{
    var elm = document.querySelectorAll('[id*=' + elm + ']');  var i;

	 if (chk.checked == 1)
    {
		  for (i = 0; i < elm.length; i++)
        {
			   elm[i].readOnly = false;
			   elm[i].style.backgroundColor = "#fff";
		  }
	 }
    else
    {
		  for (i = 0; i < elm.length; i++)
        {
			   elm[i].readOnly = true;
			   elm[i].style.backgroundColor = "#ddd";
		  }
	 }
}

function disableContent(elm, chk)
{
    var elm = document.querySelectorAll('[id*=' + elm + ']');  var i;

	 if (chk.checked == 1)
    {
		  for (i = 0; i < elm.length; i++)
        {
			   elm[i].disabled = false;
		  }
	 }
    else
    {
		  for (i = 0; i < elm.length; i++)
        {
			   elm[i].disabled = true;
		  }
	 }
}

function displayCoords(e, id, elm)
{
	 if (!e)
        e = window.event;

    var mydiv = document.getElementById(id);
	 var body = (window.document.compatMode && window.document.compatMode == "CSS1Compat")
          ? window.document.documentElement : window.document.body;

	 xpos = e.pageX ? e.pageX : e.clientX + mydiv.scrollLeft  - mydiv.clientLeft
	 ypos = e.pageY ? e.pageY : e.clientY + mydiv.scrollTop - mydiv.clientTop,

//    xpos = mydiv.clientLeft
//    ypos = mydiv.clientTop,

    txt = "Xpos="+xpos+"; Ypos="+ypos+"  ";

    document.getElementById(elm).value = txt;
}

function showHide(id)
{
    if (document.getElementById)
    {
        var mydiv = document.getElementById(id);
        mydiv.style.display = mydiv.style.display == 'block' ? 'none' : 'block';
    }
}

$(function()
{
    var polar_to_cartesian, svg_circle_arc_path, animate_arc;

    polar_to_cartesian = function(cx, cy, radius, angle) {
        var radians;
        radians = (angle - 90) * Math.PI / 180.0;
        return [Math.round((cx + (radius * Math.cos(radians))) * 100) / 100, Math.round((cy + (radius * Math.sin(radians))) * 100) / 100];
    };

    svg_circle_arc_path = function(x, y, radius, start_angle, end_angle) {
        var end_xy, start_xy;
        start_xy = polar_to_cartesian(x, y, radius, end_angle);
        end_xy = polar_to_cartesian(x, y, radius, start_angle);
        return "M " + start_xy[0] + " " + start_xy[1] + " A " + radius + " " + radius + " 0 0 0 " + end_xy[0] + " " + end_xy[1];
    };

    animate_arc = function(ratio, svg, value, perc, unit, y)
    {
        var arc, center, radius, startx, starty;

        arc = svg.path('');
        center = 500;
        radius = 450;
        startx = 0;
        starty = 450;

        return Snap.animate(0, ratio, (function(val)
        {
            var path;
            arc.remove();
            path = svg_circle_arc_path(500, y, 450, -90, val * 180.0 - 90);
            arc = svg.path(path);
            arc.attr({class: 'data-arc'});
            perc.text(value + " " + unit);
        }), Math.round(2000 * ratio), mina.easeinout);
    };

    draw_peak = function(peak, svg, y)
    {
        var arc, center, radius, startx, starty;

        arc = svg.path('');
        center = 500;
        radius = 450;
        startx = 0;
        starty = 450;

        return Snap.animate(0, peak, (function(val)
        {
            var path;
            arc.remove();
            path = svg_circle_arc_path(500, y, 450, val * 180.0 - 91, val * 180.0 - 90);
            arc = svg.path(path);
            arc.attr({class: 'data-peak'});
        }), Math.round(2000 * peak), mina.easeinout);
    };

    $('.widget-row').each(function()
    {
        var ratio, svg, perc, unit, value, peak, y;

        ratio = $(this).data('ratio');
        svg = Snap($(this).find('svg')[0]);
        perc = $(this).find('text._content');
        unit = $(this).data('unit');
        value = $(this).data('value');
        peak = $(this).data('peak');
        y = $(this).data('y');
        animate_arc(ratio, svg, value, perc, unit, y);
        draw_peak(peak, svg, y);
    });
});
