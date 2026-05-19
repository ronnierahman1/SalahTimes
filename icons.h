#pragma once
#include "globals.h"   // EPaper + colors available here

// Weather icon types
enum IconType { SUN, PARTLY, CLOUD, RAIN, STORM, SNOW, FOG, WINDY };

// Map WMO → IconType
IconType iconForWMO(int code, bool isDay);

// Draw weather icons
void drawWeatherIcon(IconType t, int cx, int cy, int s);
void drawWeatherIconTL(IconType t, int left, int top, int s);

// Sunrise/Sunset (y is horizon baseline)
void drawSunriseIcon(int x, int y, int s, int m = 1);
void drawSunsetIcon (int x, int y, int s, int m = 1);

// “NN°C”
void drawTempDegC(int x, int y, float t, int textSize = 5);
int  tempDegCWidth(float t, int textSize);

// TL anchor box size
extern const int ICON_BOX_BASE;
