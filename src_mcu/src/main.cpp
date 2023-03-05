// clang-format off
/*******************************************************************************
  Dennis van Gils
  17-02-2021

  80 RPM: large rolls

  oscil 250 ms @ 2.33 Hz: tiny rolls on inside
  oscil 250 ms @ 2.58 Hz: double spirals, outside up, inside down
  The transfer between the two cases is amazing to watch, both ways.

  x: max ~ 240 rpm SINGLE --> 800 steps per sec (needs 17 Volts)
  v: @ I2C clock = 1.6e6 we can achieve max ~1600 steps per sec

  TODO:
  - implement runSpeedToPosition for oscillatory movement
  - float `_Re_estim`: Estimated Reynolds number in case of water @ 22 'C

  STRONG SILENT DRIVING (I2C_SCL_FREQ = 1600000):
    f [Hz]  style               V_motor [V] comment
    1.5     double              6
    2.0     single or double    7
    4.0     single              18
    4.5     single              19          safe max speed

  DO NOT EXCEED V_motor ABOVE 19 V. At 20 V the current all of a sudden jumps up
  to huge values (> 0.5 A).

  *** Idea linear variable power supply:

    ---> https://www.conrad.com/p/conrad-components-psu-assembly-kit-input-voltage-range-30-v-ac-max-output-voltage-range-12-30-v-dc-198226
    or (?)
    https://www.conrad.com/p/h-tronic-psu-component-input-voltage-range-30-v-ac-max-output-voltage-range-1-30-v-dc-3-a-116718

    with

    ---> https://learn.adafruit.com/ds3502-i2c-potentiometer/overview
    buy: https://nl.mouser.com/new/adafruit/adafruit-ds3502-i2c-digital-potentiometer/

  *** Other options:

    https://www.adafruit.com/product/184

    https://nl.farnell.com/microchip/mcp4261-103e-p/ic-dpot-5-5v-10kr-14-pdip-spi/dp/1578446

  *** MCP4xxx libraries

    https://github.com/dreamcat4/Mcp4261
    https://github.com/teabot/McpDigitalPot
    https://github.com/jmalloc/arduino-mcp4xxx
    https://github.com/sleemanj/MCP41_Simple
    https://github.com/ArduinoMax/MCP41xxx

 ******************************************************************************/
// clang-format on

#include <Arduino.h>
#include <Wire.h>

#include "Adafruit_MotorShield.h"
#include "Adafruit_NeoPixel_ZeroDMA.h"
#include "DvG_NeoPixel_Effects.h"
#include "DvG_SerialCommand.h"
#include "DvG_Stepper.h"

// NEOPIXEL
// --------
#define PIN_NEOPIXEL 5
#define NUM_LEDS 16
uint8_t brightness = 50; // 200
uint8_t running_effect_no = 1;

Adafruit_NeoPixel_ZeroDMA strip(NUM_LEDS, PIN_NEOPIXEL, NEO_GRBW);
DvG_NeoPixel_Effects npe = DvG_NeoPixel_Effects(&strip);

// STEPPER
// -------
#define STEPS_PER_REV 200 // As specified by the stepper motor
#define STEPPER_PORT 2    // Motor connected to port #2 (M3 and M4)
float speed = 1.0;        // [rev per sec]
bool oscillating = false;
// Oscillating at fixed distance?
// Oscillating at fixed frequency?
// Oscillating around a fixed position vs kicked in forwards movement only?

Adafruit_MotorShield AFMS = Adafruit_MotorShield();
Adafruit_StepperMotor *stepper = AFMS.getStepper(STEPS_PER_REV, STEPPER_PORT);
DvG_Stepper Astepper(stepper, STEPS_PER_REV);

// Set a faster I2C clock frequency, beneficial for faster stepping.
// Arduino M0 Pro, SAMD21 chipset specs:
//   supports: 100 kHz, 400 kHz, 1 MHz, 3.4 MHz
//   default : 100 kHz
// @ 400 kHz --> safe max  800 steps / sec
// @   1 MHz --> safe max 1600 steps / sec
// @ 1.6 MHz --> safe max 2400 steps / sec
// @ 2.0 MHz --> did not run
// @ 3.4 MHz --> did not run
#define I2C_SCL_FREQ 1600000 // [Hz]

// SERIAL
// ------
#define Ser Serial
DvG_SerialCommand sc(Ser); // Instantiate serial command listener

void printSpeed() {
  Ser.print("f = ");
  Ser.print(Astepper.speed());
  Ser.print(" Hz, ");
  Ser.print(Astepper.speed_steps_per_sec());
  Ser.print(" steps/s, ");
  switch (Astepper.style()) {
    case SINGLE:
      Ser.println("SINGLE");
      break;
    case DOUBLE:
      Ser.println("DOUBLE");
      break;
    case INTERLEAVE:
      Ser.println("INTERLEAVE");
      break;
    case MICROSTEP:
      Ser.println("MICROSTEP");
      break;
  }
}

/*------------------------------------------------------------------------------
    Setup
------------------------------------------------------------------------------*/

void setup() {
  Ser.begin(115200);
  Ser.print("Setup... ");

  // NeoPixel
  strip.begin();
  strip.setBrightness(brightness);
  strip.show(); // Initialize all pixels to 'off'

  // Stepper
  AFMS.begin(); // Create with the default maximum PWM frequency of 1.6 kHz
                // (1526 Hz according to spec sheet)
  Astepper.turn_off();
  Astepper.setSpeed(speed);
  Astepper.setStyle(SINGLE); // SINGLE, DOUBLE, INTERLEAVE, MICROSTEP

  // Set a faster I2C SCL frequency
  Wire.begin();
  Wire.setClock(I2C_SCL_FREQ);

  Ser.println("done.");
  printSpeed();
}

/*------------------------------------------------------------------------------
    Loop
------------------------------------------------------------------------------*/
uint32_t tick = 0;
uint32_t now = 0;
uint32_t T_oscil = 250000; // [us]

void loop() {
  char *strCmd; // Incoming serial command string
  static bool fOverrideWithWhite = false;
  static bool fOverrideWithGreen = false;

  // -------------------------------------------------------------------------
  //   Process incoming serial command when available
  // -------------------------------------------------------------------------

  if (sc.available()) {
    strCmd = sc.getCmd();

    if (strcmp(strCmd, "?") == 0) {
      Ser.println("Mini Taylor-Couette demo Pfister");
    } else if (strcmp(strCmd, "=") == 0) {
      brightness > 245 ? brightness = 255 : brightness += 10;
      Ser.print("brightness: ");
      Ser.println(brightness);
      strip.setBrightness(brightness);
    } else if (strcmp(strCmd, "-") == 0) {
      brightness < 10 ? brightness = 0 : brightness -= 10;
      Ser.print("brightness: ");
      Ser.println(brightness);
      strip.setBrightness(brightness);
    } else if (strcmp(strCmd, "w") == 0) {
      fOverrideWithWhite = not(fOverrideWithWhite);
      fOverrideWithGreen = false;
      Ser.print("Only white: ");
      Ser.println(fOverrideWithWhite);
    } else if (strcmp(strCmd, "g") == 0) {
      fOverrideWithWhite = false;
      fOverrideWithGreen = not(fOverrideWithGreen);
      Ser.print("Only green: ");
      Ser.println(fOverrideWithGreen);
    } else if (strncmp(strCmd, "f", 1) == 0) {
      speed = parseFloatInString(strCmd, 1);
      Astepper.setSpeed(speed);
      printSpeed();
    } else if (strcmp(strCmd, ",") == 0) {
      speed = (speed > 0 ? speed - .05 : speed + .05);
      Astepper.setSpeed(speed);
      printSpeed();
    } else if (strcmp(strCmd, ".") == 0) {
      speed = (speed > 0 ? speed + .05 : speed - .05);
      Astepper.setSpeed(speed);
      printSpeed();
    } else if (strcmp(strCmd, "1") == 0) {
      Astepper.setStyle(SINGLE);
      printSpeed();
    } else if (strcmp(strCmd, "2") == 0) {
      Astepper.setStyle(DOUBLE);
      printSpeed();
    } else if (strcmp(strCmd, "3") == 0) {
      Astepper.setStyle(INTERLEAVE);
      printSpeed();
    } else if (strcmp(strCmd, "4") == 0) {
      Astepper.setStyle(MICROSTEP);
      printSpeed();
    } else {
      if (Astepper.running()) {
        Astepper.turn_off();
        Ser.println("Release");
      } else {
        Astepper.turn_on();
        Ser.println("Run");
      }
    }
  }

  // npe.fullColor(strip.Color(0, 200, 255), 1000);
  if (fOverrideWithGreen) {
    // npe.fullColor(strip.Color(255, 140, 0), 1000);
    npe.fullColor(strip.Color(0, 255, 0, 0), 1000);
  } else if (fOverrideWithWhite) {
    npe.fullColor(strip.Color(0, 0, 0, 255), 1000);
  } else {
    npe.rainbowTemporal(50);
  }

  /*
  now = micros();
  if (now - tick > T_oscil)
  {
      tick += T_oscil;
      speed = -speed;
      Astepper.setSpeed(speed);
  }
  /*/

  // Step when necessary
  if (Astepper.running()) {
    if (!oscillating) {
      Astepper.runSpeed();
    } else {
    }
  }

  /*
  if (running_effect_no == 1) {
      // White
      npe.fullColor(strip.Color(0, 0, 0, 255), 1000);

  } else if (running_effect_no == 2) {
      npe.rainbowSpatial(5, 1);
      //npe.rainbowCycle(0, 10);

  } else if (running_effect_no == 3) {
      npe.rainbowSpatial(0, 10);

  } else if (running_effect_no == 4) {
      npe.rainbowTemporal(50);

  } else if (running_effect_no == 5) {
      // Red
      npe.fullColor(strip.Color(255, 0, 0), 1000);

  } else if (running_effect_no == 6) {
      // Green
      npe.colorWipe(strip.Color(0, 255, 0), 100);

  } else if (running_effect_no == 7) {
      // Green
      npe.fullColor(strip.Color(0, 255, 0), 1000);

  } else if (running_effect_no == 8) {
      // Blue
      npe.colorWipe(strip.Color(0, 0, 255), 100);

  } else if (running_effect_no == 9) {
      // Blue
      npe.fullColor(strip.Color(0, 0, 255), 1000);

  } else if (running_effect_no == 10) {
      // White
      npe.colorWipe(strip.Color(0, 0, 0, 255), 100);
  }
  */

  /*
  if (fOverrideWithWhite) {
      npe.finish();
      running_effect_no = 0;
  }
  if (fOverrideWithGreen) {
      npe.finish();
      running_effect_no = 6;
  }

  if (npe.effectIsDone()) {
      running_effect_no++;
      if (running_effect_no > 10) {
      running_effect_no = 1;
      }
  }
  */
}
