#pragma once
#include <pebble.h>

// local storage keys
#define DATA_WEATHER_TEMP 108
#define DATA_WEATHER_ICON 109
#define DATA_WEATHER_TIME 110

// app keys for communication
// incoming data
#define KEY_TOWATCH_WEATHER_TEMP 10
#define KEY_TOWATCH_WEATHER_ICON 11
// outgoing flags
#define KEY_TOPHONE_GETWEATHER 12
// ready
#define KEY_TOWATCH_READY 13


void sendStateToPhone();
void registerCommunication();
void destroyCommunication();
void requestWeatherUpdate();
