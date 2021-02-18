#ifndef DvG_NeoPixel_Effects_h
#define DvG_NeoPixel_Effects_h

//#define Use_Adafruit_NeoPixel
#define Use_Adafruit_NeoPixel_ZeroDMA

#include <Arduino.h>
#ifdef Use_Adafruit_NeoPixel_ZeroDMA
  #include "Adafruit_NeoPixel_ZeroDMA.h"
#else
  #include "Adafruit_NeoPixel.h"
#endif

#ifndef DvG_NeoPixel_Effects_h
byte my_gamma[256] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };
#endif

class DvG_NeoPixel_Effects{
  public:
    #ifdef Use_Adafruit_NeoPixel_ZeroDMA
    DvG_NeoPixel_Effects(Adafruit_NeoPixel_ZeroDMA* thisStrip);
    #else
    DvG_NeoPixel_Effects(Adafruit_NeoPixel* thisStrip);
    #endif

    uint32_t Wheel(byte WheelPos);
    uint8_t red(uint32_t c);
    uint8_t green(uint32_t c);
    uint8_t blue(uint32_t c);

    bool effectIsDone(void);
    void finish(void);

    // Effects
    void holdAndWait(uint32_t wait);
    void fullColor(uint32_t c, uint16_t wait);
    void colorWipe(uint32_t c, uint16_t wait);
    void rainbowSpatial(uint16_t wait, uint8_t num_cycles);
    void rainbowTemporal(uint16_t wait);

  private:
    #ifdef Use_Adafruit_NeoPixel_ZeroDMA
    Adafruit_NeoPixel_ZeroDMA* strip;
    #else
    Adafruit_NeoPixel* strip;
    #endif
    uint16_t iPx; // Pixel number, used to iterate over total number of pixels
    uint32_t j;   // Arbitrary counter for effects
    uint32_t last_update;
    uint32_t now;
    bool effect_is_done;
    void startup(void);
};

#endif
