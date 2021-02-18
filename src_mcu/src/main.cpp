/*******************************************************************************
  Dennis van Gils
  17-02-2021

  80 RPM: large rolls
  oscil 250 ms @ 140 rpm: tiny rolls on inside
  oscil 250 ms @ 155 rpm: double spirals, outside up, inside down

  x: max ~ 240 rpm SINGLE --> 800 steps per sec (needs 17 Volts)
  v: @ I2C clock = 1.6e6 we can achieve max ~1600 steps per sec

  In AccelStepper.cpp
    Change millis() timer to micros():
      "
      boolean AccelStepper::runSpeed()
      {
        unsigned long time = micros();
      "
    Change speed calculation:
      "
      void AccelStepper::setSpeed(float speed)
      {
        _speed = speed;
        _stepInterval = abs(1000000.0 / _speed);
      }
      "
  In Adafruit_MotorShield.h
    #define MICROSTEPS 8

  TODO: capture functions and vars into classes

 ******************************************************************************/

#include <Arduino.h>
#include "Wire.h"
#include "AccelStepper.h"
#include "Adafruit_MotorShield.h"
#include "Adafruit_NeoPixel_ZeroDMA.h"
#include "DvG_NeoPixel_Effects.h"
#include "DvG_SerialCommand.h"

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
int16_t step_rpm = 120;
//uint8_t step_style = SINGLE;
//uint8_t step_style = DOUBLE;
//uint8_t step_style = INTERLEAVE;
uint8_t step_style = SINGLE;

// Step signal on PIN13 for monitoring on oscilloscope
uint32_t pin13;
volatile uint32_t *mode13;
volatile uint32_t *out13;

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
#define STEPS_PER_REV 200
#define STEPPER_PORT 2
Adafruit_StepperMotor *stepper = AFMS.getStepper(STEPS_PER_REV, STEPPER_PORT);

// AccelStepper
void toggle_pin13()
{
    // Step signal on PIN13 for monitoring on oscilloscope
    static bool fToggle = true;

    if (fToggle)
    {
        *out13 |= pin13;
    }
    else
    {
        *out13 &= ~pin13;
    }
    fToggle = not(fToggle);
}

void toggle_pin13_v2()
{
    // Step signal on PIN13 for monitoring on oscilloscope
    static bool fToggle = false;
    static uint8_t cnt = 0;
    uint8_t N;

    switch (step_style)
    {
    case SINGLE:
    case DOUBLE:
    default:
        N = 2;
        break;
    case INTERLEAVE:
        N = 4;
        break;
    case MICROSTEP:
        N = 16;
        break;
    }

    if (cnt == 0)
    {
        if (fToggle)
        {
            *out13 |= pin13;
        }
        else
        {
            *out13 &= ~pin13;
        }
        fToggle = not(fToggle);
    }

    cnt++;
    if (cnt == N)
    {
        cnt = 0;
    }
}

void forwardstep()
{
    toggle_pin13_v2();
    stepper->onestep(FORWARD, step_style);
}

void backwardstep()
{
    toggle_pin13_v2();
    stepper->onestep(BACKWARD, step_style);
}

float rpm2sps(int16_t input_step_rpm)
{
    float step_sps;

    switch (step_style)
    {
    case SINGLE:
    case DOUBLE:
    default:
        step_sps = input_step_rpm / 60. * STEPS_PER_REV;
        break;
    case INTERLEAVE:
        step_sps = input_step_rpm / 60. * STEPS_PER_REV * 2;
        break;
    case MICROSTEP:
        step_sps = input_step_rpm / 60. * STEPS_PER_REV * 8;
        break;
    }

    return step_sps;
}

AccelStepper Astepper(forwardstep, backwardstep);

// SERIAL
// ------
#define Ser Serial
DvG_SerialCommand sc(Ser); // Instantiate serial command listener

// OTHER
// -----
bool fRun = false;

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
    Ser.println(rpm2sps(step_rpm));
    Astepper.setSpeed(rpm2sps(step_rpm));

// Set a faster I2C SCL frequency
#define I2C_SCL_FREQ 1600000 // [Hz] (800000)
    Wire.begin();
    sercom3.disableWIRE();
    SERCOM3->I2CM.BAUD.bit.BAUD = SystemCoreClock / (2 * I2C_SCL_FREQ) - 1;
    sercom3.enableWIRE();

    // Step signal on PIN13 for monitoring on oscilloscope
    pin13 = digitalPinToBitMask(13);
    mode13 = portModeRegister(digitalPinToPort(13));
    out13 = portOutputRegister(digitalPinToPort(13));
    *mode13 |= pin13; // set pin 13 port as ouput

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

    now = micros();
    if (now - tick > T_oscil)
    {
        tick += T_oscil;
        step_rpm = -step_rpm;
        Astepper.setSpeed(rpm2sps(step_rpm));
    }

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
        else if (strncmp(strCmd, "s", 1) == 0)
        {
            step_rpm = floor(parseFloatInString(strCmd, 1));
            step_rpm = constrain(step_rpm, 0, 600);

            Ser.print("step_rpm: ");
            Ser.println(step_rpm);
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
        }
        else if (strcmp(strCmd, ",") == 0)
        {
            if (step_rpm > 0)
            {
                step_rpm -= 5;
            }
            else
            {
                step_rpm += 5;
            }
            Ser.print("step_rpm: ");
            Ser.println(abs(step_rpm));
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
        }
        else if (strcmp(strCmd, ".") == 0)
        {
            if (step_rpm > 0)
            {
                step_rpm += 5;
            }
            else
            {
                step_rpm -= 5;
            }
            Ser.print("step_rpm: ");
            Ser.println(abs(step_rpm));
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
        }
        else if (strcmp(strCmd, "1") == 0)
        {
            Ser.println("Single stepping");
            step_style = SINGLE;
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
        }
        else if (strcmp(strCmd, "2") == 0)
        {
            Ser.println("Double stepping");
            step_style = DOUBLE;
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
        }
        else if (strcmp(strCmd, "3") == 0)
        {
            Ser.println("Interleaved stepping");
            step_style = INTERLEAVE;
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
        }
        else if (strcmp(strCmd, "4") == 0)
        {
            Ser.println("Micro stepping");
            step_style = MICROSTEP;
            Ser.println(rpm2sps(step_rpm));
            Astepper.setSpeed(rpm2sps(step_rpm));
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
