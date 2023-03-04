#include "DvG_NeoPixel_Effects.h"

#ifdef Use_Adafruit_NeoPixel_ZeroDMA
DvG_NeoPixel_Effects::DvG_NeoPixel_Effects(
    Adafruit_NeoPixel_ZeroDMA *thisStrip) {
  strip = thisStrip;
  effect_is_done = true;
}
#else
DvG_NeoPixel_Effects::DvG_NeoPixel_Effects(Adafruit_NeoPixel *thisStrip) {
  strip = thisStrip;
  effect_is_done = true;
}
#endif

bool DvG_NeoPixel_Effects::effectIsDone(void) { return effect_is_done; }

uint32_t DvG_NeoPixel_Effects::Wheel(byte WheelPos) {
  // Input a value 0 to 255 to get a color value.
  // The colours are a transition r - g - b - back to r.
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return strip->Color(255 - WheelPos * 3, 0, WheelPos * 3, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return strip->Color(0, WheelPos * 3, 255 - WheelPos * 3, 0);
  }
  WheelPos -= 170;
  return strip->Color(WheelPos * 3, 255 - WheelPos * 3, 0, 0);
}

uint8_t DvG_NeoPixel_Effects::red(uint32_t c) { return (c >> 16); }

uint8_t DvG_NeoPixel_Effects::green(uint32_t c) { return (c >> 8); }

uint8_t DvG_NeoPixel_Effects::blue(uint32_t c) { return (c); }

void DvG_NeoPixel_Effects::startup(void) {
  effect_is_done = false;
  last_update = now;
}

void DvG_NeoPixel_Effects::finish(void) {
  effect_is_done = true;
  iPx = 0;
  j = 0;
}

/*******************************************************************************
   EFFECTS
*******************************************************************************/

void DvG_NeoPixel_Effects::holdAndWait(uint32_t wait) {
  now = millis();
  if (effect_is_done) {
    startup();
  }
  if (now > last_update + wait) {
    finish();
  }
}

void DvG_NeoPixel_Effects::fullColor(uint32_t c, uint16_t wait) {
  now = millis();
  if (effect_is_done) {
    startup();
    for (iPx = 0; iPx < strip->numPixels(); iPx++) {
      strip->setPixelColor(iPx, c);
    }
    strip->show();
  }

  if (now > last_update + wait) {
    finish();
  }
}

void DvG_NeoPixel_Effects::colorWipe(uint32_t c, uint16_t wait) {
  // Fill the dots one after the other with a color
  now = millis();
  if (effect_is_done | (now > last_update + wait)) {
    startup();
    strip->setPixelColor(iPx, c);
    strip->show();

    iPx++;
    if (iPx == strip->numPixels()) {
      finish();
    }
  }
}

void DvG_NeoPixel_Effects::rainbowSpatial(uint16_t wait, uint8_t num_cycles) {
  now = millis();
  if (effect_is_done | (now > last_update + wait)) {
    startup();
    for (iPx = 0; iPx < strip->numPixels(); iPx++) {
      strip->setPixelColor(iPx,
                           Wheel(((iPx * 256 / strip->numPixels()) + j) & 255));
    }
    strip->show();

    j++;
    if (j == 256 * num_cycles) {
      finish();
    }
  }
}

void DvG_NeoPixel_Effects::rainbowTemporal(uint16_t wait) {
  now = millis();
  if (effect_is_done | (now > last_update + wait)) {
    startup();
    for (iPx = 0; iPx < strip->numPixels(); iPx++) {
      strip->setPixelColor(iPx, Wheel(j & 255));
    }
    strip->show();

    j++;
    if (j == 256) {
      finish();
    }
  }
}
