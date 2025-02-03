(function(window, document){
    var WindyForecastAsync = function(opts, node) {

                const dev = false;

                const prodHost = '//windy.app/widgets-code/forecast';
                const devHost = document.location.origin;
                const host = dev ? devHost : prodHost;

        // version from widget perams or write it manually:
        this.appVersionManual = '1.4.2';
        this.appVersion = findAppVersion() || this.appVersionManual;

        // write the theme from widget perams
        this.themeMode = node.dataset.thememode;
        this.customCss = node.dataset.customcss;

        //set units from browser locale
        this.country = getCountryFromIP('https://geolocation-db.com/json/');
        let unitCountry = getDefaultFor(this.country);
        let units = getUnitFromCountry(unitCountry);
        this.tempUnit = units.temp;
        this.windUnit = units.wind;
        this.heightUnit = units.height;
        this.timeUnit = units.time;
        this.pressureUnit = units.pressure;

        //set units cookie
        let unitsShorten = [-1, -1, -1, -1, -1];

        var hostname = 'windy.app';
        this.url = dev ? '/' : '//' + hostname + '/widgets-code/forecast/';

        const widgetsHref = 'https://windy.app/widgets';

        this.fields = node.dataset.fields;

        if (node.dataset.share) {
            this.share = JSON.parse(node.dataset.share);
        }

        this.allFields = [
        'wind-direction',
        'wind-speed',
        'wind-gust',
        'air-temp',
        'clouds',
        'precipitation',
        'waves-direction',
        'waves-height',
        'waves-period',
        'tides',
        'moon-phase'];

        let locationHostname = location.hostname;
        let fullLog = false;
        let halfLog = false;

        fields = fields? fields.toLowerCase() : allFields.join(',');
        // if (!fields) {
        //     fields = allFields.join(',');
        // } else {
        //     fields = this.fields.toLowerCase();
        // }

        //create share button
        const addShareButtonIn = (parentNode, share) => {

            const testVersion = share.icon ? share.icon : 'A';

            const iconsSize = share.iconsSize ? share.iconsSize : 16;

            const amplitudeName = share.amplitude ? share.amplitude : 'SHARE';

            const sizeAspRatio = iconsSize / 16;

            const shareButton = document.createElement('div');
            shareButton.classList.add('share-button');

            const shareButtonIcon = document.createElement('div');

            const widgetNode = parentNode.parentNode.parentNode;

            const widgetId = widgetNode.id;
            const spotID = widgetId.split('_')[1];





            shareButtonIcon.classList.add('share-button-icon');

                    //style here for test purposes
                    const styleSheet = document.createElement('style');
                    styleSheet.innerHTML = `
                    .share-button {
                        position: absolute;
                        right: 10px;
                        top: 0px;
                        aspect-ratio: 1/1;
                        margin: 0 15px;
                        height: fit-content;
                        border: ${1 * sizeAspRatio}px solid #000 !important;
                        border-radius: ${4 * sizeAspRatio}px;
                        cursor: pointer;
                        z-index: 100;
                        display: flex;
                        align-items: center;
                        justify-content: center;
                        transition: width 0.2s ease;
                        min-height: ${23 * sizeAspRatio}px;
                        margin-top: 3px;
                        transition: all 0.2s ease-in-out;
                    }
                    .share-button-icon {
                        margin: ${3 * sizeAspRatio}px;
                        width: ${iconsSize}px;
                        height: ${iconsSize}px;
                        background-image: url(${host}/img/icons/share_${testVersion}.svg);
                        background-size: ${iconsSize}px ${iconsSize}px;
                        background-repeat: no-repeat;
                        background-position: center;
                    }
                    .share-button-tooltip {
                        transform: translateX(-50%);
                        padding: 5px 10px;
                        border-radius: 5px;
                        background-color: #000;
                        color: #fff;
                        font-size: 12px;
                        font-weight: 500;
                        opacity: 0;
                        transition: opacity 0.2s ease-in-out;
                    }
                    .share-button-links {
                        display: none;
                        align-items: center;
                        justify-content: center;
                        margin: 5px 0;
                    }
                    .share-button-link {
                        display: none;
                        margin: ${5 * sizeAspRatio}px;
                        width: ${iconsSize}px;
                        height: ${iconsSize}px;
                        background-size: ${iconsSize}px ${iconsSize}px;
                        background-repeat: no-repeat;
                        background-position: center;
                    }

                    .share-button_active {
                        aspect-ratio: auto;
                    }
                    .share-button-link_active {
                        display: block;
                    }

                    .share-button:hover {
                        background-color: #27455a;
                    }

                    .share-button:hover .share-button-icon {
                        filter: invert(1);
                    }
                    .share-button-link:hover {
                        filter: brightness(5);
                    }
                    `;
                    document.head.appendChild(styleSheet);

                    // color dark windy: #27455a, inverted: #d6b8a4

                    const title = 'Windy';
                    const text = 'Check out the weather forecast for your spot: ' + window.location.href.split('?')[0] + '?share=' + 'true';
                    const ths = this;

                    function getShareUrl() {
                        // current url witout query params
                        return window.location.href.split('?')[0] + '?shareForecast=1&tempUnit=' + ths.tempUnit + '&windUnit=' + ths.windUnit;
                    }

                    //on click share-button show share-button-links
                    shareButton.addEventListener('click', async () => {

                        const screenshotUrl = 'https://windyapp.co/v10/screenshot?type=forecast' +
                                              '&thememode=white&spot_id=' + spotID + '&tempunit=' +
                                              this.tempUnit + '&windunit=' + this.windUnit +
                                              '&width=1108&height=626';

                        //if navigator share is available
                        if (navigator.share) {
                            try {
                                const file = await fetch(screenshotUrl)
                                    .then(response => response.blob())
                                    .then(blob => new File([blob], 'image.jpg', {
                                        type: 'image/jpeg'
                                    }));

                                await navigator.share({
                                    title: title,
                                    text: text,
                                    url: getShareUrl(),
                                    files: [file],
                                });

                            } catch (error) {
                                console.error('Error sharing the page:', error);
                            }

                        } else {
                            shareButtonLinkFacebook.classList.toggle('share-button-link_active');
                            shareButtonLinkTwitter.classList.toggle('share-button-link_active');
                            shareButton.classList.toggle('share-button_active');

                            window.apmlitudeLogForecast(amplitudeName + '_CLICK_ICON-' + testVersion + '_NOT_NAVIGATOR_SHARE')

                        }


                        window.apmlitudeLogForecast(amplitudeName + '_CLICK_ICON-' + testVersion)
                    });

                    //if navigator share is not available
                    //add links to share button
                    const shareButtonLinkFacebook = document.createElement('div');
                    const shareButtonLinkTwitter = document.createElement('div');

                    shareButtonLinkFacebook.classList.add('share-button-link');
                    shareButtonLinkTwitter.classList.add('share-button-link');

                    shareButtonLinkFacebook.style.backgroundImage = `url(${host}/img/icons/facebook.svg)`;
                    shareButtonLinkTwitter.style.backgroundImage = `url(${host}/img/icons/twitter.svg)`;

                    shareButtonLinkFacebook.addEventListener('click', () => {
                        const shareUrl = getShareUrl();
                        const shareUrlEncoded = encodeURIComponent(shareUrl);
                        const shareUrlFacebook = 'https://www.facebook.com/sharer/sharer.php?u=' + shareUrlEncoded;
                        window.open(shareUrlFacebook, '_blank');
                        window.apmlitudeLogForecast(amplitudeName + '_CLICK_FACEBOOK')
                    });

                    shareButtonLinkTwitter.addEventListener('click', () => {
                        const shareUrl = getShareUrl();
                        const shareUrlEncoded = encodeURIComponent(shareUrl);
                        const shareUrlTwitter = 'https://twitter.com/intent/tweet?url=' + shareUrlEncoded;
                        window.open(shareUrlTwitter, '_blank');
                        window.apmlitudeLogForecast(amplitudeName + '_CLICK_TWITTER')
                    });

                    shareButton.appendChild(shareButtonLinkFacebook);
                    shareButton.appendChild(shareButtonLinkTwitter);


            shareButton.appendChild(shareButtonIcon);

            //append to last child of parent node
            parentNode.appendChild(shareButton);
        }

        const addDaysBordersTo = (cell, hoursArray = [7, 8, 8, 8, 8, 8, 8, 8, 8, 1]) => {

            //get height and width of the cell
            const height = cell.offsetHeight;
            const width = cell.offsetWidth;

            const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
            svg.setAttribute('width', width);
            svg.setAttribute('height', height);

            const allHours = hoursArray.reduce((a, b) => a + b, 0);

            let x = 0;
            let y = 0;
            let xStep = width / allHours;

            for (let i = 0; i < hoursArray.length; i++) {
                const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
                line.setAttribute('x1', x);
                line.setAttribute('y1', y);
                line.setAttribute('x2', x);
                line.setAttribute('y2', height);
                line.setAttribute('stroke', '#d6dce0');
                line.setAttribute('stroke-width', 1);
                svg.appendChild(line);
                x += hoursArray[i] * xStep;
            }

            cell.querySelector('svg').appendChild(svg);

        }



        const mapDataXY = (data, tempData, width, height, cellWidth = false) => {

            // calculate the x and y coordinates for each data point
            const xStep = cellWidth ? cellWidth : width / (data.length - 1);
            const yMax = Math.max(...data);
            const yMin = Math.min(...data);
            const yScale = height / (yMax - yMin);
            const points = data.map((value, index) => {
              const x = index * xStep;
              const y = height - (value - yMin) * yScale;
              return { x, y, value, airTemp: tempData[index]};
            });

            return points;
        }

        const lineSmoothGraphForPoints_withGaps = (points, width, height, element, cellWidth = false) => {
            // create an SVG element
            const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
            svg.setAttribute('width', width);
            svg.setAttribute('height', height);

            const xStep = cellWidth ? cellWidth : width / (points.length - 1);

            // path element
            let pathString = "";
            let pathLine;
            for (let i = 0; i < points.length; i++) {
                let m = i + 1;
                if (m > points.length-1) { m = i;}
                if (points[i].value === 0) {
                    if (pathString.length > 0) {
                        pathLine.setAttribute('d', pathString);
                        svg.appendChild(pathLine);
                        pathString = "";
                    }
                } else {
                    if(pathString.length === 0) {
                        pathLine = document.createElementNS('http://www.w3.org/2000/svg', 'path');
                        pathLine.setAttribute('vector-effect', 'non-scaling-stroke');
                    }
                    if(i === 0 || points[i-1].value === 0) {
                        pathString += `M${points[i].x},${height}`;
                    } else {
                        pathString += `C${points[i-1].x + xStep/2},${points[i-1].y} ${points[i-1].x + xStep/2},${points[i].y} ${points[i].x},${points[i].y}`;

                    } if (i === points.length - 1 || points[m].value === 0) {
                        //end of each path
                        if (m > points.length-1) {
                            pathString += `L${points[i].x},${height}Z`;
                        } else {
                        pathString += `C${points[i].x + xStep/2},${points[i].y} ${points[i].x + xStep/2},${height} ${points[m].x},${height}Z`;
                        }
                    }

            } }
            if (pathString.length > 0) {
                pathLine.setAttribute('d', pathString);
                svg.appendChild(pathLine);
            }

            element.appendChild(svg);
         }

        const precipAssembler = (data, tempData, cell, height = 50, style = 'light') => {

            //add style to cell
            cell.classList.add('percip-comined');

            const width = cell.offsetWidth;
            const cellWidth = 32;

            // if value less than 100, set to 0
            data = data.map(value => value < 20 ? 0 : value);

            const points = mapDataXY(data, tempData, width, height, cellWidth);

            // create an SVG element
            const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
            svg.setAttribute('width', width);
            svg.setAttribute('height', height);

            lineSmoothGraphForPoints_withGaps(points, width, height, svg, cellWidth);

            // create text elements for every data point
            points.forEach(({ x, y, value, airTemp }) => {

                const centerOffset = x + cellWidth / 2;

                const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
                text.setAttribute('y', height - 5);
                text.setAttribute('x', centerOffset);
                // if value is 0, show dash
                text.textContent = (value == 0) ? '-' : Math.round(value / 10) / 10;

                        //add icon for every data point
                        let iconLinkIndex = '0';

                        if (value < 10) {

                        } else if (value < 100) {
                            iconLinkIndex = '1';
                        } else if (value < 300) {
                            iconLinkIndex = '2';
                        } else  {
                            iconLinkIndex = '3'
                        }

                        precType = airTemp < 0 ? 'snow' : 'drop';

                            let iconSize = 16;
                            const dropIcon = document.createElementNS('http://www.w3.org/2000/svg', 'image');
                            dropIcon.setAttributeNS('http://www.w3.org/1999/xlink', 'href', host + '/img/icons/' + precType + iconLinkIndex + '-' + style + '.svg' + '?v' + this.appVersion);
                            dropIcon.setAttribute('width', iconSize);
                            dropIcon.setAttribute('height', iconSize);
                            dropIcon.setAttribute('x', centerOffset - iconSize / 2);
                            dropIcon.setAttribute('y', height - 32);
                            svg.appendChild(dropIcon);


                svg.appendChild(text);
            });

            cell.appendChild(svg);
        }


        // require once
        if(!document.querySelector('.windy-forecast')) {

            let cssRef = (this.themeMode == 'white') ? (this.url + 'white.css?v' + this.appVersion) : (this.url + 'dark.css?v' + this.appVersion);
            if (this.customCss != null && this.customCss != '')
               cssRef = this.customCss;

            var link = document.createElement('link');
            link.rel = 'preload';
            link.as = 'style';
            link.href = cssRef;
            document.head.appendChild(link);

            var link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            link.href = cssRef;
            document.head.appendChild(link);

            var link = document.createElement('link');
            link.rel = 'preload';
            link.as = 'style';
            link.href = (this.themeMode == 'white') ? (this.url + 'widget-white.css?v' + this.appVersion) : (this.url + 'widget.css?v' + this.appVersion);
            document.head.appendChild(link);

            var link = document.createElement('link');
            link.rel = 'stylesheet';
            link.type = 'text/css';
            link.href = (this.themeMode == 'white') ? (this.url + 'widget-white.css?v' + this.appVersion) : (this.url + 'widget.css?v' + this.appVersion);
            document.head.appendChild(link);
        }

        //find app version from file name
        function findAppVersion() {
            let scripts = document.getElementsByTagName('script');
            let appVersion;
            for (let i = 0; i < scripts.length; i++) {
                if (scripts[i].src.includes('windy_forecast_async.js')) {
                    appVersion = scripts[i].src.split('windy_forecast_async.js?v')[1];
                }
            }
            return appVersion;
        }

        //new units system
        function getUnitFromCountry(country) {
            let units;
            switch (country) {
                case 'US':
                    return units = {
                        temp: 'F',
                        wind: 'mph',
                        height: 'ft',
                        time: '12h',
                        pressure: 'inHg',
                    }
                case 'GB':
                    return units = {
                        temp: 'C',
                        wind: 'knots',
                        height: 'ft',
                        time: '24h',
                        pressure: 'hPa',
                    }
                case 'OTHER_12_HOUR':
                    return units = {
                        temp: 'C',
                        wind: 'm/s',
                        height: 'm',
                        time: '12h',
                        pressure: 'hPa',
                    }
                case 'CIS':
                    return units = {
                        temp: 'C',
                        wind: 'm/s',
                        height: 'm',
                        time: '24h',
                        pressure: 'MmHg',
                    }
                case 'OTHER_24_HOUR':
                    return units = {
                        temp: 'C',
                        wind: 'm/s',
                        height: 'm',
                        time: '24h',
                        pressure: 'hPa',
                    }
                default:
                    return units = {
                        temp: 'C',
                        wind: 'm/s',
                        height: 'm',
                        time: '24h',
                        pressure: 'hPa',
                    }
            }
        }

        //get country code from browser language
       function getCountryFromLang(){
            const locLang = navigator.language;
            if(locLang.includes('US')){
                //US
                return 'US';
            } else if(locLang.includes('GB')){
                //GB
                return 'GB';
            } else if(locLang.includes('CA') || locLang.includes('AU') || locLang.includes('NZ') || locLang.includes('PH')){
                //12
                return 'OTHER_12_HOUR';
            } else if(locLang.includes('ru') || locLang.includes('UA') || locLang.includes('BY') || locLang.includes('KZ') || locLang.includes('MD')){
                //USSR
                return 'CIS';
            } else {
                //24
                return 'OTHER_24_HOUR';
            }
        }

        //get country code from ip address
        function getCountryFromIP(ipProvider) {
            let country;
            let xhr = new XMLHttpRequest();
            xhr.open('GET', ipProvider, false);
            xhr.onload = function() {
                if (xhr.status === 200) {
                    country = JSON.parse(xhr.responseText).country_code;
                }
                else {
                    country = getCountryFromLang();
                }
            };
            return country;
        }

        //get default units for country
        function getDefaultFor(country) {
            switch (country) {
                case "US":
                    return 'US';
                case "GB":
                    return 'GB';
                case "RU":
                case "BY":
                case "UA":
                case "EE":
                case "LV":
                case "LT":
                case "MD":
                case "AM":
                case "AZ":
                case "GE":
                case "KZ":
                case "TM":
                case "UZ":
                case "TJ":
                case "KG":
                    return 'CIS';
                case "AU":
                case "CA":
                case "NZ":
                case "PH":
                    return 'OTHER_12_HOUR';
                default:
                    return 'OTHER_24_HOUR';
            }
        }


//TODO: fix moonData in backend (data.php)
        function getBackendName(fieldName) {
            const objNames = {
            'wind-direction': 'windDirection',
            'wind-speed': 'windSpeed',
            'wind-gust': 'windGust',
            'air-temp': 'airTemp',
            'clouds': 'cloudsHigh',
            'precipitation': 'precipitation',
            'waves-direction': 'wavesDirection',
            'waves-height': 'wavesHeight',
            'waves-period': 'wavesPeriod',
            'tides': 'tides',
            'moon-phase': 'timestamp'
            };
            return objNames[fieldName];
        }

        function dim() {
            const mainEl = document.getElementById('windywidgetmain');
            mainEl && mainEl.classList.add('loading');
        }

        function showLoader(){
            const parentNode = document.querySelector('div[data-windywidget="forecast"]');
            const loader = '<div class="loader-forecast"></div>';
            parentNode.insertAdjacentHTML('afterbegin',loader);
        }
        showLoader();

        function hideLoader (){
            if(document.querySelector('.loader-forecast')){
                const loader = document.querySelector('.loader-forecast');
                loader.style.display = 'none';
            }
        }

        function showHtmlTemplate(spotInfo, data, node, opts, model, updated, isPro) {

            var ts = new Date().getTime() / 1000;
            var spotUrl = spotInfo.spotUrl + '/' + spotInfo.spotName;
            var spotName = spotInfo.spotName;
            var spotGmtOffset = spotInfo.spotGmtOffset;
            var spotTimezone = spotInfo.spotTimezone;
            var favoriteCount = spotInfo.favoriteCount;
            var spotID = spotInfo.spotID;
            this.spotID = spotID;
            var spotTime = spotInfo.spotTime;

            let arrowImageLink = '';

                let modelHTML = ''

            let windyLogoSVG = '';
            let windyWidgetModifier = '';
            let downloadButtonApple = '';
            let downloadButtonAndroid = '';


            //logo and download buttons
            if(location.origin === "https://windy.app" && !location.href.match(widgetsHref)) {
                modelHTML = !isPro ? '<button id="model-select" style="border-radius: 6px;background-color: #0583f4;text-decoration: none;color: #fff;font-size: 14px;margin-left: 5px;padding: 2px 10px;cursor: pointer;line-height: 28px;">Advanced forecast</button>' :
                `<ul class="model-list" id="modelList" style="display:inline-flex">
                    <li class="model-list__item" data-model-name="gfs27_long" >GFS27</li>
                    <li class="model-list__item" data-model-name="ecmwf">ECMWF <span class="model-list__span">PRO</span></li>
                    <li class="model-list__item" data-model-name="iconglobal">ICON13 <span class="model-list__span">PRO</span></li>
                </ul>`;
                windyWidgetModifier = '_windy-site'
                arrowImageLink = 'deg);width:35px !important; height:35px !important; max-width: 35px !important; max-height: 35px !important" src="https://windy.app/widgets-code/forecast/img/cws-arr-white.svg">';
            } else {
                modelHTML = model;
                windyWidgetModifier = ''
                if(this.themeMode =='dark'){
                    arrowImageLink = 'deg);width:35px !important; height:35px !important; max-width: 35px !important; max-height: 35px !important" src="https://windy.app/widgets-code/forecast/img/cws-arr.svg">';
                    windyLogoSVG = 'https://windy.app/widgets-code/forecast/img/w3.svg';
                    downloadButtonApple = 'https://windy.app/widgets-code/forecast/img/a.svg';
                    downloadButtonAndroid = 'https://windy.app/widgets-code/forecast/img/g.svg';
                } else {
                    arrowImageLink = 'deg);width:35px !important; height:35px !important; max-width: 35px !important; max-height: 35px !important" src="https://windy.app/widgets-code/forecast/img/cws-arr-white.svg">';
                    windyLogoSVG = 'https://windy.app/widgets-code/forecast/img/w3-black.svg';
                    downloadButtonApple = 'https://windy.app/widgets-code/forecast/img/a-black.svg';
                    downloadButtonAndroid = 'https://windy.app/widgets-code/forecast/img/g-black.svg';
                }
            }

            const sitehostname = document.location.origin;
            const appsURL = 'https://windyapp.onelink.me/VcDy/j5dimbtn?af_channel=' + sitehostname;

            //If updated time too long ago, don't show it
            let updatedHours = parseInt(updated);

            (updatedHours < 12) ? '' : apmlitudeLogForecast('ERROR', false, 'updated_long_ago');
            let updatedHtml = (updatedHours < 12) ? 'updated ' + updated + ', ' : 'Model: ';


            var html = '<div class="windy-forecast" id="widget_' + spotID + '">';
            html += `<div class="widget-header${windyWidgetModifier}" id="windywidgetheader">`;
            html += '<table width="100%" cellspacing="0" cellpadding="0" border="0">';
            html += '<tr>';
            html += '<td class="windywidgetcurwindDirection"><img style="transform: rotate(' + data[0].windDirection + 'deg); -webkit-transform: rotate(' + data[0].windDirection + arrowImageLink + '</td>';
            html += '<td style="vertical-align: middle;padding-left:12px;" class="windywidgetheaderText" width="100%">';
            html += '<span id="windywidgetcurwindSpeed" data-value="' + data[0].windSpeed + '"></span> <span id="windywidgetcurwindspeedUnit" onclick="updateWind (' + spotID + ');">m/s</span>';
            html += '<a id="windywidgetspotname" target="__blank" href="' + spotUrl + '" onclick="ga(\'send\', \'event\', \'' + (opts.appid == 'windyapp' ? 'site_' : '') + 'widget_button\', \'go_to_site\', \'site_spot\');">' + spotName + '</a><br>';
            html += '<div class="windywidgettimezone" style="float:none;">' + spotTime + ', GMT' + spotGmtOffset + '</div>';
            html += '<div class="windywidgettimezone" style="float:none;">' + updatedHtml + modelHTML + '</div>';
            html += '</td>';
            html += '<td nowrap="1" class="windywidgetfavoriteCount" style="vertical-align: top; padding-top: 11px"><img src="https://windy.app/widgets-code/forecast/img/star.svg" style="width:19px;height:19px;max-width:19px"/></td>';
            html += '<td nowrap="1" class="windywidgetfavoriteCount" style="vertical-align: top; padding-top: 11px">' + favoriteCount + '</td>';
            html += '</tr>';
            html += '</table>';

            html += '<div id="headerLinks" class="widget-header__links">';

            html += '<div class="windywidgetpoweredBy">';
            html += '<a target="__blank" href="https://windy.app" onclick="ga("send", "event", "widget_button", "go_to_site", "site");">';
            html += '<img src="' + windyLogoSVG + '" style="width:100px !important;height: 23px !important;" alt="WINDY.APP"></a></div>';

            html += '<div class="windywidgetApps">';
            html += '</a>&nbsp;&nbsp;&nbsp;&nbsp;<a id="windywidgetAppsIos" target="__blank" href="' + appsURL + '" onclick="ga(\'send\', \'event\', \'widget_button\', \'go_to_store\', \'android\');">';
            html += '<img src="' + downloadButtonApple + '" style="width:75px !important;height: 26px !important; display: inline-block !important;" alt="Download on the Appstore">';
            html += '</a>&nbsp;&nbsp;&nbsp;&nbsp;';
            html += '<a id="windywidgetAppsAndroid" target="__blank" href="' + appsURL + '" onclick="ga(\'send\', \'event\', \'widget_button\', \'go_to_store\', \'android\');">';
            html += '<img src="' + downloadButtonAndroid + '" style="width:75px !important;height: 26px !important; display: inline-block !important;" alt="Get it on Google Play">';
            html += '</div>';
            html += '</div>';

            html += '</div>';

            html += '<div id="windywidgetmain">';
            html += '<div id="windywidgetdates"></div>';
            html += '<div id="windywidgetslider"><div></div></div>';
            html += '<div style="clear:both;height:1px"></div>';
            html += `<div id="windywidgetlegend"><table>
            <tr class="windywidgetdays"><td>&nbsp;</td></tr>
            <tr class="windywidgethours"><td><a title="' + spotTimezone + '" style="cursor: pointer;">Time</a></td></tr>
            <tr class="windywidgetwindDirection id-wind-direction"><td>Wind direction</td></tr>
            <tr class="windywidgetwindSpeed id-wind-speed"><td onclick="updateWind (${spotID}); apmlitudeLogForecast('CHANGE_UNITS_OF_MEASUREMENT', 'WIND');">Wind speed (km/h)<div></div></td></tr>
            <tr class="windywidgetwindGust id-wind-gust"><td onclick="updateWind (${spotID}); apmlitudeLogForecast('CHANGE_UNITS_OF_MEASUREMENT', 'WIND');">Wind gusts (km/h)<div></div></td></tr>
            <tr class="windywidgetairTemp id-air-temp"><td onclick="updateTemp (${spotID}); apmlitudeLogForecast('CHANGE_UNITS_OF_MEASUREMENT', 'TEMP');"></td></tr></td></tr>
            <tr class="windywidgetclouds id-clouds"><td><a title="Cloud cover high / mid/ low" style="cursor: pointer;">Cloud coverage</a></td></tr>
            <tr id="percipCellName"><td><a title="Precipitation mm/3h" style="cursor: pointer;">Precipitation (mm/3h)</div></a></td></tr>
            <tr class="windywidgetwaves id-waves-direction"><td>Waves direction</td></tr>
            <tr class="windywidgetwavesheight id-waves-height"><td onclick="updateHeight (${spotID}); apmlitudeLogForecast('CHANGE_UNITS_OF_MEASUREMENT', 'HEIGHT');"></td></tr>
            <tr class="windywidgetwavesperiod id-waves-period"><td><a title="Waves period, sec" style="cursor: pointer;">Waves period (s)</a></td></tr>
            <tr class="windywidgettides id-tides"><td title="Tides, m/ft" style="cursor: pointer;" onclick="updateHeight (${spotID}); apmlitudeLogForecast('CHANGE_UNITS_OF_MEASUREMENT', 'HEIGHT');"></td></tr>
            <tr class="windywidgetmoonphase id-moon-phase" style="height: 32px;"><td>Moon Phase</td></tr>
            </table></div>`;
            html += '<div id="windywidgettable" data-nosnippet></div>';
            html += '<div id="windywidgetclear" style="clear:both;"></div>';
            html += '<div id="windywidgetclear" style="clear:both;"></div>';
            html += '</div>';
            html += '</div>';

            node.innerHTML = html;
        }


        // Цвета для скорости и порывов ветра, входные данные в метрах в секунду
        function colorForWindSpeed (speed) {
            var h = 0;
            var lowind = 1;
            var sat = 0.8;
            var br = (this.themeMode == 'white') ?  0.65 : 0.85;


            if (speed < 3.0) {
                //lowind = 1.0 - Math.sqrt (1.0 - 3.0 / 5.5);
                sat = (this.themeMode == 'white') ?  0.5 : 0.55;
                br = (this.themeMode == 'white') ?  0.95 : 0.65;
            } else if (speed < 5.5) {
                lowind = 1.0 - Math.sqrt (1.0 - speed / 5.5);
                sat = (this.themeMode == 'white') ? 0.5 + 0.3 * ((speed - 3) / 2.5) : 0.55 + 0.25 * ((speed - 3) / 2.5);
                br = (this.themeMode == 'white') ? 0.95 - 0.3 * ((speed - 3) / 2.5) : 0.65 + 0.2 * ((speed - 3) / 2.5);

            } else if (speed > 12.0) {
                lowind = 1.0 - (speed - 12.0) / speed * 1.0 / 2.0;
            }

            var sv = speed / 25.0;

            if (sv > 1.0) sv = 1.0;

            h = (1.0 - sv * lowind) + 0.5833;

            if (h > 1.0) {
                h -= 1.0;
            }

            // convert HSB to HSL
            // http://stackoverflow.com/a/31322679/2207790
            var l = (this.themeMode == 'white') ? br : (2 - sat) * br / 2;

            if(this.themeMode != 'white'){
                sat = l&&l<1 ? sat * br / (l < 0.5 ? l * 2 : 2 - l * 2) : sat;
            }
            //sat = l&&l<1 ? sat * br / (l < 0.5 ? l * 2 : 2 - l * 2) : sat;

            return 'hsl(' + Math.round (h * 360) + ',' + Math.round (sat * 100) + '%,' + Math.round (l * 100) + '%)';
        }

        // Цвета для температуры, входной параметр в градусах цельсия
        function colorForTemperature(tempCelsium) {
            var brightness = 0.8;
            var sat = 1.0;

            var coldVal = 240.0 / 360.0;
            var bestVal = 190.0 / 360.0;
            var bestPlusVal = 70.0 / 360.0;
            var hotVal = 0.0 / 360.0;

            var coldTemp = -20;
            var bestTemp = 0;
            var hotTemp = 40;
            var val = 0.0;

            if (tempCelsium < coldTemp) {
                tempCelsium = coldTemp;
            } else if (tempCelsium > hotTemp) {
                tempCelsium = hotTemp;
            }

            if (tempCelsium <= bestTemp) {
                val = ((tempCelsium - coldTemp) / (bestTemp - coldTemp)) * (bestVal - coldVal) + coldVal;
            } else if (tempCelsium > bestTemp) {
                val = ((tempCelsium - bestTemp) / (hotTemp - bestTemp)) * (hotVal - bestPlusVal) + bestPlusVal;
            }

            if (tempCelsium > 10.0) {
                brightness = 1.0;
                sat = 0.5 + Math.pow(Math.abs(tempCelsium - 10), 1.0 / 2.0) / 30.0;
            } else if (tempCelsium > 0.0) {
                brightness = 1.0;
                sat = 0.15 + Math.pow(Math.abs(tempCelsium), 1.0 / 3.0) / 6.15;
            } else {
                brightness = 1.0 - Math.sqrt(Math.sqrt(Math.abs(tempCelsium))) / 100.0;
                sat = 0.2 + Math.sqrt(Math.sqrt(Math.abs(tempCelsium))) / 4.0;
            }

            // convert HSB to HSL
            // http://stackoverflow.com/a/31322679/2207790
            var l = (2 - sat) * brightness / 2;
            sat = l && l < 1 ? sat * brightness / (l < 0.5 ? l * 2 : 2 - l * 2) : sat;

            return 'hsla(' + Math.round(val * 360) + ',' + Math.round(sat * 100) + '%,' + Math.round(l * 100) + '%,1.0)';
        }

        // Цвета для давления, входной параметр в mmHg
        function colorForPressure(mmHg) {
            var brightness = 0.8;
            var sat = 1.0;

            var coldVal = 240.0 / 360.0;
            var bestVal = 190.0 / 360.0;
            var bestPlusVal = 70.0 / 360.0;
            var hotVal = 0.0 / 360.0;

            var coldTemp = -20;
            var bestTemp = 0;
            var hotTemp = 40;
            var val = 0.0;

            if (mmHg < coldTemp) {
                mmHg = coldTemp;
            } else if (mmHg > hotTemp) {
                mmHg = hotTemp;
            }

            if (mmHg <= bestTemp) {
                val = ((mmHg - coldTemp) / (bestTemp - coldTemp)) * (bestVal - coldVal) + coldVal;
            } else if (mmHg > bestTemp) {
                val = ((mmHg - bestTemp) / (hotTemp - bestTemp)) * (hotVal - bestPlusVal) + bestPlusVal;
            }

            if (mmHg > 10.0) {
                brightness = 1.0;
                sat = 0.5 + Math.pow(Math.abs(mmHg - 10), 1.0 / 2.0) / 30.0;
            } else if (mmHg > 0.0) {
                brightness = 1.0;
                sat = 0.15 + Math.pow(Math.abs(mmHg), 1.0 / 3.0) / 6.15;
            } else {
                brightness = 1.0 - Math.sqrt(Math.sqrt(Math.abs(mmHg))) / 100.0;
                sat = 0.2 + Math.sqrt(Math.sqrt(Math.abs(mmHg))) / 4.0;
            }

            // convert HSB to HSL
            // http://stackoverflow.com/a/31322679/2207790
            var l = (2 - sat) * brightness / 2;
            sat = l && l < 1 ? sat * brightness / (l < 0.5 ? l * 2 : 2 - l * 2) : sat;

            return 'hsla(' + Math.round(val * 360) + ',' + Math.round(sat * 100) + '%,' + Math.round(l * 100) + '%,1.0)';
        }

        function colorForClouds(cloudsCover) {
            var brightness = 1;
            var sat = 0;
            var val = 0;

            brightness = brightness - cloudsCover / 300;

            // convert HSB to HSL
            // http://stackoverflow.com/a/31322679/2207790
            var l = (2 - sat) * brightness / 2;
            sat = l && l < 1 ? sat * brightness / (l < 0.5 ? l * 2 : 2 - l * 2) : sat;

            var alpha = 1.0;
            alpha = cloudsCover / 100.0;

            return 'hsla(' + Math.round(val * 360) + ',' + Math.round(sat * 100) + '%,' + Math.round(l * 100) + '%, ' + alpha + ')';
        }


        // //www.mvps.org/directx/articles/catmull/
        function spline(P0, P1, P2, P3, t) {
            return 0.5 * ((2 * P1) + (-P0 + P2) * t + (2 * P0 - 5 * P1 + 4 * P2 - P3) * t * t + (-P0 + 3 * P1 - 3 * P2 + P3) * t * t * t);
        }

        // Высота квадратика - 4.0мм все что выше - просто упирается в его границу.
    	// Минимальное значение квадратика 0.2
        function precipitationToHeight(precipitation) {
            return (4 - precipitation) / (4 - 0.2);
        }

        // Отображение цифр:
    	// Все значения до 10. - показываются с точностью до десятых (7.5 4.0)
    	// Все значения больше 10 - целыми (21 вместо 21.5 и т.д.)
        function num2str(n) {
            if (n < 10 && n > -10) return n.toFixed(1);
            return '' + Math.round(n);
        }

        var monthNames = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];

        function date2month(d) {
            return monthNames[d.getUTCMonth()];
        }

        var dayNames = ['Su', 'Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa'];

        function date2day(d) {
            return dayNames[d.getUTCDay()];
        }

        function moonElement(moonTimestamp, dayEndTimestamp, dayCells, moonSVGForDay, position, lastTimestamp) {
            //cell size in hours
            let cellSize = (opts.everyhour ? 1 : 3);
            //shifts dayEndTimestamp to 00:00 (instead of сell timestamp)
            let trueDayEndTimestamp = dayEndTimestamp + (cellSize * 60 * 60);
            //offset in hours
            let offset = (trueDayEndTimestamp - moonTimestamp)/(60 * 60);
            //length of shown day in table
            let dayLength = dayCells * cellSize
            let nextDayLength = 24;
            //moon event shift relative to 00:00
            let offsetAspect = 1 - (offset / dayLength);
            //if moon event happens before 00:00
            if (offset < 0){
                offsetAspect = 1 + (Math.abs(offset)/nextDayLength);
            }
            //moon in highest point on sky
            let moonTop = 0;
            //moon rise/set
            if (position === 'rise' || position === 'set') {moonTop = 16}
            //time data for testing purpose
            let moonTime = new Date(moonTimestamp * 1000);
            //html div assembly
            let element = '<div style="position: absolute; height: 32px; width: 32px; top: ' + moonTop + '; left: ' +
                    (offsetAspect * 100) + '%" data-position="' + position + '" data-time="' + moonTime + '">'
                    + moonSVGForDay + '</div>';

            //if moon event happens after forecast grid (for the last cell)
            if (moonTimestamp > lastTimestamp || moonTimestamp == -1) {element = '';}

            return element;
        }

        function drawSVGMoon(moonPhaseData) {
            //moon phases sync
            let moonPhasePercent;
            if (moonPhaseData < 0) {moonPhasePercent = moonPhaseData * -0.5}
            else {moonPhasePercent = (moonPhaseData * -0.5) + 100}
            let coordinate = Math.round(moonPhasePercent * 0.84);

            let pointX = 0;
            let pointY = 0;
            let pointX2 = 0;
            let pointY2 = 0;
            let sweepFlag = 0;

            if (moonPhasePercent >= 0 && moonPhasePercent < 25) {
                // phase 1: &&-0&
                pointX = -5 + coordinate;
                pointY = coordinate;
                pointX2 = pointX;
                pointY2 = 32 - pointY;
                sweepFlag = 0;
            } else if (moonPhasePercent >= 25 && moonPhasePercent <= 50) {
                // phase 2: 0&-00
                pointX = -5 + coordinate;
                pointY = 42 - coordinate;
                pointX2 = pointX;
                pointY2 = 32 - pointY;
                sweepFlag = 0
            } else if (moonPhasePercent >= 50 && moonPhasePercent < 75) {
                // phase 3: 00-&0
                sweepFlag = 1;
                coordinate = coordinate - 42;
                pointX = -5 + coordinate;
                pointY = coordinate;
                pointX2 = pointX;
                pointY2 = 32 - pointY;
            } else if (moonPhasePercent >= 75 && moonPhasePercent < 100) {
                // phase 4: &0-&&
                sweepFlag = 1;
                coordinate = coordinate - 42;
                pointX = -5 + coordinate;
                pointY = 42 - coordinate;
                pointX2 = pointX;
                pointY2 = 32 - pointY;
            }

            //Dark Side of the Moon
            //                                       upper Bezier Curve          bottom Bezier Curve
            let moonSVGPathDark = '<path d="M 16 0 C ' + pointX + ' ' + pointY + ' ' + pointX2 + ' ' + pointY2 +
                //                 Arc Curve left/right
                ' 16 32 A 1 1 0 0 ' + sweepFlag + ' 16 0 Z" style="fill: #5e768e;"/>'
            //background circle
            let moonSVGPathLight = '<path style="fill: #cfd9e0; stroke-width: 2px;" d="M 16 1 A 1 1 0 0 0 16 31 A 1 1 0 0 0 16 1"/>';
            // outer ring
            let moonSVGPathOut = '<path style="fill: none; stroke: #5e768e; stroke-width: 2px;" d="M 16 1 A 1 1 0 0 0 16 31 A 1 1 0 0 0 16 1"/>';
            // SVG assembly
            let moonPhaseSVG = '<svg style="width: 80%; padding: 3px" xmlns="http://www.w3.org/2000/svg" id="Layer_1" data-name="Layer 1" viewBox="0 0 32 32">'
               + moonSVGPathLight + moonSVGPathDark + moonSVGPathOut + '</svg>';
            return  moonPhaseSVG;
        }

        //units dictoinary
        let unitDict = {
            temp: {
                0: 'C',
                1: 'F'
            },
            wind: {
                0: 'knots',
                1: 'bft',
                2: 'm/s',
                3: 'mph',
                4: 'km/h'
            },
            height: {
                0: 'm',
                1: 'ft'
            },
            time: {
                0: '24h',
                1: '12h'
            },
            pressure: {
                0: 'hPa',
                1: 'inHg'
            }
        }

        function setCookie(type, value) {
            if (type === 'temp') {
                unitsShorten[0] = value;
            } else if (type === 'wind') {
                unitsShorten[1] = value;
            } else if (type === 'height') {
                unitsShorten[2] = value;
            } else if (type === 'time') {
                unitsShorten[3] = value;
            } else if (type === 'pressure') {
                unitsShorten[4] = value;
            } else if (type === 'model') {
                unitsShorten[5] = value;
            }
            document.cookie = "forecast_widget_units=" + unitsShorten + "; expires=Fri, 31 Dec 9999 23:59:59 GMT; path=/";
            return true;
        }

        var cookie = document.cookie.split('; ').find(row => row.startsWith('forecast_widget_units'));
        let c_vals = cookie ? cookie.split('=')[1] : 0;
        let regexWind = /knots|bft|m\/s|mph|km\/h/g;

        //assign value only if set
        if (c_vals) {
            c_vals = c_vals.split(',');
            if (c_vals.length >= 5) {
                if (c_vals[0] >= 0) {this.tempUnit = unitDict.temp[c_vals[0]]};
                if (c_vals[1] >= 0) {this.windUnit = unitDict.wind[c_vals[1]];};
                if (c_vals[2] >= 0) {this.heightUnit = unitDict.height[c_vals[2]]};
                if (c_vals[3] >= 0) {this.timeUnit = unitDict.time[c_vals[3]]};
                if (c_vals[4] >= 0) {this.pressureUnit = unitDict.pressure[c_vals[4]]};
                if (c_vals[5]) {this.model = c_vals[5]};
            } else if (units.length == 1) {
                //compatibility with old cookies
                if (units[0] >= 0 && units[0] < 5) {
                    this.windUnit = unitDict.wind[units[0]]
                }
            } else if (regexWind.test(unit)) {
                //compatibility with older cookies
                this.windUnit = unit;
            }
        }

        // m/s заменяются на mph, потом на кмч, потом на узлы, потом на бофорты и потом обратно на m/s
        window.updateWind = function (spotID, increment = true) {
            var units = Object.values(unitDict.wind);
            var tdws = document.querySelectorAll('#widget_' + spotID + ' .windywidgetwindSpeed td');
            var tdwg = document.querySelectorAll('#widget_' + spotID + ' .windywidgetwindGust td');
            var tdcws = document.querySelectorAll('#widget_' + spotID + ' #windywidgetcurwindSpeed');
            var tdcwsu = document.querySelectorAll('#widget_' + spotID + ' #windywidgetcurwindspeedUnit');
            let current = units.indexOf(this.windUnit) % units.length;
            var next = (units.indexOf(this.windUnit) + 1) % units.length;
            increment && (this.windUnit = units[next]) && (current = next);

            tdws[0].innerHTML = 'Wind speed (' + units[current] + ')';
            tdwg[0].innerHTML = 'Wind gusts (' + units[current] + ')';
            tdcwsu[0].innerHTML = units[current];
            var convert = [
                function (mps) {
                    return mps * 1.94;
                },
                function (mps) {
                    if(mps <= 0.3) { return 0; } else
                    if(mps <= 1.6) { return 1; } else
                    if(mps <= 3.4) { return 2; } else
                    if(mps <= 5.5) { return 3; } else
                    if(mps <= 8.0) { return 4; } else
                    if(mps <= 10.8) { return 5; } else
                    if(mps <= 13.9) { return 6; } else
                    if(mps <= 17.2) { return 7; } else
                    if(mps <= 20.8) { return 8; } else
                    if(mps <= 24.5) { return 9; } else
                    if(mps <= 28.5) { return 10; } else
                    if(mps <= 32.7) { return 11; } else
                    { return 12; }
                },
                function (mps) {
                    return mps;
                },
                function (mps) {
                    return mps * 2.24;
                },
                function (mps) {
                    return mps * 3.6;
                },

            ];
            tdcws_val = convert[current](parseFloat(tdcws[0].dataset.value));
            tdcws[0].innerHTML = units[current] != 'bft' ? num2str(tdcws_val) : tdcws_val;
            for (var i = 1; i < tdws.length; i++) {
                var tdws_val = convert[current](parseFloat(tdws[i].dataset.value));
                var tdwg_val = convert[current](parseFloat(tdwg[i].dataset.value));
                if(units[current] != 'bft') {
                    tdws_val = num2str(tdws_val);
                    tdwg_val = num2str(tdwg_val);
                }
                tdws[i].innerHTML = tdws_val;
                tdwg[i].innerHTML = tdwg_val;
            }
            setCookie('wind', current);
        };

        // а если по температуре клац - то она становится фаренгейтом а потом цельсием и так по кругу
        window.updateTemp = function (spotID, increment = true) {
            var units = Object.values(unitDict.temp);
            var td = document.querySelectorAll('#widget_' + spotID + ' .windywidgetairTemp td');
            var next = (units.indexOf(this.tempUnit) + 1) % units.length;
            let current = (units.indexOf(this.tempUnit));
            increment && (this.tempUnit = units[next]) && (current = next);

            td[0].innerHTML = 'Temperature (&deg;' + units[current] + ')';
            var convert = [
                function (c) {
                    return c;
                },
                function (c) {
                    return c * 1.8 + 32;
                }
            ];
            for (var i = 1; i < td.length; i++) {
                if(this.themeMode == 'white'){
                    td[i].innerHTML = num2str(convert[current](parseFloat(td[i].dataset.value)));
                } else {
                    td[i].innerHTML = num2str(convert[current](parseFloat(td[i].dataset.value))) + '&deg;';
                }
            }
            setCookie('temp', current);
        };

        // а если по высоте клац - то она становится футом а потом метром и так по кругу
        window.updateHeight = function (spotID, increment = true) {

            //[meters, feet] 0-1
            var units = Object.values(unitDict.height);
            //waves
            var tdwh = document.querySelectorAll('#widget_' + spotID + ' .windywidgetwavesheight td');
            //tides
            var tdti = document.querySelectorAll('#widget_' + spotID + ' .windywidgettideInfo');

            //0-1 to meters-feet
            var next = (units.indexOf(this.heightUnit) + 1) % units.length;
            let current = (units.indexOf(this.heightUnit));
            increment && (this.heightUnit = units[next]) && (current = next);

            //waves height
            tdwh[0].innerHTML = 'Waves height (' + units[current] + ')';
            //tides height
            document.querySelector('#widget_' + spotID + ' .windywidgettides td').innerHTML = 'Tides (' + units[current] + ')';

            //return [meters, feet]
            var convert = [
                //metres to meters
                function (c) {
                    return c;
                },

                //metres to feet
                function (c) {
                    return c * 3.28084;
                }
            ];

            //for every td in WAWES height
            for (var i = 1; i < tdwh.length; i++) {
                if(tdwh[i].dataset.value != 'false') {
                    tdwh[i].innerHTML = num2str(convert[current](parseFloat(tdwh[i].dataset.value)));
                }
            }
            //for every td in TIDES height
            for (var i = 0; i < tdti.length; i++) {
                tdti[i].innerHTML = tdti[i].dataset.time + ' ' + num2str(convert[current](parseFloat(tdti[i].dataset.height))) + units[current];

            }

            //set cookie for height in 0-1 format
            setCookie('height', current);
        };

        window.showTides = function(show, spotID) {
    	    var els = document.querySelectorAll('#widget_' + spotID + ' .windywidgettides, ' +
                '#widget_' + spotID + ' .windywidgettideInfo');
    		for (var i=0; i<els.length; ++i) {
    		  els[i].style.display = show ? '' : 'none';
    		}
        }

        window.showWaves = function(show, spotID) {
    	    var els = document.querySelectorAll('#widget_' + spotID + ' .windywidgetwaves,' +
    	    	'#widget_' + spotID + ' .windywidgetwavesheight,' +
    	    	'#widget_' + spotID + ' .windywidgetwavesperiod');
    		for (var i=0; i<els.length; ++i) {
    		  els[i].style.display = show ? '' : 'none';
    		}
        }

        var showForecast = function(data, tides, spotID, spotInfo, moonData) {
            var canvas = document.createElement('canvas');
            canvas.width = 32;
            canvas.height = 45;
            var context = canvas.getContext('2d');

            var dates = '<table><tr>', dates_i = 0;
            var days = '<tr class="windywidgetdays" id="windyWidgetDays">';
            var hours = '<tr class="windywidgethours">';
            var windDirection = '<tr class="windywidgetwindDirection id-wind-direction">';
            var windSpeed = '<tr class="windywidgetwindSpeed id-wind-speed">';
            var windGust = '<tr class="windywidgetwindGust id-wind-gust">';
            var airTemp = '<tr class="windywidgetairTemp id-air-temp">';
            var airPres = '<tr class="windywidgetairPres">';
            var cloudsHigh = '<tr class="windywidgetclouds id-clouds">';
            var cloudsMid = '<tr class="windywidgetclouds id-clouds">';
            var cloudsLow = '<tr class="windywidgetclouds id-clouds">';
            var precipitation = '<tr class="windywidgetprecipitation id-precipitation">';
            var waves = '<tr class="windywidgetwaves id-waves-direction">';
            var wavesHeight = '<tr class="windywidgetwavesheight id-waves-height">';
            var wavesPeriod = '<tr class="windywidgetwavesperiod id-waves-period">';
            var moonPhase = '<tr class="windywidgetmoonphase id-moon-phase">';
            let lineGraphPresipitation = '<tr id="percipCell">';
            let lineGraphPressure = '<tr id="pressureCell">';

            var sliderGradient = [];

            var hasWavesData = false;
            var hasTidesData = false;


            /************
             * Waves
             ************/

            // max height for waves
            // it's better to have it in spotInfo
            var maxWavesHeight = 0;
            for (var i = 0; i < data.length; i++) {
                if(data[i].wavesHeight > maxWavesHeight)
                    maxWavesHeight = data[i].wavesHeight;
            }


            /************
             * Tides
             ************/

             tideInfo = [];
             tideInfoHtml = '';

             if(tides) {

                // settings
                var tideUpColor = (this.themeMode == 'white') ? '#548BC2' : '#2f9328';
                var tideDownColor = (this.themeMode == 'white') ? '#d9ecff' : '#2e688e';
                var tideCurveColor = (this.themeMode == 'white') ? '#577CAD' : '#0070c0';
                var tideTextColor = (this.themeMode == 'white') ? '#758B9B' : '#bbb';
                var tideTextFont = '10px Arial';

                // flip values
                for(var i = 1; i < tides.length - 1; i++) {
                    tides[i].tideHeight = -tides[i].tideHeight;
                }

                // remove double values
                for(var i = 1; i < tides.length - 1; i++) {
                    if(tides[i].tideHeight == tides[i+1].tideHeight) {
                        tides.splice(i, 1);
                    }
                }

                // finding peaks
                var tidePeaks = [];
                var side = tides[0].tideHeight >= 0;
                var prevSide = side;
                var foundPeak = false;
                for(var i = 1; i < tides.length - 1; ++i) {
                    side = tides[i].tideHeight >= 0;
                    if (foundPeak && side != prevSide) {
                        foundPeak = false;
                        prevSide = side;
                    }
                    if (!foundPeak &&
                        Math.abs(tides[i-1].tideHeight) <= Math.abs(tides[i].tideHeight) &&
                        Math.abs(tides[i].tideHeight) > Math.abs(tides[i+1].tideHeight)
                    ) {
                        foundPeak = true;
                        tidePeaks.push(tides[i]);
                        if(!hasTidesData) {
        	                hasTidesData = Math.abs(Math.round(tides[i].tideHeight * 10, 1)) > 0;
        	            }
                    }
                }

                // init
                var tideCnv = document.createElement('canvas');
                tideCnv.width = data.length * canvas.width - 10;
                tideCnv.height = 100;
                var canvasOffset = 5;
                var tideCtx = tideCnv.getContext('2d');

                // calculate curve
                var tideCurve = [];
                var secs = data[data.length-1].timestamp - data[0].timestamp;
                var x = x0 = x1 = x2 = x3 = x4 = 0;
                var y = y0 = y1 = y2 = y3 = y4 = 0;
                var prev1 = prev2 = next1 = next2 = 0;

                for(var i=0; i<tides.length; i++) {
                    x = tideCnv.width * ((tides[i].timestamp - data[0].timestamp) / secs) + canvasOffset;
                    y = tides[i].tideHeight * 30 + 50;
                    tides[i].x = x;
                    tides[i].y = y;
                    tideCurve.push({"x": x, "y": y});

                }

                // draw regions
                tideCtx.fillStyle = '#28465a';
                tideCtx.beginPath();
                tideCtx.moveTo(0, tideCnv.height/2);

                var x = 0, y = 0;
                var up = upSwitch = false;
                var tl = tideCurve.length

                for(var i=0; i<tl; i++) {

                    x = tideCurve[i].x;
                    y = tideCurve[i].y;
                    up = (i < tl-1 && y > tideCurve[i+1].y);

                    if(up != upSwitch && i<tl-1 && i>1 && y != tideCurve[i-1].y) {

                        tideCtx.fillStyle = up ? tideDownColor : tideUpColor;
                        tideCtx.lineTo(tideCurve[i+1].x, tideCurve[i+1].y);
                        tideCtx.lineTo(tideCurve[i+1].x, tideCnv.height/2);
                        tideCtx.fill();
                        tideCtx.closePath();
                        tideCtx.beginPath();
                        tideCtx.moveTo(x, tideCnv.height/2);
                        upSwitch = up;

                    }

                    tideCtx.lineTo(x, y);
                }

                tideCtx.lineTo(tideCnv.width, tideCnv.height/2);
                tideCtx.fill();
                tideCtx.closePath();

                // dashed center line
                tideCtx.strokeStyle = '#28465a';
                tideCtx.setLineDash([5]);
                tideCtx.beginPath();
                tideCtx.moveTo(0, tideCnv.height/2);
                tideCtx.lineWidth = 1;
                tideCtx.lineTo(tideCnv.width, tideCnv.height/2);
                tideCtx.stroke();
                tideCtx.setLineDash([]);

                // main stroke
                tideCtx.beginPath();
                for(var i=0; i<tl; i++) {
                    tideCtx.lineTo(tideCurve[i].x, tideCurve[i].y);
                }
                tideCtx.strokeStyle = tideCurveColor;
                tideCtx.lineWidth = 2;
                tideCtx.stroke();
                tideCtx.closePath();

                // info
                for(var i=0; i<tidePeaks.length-1; i++) {
                    if(tidePeaks[i].x) {
                        var th = tidePeaks[i].tideHeight;
                        var time = new Date(tidePeaks[i].timestamp * 1000);
                        time.setHours(time.getHours() + parseInt(spotInfo.spotGmtOffset) + time.getTimezoneOffset() / 60);
                        time = time.getHours() + ':' + ('0' + time.getMinutes()).slice(-2);
                        var height = Number(-th).toFixed(1);
                        let tidePeakShift = (fields.includes('moon-phase') ? [-19,-36,66,-24] : [12,-5,98,8]);
                        var top = Math.round(tidePeaks[i].y) + (th > 0 ? tidePeakShift[0] : tidePeakShift[1]);
                        if(top > tidePeakShift[2]) top = tidePeakShift[2];
                        if(top < tidePeakShift[3]) top = tidePeakShift[3];
                        tideInfoHtml += '<div class="windywidgettideInfo id-tides" ' +
                                        'style="left:' + (Math.round(tidePeaks[i].x) - 25) + 'px; top:' + top + 'px;" ' +
                                        'data-time="' + time + '" data-height="' + height + '"></div>';
                    }
                }

            }
            var tidesHtml = '<tr class="windywidgettides id-tides">';

            var tmpCnv = document.createElement('canvas');
            tmpCnv.width = 32;
            tmpCnv.height = 100;
            var tmpCtx = tmpCnv.getContext('2d');
            var dows = ['SUN', 'MON', 'TUE', 'WED', 'THU', 'FRI', 'SAT'];
            let m = 0;
            let DataForDay = [];
            let last_i = 0;
            let hoursCount = 1;
            let allHours = [];
            // main loop
            for (var i = 0, j = 1, curDay = 0; i < data.length; i++) {

                var date1 = new Date(data[i].timestamp * 1000);
                date1.setHours(date1.getHours() + parseInt(spotInfo.spotGmtOffset));
                var date2 = new Date(data[Math.min(i + 1, data.length - 1)].timestamp * 1000);
                date2.setHours(date2.getHours() + parseInt(spotInfo.spotGmtOffset));

                var dayEnds = date1.getUTCDay() != date2.getUTCDay();

                // days
                if (dayEnds || (i == data.length - 1)) {
                    var date0 = new Date(data[dates_i].timestamp * 1000);
                    var newMonth = (i == 0) || (date0.getUTCMonth() != date1.getUTCMonth());

                    dates += '<td width="' + Math.floor(100 * (i + 1 - dates_i) / data.length) + '%">' +
                        (newMonth ? date2month(date1) + '&nbsp;' : '') + date1.getUTCDate() + '</td>';
                    dates_i = i;

                    textDate = dows[date1.getUTCDay()] + ', ' + String(date2month(date1)).toUpperCase() + ' ' + date1.getUTCDate();

                    days += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') +
                        ' colspan="' + j + '" style="width: ' + (32 * j) + 'px; white-space:nowrap"><div>';

                    let moonSVGForDay = drawSVGMoon(moonData[m].phase);
                    let lastTimestamp = data[data.length - 1].timestamp;

                    moonPhase += '<td class="' + (dayEnds ? 'windywidgetdayEnds' : '') + '" style="position: relative; height: 32px" colspan="' + j + '"z>' +
                        moonElement(moonData[m].rise, data[i].timestamp, j, moonSVGForDay, 'rise', lastTimestamp) +
                        moonElement(moonData[m].transit, data[i].timestamp, j, moonSVGForDay, 'high', lastTimestamp) +
                        moonElement(moonData[m].set, data[i].timestamp, j, moonSVGForDay, 'set', lastTimestamp) +
                        '</td>';

                    if(curDay == 0) {
                        days += 'TODAY';
                    } else if(curDay == 1) {
                        days += 'TOMORROW';
                    } else {
                        days += (j > 1) ? textDate : '';
                    }

                    last_i = i;

                    m++;
                    curDay++;
                    days += '</div></td>';
                    j = 1;
                    allHours.push(hoursCount);
                    hoursCount = 1;
                } else {
                    j++;
                    hoursCount++;
                }

                // hours
                var night = '';
                if (data[i].nightEnd > data[i].nightStart) {
                    if (data[i].nightEnd - data[i].nightStart == 1) {
                        night = ' id="windywidgetnight"';
                    } else {
                        if (data[i].nightEnd < 1) {
                            var pct = Math.floor(100 * data[i].nightEnd);
                            night = ' style="background: linear-gradient(to right, #203747, #203747 ' + pct + '%, transparent ' + (pct + 1) + '%, transparent)"';
                        } else {
                            var pct = Math.floor(100 * data[i].nightStart);
                            night = ' style="background: linear-gradient(to right, transparent, transparent ' + pct + '%, #203747 ' + (pct + 1) + '%, #203747)"';
                        }
                    }
                }
                hours += '<td ' + 'data-ts="' + data[i].timestamp + '"' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + night + '>' + ('0' + date1.getUTCHours()).substr(-2) + '</td>';

                // wind direction
                windDirection += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') +
                    ' style="transform: rotate(' + data[i].windDirection + 'deg); -webkit-transform: rotate(' + data[i].windDirection + 'deg)">&#8593;</td>';

                // wind speed
                var windSpeedColor1 = colorForWindSpeed(0.5 * (data[i].windSpeed + data[Math.max(i - 1, 0)].windSpeed));
                var windSpeedColor2 = colorForWindSpeed(data[i].windSpeed);
                var windSpeedColor3 = colorForWindSpeed(0.5 * (data[i].windSpeed + data[Math.min(i + 1, data.length - 1)].windSpeed));
                windSpeed += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    windSpeedColor1 + ', ' + windSpeedColor2 + ', ' + windSpeedColor3 + ')" data-value="' + data[i].windSpeed + '"></td>';

                sliderGradient.push(windSpeedColor2);

                // wind gust
                var windGustColor1 = colorForWindSpeed(0.5 * (data[i].windGust + data[Math.max(i - 1, 0)].windGust));
                var windGustColor2 = colorForWindSpeed(data[i].windGust);
                var windGustColor3 = colorForWindSpeed(0.5 * (data[i].windGust + data[Math.min(i + 1, data.length - 1)].windGust));
                windGust += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    windGustColor1 + ', ' + windGustColor2 + ', ' + windGustColor3 + ')" data-value="' + data[i].windGust + '"></td>';

                // air temperature
                var airTempColor1 = colorForTemperature(0.5 * (data[i].airTemp + data[Math.max(i - 1, 0)].airTemp));
                var airTempColor2 = colorForTemperature(data[i].airTemp);
                var airTempColor3 = colorForTemperature(0.5 * (data[i].airTemp + data[Math.min(i + 1, data.length - 1)].airTemp));
                airTemp += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    airTempColor1 + ', ' + airTempColor2 + ', ' + airTempColor3 + ')" data-value="' + data[i].airTemp + '"></td>';

                // air pressure
                var airPresColor1 = colorForPressure(0.5 * (data[i].airPres + data[Math.max(i - 1, 0)].airPres));
                var airPresColor2 = colorForPressure(data[i].airPres);
                var airPresColor3 = colorForPressure(0.5 * (data[i].airPres + data[Math.min(i + 1, data.length - 1)].airPres));
                airPres += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    airPresColor1 + ', ' + airPresColor2 + ', ' + airPresColor3 + ')">' + data[i].airPres + '</td>';

                // cloud high
                var CloudsHighColor1 = colorForClouds(0.5 * (data[i].cloudsHigh + data[Math.max(i - 1, 0)].cloudsHigh));
                var CloudsHighColor2 = colorForClouds(data[i].cloudsHigh);
                var CloudsHighColor3 = colorForClouds(0.5 * (data[i].cloudsHigh + data[Math.min(i + 1, data.length - 1)].cloudsHigh));
                cloudsHigh += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    CloudsHighColor1 + ', ' + CloudsHighColor2 + ', ' + CloudsHighColor3 + ')" data-value="' + data[i].cloudsHigh + '"></td>';


                // cloud medium
                var CloudsMidColor1 = colorForClouds(0.5 * (data[i].cloudsMid + data[Math.max(i - 1, 0)].cloudsMid));
                var CloudsMidColor2 = colorForClouds(data[i].cloudsMid);
                var CloudsMidColor3 = colorForClouds(0.5 * (data[i].cloudsMid + data[Math.min(i + 1, data.length - 1)].cloudsMid));
                cloudsMid += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    CloudsMidColor1 + ', ' + CloudsMidColor2 + ', ' + CloudsMidColor3 + ')" data-value="' + data[i].cloudsMid + '"></td>';


                // cloud low
                var CloudsLowColor1 = colorForClouds(0.5 * (data[i].cloudsLow + data[Math.max(i - 1, 0)].cloudsLow));
                var CloudsLowColor2 = colorForClouds(data[i].cloudsLow);
                var CloudsLowColor3 = colorForClouds(0.5 * (data[i].cloudsLow + data[Math.min(i + 1, data.length - 1)].cloudsLow));
                cloudsLow += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background: linear-gradient(to right, ' +
                    CloudsLowColor1 + ', ' + CloudsLowColor2 + ', ' + CloudsLowColor3 + ')" data-value="' + data[i].cloudsLow + '"></td>';


                // precipitation
                t = data[i].precipitation;

                var icon = data[i].isPrecipFrozen ? 'ice' : 'wet';
                if (t <= 0.3) {
                    icon += 0;
                } else if (t <= 1.0) {
                    icon += 1;
                } else if (t <= 3.0) {
                    icon += 2;
                } else {
                    icon += 3;
                }

                var themePrefix = '';

                var drop_icon = t < 0.2 ? 0 : Math.ceil(t);
                if(drop_icon > 3) drop_icon = 3;

                if(this.themeMode == 'white'){
                    themePrefix = '-white';
                }


                precipitation += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + '>' +
                    '<img src="//windy.app/widget/img/drop' + drop_icon + themePrefix + '.svg" style="width:15px !important;height: 18px !important;">' +
                    ((t > 0.2) ? t.toFixed(1) : '-') +
                    '</td>';

                // waves
                if(data[i].wavesHeight) hasWavesData = true;
                var wavesHeightVal = data[i].wavesHeight === false ? '0' : (data[i].wavesHeight / maxWavesHeight * 100);
                waves += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background-size: 80% ' + wavesHeightVal + '%">' +
                    '<div style="transform: rotate(' + ((data[i].wavesDirection + 180) % 360) + 'deg); -webkit-transform: rotate(' + ((data[i].wavesDirection + 180) % 360) + 'deg)">' +
                    (data[i].wavesHeight ? '&#8593;' : '') +
                    '</div></td>';

                wavesHeight += '<td  data-value="' + data[i].wavesHeight + '"' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + '>-</td>';

                var dwp = data[i].wavesPeriod ? data[i].wavesPeriod + '\'' : '-';
                wavesPeriod += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + '>' + dwp + '</td>';

                // tides
                if(tides) {
                    var imageData = tideCtx.getImageData(i * tmpCnv.width, 0, (i+1) * tmpCnv.width, tmpCnv.height);
                    tmpCtx.putImageData(imageData, 0, 0);
                    tidesHtml += '<td' + (dayEnds ? ' class="windywidgetdayEnds"' : '') + ' style="background-image: url(' + tmpCnv.toDataURL() + ')"></td>';
                }

            }

            dates += '</tr></table>';

            days += '</tr>';
            hours += '</tr>';
            windDirection += '</tr>';
            windSpeed += '</tr>';
            windGust += '</tr>';
            airTemp += '</tr>';
            airPres += '</tr>';
            precipitation += '</tr>';
            waves += '</tr>';
            wavesHeight += '</tr>';
            wavesPeriod += '</tr>';
            tidesHtml += '</tr>';
            moonPhase += '</tr>';

            document.querySelector("#widget_" + spotID + " #windywidgettable").innerHTML = '<table id="cellsTable">' +
                days +
                hours +
                windDirection +
                windSpeed +
                windGust +
                airTemp +
                //airPres +
                cloudsHigh +
                cloudsMid +
                cloudsLow +
                lineGraphPresipitation +
                lineGraphPressure +
                waves +
                wavesHeight +
                wavesPeriod +
                tidesHtml +
                moonPhase +
                '</table>' +
                tideInfoHtml;

            document.querySelector('#widget_' + spotID + ' #windywidgetdates').innerHTML = dates;


    // NEW PRECIPITATION
            let pressCell = document.querySelector('#pressureCell');

            let precipCell = document.querySelector('#percipCell');
            let precipCellTitle = document.querySelector('#percipCellName');

            precData = data.map((item) => Math.round(100*item.precipitation));
            tempData = data.map((item) => item.airTemp);

                //temporarily solution to allign everything
                let precipCellHeight = 50;
                precipCellTitle.style.height = precipCellHeight + 'px';
                precipCell.style.height = precipCellHeight + 'px';

                // add Days Separrators To - name this  in other words
                //


            if (this.fields.includes('precipitation')) {
                precipAssembler(precData, tempData, precipCell);
                addDaysBordersTo(precipCell, allHours)
            }

    // SHARE BUTTON
    //object share with size parametrs and iconTest

            if (this.share) {
                let headerLinks = document.querySelector('#headerLinks');
                addShareButtonIn(headerLinks, this.share);
            }

    // ANALYTICS

            //log hover over tbble in view
            const tableInView = document.getElementById('windywidgettable');
            logHoverOver(tableInView, 'FORECAST');



            //set units from widget params and check if they are valid
            if (/C|F/.test(node.dataset.tempunit)) {
                this.tempUnit = node.dataset.tempunit;
            }

            if (/knots|bft|m\/s|mph|km\/h/.test(node.dataset.windunit)) {
                this.windUnit = node.dataset.windunit;
            }

            if (/m|ft/.test(node.dataset.heightunit)) {
                this.heightUnit = node.dataset.heightunit;
            }

            updateWind(spotID, false);
            updateTemp(spotID, false);
            updateHeight(spotID, false);

            showTides(tides.length && hasTidesData && !opts.hidetides, spotID);
            showWaves(hasWavesData && !opts.hidewaves, spotID);

            // slider
            document.querySelector('#widget_' + spotID + ' #windywidgetslider').style.background = 'linear-gradient(to right, ' + sliderGradient.join(', ') + ')';
            sliderGradient = null;

            var sliderKnob = document.querySelector('#widget_' + spotID + ' #windywidgetslider div');
            let table = document.querySelector('#widget_' + spotID + ' #windywidgettable table');

            function alignDays() {
                var days = document.querySelector('#widget_' + spotID + ' #windywidgettable .windywidgetdays td div');
                for (var i = 0; i < days.length; i++) {
                    days[i].style.left = Math.floor(
                            Math.min(Math.max(10,
                                10 + table.parentNode.scrollLeft - (days[i].parentNode.offsetLeft - days[0].parentNode.offsetLeft)
                            ), days[i].parentNode.offsetWidth - days[i].offsetWidth - 10)
                        ) + 'px';
                }
            }

            function slide() {
                table.parentNode.scrollLeft = (table.offsetWidth - table.parentNode.offsetWidth) * sliderKnob.offsetLeft / (sliderKnob.parentNode.offsetWidth - sliderKnob.offsetWidth);
                alignDays();
            }

            function down(e) {
                e.preventDefault();
                var extra = sliderKnob.offsetLeft - (e.touches ? e.touches[0].pageX : e.pageX);

                function move(e) {
                    sliderKnob.style.left = Math.max(0, Math.min(sliderKnob.parentNode.offsetWidth - sliderKnob.offsetWidth,
                            (e.touches ? e.touches[0].pageX : e.pageX) + extra
                        )) + 'px';

                    slide();
                }

                window.addEventListener('mousemove', move);
                window.addEventListener('touchmove', move);
                function up() {
                    window.removeEventListener('mousemove', move);
                    window.removeEventListener('touchmove', move);
                }

                window.addEventListener('mouseup', up);
                window.addEventListener('touchend', up);
                window.addEventListener('touchcancel', up);
            }

            sliderKnob.addEventListener('mousedown', down);
            sliderKnob.addEventListener('touchstart', down);

            sliderKnob.parentNode.addEventListener('mousedown', function (e) {
                if (e.target != sliderKnob) {
                    sliderKnob.style.left = Math.max(0, Math.min(sliderKnob.parentNode.offsetWidth - sliderKnob.offsetWidth,
                            (e.touches ? e.touches[0].pageX : e.pageX) - sliderKnob.parentNode.offsetLeft - 0.5 * sliderKnob.offsetWidth
                        )) + 'px';

                    slide();
                }
            })

            function updateSlider() {
                // update slider according to the table
                sliderKnob.offsetLeft = 50;
                sliderKnob.style.left = Math.max(0, Math.min(sliderKnob.parentNode.offsetWidth - sliderKnob.offsetWidth,
                        table.parentNode.scrollLeft * (sliderKnob.parentNode.offsetWidth - sliderKnob.offsetWidth) / (table.offsetWidth - table.parentNode.offsetWidth)
                    )) + 'px';

                const maxScroll = table.offsetWidth - table.parentNode.offsetWidth - 5;
                if(table.parentNode.scrollLeft > maxScroll && !fullLog) {
                    apmlitudeLogForecast('FULL_SCROLL');
                    fullLog = true;
                } else if (table.parentNode.scrollLeft > (maxScroll / 2)  && !halfLog) {
                    apmlitudeLogForecast('HALF_SCROLL');
                    halfLog = true;
                }
            }
            document.querySelector('#widget_' + spotID + ' #windywidgettable').addEventListener('scroll', updateSlider);

            if (!/Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent)) {
                table.style.userSelect = 'none';
                table.style.webkitUserSelect = 'none';
                table.style.cursor = 'grab';
            }

            var oldScroll = 0,
                oldMouseX = 0,
                isGrabbed = false,
                el = document.getElementById('windywidgettable');

            table.onmousedown = (event) => {
                oldScroll = el.scrollLeft;
                oldMouseX = event.clientX;
                isGrabbed = true;
                table.style.cursor = 'grabbing';
                };

            table.onmousemove = (event) => {
                if(!isGrabbed) return;
                el.scrollLeft += (oldMouseX - event.clientX);
                oldMouseX = event.clientX;
                oldScroll = el.scrollLeft;
            };

            table.onmouseup = (event) => {
                isGrabbed = false;
                table.style.cursor = 'grab';
            };

            // when mouse leaves the table, stop grabbing
            const widgetTableInView = document.getElementById('windywidgettable');

            widgetTableInView.onmouseleave = (event) => {
                isGrabbed = false;
                table.style.cursor = 'grab';
            };

            // when clicking right mouse button, stop grabbing
            table.oncontextmenu = (event) => {
                isGrabbed = false;
                table.style.cursor = 'grab';
            };

            function resize() {
                sliderKnob.style.width = Math.floor(100 * table.parentNode.offsetWidth / table.offsetWidth) + '%';
                sliderKnob.style.left = Math.min(sliderKnob.offsetLeft, sliderKnob.parentNode.offsetWidth - sliderKnob.offsetWidth);
                slide();
            }

            window.addEventListener('resize', resize);
            resize();
        };

        //test to compare ip country provider
        let countryMismatch = false;
        this.country_ipapi = getCountryFromIP('https://ipapi.co/json/');
        if (this.country != this.country_ipapi) {
            countryMismatch = this.country + ' | ' + this.country_ipapi;
        }


        window.apmlitudeLogForecast = (type, unit = false, param = false) => {
            let country = this.country;
            if (countryMismatch) {
                country = countryMismatch;
            }
            //if app versions does not match, log it
            let appVersionCombined = this.appVersion;
            if (this.appVersion != this.appVersionManual) {
                appVersionCombined = this.appVersion + ' | ' + this.appVersionManual;
            }

            try {
                let chosenUnit = false;
                if (unit) {
                    if (unit == 'WIND') {
                        chosenUnit = this.windUnit;
                    } else if (unit == 'TEMP') {
                        chosenUnit = this.tempUnit;
                    } else if (unit == 'HEIGHT') {
                        chosenUnit = this.heightUnit;
                    }
                    amplitude.getInstance('forecast_widget').logEvent(type, {'site': locationHostname, 'CHANGED_UNIT': unit, 'locale_from_lang': navigator.language, 'app_vesion':appVersionCombined, 'ip_country_name': country, 'chosen_unit': chosenUnit});
                } else if (param) {
                    amplitude.getInstance('forecast_widget').logEvent(type, {'site': locationHostname, 'error': param, 'locale_from_lang': navigator.language, 'app_vesion':appVersionCombined, 'ip_country_name': country});
                } else {
                    amplitude.getInstance('forecast_widget').logEvent(type, {'site': locationHostname, 'locale_from_lang': navigator.language, 'app_vesion':appVersionCombined, 'ip_country_name': country});
                }
            } catch(e) {
                console.log('Problem with amplitude log: ', e);
            }
        }

        //if mouse hover more than one second over the element, log it
        let logHoverOver = (element, type) => {
            let timeout;
            element.addEventListener('mouseover', () => {
                addTimeout(1);
                addTimeout(5);
            }, {once: true});



            function addTimeout(seconds = 1) {
                timeout = setTimeout(() => {
                    apmlitudeLogForecast('HOVER_OVER_' + type + '_' + seconds + 's');
                    element.removeEventListener('mouseover', addTimeout);
                }, 1000 * seconds);
            }
        }

        const initModelSelector = () => {
            const modelListElements = document.querySelectorAll('#widget_' + this.spotID + ' #modelList .model-list__item');
            const handleModelClick = (element) => {
                modelListElements.forEach((item) => item.classList.remove('active'));
                element.classList.add('active');
                loadModel(element.dataset.modelName);
            }
            modelListElements.forEach((item) => {
                if (this.model == item.dataset.modelName) {
                    item.classList.add('active');
                }
                item.addEventListener('click', (evt) => {
                    if (evt.target.classList.contains('model-list__item')) {
                        handleModelClick(evt.target);
                    } else {
                        handleModelClick(evt.target.parentElement);
                    }

                })
            });
        }

        const loadModel = (modelName) => {

            this.model = !opts.model && setCookie('model', modelName) ? modelName : opts.model;
            data = document.createElement('script');
            dim();
            data.onload = function(e) {

                hideLoader();
                forecast = window['wf' + opts.appid];
                const widgetData = JSON.parse(forecast.data);
                showHtmlTemplate(forecast.spotInfo, widgetData, node, opts, forecast.model, forecast.updated, forecast.isPro);
                initModelSelector();

                var tides = forecast.tides.length ? JSON.parse(forecast.tides) : false;

                const moonDataForecast = forecast.moonData.solunarData.moonData ? forecast.moonData.solunarData.moonData : false;

                showForecast(widgetData, tides, forecast.spotInfo.spotID, forecast.spotInfo, moonDataForecast);

                //QR Code or links on mobile
                const windyUrl = '//windy.app/widgets-code/forecast';
                const buttonsApps = document.querySelectorAll('.windywidgetApps');
                const QRImgLink = windyUrl + '/img/qr-codes/qr-code-widget.png';
                const sitehostname = document.location.origin;
                const appsURL = 'https://windyapp.onelink.me/VcDy/j5dimbtn?af_channel=' + sitehostname;
                if (isMobile()) {
                    document.getElementById('windywidgetAppsAndroid').href = appsURL;
                    document.getElementById('windywidgetAppsIos').href = appsURL;
                } else {
                    const imgPopUp = document.createElement('img');
                    const wrapPopUp = document.createElement('div');
                    const linkPopUp = document.createElement('a');
                    const infoPopUp = document.createElement('span');
                    const xPopUp = document.createElement('button');
                    const widgetBody = document.querySelector('.windy-forecast');

                    wrapPopUp.classList.add('popup');
                    imgPopUp.src = QRImgLink;
                    linkPopUp.href = appsURL;
                    xPopUp.textContent = '+';
                    infoPopUp.textContent = 'Scan code with camera';

                    linkPopUp.appendChild(imgPopUp);
                    wrapPopUp.appendChild(infoPopUp);
                    wrapPopUp.appendChild(xPopUp);
                    wrapPopUp.appendChild(linkPopUp);
                    addStoreButtons(wrapPopUp, appsURL)
                    widgetBody.appendChild(wrapPopUp);

                    const popup = document.querySelector('.popup');
                    buttonsApps.forEach((button) => {
                        button.addEventListener('click', function (){
                            openPopup(popup);
                        });
                        button.style.cursor = 'pointer';
                    });
                    document.getElementById('windywidgetAppsAndroid').removeAttribute("href");
                    document.getElementById('windywidgetAppsIos').removeAttribute("href");
                    document.querySelector('.windywidgetApps').title = 'SHOW QR CODE';
                    popup.addEventListener('click', function (){closePopup(popup)});
                }

                function addStoreButtons (element, link) {
                    const appStoreImg = 'https://windy.app/img/btn-appstore.svg'
                    const googleStoreImg = 'https://windy.app/img/btn-googleplay.svg'

                    const buttonsWrap = document.createElement('div');
                    const buttonAppStore = document.createElement('img');
                    const buttonPlayStore = document.createElement('img');
                    const buttonLink = document.createElement('a');

                    buttonAppStore.src = appStoreImg;
                    buttonPlayStore.src = googleStoreImg;
                    buttonLink.href = link;

                    buttonsWrap.appendChild(buttonAppStore);
                    buttonsWrap.appendChild(buttonPlayStore);
                    buttonLink.appendChild(buttonsWrap);

                    element.appendChild(buttonLink);
                }

                function openPopup(element) {
                    element.classList.add('active');
                }

                function closePopup(element) {
                    element.classList.remove('active');
                }

                function isMobile() {
                    return /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
                }

                allFields.forEach(item => {
                    if (!fields.includes(item) || ((item!='tides') && !widgetData[0][getBackendName(item)] === '') || ( (item==='tides') && !tides)) {

                        const elem = document.querySelectorAll(`.id-${item}`);
                        elem.forEach(item => {
                            item.style.display = 'none'
                        })
                    }
                })

                if(location.hostname.match(hostname) && !location.href.match(widgetsHref)) {
                    //hide windy logo and app buttons
                    document.querySelector('.windywidgetApps').style.display = 'none';
                    document.querySelector('.windywidgetpoweredBy').style.display = 'none';
                    amplitude.getInstance('forecast_widget').init("adb1a003d84140fa09974fc5d3933ad6");
                } else if (location.href.match(widgetsHref)) {
                    //on /widgets page shows download buttons, but doesn't add analytics
                    amplitude.getInstance('forecast_widget').init("adb1a003d84140fa09974fc5d3933ad6");
                } else {
                    //GA
                    (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
                    (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
                    m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
                    })(window,document,'script','//www.google-analytics.com/analytics.js','ga');
                    ga('create', 'UA-63657223-1', 'auto');
                    ga('send', 'event', 'widget1_show');

                        //Amplitude
                    (function(e,t){var n=e.amplitude||{_q:[],_iq:{}};var r=t.createElement("script")
                    ;r.type="text/javascript"
                    ;r.integrity="sha384-+EO59vL/X7v6VE2s6/F4HxfHlK0nDUVWKVg8K9oUlvffAeeaShVBmbORTC2D3UF+"
                    ;r.crossOrigin="anonymous";r.async=true
                    ;r.src="https://cdn.amplitude.com/libs/amplitude-8.17.0-min.gz.js"
                    ;r.onload=function(){if(!e.amplitude.runQueuedFunctions){
                    console.log("[Amplitude] Error: could not load SDK")}}
                    ;var i=t.getElementsByTagName("script")[0];i.parentNode.insertBefore(r,i)
                    ;function s(e,t){e.prototype[t]=function(){
                    this._q.push([t].concat(Array.prototype.slice.call(arguments,0)));return this}}
                    var o=function(){this._q=[];return this}
                    ;var a=["add","append","clearAll","prepend","set","setOnce","unset","preInsert","postInsert","remove"]
                    ;for(var c=0;c<a.length;c++){s(o,a[c])}n.Identify=o;var u=function(){this._q=[]
                    ;return this}
                    ;var l=["setProductId","setQuantity","setPrice","setRevenueType","setEventProperties"]
                    ;for(var p=0;p<l.length;p++){s(u,l[p])}n.Revenue=u
                    ;var d=["init","logEvent","logRevenue","setUserId","setUserProperties","setOptOut","setVersionName","setDomain","setDeviceId","enableTracking","setGlobalUserProperties","identify","clearUserProperties","setGroup","logRevenueV2","regenerateDeviceId","groupIdentify","onInit","logEventWithTimestamp","logEventWithGroups","setSessionId","resetSessionId"]
                    ;function v(e){function t(t){e[t]=function(){
                    e._q.push([t].concat(Array.prototype.slice.call(arguments,0)))}}
                    for(var n=0;n<d.length;n++){t(d[n])}}v(n);n.getInstance=function(e){
                    e=(!e||e.length===0?"$default_instance":e).toLowerCase()
                    ;if(!Object.prototype.hasOwnProperty.call(n._iq,e)){n._iq[e]={_q:[]};v(n._iq[e])
                    }return n._iq[e]};e.amplitude=n})(window,document);
                    amplitude.getInstance('forecast_widget').init("adb1a003d84140fa09974fc5d3933ad6");
                    };

            }
            data.type = 'text/javascript';
            data.async = 1;
            data.src = '//windy.app/widget/data.php?id=wf' + opts.appid + (this.model ? '&model=' + this.model : '') + '&' + (opts.spotid ? 'spotID=' + opts.spotid : 'lat=' + opts.lat + '&lon=' + opts.lng) + (opts.everyhour ? '&everyHourForecast=1' : '') + (opts.sid ? '&checkUser=1' : '');
            document.body.appendChild(data);

        }
        loadModel(this.model ? this.model : 'gfs27_long');

    };

    var nodes = document.querySelectorAll('div[data-windywidget="forecast"]');
    for(var i=0; i<nodes.length; i++) {
        if(nodes[i].dataset.appid == 's7') continue;
        WindyForecastAsync(nodes[i].dataset, nodes[i]);
    }

})(window, document);
