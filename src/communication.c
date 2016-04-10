#include <pebble.h>
#include "communication.h"
#include "ATLAS.h"
#include "enamel.h"

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

  Tuple* _weatherTemp = dict_find(iter, KEY_TOWATCH_WEATHER_TEMP);
  Tuple* _weatherIcon = dict_find(iter, KEY_TOWATCH_WEATHER_ICON);

  if (_weatherIcon && _weatherTemp) {
    persist_write_int(DATA_WEATHER_TIME, time(NULL)); // Cache for 3 hours
    persist_write_int(DATA_WEATHER_TEMP, _weatherTemp->value->int32);
    persist_write_int(DATA_WEATHER_ICON, _weatherIcon->value->int32);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "REC: weather T:%i I:%i",  (int)_weatherTemp->value->int32, (int)_weatherIcon->value->int32);
    updateWeather();
  }

  time_t _t = time(NULL);
  struct tm* _time = localtime(&_t);
  tickHandler(_time, HOUR_UNIT);

  layer_mark_dirty(getLayer());
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

  enamel_init(0, 512);
  enamel_register_custom_inbox_received(inboxReceiveHandler);

  app_message_register_inbox_dropped(inboxRecieveFailed);
  app_message_register_outbox_sent(outboxSendOK);
  app_message_register_outbox_failed(outboxSendFailed);

  _isDone = true;
}

void destroyCommunication() {
  enamel_deinit();
}
