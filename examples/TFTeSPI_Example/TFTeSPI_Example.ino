/*
 * ******************** *
 * TFTeSPI_Example.ino  *
 * Author: Larry Coffey *
 *   Date: 2026-11-20   *
 * ******************** *
*/

#include "display.h"

uint16_t x = 0;
uint16_t y = 0;
uint16_t z = 0;
uint16_t xRaw = 0;
uint16_t yRaw = 0;

void setup() {
  Serial.begin(115200);

  initDisplay();
  initTouch();

  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Touch the screen!", 160, 120, 2);

  if (!calibrated) calibrateTouch();
}


void loop() {
  //if (ts.tirqTouched()) Serial.println("TIRQ: true, "); // Do not use Touch IRQ
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    x = p.x;
    y = p.y;
    z = p.z;
    xRaw = p.xRaw;
    yRaw = p.yRaw;
    Serial.printf("Touch: %d, %d, %d, %d, %d\n", x, y, z, xRaw, yRaw);
    // Clear previous text or screen if desired
    tft.fillScreen(TFT_BLACK);

    // Draw a Line at touch location
    tft.drawLine(x, y - 5, x, y + 5, TFT_YELLOW);
    tft.drawLine(x - 5, y, x + 5, y, TFT_YELLOW);

    // Display coordinates
    String coord = "X: " + String(x) + " Y: " + String(y) + " Z: " + String(z);
    tft.drawString(coord, 120, 20, 2);

    coord = "XRaw: " + String(xRaw) + " YRaw: " + String(yRaw);
    tft.drawString(coord, 120, 60, 2);
  }
  //}
  delay(10);
}

