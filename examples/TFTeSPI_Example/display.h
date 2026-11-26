/*
 * ******************** *
 * display.h            *
 * Author: Larry Coffey *
 *   Date: 2026-11-20   *
 * ******************** *
*/

#ifndef DISPLAY_H
#define DISPLAY_H
#endif

#include <SPI.h>
#include <TFT_eSPI.h>
#include "XPT2046_Bitbang.h"

// --- Touch Pin Configuration (CYD Standard) ---
#define TS_IRQ   -1 // IRQ cannot be used with BitBanged SPI
#define TS_SCK   25
#define TS_MOSI  32
#define TS_MISO  39
#define TS_CS    33


// --- Calibration Constants ---
// These values may need adjustment based on your specific panel
uint16_t TS_MINX = 215;
uint16_t TS_MAXX = 3670;

uint16_t TS_MINY = 280;
uint16_t TS_MAXY = 3820;

#define DEFAULT_ROTATION 3 // Landscape inverted

// Global Objects
extern TFT_eSPI tft;
extern XPT2046_Bitbang ts;

// Function Prototypes
void initDisplay();
void initTouch();
bool calibrated = false;
void calibrateTouch();
// --- Implementation ---
// Note: Using 'inline' or defining in a header requires care; 
// for simple sketches, we define them here.
TFT_eSPI tft = TFT_eSPI();
XPT2046_Bitbang ts(TS_SCK, TS_MISO, TS_MOSI, TS_CS, TS_IRQ);

void initDisplay() {
    tft.init();
    tft.setRotation(DEFAULT_ROTATION); // Landscape
    
    // Turn on backlight
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    tft.fillScreen(TFT_BLACK);
}

void initTouch() {
  ts.begin();
  ts.setRotation(DEFAULT_ROTATION); // match display
  ts.setDimensions(240, 320); // Width, Height
  ts.setCalibration(TS_MINX, TS_MAXX, TS_MINY, TS_MAXY);
  //ts.setCalibration(0,4095, 0, 4095); //
}

#define rx 
#define ry 
void calibrateTouch() {  // Touch Screen Calibration function displays your settings for TS_MINX and TS_MAXX etc.
  int16_t minX = 9999, maxX = 0, minY = 9999, maxY = 0, x = 0, y = 0, xRaw = 0, yRaw = 0;
  tft.setRotation(3); // Force LANDSCAPE to calibrate (1 or 3)
  int rot = tft.getRotation();
  int landscape = rot % 2;

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setTextSize(2);
  char *corner[4] = { "TL", "TR", "BL", "BR" };
  int dots[8][2] = {
    { 2, 3 },
    { 236, 3 },
    { 2, 318 },
    { 236, 318 },
    { 5, 3 },
    { 318, 3 },
    { 5, 236 },
    { 318, 236 }
  };
  int hits[4][2] = {
    { 0, 0 },
    { 0, 0 },
    { 0, 0 },
    { 0, 0 }
  };

  for (int i = 0; i < 4; i++) {
    tft.fillScreen(TFT_BLACK);
    char buff[30];
    sprintf(buff, "Touch #%d, %s corner", i + 1, corner[i]);
    tft.setCursor(5, 100);
    tft.println(buff);
    int j = i + landscape * 4;
    tft.fillCircle(dots[j][0], dots[j][1], 2, TFT_RED);
    bool ok = false;
    while (!ok) {
      if (ts.touched()) {
        TS_Point p = ts.getPoint();
        x = p.x;
        y = p.y;
        xRaw = p.xRaw;
        yRaw = p.yRaw;
        minX = (xRaw < minX) ? xRaw : minX;
        maxX = (xRaw > maxX) ? xRaw : maxX;
        minY = (yRaw < minY) ? yRaw : minY;
        maxY = (yRaw > maxY) ? yRaw : maxY;
        tft.fillCircle(dots[j][0], dots[j][1], 2, TFT_GREEN);
        ok = true;
        hits[i][0] = x;
        hits[i][1] = y;
        tft.fillRect(5, 100, 235, 25, TFT_BLACK);
        sprintf(buff, "Touch point #%d...", i + 1);
        tft.setCursor(5, 100);
        tft.println(buff);
        sprintf(buff, "X: %d, y: %d,\n xRaw: %d,\n yRaw: %d", x, y, xRaw, yRaw);
        tft.setCursor(5, 140);
        tft.println(buff);
        if (x < 10) x += 5;
        else if (x > 200) x -= 5;
        if (y < 10) y += 5;
        else if (y > 200) y -= 5;
        tft.fillCircle(x, y, 3, TFT_GREENYELLOW);
        delay(1000);
        sprintf(buff, "\n Touch to continue.");
        tft.print(buff);
        while (!ts.touched()) {};
      }
      delay(500);
    }
  }
  //TS_MINX = minX;
  //TS_MAXX = maxX;
  //TS_MINY = minY;
  //TS_MAXY = maxY;
  calibrated = true;  // Adjust values @ line 26-31 for setCalibration() function call @ line 65
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(DEFAULT_ROTATION);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(0, 5);
  tft.setTextSize(2);
  char buff[80];
  for (int i = 0; i < 4; i++) {
    sprintf(buff, "%s hit:%d, x: %3d, y: %3d\n", corner[i], i, hits[i][0], hits[i][1]);
    Serial.print(buff);
    tft.print(buff);
  }
  sprintf(buff, "\n minX: %d, maxX: %d,\n minY: %d, maxY: %d", minX, maxX, minY, maxY);
    Serial.print(buff);
    tft.print(buff);
    sprintf(buff, "\n\nUpdate your Cal. values,\n if necessary...");
    Serial.print(buff);
    tft.print(buff);
  delay(500);
  sprintf(buff, "\n Touch to continue.");
  tft.print(buff);
  while (!ts.touched()) {};
  tft.setTextSize(2);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 5);
  sprintf(buff, "   minX: %3d,   maxX: %4d\nTS_MINX: %3d,TS_MAXX: %4d\n", minX, maxX, TS_MINX, TS_MAXX);
  Serial.print(buff);
  tft.print(buff);
  sprintf(buff, "\n   minY: %3d,   maxY: %4d\nTS_MINY: %3d,TS_MAXY: %4d\n", minY, maxY, TS_MINY, TS_MAXY);
  Serial.print(buff);
  tft.print(buff);
  delay(500);
  tft.setTextColor(TFT_GREEN);
  //tft.setCursor(0, 5);
  tft.println("\nTemporary Cal. done!\n\n(To make permanent...\nUpdate TS_MINX, TS_MAXX,\nTS_MINY, TS_MAXY and\nCalibrated in display.h");
  while (!ts.touched()) {};
  tft.setTextSize(1);
}


