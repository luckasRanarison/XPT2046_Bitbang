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

#include "XPT2046_Bitbang.h"

#define CMD_READ_Z1 0xB1
#define CMD_READ_Z2 0xC1
#define CMD_READ_X 0x91
#define CMD_READ_Y 0xD1

#define DELAY 5
#define Z_THRESHOLD 300
#define Z_THRESHOLD_INT 75
#define MSEC_THRESHOLD 3

static XPT2046_Bitbang *isrPinptr;
void isrPin(void);

void XPT2046_Bitbang::begin() {
  pinMode(clkPin, OUTPUT);
  pinMode(misoPin, INPUT);
  pinMode(mosiPin, OUTPUT);
  pinMode(csPin, OUTPUT);

  digitalWrite(csPin, HIGH);
  digitalWrite(clkPin, LOW);

  if (tirqPin != 255) {
    pinMode(tirqPin, INPUT);
    attachInterrupt(digitalPinToInterrupt(tirqPin), isrPin, FALLING);
    isrPinptr = this;
  }
}

ISR_PREFIX
void isrPin(void) {
  XPT2046_Bitbang *o = isrPinptr;
  o->isrWake = true;
}

TS_Point XPT2046_Bitbang::getPoint() {
  update();
  return TS_Point(x, y, z, xRaw, yRaw);
}

bool XPT2046_Bitbang::tirqTouched() { return (isrWake); }

bool XPT2046_Bitbang::touched() {
  update();
  return (z >= Z_THRESHOLD);
}

bool XPT2046_Bitbang::bufferEmpty() {
  return ((millis() - lastMs) < MSEC_THRESHOLD);
}

static int16_t besttwoavg(int16_t _x, int16_t _y, int16_t _z) {
  int16_t da, db, dc;
  int16_t reta = 0;

  if (_x > _y)
    da = _x - _y;
  else
    da = _y - _x;
  if (_x > _z)
    db = _x - _z;
  else
    db = _z - _x;
  if (_z > _y)
    dc = _z - _y;
  else
    dc = _y - _z;

  if (da <= db && da <= dc)
    reta = (_x + _y) >> 1;
  else if (db <= da && db <= dc)
    reta = (_x + _z) >> 1;
  else
    reta = (_y + _z) >>
           1; //    else if ( dc <= da && dc <= db ) reta = (_x + _y) >> 1;

  return (reta);
}

uint16_t XPT2046_Bitbang::readSPI(byte command) {
  uint16_t result = 0;

  for (int i = 7; i >= 0; i--) {
    digitalWrite(mosiPin, command & (1 << i));
    digitalWrite(clkPin, LOW);
    delayMicroseconds(DELAY);
    digitalWrite(clkPin, HIGH);
    delayMicroseconds(DELAY);
  }

  digitalWrite(mosiPin, LOW);
  digitalWrite(clkPin, LOW);

  for (int i = 15; i >= 0; i--) {
    digitalWrite(clkPin, HIGH);
    delayMicroseconds(DELAY);
    digitalWrite(clkPin, LOW);
    delayMicroseconds(DELAY);
    result |= (digitalRead(misoPin) << i);
  }

  return result >> 4;
}

// TODO: perhaps a future version should offer an option for more oversampling,
//       with the RANSAC algorithm https://en.wikipedia.org/wiki/RANSAC

void XPT2046_Bitbang::update() {
  int16_t data[6];
  int _zRaw;

  if (!isrWake)
    return;

  uint32_t now = millis();

  if (now - lastMs < MSEC_THRESHOLD)
    return;

  digitalWrite(csPin, LOW);

  int16_t z1 = readSPI(CMD_READ_Z1);
  int16_t z2 = readSPI(CMD_READ_Z2);

  _zRaw = (z1 + 4095) - z2;

  if (_zRaw >= Z_THRESHOLD) {
    data[0] = readSPI(CMD_READ_X);
    data[1] = readSPI(CMD_READ_Y); // make 3 x-y measurements
    data[2] = readSPI(CMD_READ_X);
    data[3] = readSPI(CMD_READ_Y);
  }

  data[4] = readSPI(CMD_READ_X); // Last Y touch power down
  data[5] = readSPI(CMD_READ_Y);

  digitalWrite(csPin, HIGH);

  // Serial.printf("z=%d  ::  z1=%d,  z2=%d  ", z, z1, z2);
  if (_zRaw < 0)
    _zRaw = 0;

  if (_zRaw < Z_THRESHOLD) { //	if ( !touched ) {
    // Serial.println();
    z = 0;
    x = -1;
    y = -1;

    if (_zRaw < Z_THRESHOLD_INT) { //	if ( !touched ) {
      if (tirqPin != 255)
        isrWake = false;
    }

    return;
  }

  z = _zRaw;

  // Average pair with least distance between each measured x then y
  // Serial.printf("    z1=%d,z2=%d  ", z1, z2);
  // Serial.printf("p=%d,  %d,%d  %d,%d  %d,%d", _zRaw,
  // data[0], data[1], data[2], data[3], data[4], data[5]);
  int16_t _xRaw = besttwoavg(data[0], data[2], data[4]);
  int16_t _yRaw = besttwoavg(data[1], data[3], data[5]);

  // Serial.printf("    %d,%d", x, y);
  // Serial.println();
  if (_zRaw < Z_THRESHOLD)
    return;

  int16_t _newX, _newY;
  uint32_t delta = now - lastMs; // delta, dx, dy *** Are No Longer Used ***

  switch (rotation) { // Accurate single calibration valid for all views
  case 0:  //PORTRAIT
    _newX = constrain(map(_yRaw, yMax, yMin, 0, screenWidth), 0, screenWidth - 1);   // x is y reversed
    _newY = constrain(map(_xRaw, xMin, xMax, 0, screenHeight), 0, screenHeight - 1); // y is x
    break;
  case 1:  //LANDSCAPE
    _newX = constrain(map(_xRaw, xMin, xMax, 0, screenHeight), 0, screenHeight - 1); // x is x
    _newY = constrain(map(_yRaw, yMin, yMax, 0, screenWidth), 0, screenWidth - 1);   // y is y
    break;
  case 2:  //PORTRAIT REV
    _newX = constrain(map(_yRaw, yMin, yMax, 0, screenWidth), 0, screenWidth - 1);   // x is y
    _newY = constrain(map(_xRaw, xMax, xMin, 0, screenHeight), 0, screenHeight - 1); // y is x reversed
    break;
  default: // 3 LANDSCAPE REV
    _newX = constrain(map(_xRaw, xMax, xMin, 0, screenHeight), 0, screenHeight - 1); // x is x reversed
    _newY = constrain(map(_yRaw, yMax, yMin, 0, screenWidth), 0, screenWidth - 1);   // y is y reversed
    }
  
  x = _newX; // replace old with the new
  y = _newY;
  xRaw = _xRaw;
  yRaw = _yRaw;
  lastMs = now; // good read completed, set wait *** needs reimplimented ***
}
