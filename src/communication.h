#pragma once
#include <pebble.h>

// local storage keys
#define OPT_ANALOGUE 100
#define OPT_TEMP_UNIT 101
#define OPT_CALENDAR 102
#define OPT_WEATHER 103
#define OPT_BATTERY 104
#define OPT_DECAY 105
#define OPT_ACTIVITY 106
#define OPT_BLUETOOTH 107
#define DATA_WEATHER_TEMP 108
#define DATA_WEATHER_ICON 109
#define DATA_WEATHER_TIME 110

// app keys for communication
// settings
#define KEY_TOWATCH_ANALOGUE 0
#define KEY_TOWATCH_CELSIUS 1
#define KEY_TOWATCH_FAHRENHEIT 2
#define KEY_TOWATCH_KELVIN 3
#define KEY_TOWATCH_CALENDAR 4
#define KEY_TOWATCH_WEATHER 5
#define KEY_TOWATCH_BATTERY 6
#define KEY_TOWATCH_DECAY 7
#define KEY_TOWATCH_ACTIVITY 8
#define KEY_TOWATCH_BLUETOOTH 9
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
