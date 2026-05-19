#pragma once
#include "weather_data.h"

// Returns true on success; fills WeatherState
bool fetchCurrentAndHourly(WeatherState& S);
bool fetchDaily(WeatherState& S);
// NEW: fetch today’s & (first item of) tomorrow’s prayer times + today’s Hijri date
bool fetchPrayersAndHijri(WeatherState& S);
