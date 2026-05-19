#include <time.h>
#include "ui.h"
#include "config.h"
#include "text_metrics.h"
#include "icons.h"
// Fancy GFX fonts (Adafruit/Seeed_GFX compatible)
#include <Adafruit_GFX.h>
// #include <Fonts/FreeSans9pt7b.h>
// #include <Fonts/FreeSans12pt7b.h>
// #include <Fonts/FreeSans18pt7b.h>
// #include <Fonts/FreeSans24pt7b.h>
// #include <Fonts/FreeSansBold12pt7b.h>
// #include <Fonts/FreeSansBold18pt7b.h>
// #include <Fonts/FreeSansBold24pt7b.h>
#include "FreeSansBold36pt7b.h"
#include "FreeSansBold48pt7b.h"


// NEW (drop-in)
static uint16_t gfxTextWidth(EPaper& d, const String& s) {
  return d.textWidth(s);                 // TFT_eSPI API
}
static uint16_t gfxTextHeight(EPaper& d) {
  return d.fontHeight();                 // height for current font
}
static void drawStringRight(EPaper& d, const String& s, int xRight, int y) {
  d.drawString(s, xRight - (int)d.textWidth(s), y);
}

// Canvas geometry
static constexpr int WIDTH = 800, HEIGHT = 480;
static constexpr int HEADER_H = 40;

static constexpr int NOW_TOP = HEADER_H;
static constexpr int NOW_H   = 200;
static constexpr int NOW_LEFT_W = 400;

static constexpr int HOURLY_Y = NOW_TOP + NOW_H;
static constexpr int HOURLY_H = 100;

static constexpr int DAILY_Y  = HOURLY_Y + HOURLY_H;
static constexpr int DAILY_H  = HEIGHT - DAILY_Y;

// Header
static void drawHeaderBar(EPaper& epaper, const String& lastUpdated) {
  epaper.fillRect(0,0,WIDTH,HEADER_H,TFT_WHITE);
  epaper.drawFastHLine(0,HEADER_H,WIDTH,TFT_BLACK);
  epaper.setTextSize(2);
  String line = "XIAO ePaper | Weather: Open-Meteo | Last updated: " + lastUpdated;
  epaper.drawString(line, 10, 10);
}

void drawClockBox(EPaper& epaper, const WeatherState& S) {
    const int WIDTH  = 800, HEIGHT = 480;
    const int HEADER_H = 40;
    const int NOW_TOP = HEADER_H, NOW_H = 200, NOW_LEFT_W = 400;

    const int CLOCK_X = NOW_LEFT_W, CLOCK_Y = NOW_TOP + 10;
    const int CLOCK_W = WIDTH - CLOCK_X - 10, CLOCK_H = NOW_H - 20;
    const int main_clock_offset = 5, date_Y_offset = 45;
    // clear the box
    epaper.fillRect(CLOCK_X + 5, CLOCK_Y, CLOCK_W - 10, CLOCK_H, TFT_WHITE);

    // time/date
    struct tm ti; if (!getLocalTime(&ti)) return;
    char timeStr[16];  strftime(timeStr, sizeof(timeStr), "%H:%M", &ti);
    char dateStr[32];  strftime(dateStr, sizeof(dateStr), "%a, %d %b %Y", &ti);

    epaper.setTextSize(6);
    int tx = CLOCK_X + main_clock_offset, ty = CLOCK_Y + main_clock_offset;
    epaper.drawString(timeStr, tx, ty);

    epaper.setTextSize(2);
    epaper.drawString(dateStr, tx, ty + date_Y_offset);

    // Hijri date (today)
    if (S.todayPray.hijriPretty.length()) {
        epaper.drawString("Hijri: " + S.todayPray.hijriPretty, tx, ty + date_Y_offset + 24);
    }
    epaper.setTextSize(2);
    // Table of prayers (today)
    auto mm2str = [](int m)->String {
        if (m<0) return String("--:--");
        int H = (m/60)%24, M = m%60; char b[6]; snprintf(b,sizeof(b),"%02d:%02d",H,M); return String(b);
    };

    const int rowY0   = ty + date_Y_offset + 24 + 22;   // start below Hijri
    const int rowH    = 10;
    const int col1X   = tx;                  // label
    const int col2X   = tx + 120;            // time

    epaper.setTextSize(1);
    epaper.drawString("Prayers (today)", col1X, rowY0);
    int y = rowY0 + rowH;

    epaper.drawString("Fajr",    col1X, y); epaper.drawString(mm2str(S.todayPray.fajr),    col2X, y); y+=rowH;
    epaper.drawString("Dhuhr",   col1X, y); epaper.drawString(mm2str(S.todayPray.dhuhr),   col2X, y); y+=rowH;
    epaper.drawString("Asr",     col1X, y); epaper.drawString(mm2str(S.todayPray.asr),     col2X, y); y+=rowH;
    epaper.drawString("Maghrib", col1X, y); epaper.drawString(mm2str(S.todayPray.maghrib), col2X, y); y+=rowH;
    epaper.drawString("Isha",    col1X, y); epaper.drawString(mm2str(S.todayPray.isha),    col2X, y); y+=rowH;

    // Countdown: next event (Adhan/Jama'a), updates every minute (this function is called every minute)
    auto nextLabelAndDelta = [&](const PrayerDay& d, int nowMin)->std::pair<String,int> {
        struct Item { const char* label; int at; const char* altLabel; int altAt; };
        // For each prayer: Adhan then Jama'a
        Item seq[] = {
        {"Adhan Fajr",    d.fajr,    "Jama'a Fajr",    d.fajrJ},
        {"Adhan Dhuhr",   d.dhuhr,   "Jama'a Dhuhr",   d.dhuhrJ},
        {"Adhan Asr",     d.asr,     "Jama'a Asr",     d.asrJ},
        {"Adhan Maghrib", d.maghrib, "Jama'a Maghrib", d.maghribJ},
        {"Adhan Isha",    d.isha,    "Jama'a Isha",    d.ishaJ},
        };
        // walk today
        for (auto &it : seq) {
        if (it.at >= 0 && nowMin < it.at) return {String(it.label), it.at - nowMin};
        if (it.at >= 0 && it.altAt >= 0 && nowMin >= it.at && nowMin < it.altAt)
            return {String(it.altLabel), it.altAt - nowMin};
        }
        // after Isha Jama'a → next Adhan is tomorrow Fajr
        if (S.tomorrowPray.fajr >= 0) {
        int tillMid = 24*60 - nowMin;
        return {String("Adhan Fajr"), tillMid + S.tomorrowPray.fajr};
        }
        return {String("Adhan Fajr"), -1};
    };

    int nowMin = ti.tm_hour*60 + ti.tm_min;
    auto nd = nextLabelAndDelta(S.todayPray, nowMin);

    auto fmtDelta = [](int dm)->String{
        if (dm < 0) return String("--");
        int h = dm/60, m = dm%60;
        if (h>0) { char b[32]; snprintf(b,sizeof(b),"%dh %dm",h,m); return String(b); }
        char b[16]; snprintf(b,sizeof(b),"%dm",m); return String(b);
    };

    String line = "Next: " + nd.first + " in " + fmtDelta(nd.second);
    epaper.drawString(line, col1X, y + 8);  // show under the table
}



// HH label from ISO time "YYYY-MM-DDTHH:MM"
static String hhLabel(const String& iso) {
  int i = iso.indexOf('T'); return (i<0) ? iso : iso.substring(i+1,i+3);
}

// Left panel: big temp, details, right-aligned weather icon, sunrise/sunset
static void drawNowLeft(EPaper& epaper, const WeatherState& S) {
  epaper.drawFastVLine(NOW_LEFT_W, NOW_TOP, NOW_H, TFT_BLACK);

  const int MARGIN_L    = 30;
  const int TOP_PAD     = 10;
  const int TEMP_SIZE   = 5;
  const int DETAIL_SIZE = 2;
  const int LINE_H      = glyphHeight(DETAIL_SIZE);
  const int V_GAP       = LINE_H / 2;
  const int ICON_SCALE  = 3;

  // Big temp
  const int tempX = MARGIN_L;
  const int tempY = NOW_TOP + TOP_PAD;
  drawTempDegC(tempX, tempY, S.currentTemp, TEMP_SIZE);

  // Weather icon, right-aligned in left pane
  const int numH     = glyphHeight(TEMP_SIZE);
  const int iconBox  = ICON_BOX_BASE * ICON_SCALE;
  int iconTop        = tempY + (numH - iconBox) / 2 + 10;
  const int iconLeft = NOW_LEFT_W - MARGIN_L - iconBox;
  drawWeatherIconTL(iconForWMO(S.currentCode, true), iconLeft, iconTop, ICON_SCALE);

  // Detail lines
  epaper.setTextSize(DETAIL_SIZE);
  int y = tempY + numH + V_GAP;

  epaper.drawString("Feels ", MARGIN_L, y);
  const int feelsX = MARGIN_L + textWidth("Feels ", DETAIL_SIZE);
  drawTempDegC(feelsX, y, S.currentFeels, DETAIL_SIZE);
  y += LINE_H + V_GAP;

  char humBuf[24];  snprintf(humBuf, sizeof(humBuf), "Hum %d%%", S.currentHum);
  epaper.drawString(humBuf, MARGIN_L, y);  y += LINE_H + V_GAP;

  char windBuf[32]; snprintf(windBuf,sizeof(windBuf),"Wind %.0f %s", isnan(S.currentWind)?0:S.currentWind, WIND_UNIT);
  epaper.drawString(windBuf, MARGIN_L, y); y += LINE_H + V_GAP;

  char precipBuf[32]; snprintf(precipBuf,sizeof(precipBuf), "Precip %.1f mm", isnan(S.currentPrecip)?0:S.currentPrecip);
  epaper.drawString(precipBuf, MARGIN_L, y);

  // Sunrise/Sunset immediately after precip (at bottom of left pane)
  if (S.dailyCount > 0) {
    String up = S.dailySunrise[0], dn = S.dailySunset[0];
    up = (up.length() >= 16) ? up.substring(11,16) : String("--:--");
    dn = (dn.length() >= 16) ? dn.substring(11,16) : String("--:--");

    const int innerW   = NOW_LEFT_W - 2*MARGIN_L;
    const int colW     = innerW / 2;
    const int leftColX = MARGIN_L;
    const int rightColX= MARGIN_L + colW;

    const int BOTTOM_PAD = V_GAP;
    const int srY        = NOW_TOP + NOW_H - BOTTOM_PAD;

    const int sIcon   = 1, m = 2;
    const int U       = sIcon * m;
    const int ICON_W  = 12 * U;                 // matches icon drawing width
    const int ICON_PAD= glyphWidth(DETAIL_SIZE);
    const int textY   = srY - LINE_H;

    epaper.setTextSize(DETAIL_SIZE);

    int iconLeftL   = leftColX + glyphWidth(DETAIL_SIZE);
    int srCenterX   = iconLeftL + (ICON_W / 2);
    drawSunriseIcon(srCenterX, srY, sIcon, m);
    epaper.drawString(up, iconLeftL + ICON_W + ICON_PAD, textY);

    int iconLeftR   = rightColX + glyphWidth(DETAIL_SIZE);
    int ssCenterX   = iconLeftR + (ICON_W / 2);
    drawSunsetIcon(ssCenterX, srY-10, sIcon, m);
    epaper.drawString(dn, iconLeftR + ICON_W + ICON_PAD, textY);
  }
}

static void drawHourly(EPaper& epaper, const WeatherState& S) {
  epaper.drawFastHLine(0, HOURLY_Y, WIDTH, TFT_BLACK);
  epaper.setTextSize(2);
  epaper.drawString("Next hours", 10, HOURLY_Y + 10);

  int cellW = WIDTH / max(1, HOURLY_SHOW);
  int y0 = HOURLY_Y + 40;
  for (int i=0;i<S.hourlyCount;i++){
    int cx = i*cellW + cellW/2;
    epaper.setTextSize(1);
    epaper.drawString(hhLabel(S.hourlyTime[i]), cx-10, y0-10);
    drawWeatherIcon(iconForWMO(S.hourlyCode[i], true), cx, y0+25, 1);
    char tbuf[12]; snprintf(tbuf,sizeof tbuf,"%.0f", S.hourlyTemp[i]);
    epaper.drawString(tbuf, cx-8, y0+45);
  }
}

static void drawDaily(EPaper& epaper, const WeatherState& S) {
  epaper.drawFastHLine(0, DAILY_Y, WIDTH, TFT_BLACK);
  epaper.setTextSize(2);
  epaper.drawString("7-day", 10, DAILY_Y + 10);

  if (S.dailyCount <= 0) return;

  int cellW = WIDTH / S.dailyCount;
  int y0 = DAILY_Y + 30;

  for (int i=0;i<S.dailyCount;i++){
    int x = i*cellW, xText = x + 6, xMid = x + cellW/2;
    epaper.drawFastVLine(x, DAILY_Y, DAILY_H, TFT_BLACK);

    epaper.setTextSize(1);
    String mmdd = S.dailyDate[i].length() >= 5 ? S.dailyDate[i].substring(5) : S.dailyDate[i];
    epaper.drawString(S.dailyDayAbbr[i] + " " + mmdd, xText, y0);

    drawWeatherIcon(iconForWMO(S.dailyCode[i], true), xMid, y0+30, 1);

    char tbuf[24]; snprintf(tbuf,sizeof tbuf,"Max: %.0f/ Min: %.0f", S.dailyMax[i], S.dailyMin[i]);
    epaper.drawString(tbuf, xText, y0+45);

    char pbuf[16]; snprintf(pbuf,sizeof pbuf,"Precip:%d%%", S.dailyPrecipProb[i]);
    epaper.drawString(pbuf, xText, y0+60);

    char wbuf[24]; snprintf(wbuf,sizeof wbuf,"Wind:%.0f %s", S.dailyWindMax[i], WIND_UNIT);
    epaper.drawString(wbuf, xText, y0+75);

    // small sunrise/sunset in daily grid (compact)
    String up = S.dailySunrise[i], dn = S.dailySunset[i];
    up = (up.length() >= 16) ? up.substring(11,16) : String("");
    dn = (dn.length() >= 16) ? dn.substring(11,16) : String("");
    int srY = y0 + 95;
    drawSunriseIcon(xText + 14, srY, 1, 1);
    epaper.drawString(up, xText + 26, srY - glyphHeight(1));
    drawSunsetIcon (xText + 64, srY-5, 1, 1);
    epaper.drawString(dn, xText + 76, srY - glyphHeight(1));
  }
}

void renderAllDeepClean(EPaper& epaper, const WeatherState& S, const String& lastUpdated) {
  deepClean(epaper);
//   epaper.fillScreen(TFT_WHITE);
  drawHeaderBar(epaper, lastUpdated);
  drawNowLeft(epaper, S);
  drawClockBox(epaper, S);
  drawHourly(epaper, S);
  drawDaily(epaper, S);
  epaper.update();
}

void renderAll(EPaper& epaper, const WeatherState& S, const String& lastUpdated) {  
  fullClearOnce(epaper);
  drawHeaderBar(epaper, lastUpdated);
  drawNowLeft(epaper, S);
  drawClockBox(epaper, S);
  drawHourly(epaper, S);
  drawDaily(epaper, S);
  epaper.update();
}


void fullClearOnce(EPaper& epaper) { epaper.fillScreen(TFT_WHITE); epaper.update(); }
void deepClean    (EPaper& epaper) { epaper.fillScreen(TFT_BLACK); epaper.update();
                                     epaper.fillScreen(TFT_WHITE); epaper.update(); }


// ───────────────────────── Salah Timetable (full-screen) ─────────────────────────
static constexpr int SCR_W = 800, SCR_H = 480;

static String mmToHHMM12(int m) {
  if (m < 0) return String("--:--");
  int H = (m/60) % 24, M = m % 60;
  int h12 = H % 12; if (h12 == 0) h12 = 12;
  char buf[16]; snprintf(buf, sizeof(buf), "%d:%02d %s", h12, M, (H<12)?"AM":"PM");
  return String(buf);
}
static String isoToHHMM12(const String& iso) {          // "YYYY-MM-DDTHH:MM..."
  if (iso.length() < 16) return String("--:--");
  int H = iso.substring(11,13).toInt();
  int M = iso.substring(14,16).toInt();
  return mmToHHMM12(H*60 + M);
}
static int   nowMinutesLocal(){
  struct tm ti{}; if (!getLocalTime(&ti)) return -1;
  return ti.tm_hour*60 + ti.tm_min;
}
static void  dottedH(EPaper& ep, int y, int x0, int x1) {
  for (int x = x0; x < x1; x += 8) ep.drawPixel(x, y, TFT_BLACK);
}
static int   txtW(const String& s, int sz){ return textWidth(s, sz); }
static int   lineH(int sz){ return glyphHeight(sz); }

static String nextPrayerName(const WeatherState& S, int &nextAtMin){
  struct P { const char* name; int t; } p[] = {
    {"FAJR",    S.todayPray.fajr},
    {"DHUHR",   S.todayPray.dhuhr},
    {"ASR",     S.todayPray.asr},
    {"MAGHRIB", S.todayPray.maghrib},
    {"ISHA",    S.todayPray.isha},
  };
  int nowM = nowMinutesLocal();
  if (nowM < 0) { nextAtMin = -1; return String("FAJR"); }
  for (auto &e : p) { if (e.t >= 0 && nowM < e.t) { nextAtMin = e.t; return String(e.name); } }
  // past Isha → tomorrow Fajr (if available)
  nextAtMin = (S.tomorrowPray.fajr>=0) ? (24*60 + S.tomorrowPray.fajr) : -1;
  return String("FAJR");
}

// Encoding: -1 = no valid next prayer
// 0..1439  = minutes from midnight today
// 1440..2879 = minutes from midnight tomorrow
int nextPrayerTime(const WeatherState& S) {
  int nextAtMin = -1;
  (void)nextPrayerName(S, nextAtMin);   // fills nextAtMin with the right encoding
  return nextAtMin;
}

void drawHeaderDateTime(EPaper& epaper, const WeatherState& S) {
  // Build "HH:mm  Tuesday, April 22, 2025  24 Shawwal 1446 AH"
  struct tm ti{}; 
  getLocalTime(&ti);

  char gDate[64];  strftime(gDate,  sizeof(gDate),  "%A, %B %d, %Y", &ti);
  char gTime[8];   strftime(gTime,  sizeof(gTime),  "%H:%M",          &ti);

  String hijri = S.todayPray.hijriPretty.length()
                 ? S.todayPray.hijriPretty
                 : String("-- Hijri --");

  char header[160];
  snprintf(header, sizeof(header), "%s | %s | %s", gTime, gDate, hijri.c_str());

  // Clear ONLY the header strip
  epaper.setFreeFont(&FreeSans12pt7b);

  // adjust these to your layout constants
  const int HEADER_Y  = 6;     // text baseline
  const int HEADER_H  = 40;    // height of the header band (>= epaper.fontHeight())
  const int HEADER_XL = 0;
  const int HEADER_XR = SCR_W; // 800 if you use a constant

  epaper.fillRect(HEADER_XL, 0, HEADER_XR - HEADER_XL, HEADER_H, TFT_WHITE);
  epaper.update();
  epaper.fillRect(HEADER_XL, 0, HEADER_XR - HEADER_XL, HEADER_H, TFT_WHITE);

  // Re-draw header text
  epaper.drawString(header, 10, HEADER_Y);

  // (Optional) divider line at bottom of header
  epaper.drawFastHLine(0, HEADER_H - 2, HEADER_XR, TFT_BLACK);

  // NOTE: No epaper.update() here → caller decides whether to do a partial or full refresh.
  // If your driver supports windowed refresh, you can do:
  // epaper.updateWindow(HEADER_XL, 0, HEADER_XR - HEADER_XL, HEADER_H);
  epaper.update();
}


// currentTime: local time (from getLocalTime)
// nextMinutesEnc: see encoding above
bool hasNextPrayerTimeElapsed(const tm& currentTime, int nextMinutesEnc)
{
  if (nextMinutesEnc < 0) return false;           // no valid next prayer

  const int cur = currentTime.tm_hour * 60 + currentTime.tm_min; // 0..1439

  // Put current time and "next" into the same 0..2879 window if next is tomorrow
  int curAbs = cur;
  if (nextMinutesEnc >= 1440) {
    // next is tomorrow ⇒ compare against current time shifted by +1440
    curAbs += 1440;
  }

  return curAbs >= nextMinutesEnc;
}

void renderSalahTimetable(EPaper& epaper, const WeatherState& S, const String& lastUpdated) {
  epaper.fillRect(0,0,SCR_W,SCR_H,TFT_WHITE);

  drawHeaderDateTime(epaper, S);
  epaper.drawFastHLine(0, 40, SCR_W, TFT_BLACK);

  // ── Next section ──
  int nextAt = -1;
  String nextName = nextPrayerName(S, nextAt);
  String nextTime = (nextAt>=0) ? mmToHHMM12(nextAt % (24*60)) : String("--:--");

  epaper.setFreeFont(&FreeSans9pt7b);                            // label "Next"
  epaper.drawString("Next", 10, 54);

  epaper.setFreeFont(&FreeSansBold24pt7b);                       // big NEXT PRAYER (left)
  epaper.drawString(nextName, 10, 78);

  // Big time (right)
  String timeHHMM = nextTime.substring(0, nextTime.length()-3);
  String ampm     = nextTime.substring(nextTime.length()-2);

  epaper.setFreeFont(&FreeSansBold24pt7b);                       // HH:MM
  int timeW = gfxTextWidth(epaper, timeHHMM);
  int timeX = SCR_W - 60 - timeW;
  epaper.drawString(timeHHMM, timeX, 78);

  epaper.setFreeFont(&FreeSansBold18pt7b);                       // AM/PM
  epaper.drawString(ampm, timeX + timeW + 6, 88);

  epaper.drawFastHLine(0, 130, SCR_W, TFT_BLACK);

  // ── Table header ──
  const int colLx = 10;
  const int colRx = SCR_W - 10;
  int y = 135;

  epaper.setFreeFont(&FreeSansBold12pt7b);                       // "Prayer" / "Time"
  epaper.drawString("Prayer", colLx, y);
  drawStringRight(epaper, "Time", colRx, y);

  // dottedH(epaper, y + gfxTextHeight(epaper) + 10, 0, SCR_W);
  y += 38;

  // rows
  struct Row { const char* name; String time; };
  Row rows[] = {
    {"Fajr",    mmToHHMM12(S.todayPray.fajr)},
    {"Sunrise", isoToHHMM12(S.dailySunrise[0])},
    {"Dhuhr",   mmToHHMM12(S.todayPray.dhuhr)},
    {"Asr",     mmToHHMM12(S.todayPray.asr)},
    {"Maghrib", mmToHHMM12(S.todayPray.maghrib)},
    {"Isha",    mmToHHMM12(S.todayPray.isha)},
  };

  epaper.setFreeFont(&FreeSansBold12pt7b);                       // table rows
  const int rowGap = 40;
  for (auto &r : rows) {
    dottedH(epaper, y - 10, 0, SCR_W);
    epaper.drawString(r.name, colLx, y);
    drawStringRight(epaper, r.time, colRx, y);
    y += rowGap;
  }
  dottedH(epaper, y - 10, 0, SCR_W);

  // ── Footer ──
  epaper.setFreeFont(&FreeSans9pt7b);
  String method = String("Method: Aladhan (method ") + String(PRAYER_METHOD) + "), shafaq " + String(PRAYER_SHAFAQ);
  epaper.drawString(method, 10, SCR_H - 38);

  String upd   = "Updated: " + (lastUpdated.length() ? lastUpdated : String("--:--"));
  drawStringRight(epaper, upd, SCR_W - 10, SCR_H - 38);

  #ifdef PRAYER_TZ
    drawStringRight(epaper, String(PRAYER_TZ), SCR_W - 10, SCR_H - 24);
  #else
    drawStringRight(epaper, String("Local time"), SCR_W - 10, SCR_H - 24);
  #endif
}
