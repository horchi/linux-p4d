

// http://192.168.200.10:61109/gauge.html
//
// Doku
//   https://canvas-gauges.com/documentation/user-guide/scripting-api

document.addEventListener("DOMContentLoaded", function(event) {

   var canvas = document.createElement('canvas');
   canvas.setAttribute('id', 'canvas');
   let container = document.getElementById("container");
   container.appendChild(canvas);
   container.style.width = '200px';
   // container.style.aspectRatio = "1 / 1";
   // container.style.height = '200px';
   container.style.background = '#4d4b4b';

   var gauge = new LinearGauge({
      renderTo: 'canvas',
      width: 200,
      height: 200,
      "minValue": 0,
      "maxValue": 120,
      "majorTicks": [0,20,40,60,80,100,120],
      "minorTicks": 5,
      "strokeTicks": false,
      "highlights": [
         {
            "from": 0,
            "to": 20,
            "color": "rgba(255,0,0,.6)"
         },
         {
            "from": 20,
            "to": 120,
            "color": "rgba(0,255,0,.6)"
         }],
      "colorPlate": "transparent",
      "colorBarProgress": "rgb(3, 130, 225)",
      "colorNumbers": "white",
      "fontNumbersSize": 30,
      "borderOuterWidth": 0,
      "borderMiddleWidth": 0,
      "borderInnerWidth": 0,
      "borderShadowWidth": 0,
      "needle": false,
      "valueBox": false,
      "barWidth": 54,
      "numbersMargin": 0,
      "barBeginCircle": 0,
      needleWidth: 0,
      tickSide: 'left',
      numberSide: 'left',
      needleSide: 'left',
      colorPlate: 'transparent'
   });

   gauge.draw();
   gauge.value = 66;
});
