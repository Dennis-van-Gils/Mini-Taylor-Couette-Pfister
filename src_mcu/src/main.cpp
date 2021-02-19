/*******************************************************************************
  Dennis van Gils
  17-02-2021

  80 RPM: large rolls
  oscil 250 ms @ 140 rpm: tiny rolls on inside
  oscil 250 ms @ 155 rpm: double spirals, outside up, inside down

  x: max ~ 240 rpm SINGLE --> 800 steps per sec (needs 17 Volts)
  v: @ I2C clock = 1.6e6 we can achieve max ~1600 steps per sec

  TODO:
  - implement runSpeedToPosition for oscillatory movement
  - float `_Re_estim`: Estimated Reynolds number in case of water @ 22 'C
  - Move `fRun` into DvG_Stepper. Perhaps create `update_step()` with inside
    the `fRun` check. Introduce `start()`, `stop()` into DvG_Stepper.
 ******************************************************************************/

#include <Arduino.h>
#include "Wire.h"
#include "Adafruit_MotorShield.h"
#include "Adafruit_NeoPixel_ZeroDMA.h"
#include "DvG_Stepper.h"
#include "DvG_NeoPixel_Effects.h"
#include "DvG_SerialCommand.h"

/*
// Redefine the default microstep division by Adafruit. Their library allows
// it to be set to 8 or 16, where the latter is their default.
#undef MICROSTEPS
#define MICROSTEPS 8
// YOU CAN"T DO THIS. Adafruit motorshield has other static `microstepcurve`
// set, depending on MICROSTEPS
*/

// NEOPIXEL
// --------
#define PIN_NEOPIXEL 5
#define NUM_LEDS 16
uint8_t brightness = 50; // 200

//Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, 5, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel_ZeroDMA strip(NUM_LEDS, PIN_NEOPIXEL, NEO_GRBW + NEO_KHZ800);
DvG_NeoPixel_Effects npe = DvG_NeoPixel_Effects(&strip);
uint8_t running_effect_no = 1;

// STEPPER
// -------
#define STEPS_PER_REV 200
#define STEPPER_PORT 2

float speed = 0.8;              // [rev per sec]
uint8_t step_style = MICROSTEP; // SINGLE, DOUBLE, INTERLEAVE, MICROSTEP

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *stepper = AFMS.getStepper(STEPS_PER_REV, STEPPER_PORT);

// Advanced stepper control
DvG_Stepper Astepper(stepper, STEPS_PER_REV, step_style);

// Set a faster I2C SCL frequency
#define I2C_SCL_FREQ 1600000 // [Hz] (800000)

// SERIAL
// ------
#define Ser Serial
DvG_SerialCommand sc(Ser); // Instantiate serial command listener

// OTHER
// -----
bool fRun = false;

void printSpeed()
{
    Ser.print("f = ");
    Ser.print(Astepper.speed());
    Ser.print(" Hz, ");
    Ser.print(Astepper.speed_steps_per_sec());
    Ser.print(" steps/s, ");
    switch (Astepper.style())
    {
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

void setup()
{
    Ser.begin(115200);
    Ser.print("Setup... ");

    // NeoPixel
    strip.begin();
    strip.setBrightness(brightness);
    strip.show(); // Initialize all pixels to 'off'

    // Stepper
    AFMS.begin(); // Create with the default frequency 1.6KHz
    Astepper.setSpeed(speed);
    printSpeed();

    // Set a faster I2C SCL frequency
    Wire.begin();
    sercom3.disableWIRE();
    SERCOM3->I2CM.BAUD.bit.BAUD = SystemCoreClock / (2 * I2C_SCL_FREQ) - 1;
    sercom3.enableWIRE();

    // Start up motor unpowered?
    if (false)
    {
        fRun = false;
        stepper->release();
    }

    Ser.println("done.");
}

/*------------------------------------------------------------------------------
    Loop
------------------------------------------------------------------------------*/
uint32_t tick = 0;
uint32_t now = 0;
uint32_t T_oscil = 250000; // [us]

void loop()
{
    char *strCmd; // Incoming serial command string
    static bool fOverrideWithWhite = false;
    static bool fOverrideWithGreen = true;

    /*
    now = micros();
    if (now - tick > T_oscil)
    {
        tick += T_oscil;
        speed = -speed;
        Astepper.setSpeed(speed);
    }
    */

    // -------------------------------------------------------------------------
    //   Process incoming serial command when available
    // -------------------------------------------------------------------------

    //Ser.println(millis());

    if (sc.available())
    {
        strCmd = sc.getCmd();

        if (strcmp(strCmd, "?") == 0)
        {
            Ser.println("Combined NeoPixel and Adafruit motor shield test");
        }
        else if (strcmp(strCmd, "=") == 0)
        {
            if (brightness > 245)
            {
                brightness = 255;
            }
            else
            {
                brightness += 10;
            }
            Ser.print("brightness: ");
            Ser.println(brightness);
            strip.setBrightness(brightness);
        }
        else if (strcmp(strCmd, "-") == 0)
        {
            if (brightness < 10)
            {
                brightness = 0;
            }
            else
            {
                brightness -= 10;
            }
            Ser.print("brightness: ");
            Ser.println(brightness);
            strip.setBrightness(brightness);
        }
        else if (strcmp(strCmd, "w") == 0)
        {
            fOverrideWithWhite = not(fOverrideWithWhite);
            fOverrideWithGreen = false;
            Ser.print("Only white: ");
            Ser.println(fOverrideWithWhite);
        }
        else if (strcmp(strCmd, "g") == 0)
        {
            fOverrideWithWhite = false;
            fOverrideWithGreen = not(fOverrideWithGreen);
            Ser.print("Only green: ");
            Ser.println(fOverrideWithGreen);
        }
        else if (strncmp(strCmd, "f", 1) == 0)
        {
            speed = parseFloatInString(strCmd, 1);
            Astepper.setSpeed(speed);
            printSpeed();
        }
        else if (strcmp(strCmd, ",") == 0)
        {
            speed = (speed > 0 ? speed - .05 : speed + .05);
            Astepper.setSpeed(speed);
            printSpeed();
        }
        else if (strcmp(strCmd, ".") == 0)
        {
            speed = (speed > 0 ? speed + .05 : speed - .05);
            Astepper.setSpeed(speed);
            printSpeed();
        }
        else if (strcmp(strCmd, "1") == 0)
        {
            Astepper.setStyle(SINGLE);
            printSpeed();
        }
        else if (strcmp(strCmd, "2") == 0)
        {
            Astepper.setStyle(DOUBLE);
            printSpeed();
        }
        else if (strcmp(strCmd, "3") == 0)
        {
            Astepper.setStyle(INTERLEAVE);
            printSpeed();
        }
        else if (strcmp(strCmd, "4") == 0)
        {
            Astepper.setStyle(MICROSTEP);
            printSpeed();
        }
        else if (strcmp(strCmd, "0") == 0)
        {
            Ser.println("SINGLE step");
            stepper->onestep(FORWARD, SINGLE); // tune to single step starting pos
        }
        else if (strcmp(strCmd, "9") == 0)
        {
            Ser.println("DOUBLE step");
            stepper->onestep(FORWARD, DOUBLE); // tune to single step starting pos
        }
        else
        {
            if (fRun)
            {
                fRun = false;
                stepper->release();
                Ser.println("Release");
            }
            else
            {
                fRun = true;
                Ser.println("Run");
            }
        }
    }

    //npe.fullColor(strip.Color(0, 200, 255), 1000);
    if (fOverrideWithGreen)
    {
        npe.fullColor(strip.Color(255, 140, 0), 1000);
    }
    else if (fOverrideWithWhite)
    {
        npe.fullColor(strip.Color(0, 0, 0, 255), 1000);
    }
    else
    {
        npe.rainbowTemporal(50);
    }

    if (fRun)
    {
        Astepper.runSpeed();
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
