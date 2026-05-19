// // ---------------- Wi-Fi ----------------

/* SalahTimetable.ino
 *  Works with:
 *   - driver.h            (provides EPaper driver built on Seeed_GFX/TFT_eSPI)
 *   - config.h            (Wi-Fi, location, API settings)
 *   - ui.h / ui.cpp       (layout + drawing, including prayer table + countdown)
 *   - weather_api.h/.cpp  (Open-Meteo + Aladhan fetching)
 *   - weather_data.h      (WeatherState, PrayerDay, etc.)
 *
 *  Screen target: 800x480 mono ePaper, layout like the attached reference image.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "driver.h"        // <- your EPaper wrapper (same folder as this sketch)
#include "config.h"        // Wi-Fi/location/behaviour          (provided)
#include "ui.h"            // renderAll()/drawClockBox(), etc.  (provided)
#include "weather_api.h"   // fetchers                           (provided)
#include "weather_data.h"  // structs                            (provided)

// ─────────────────────────── Globals ───────────────────────────
EPaper epaper;                // provided by driver.h
static WeatherState S;               // shared render state
static uint32_t lastWeatherMs = 0;   // throttle weather refresh
static uint32_t lastMinuteTick = 0;  // once/min clock box refresh

// Helpful: readable formatted "last updated" for header
static String nowPretty()
{
  struct tm ti;
  if (!getLocalTime(&ti)) return String("--:--");
  char buf[32];
  strftime(buf, sizeof(buf), "%H:%M", &ti);
  return String(buf);
}

// Wi-Fi connect (blocking, with simple dots)
static void connectWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    if (millis() - start > 20000) break; // 20s guard
  }
}

// NTP time init (uses config.h offsets)
static void initTime()
{
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  // wait until time is set
  for (int i = 0; i < 50; ++i) {
    struct tm ti; 
    if (getLocalTime(&ti)) break;
    delay(100);
  }
}

// Fetch weather (current, hourly, 7-day) and redraw
static bool refreshWeatherAndRedraw(bool deepClean = false)
{
  bool ok1 = fetchCurrentAndHourly(S);
  bool ok2 = fetchDaily(S);
  // Prayer fetch once/day (and on boot). ui.cpp depends on S.todayPray etc.
  // If we’re past 01:00 local or first boot, refresh prayer times/Hijri.
  struct tm ti{};
  getLocalTime(&ti);
  if (S.lastPrayerFetchYday != ti.tm_yday || ti.tm_hour >= 1) {
    fetchPrayersAndHijri(S);  // errors are non-fatal; table will show blanks
  }

  if (ok1 && ok2) {
    if (deepClean) renderAllDeepClean(epaper, S, nowPretty());
    else           renderAll         (epaper, S, nowPretty());
  }
  return ok1 && ok2;
}

// ───────────────────────── Arduino lifecycle ─────────────────────────

void setup()
{
  // Display up first
  epaper.begin();
  epaper.setRotation(0);
  fullClearOnce(epaper);

  // Wi-Fi + NTP
  connectWiFi();
  initTime();

  // Fetch once at boot (for Sunrise + today’s prayers/Hijri)
  fetchPrayersAndHijri(S);
  fetchDaily(S);   // provides S.dailySunrise[0]

  // Draw full-screen timetable (NOTHING else)
  renderSalahTimetable(epaper, S, nowPretty());
  epaper.update();

  lastMinuteTick = millis();
}
// tm nextSalahTime = nextPrayerTime(S);
// Keep these globals somewhere near your other globals:
static int8_t   lastMinute = -1;
// static uint32_t lastMinuteTick = 0;   // if you still want a time guard; optional
static int      nextSalahTime = -1;   // minutes from midnight for next prayer

void loop()
{
  struct tm ti{};
  if (!getLocalTime(&ti)) {
    delay(250);
    return;
  }

  // 1) Once per minute: refresh header band (HH:mm + Gregorian + Hijri)
  if (ti.tm_min != lastMinute) {
      if(ti.tm_min % 10 == 0 ) {
        deepClean(epaper);
        drawHeaderDateTime(epaper, S);
        renderSalahTimetable(epaper, S, nowPretty());

      }
      else
      {
        drawHeaderDateTime(epaper, S);
      }

    // Do a partial refresh for just the header band if your driver supports it:
    // update only [0 .. SCR_W) x [0 .. HEADER_H)
    // epaper.updateWindow(0, 0, SCR_W, HEADER_H);  // <-- prefer this if available
    // epaper.update();                                 // fallback: full refresh

    lastMinute     = ti.tm_min;
    lastMinuteTick = millis();
    
  }

  // 2) On the tick where next prayer time has passed, redraw full timetable
  if (hasNextPrayerTimeElapsed(ti, nextSalahTime)) {
    nextSalahTime = nextPrayerTime(S);               // recompute
    deepClean(epaper);
    renderSalahTimetable(epaper, S, nowPretty());
    epaper.update();
  }

  // 3) Around 01:00 local, refresh next-day data and do a deep clean
  if (ti.tm_hour == 1 && ti.tm_min < 2) {
    fetchPrayersAndHijri(S);
    fetchDaily(S);
    deepClean(epaper);
    renderSalahTimetable(epaper, S, nowPretty());
    epaper.update();
  }


  delay(250);  // short idle; header still only updates on minute change
}
