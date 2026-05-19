#pragma once

// Wi-Fi, location, and prayer settings — loaded from secrets.h (not committed)
// Copy secrets.h.example to secrets.h and fill in your values.
#include "secrets.h"

// Time / NTP (tune DST as needed)
static constexpr const char* NTP_SERVER      = "pool.ntp.org";
static constexpr long        GMT_OFFSET_SEC  = 3600;  // UTC+1
static constexpr int         DAYLIGHT_OFFSET_SEC = 0;

// Units
static constexpr const char* WIND_UNIT = "km/h";   // temp '°C' drawn via vector, no UTF-8

// Behavior
static constexpr int   HOURLY_SHOW         = 12;              // always show next 12 hours
static constexpr long  WEATHER_INTERVAL_MS = 15L * 60L * 1000L;