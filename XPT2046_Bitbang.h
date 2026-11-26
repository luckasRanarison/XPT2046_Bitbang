/* Touchscreen library for XPT2046 Touch Controller Chip
 * Copyright (c) 2015, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _XPT2046_Bitbang_h_
#define _XPT2046_Bitbang_h_

#include "Arduino.h"

class TS_Point {
public:
  TS_Point(void) : x(0), y(0), z(0), xRaw(0), yRaw(0) {}
  TS_Point(int16_t x, int16_t y, int16_t z, int16_t xRaw, int16_t yRaw) : x(x), y(y), z(z), xRaw(xRaw), yRaw(yRaw) {}
  bool operator==(TS_Point p) {
    return ((p.x == x) && (p.y == y) && (p.z == z) && (p.xRaw == xRaw) && (p.yRaw == yRaw));
  }
  bool operator!=(TS_Point p) {
    return ((p.x != x) || (p.y != y) || (p.z != z) || (p.xRaw != xRaw) || (p.yRaw != yRaw));
  }
  int16_t x, y, z, xRaw, yRaw;
};

class XPT2046_Bitbang {
public:
  constexpr XPT2046_Bitbang(uint8_t clkpin, uint8_t misopin, uint8_t mosipin,
                            uint8_t cspin, uint8_t tirq = 255)
      : clkPin(clkpin), misoPin(misopin), mosiPin(mosipin), csPin(cspin),
        tirqPin(tirq) {}
  void begin();

  TS_Point getPoint();

  bool tirqTouched();
  bool touched();

  bool bufferEmpty();
  uint8_t bufferSize() { return 1; }

  void setRotation(uint8_t n) { rotation = n % 4; }

  void setCalibration(int16_t xmin, int16_t xmax, int16_t ymin, int16_t ymax) {
    xMin = xmin;
    xMax = xmax;
    yMin = ymin;
    yMax = ymax;
  };

  void setDimensions(uint16_t width, uint16_t height) {
    screenWidth = width;
    screenHeight = height;
  }

  // protected:
  volatile bool isrWake = true;

private:
  uint16_t readSPI(byte command);
  void update();

  uint8_t clkPin, misoPin, mosiPin, csPin, tirqPin, rotation = 1;
  int16_t x = 0, y = 0, z = 0, xRaw = 0, yRaw = 0;
  int16_t screenWidth = 4095, screenHeight = 4095;
  uint16_t xMin = 0, xMax = 4095, yMin = 0, yMax = 4095;
  uint32_t lastMs = 0x80000000;

  const uint8_t MAX_MS_TRAVEL = 3;
};

#ifndef ISR_PREFIX
#if defined(ESP8266)
#define ISR_PREFIX ICACHE_RAM_ATTR
#elif defined(ESP32)
// TODO: should this also be ICACHE_RAM_ATTR ??
#define ISR_PREFIX IRAM_ATTR
#else
#define ISR_PREFIX
#endif
#endif

#endif
