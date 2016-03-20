#include <pebble.h>
#include "communication.h"
#include "ATLAS.h"

static bool s_queueUpdate = false;
static bool s_readyForBusiness = false;

void inboxReceiveHandler(DictionaryIterator *iter, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "gotMsg");

  Tuple* _ready = dict_find(iter, KEY_TOWATCH_READY);
  if (_ready && _ready->value->int32 == 1) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "rdy4bsns");
    s_readyForBusiness = true;
    if (s_queueUpdate == true) {
      s_queueUpdate = false;
      requestWeatherUpdate();
    }
    return;
  }

  Tuple* _analogue = dict_find(iter, KEY_TOWATCH_ANALOGUE);
  int _redo = false;
  if (_analogue) {
    if (persist_read_int(OPT_ANALOGUE) != _analogue->value->int32) _redo = true;
    persist_write_int(OPT_ANALOGUE, _analogue->value->int32);
  }

  Tuple* _celsius = dict_find(iter, KEY_TOWATCH_CELSIUS);
  Tuple* _fahrenheit = dict_find(iter, KEY_TOWATCH_FAHRENHEIT);
  Tuple* _kelvin = dict_find(iter, KEY_TOWATCH_FAHRENHEIT);
  if (_celsius && _celsius->value->int32 > 0) persist_write_int(OPT_TEMP_UNIT, TEMP_UNIT_C);
  else if (_fahrenheit && _fahrenheit->value->int32 > 0) persist_write_int(OPT_TEMP_UNIT, TEMP_UNIT_F);
  else if (_kelvin && _kelvin->value->int32 > 0) persist_write_int(OPT_TEMP_UNIT, TEMP_UNIT_K); // Kelvin
  if (_celsius || _kelvin || _fahrenheit) updateWeather();

  Tuple* _calendar = dict_find(iter, KEY_TOWATCH_CALENDAR);
  if (_calendar) persist_write_int(OPT_CALENDAR, _calendar->value->int32);

  Tuple* _weather = dict_find(iter, KEY_TOWATCH_WEATHER);
  if (_weather) persist_write_int(OPT_WEATHER, _weather->value->int32);

  Tuple* _battery = dict_find(iter, KEY_TOWATCH_BATTERY);
  if (_battery) persist_write_int(OPT_BATTERY, _battery->value->int32);

  Tuple* _decay = dict_find(iter, KEY_TOWATCH_DECAY);
  if (_decay) persist_write_int(OPT_DECAY, _decay->value->int32);

  Tuple* _activity = dict_find(iter, KEY_TOWATCH_ACTIVITY);
  if (_activity) persist_write_int(OPT_ACTIVITY, _activity->value->int32);

  Tuple* _bluetooth = dict_find(iter, KEY_TOWATCH_BLUETOOTH);
  if (_bluetooth) persist_write_int(OPT_BLUETOOTH, _bluetooth->value->int32);

  Tuple* _weatherTemp = dict_find(iter, KEY_TOWATCH_WEATHER_TEMP);
  Tuple* _weatherIcon = dict_find(iter, KEY_TOWATCH_WEATHER_ICON);

  if (_weatherIcon && _weatherTemp) {
    persist_write_int(DATA_WEATHER_TIME, time(NULL)); // Cache for 3 hours
    persist_write_int(DATA_WEATHER_TEMP, _weatherTemp->value->int32);
    persist_write_int(DATA_WEATHER_ICON, _weatherIcon->value->int32);
    updateWeather();
  }

  if (_redo == true) {
    time_t _t = time(NULL);
    struct tm* _time = localtime(&_t);
    tickHandler(_time, HOUR_UNIT);
  } else {
    layer_mark_dirty(getLayer());
  }
}

void inboxRecieveFailed(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "msgGet Failed! Code %i", (int)reason);
}

void outboxSendOK(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "msgSend OK.");
}

void outboxSendFailed(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "msgSend Failed! Code %i", (int)reason);
}

void requestWeatherUpdate() {
  if (s_readyForBusiness == false) {
    s_queueUpdate = true;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Qupdate");
    return;
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Req weather from phone");
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_uint8(iter, KEY_TOPHONE_GETWEATHER, 1);
  app_message_outbox_send();
}

void registerCommunication() {
  static bool _isDone = false;
  if (_isDone == true) return;

  app_message_register_inbox_received(inboxReceiveHandler);
  app_message_register_inbox_dropped(inboxRecieveFailed);
  app_message_register_outbox_sent(outboxSendOK);
  app_message_register_outbox_failed(outboxSendFailed);

  // TODO bring this down to save space
  //app_message_open(MAX_MESSAGE_SIZE, MAX_MESSAGE_SIZE);
  app_message_open(128, 128);
  _isDone = true;
}

void destroyCommunication() {
  app_message_deregister_callbacks();
}
