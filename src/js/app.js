
var weatherEnum = {
  CLEAR_DAY: 0,
  CLEAR_NIGHT: 1,
  CLOUD_DAY: 2,
  CLOUD_NIGHT: 3,
  CLOUD: 4,
  RAIN: 5,
  THUNDER: 6,
  SNOW: 7,
  MIST: 8
}

var mapWeather = {
  '01d': weatherEnum.CLEAR_DAY,
  '01n': weatherEnum.CLEAR_NIGHT,
  '02d': weatherEnum.CLOUD_DAY,
  '02n': weatherEnum.CLOUD_NIGHT,
  '03d': weatherEnum.CLOUD,
  '03n': weatherEnum.CLOUD,
  '04d': weatherEnum.CLOUD,
  '04n': weatherEnum.CLOUD,
  '09d': weatherEnum.RAIN,
  '09n': weatherEnum.RAIN,
  '10d': weatherEnum.RAIN,
  '10n': weatherEnum.RAIN,
  '11d': weatherEnum.THUNDER,
  '11n': weatherEnum.THUNDER,
  '13d': weatherEnum.SNOW,
  '13n': weatherEnum.SNOW,
  '50d': weatherEnum.MIST,
  '50n': weatherEnum.MIST,
}

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

Pebble.addEventListener('ready',
  function(e) {
    // Any saved data?
    console.log('JS: Ready event.');
    Pebble.sendAppMessage({'KEY_TOWATCH_READY': 1});
    // getWeather();
  }
);

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://timboe.github.io/ATLASWatch/index.html';
  console.log('JS: Open conf page ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('JS: Conf page returned: ' + JSON.stringify(configData));

  var dict = {};
  dict['KEY_TOWATCH_ANALOGUE']    = configData['op_analogue'];
  // Else digital

  dict['KEY_TOWATCH_CELSIUS']     = configData['op_celcius'];
  dict['KEY_TOWATCH_FAHRENHEIT']  = configData['op_fahrenheit'];
  dict['KEY_TOWATCH_KELVIN']      = configData['op_kelvin'];
  // Else Kelvin

  dict['KEY_TOWATCH_CALENDAR']    = configData['op_calendar'];
  dict['KEY_TOWATCH_WEATHER']     = configData['op_weather'];
  dict['KEY_TOWATCH_BATTERY']     = configData['op_battery'];
  dict['KEY_TOWATCH_DECAY']       = configData['op_decay'];
  dict['KEY_TOWATCH_ACTIVITY']    = configData['op_activity'];
  dict['KEY_TOWATCH_BLUETOOTH']   = configData['op_bluetooth'];

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('JS: Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('JS: Send failed!');
  });

});

Pebble.addEventListener('appmessage', function(e) {

  console.log('JS: Got an AppMessage: ' + JSON.stringify(e.payload));

  var weatherOn = e.payload['KEY_TOPHONE_GETWEATHER'];

  // Did this message include a request to update the weather?
  if (weatherOn) {
    getWeather();
  }

});

function locationSuccess(pos) {
  // Construct URL
  var myAPIKey = '19998043751c4c655bf250b29610c683';
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET',
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);
      console.log('JS got weather ' + json);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(json.main.temp - 273.15);
      console.log('JS: Temperature in ' + json.name + '  is ' + temperature);

      var weatherCode = json.weather[0].icon;
      var icon = weatherEnum.CLEAR_DAY;
      if (weatherCode in mapWeather) icon = mapWeather[weatherCode];
      console.log('JS: Conditions are code:' + weatherCode + " icon ID:" + icon);

      var dict = {};
      dict['KEY_TOWATCH_WEATHER_TEMP'] = temperature;
      dict['KEY_TOWATCH_WEATHER_ICON'] = icon;

      // Send to watchapp
      Pebble.sendAppMessage(dict, function() {
        console.log('JS: Weather send successful: ' + JSON.stringify(dict));
      }, function() {
        console.log('JS: Weather send failed!');
      });

    }
  );
}

function locationError(err) {
  console.log('JS: Error requesting location!');
}

function getWeather() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}
