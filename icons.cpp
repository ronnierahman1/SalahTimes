#include <math.h>
#include "icons.h"
#include "text_metrics.h"

// Base icon box size for TL anchoring
const int ICON_BOX_BASE = 32;

// Vector primitives
static void drawSun(int x, int y, int s) {
  int r = 8*s;
  epaper.fillCircle(x, y, r, TFT_BLACK);
  for (int i=0;i<8;i++){
    float a = i * 3.1415926f / 4.0f;
    int x1 = x + (int)((r+4*s)*cos(a));
    int y1 = y + (int)((r+4*s)*sin(a));
    int x2 = x + (int)((r+10*s)*cos(a));
    int y2 = y + (int)((r+10*s)*sin(a));
    epaper.drawLine(x1,y1,x2,y2,TFT_BLACK);
  }
}
static void drawCloud(int x,int y,int s,bool fill=true){
  int r=7*s;
  if (fill){
    epaper.fillCircle(x-6*s,y,r,TFT_BLACK);
    epaper.fillCircle(x,y-3*s,r+2*s,TFT_BLACK);
    epaper.fillCircle(x+8*s,y,r+1*s,TFT_BLACK);
    epaper.fillRoundRect(x-12*s,y,24*s,r+2*s,4*s,TFT_BLACK);
  } else {
    epaper.drawCircle(x-6*s,y,r,TFT_BLACK);
    epaper.drawCircle(x,y-3*s,r+2*s,TFT_BLACK);
    epaper.drawCircle(x+8*s,y,r+1*s,TFT_BLACK);
    epaper.drawRoundRect(x-12*s,y,24*s,r+2*s,4*s,TFT_BLACK);
  }
}
static void drawRain (int x,int y,int s){ drawCloud(x,y,s,true); for(int i=-8;i<=8;i+=8) epaper.drawLine(x+i,y+10*s,x+i-2*s,y+14*s,TFT_BLACK); }
static void drawSnow (int x,int y,int s){ drawCloud(x,y,s,true); for(int i=-6;i<=6;i+=6){ epaper.drawLine(x+i,y+10*s,x+i,y+14*s,TFT_BLACK); epaper.drawLine(x+i-2*s,y+12*s,x+i+2*s,y+12*s,TFT_BLACK);} }
static void drawStorm(int x,int y,int s){ drawCloud(x,y,s,true); epaper.fillTriangle(x,y+10*s,x-3*s,y+18*s,x+2*s,y+18*s,TFT_BLACK); }
static void drawFog  (int x,int y,int s){ drawCloud(x,y,s,true); for(int k=0;k<3;k++) epaper.drawFastHLine(x-16*s,y+10*s+4*k*s,32*s,TFT_BLACK); }
static void drawWind (int x,int y,int s){ for(int k=0;k<3;k++){ epaper.drawFastHLine(x-16*s,y-4*s+6*k*s,28*s,TFT_BLACK); epaper.drawCircle(x+14*s,y-4*s+6*k*s,2*s,TFT_BLACK);} }
static void drawPartly(int x,int y,int s){ drawCloud(x+6*s,y+2*s,s,true); drawSun(x-6*s,y-6*s,s); }

IconType iconForWMO(int code, bool isDay) {
  if (code == 0) return SUN;
  if (code == 1 || code == 2) return PARTLY;
  if (code == 3) return CLOUD;
  if (code == 45 || code == 48) return FOG;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return RAIN;
  if ((code >= 71 && code <= 77) || code == 85 || code == 86) return SNOW;
  if (code >= 95) return STORM;
  return isDay ? PARTLY : CLOUD;
}

void drawWeatherIcon(IconType t, int cx, int cy, int s){
  switch(t){ case SUN:drawSun(cx,cy,s); break; case PARTLY:drawPartly(cx,cy,s); break;
    case CLOUD:drawCloud(cx,cy,s,true); break; case RAIN:drawRain(cx,cy,s); break;
    case STORM:drawStorm(cx,cy,s); break; case SNOW:drawSnow(cx,cy,s); break;
    case FOG:drawFog(cx,cy,s); break; case WINDY:drawWind(cx,cy,s); break; }
}

void drawWeatherIconTL(IconType t, int left, int top, int s) {
  const int cx = left + (ICON_BOX_BASE * s) / 2;
  const int cy = top  + (ICON_BOX_BASE * s) / 2 + 20;
  drawWeatherIcon(t, cx, cy, s);
}

// Sunrise/Sunset (y is horizon baseline). s = base scale, m = multiplier.
void drawSunriseIcon(int x, int y, int s, int m) {
  const int U = s * m, r = 4 * U;
  epaper.drawCircle(x, y, r, TFT_BLACK);
  epaper.fillRect(x - r, y, 2 * r + 1, r + 2, TFT_WHITE);
  epaper.drawFastHLine(x - 6 * U, y, 12 * U, TFT_BLACK);
  epaper.drawLine(x,           y - r - 2 * U, x,           y - r - 4 * U, TFT_BLACK);
  epaper.drawLine(x - 3 * U,   y - r - 1 * U, x - 5 * U,   y - r - 3 * U, TFT_BLACK);
  epaper.drawLine(x + 3 * U,   y - r - 1 * U, x + 5 * U,   y - r - 3 * U, TFT_BLACK);
}
void drawSunsetIcon(int x, int y, int s, int m) {
  const int U = s * m, r = 4 * U;
  epaper.drawFastHLine(x - 6 * U, y, 12 * U, TFT_BLACK);
  epaper.drawCircle(x, y, r, TFT_BLACK);
  epaper.fillRect(x - r, y - r - 2, 2 * r + 1, r + 2, TFT_WHITE);
  epaper.drawLine(x,           y + r + 2 * U, x,           y + r + 4 * U, TFT_BLACK);
  epaper.drawLine(x - 3 * U,   y + r + 1 * U, x - 5 * U,   y + r + 3 * U, TFT_BLACK);
  epaper.drawLine(x + 3 * U,   y + r + 1 * U, x + 5 * U,   y + r + 3 * U, TFT_BLACK);
}

// “NN°C” using circle + 'C' (no UTF-8 degree)
void drawTempDegC(int x, int y, float t, int textSize) {
  char num[8];
  if (isnan(t)) strcpy(num, "--"); else snprintf(num, sizeof(num), "%.0f", t);

  epaper.setTextSize(textSize);
  epaper.drawString(num, x, y);

  const int wNum = textWidth(num, textSize);
  const int r    = max(2, textSize);
  const int cx   = x + wNum + glyphWidth(textSize);
  const int cy   = y + glyphHeight(textSize) / 3;
  epaper.drawCircle(cx, cy, r, TFT_BLACK);

  epaper.setTextSize(textSize);
  epaper.drawString("C", cx + r*2 + glyphWidth(textSize)/3, y);
}

int tempDegCWidth(float t, int textSize) {
  char num[8];
  if (isnan(t)) strcpy(num, "--"); else snprintf(num, sizeof(num), "%.0f", t);
  const int wNum = textWidth(num, textSize);
  const int r    = max(2, textSize);
  return wNum + glyphWidth(textSize) + (2*r) + glyphWidth(textSize)/3 + glyphWidth(textSize);
}
