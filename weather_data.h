#pragma once
#include <Arduino.h>

struct PrayerDay {
  // minutes from midnight local time
  int fajr = -1, dhuhr = -1, asr = -1, maghrib = -1, isha = -1;
  // jama'a (congregation) times, minutes from midnight
  int fajrJ = -1, dhuhrJ = -1, asrJ = -1, maghribJ = -1, ishaJ = -1;

  // pretty Hijri string for that day (e.g., "20 Safar 1447 AH")
  String hijriPretty;
};

struct WeatherState {
  // current
  float currentTemp = NAN, currentFeels = NAN, currentWind = NAN, currentPrecip = NAN;
  int   currentHum = -1, currentCode = 0;

  // hourly (capacity 24; we show 12)
  int   hourlyCount = 0;
  float hourlyTemp[24];
  int   hourlyCode[24];
  String hourlyTime[24];

  // daily (7 days)
  int   dailyCount = 0;
  float dailyMax[7], dailyMin[7], dailyWindMax[7];
  int   dailyCode[7], dailyPrecipProb[7];
  String dailyDate[7], dailyDayAbbr[7], dailySunrise[7], dailySunset[7];

   // ===== NEW: prayers + hijri =====
  PrayerDay todayPray;
  PrayerDay tomorrowPray;          // only Fajr is used from this for “next Adhan” after Isha
  int lastPrayerFetchYday = -1;    // guard 1/day fetch at >= 01:00
};
