/*
DvG_Stepper.cpp

Heavily modified code from:
  AccelStepper.cpp
  Copyright (C) 2009 Mike McCauley
  $Id: AccelStepper.cpp,v 1.2 2010/10/24 07:46:18 mikem Exp mikem $

 Major changes:
  * Internal step timer is based on micros() instead of millis()
  * No acceleration. Velocity is always a constant.
*/

#ifndef DvG_Stepper_h
#define DvG_Stepper_h

#include <Arduino.h>
#include "Adafruit_MotorShield.h"

// The following two digital outputs will pulse along with the stepper.
// These signals can be used as a trigger source for monitoring the coil step
// voltages on a oscilloscope.
// 'step': Alternate low/high per step.
// 'beat': That minimum number of steps `N` making up a repetitive pattern.
//         Useful for oscilloscope investigation of a coil voltage at constant
//         stepper speed.
//            SINGLE, DOUBLE: N=2
//            INTERLEAVE    : N=4
//             MICROSTEP    : N=16 when 8 microsteps, N=32 when 16 microsteps
#define PIN_TRIG_STEP 12
#define PIN_TRIG_BEAT 13

class DvG_Stepper
{
public:
    /// Constructor. You can have multiple simultaneous steppers, all moving
    /// at different speeds, provided you call their run()
    /// functions at frequent enough intervals. Current Position is set to 0, target
    /// position is set to 0.
    /// Any motor initialization should happen before hand, no pins are used or initialized.
    DvG_Stepper(Adafruit_StepperMotor *stepper, uint8_t style);

    void setStepStyle(uint8_t style);

    /// Set the target position. The run() function will try to move the motor
    /// from the current position to the target position set by the most
    /// recent call to this function.
    /// \param[in] absolute The desired absolute position. Negative is
    /// anticlockwise from the 0 position.
    void moveTo(long absolute);

    /// Set the target position relative to the current position
    /// \param[in] relative The desired position relative to the current position. Negative is
    /// anticlockwise from the current position.
    void move(long relative);

    /// The distance from the current position to the target position.
    /// \return the distance from the current position to the target position
    /// in steps. Positive is clockwise from the current position.
    long distanceToGo();

    /// The most recently set target position.
    /// \return the target position
    /// in steps. Positive is clockwise from the 0 position.
    long targetPosition();

    /// The currently motor position.
    /// \return the current motor position
    /// in steps. Positive is clockwise from the 0 position.
    long currentPosition();

    /// Resets the current position of the motor, so that wherever the mottor
    /// happens to be right now is considered to be the new position. Useful
    /// for setting a zero position on a stepper after an initial hardware
    /// positioning move.
    /// \param[in] position The position in steps of wherever the motor
    /// happens to be right now.
    void setCurrentPosition(long position);

    /// Sets the desired constant speed for use with runSpeed().
    /// \param[in] speed The desired constant speed in steps per
    /// second. Positive is clockwise. Speeds of more than 1000 steps per
    /// second are unreliable. Very slow speeds may be set (eg 0.00027777 for
    /// once per hour, approximately. Speed accuracy depends on the Arduino
    /// crystal. Jitter depends on how frequently you call the runSpeed() function.
    void setSpeed(float speed);

    /// The most recently set speed
    /// \return the most recent speed in steps per second
    float speed();

    /// Poll the motor and step it if a step is due, implementing
    /// constant velocity to achive the target position. You must call this as
    /// fequently as possible, but at least once per minimum step interval,
    /// preferably in your main loop.
    /// \return true if the motor is at the target position.
    bool run();

    /// Poll the motor and step it if a step is due, implmenting a constant
    /// speed as set by the most recent call to setSpeed().
    /// \return true if the motor was stepped.
    bool runSpeed();

    /// Moves the motor to the target position and blocks until it is at
    /// position. Dont use this in event loops, since it blocks.
    void runToPosition();

    /// Runs at the currently selected speed until the target position is reached
    /// Does not implement accelerations.
    bool runSpeedToPosition();

    /// Moves the motor to the new target position and blocks until it is at
    /// position. Dont use this in event loops, since it blocks.
    /// \param[in] position The new target position.
    void runToNewPosition(long position);

protected:
    /// Called to execute a step using stepper functions. Only called when a new step is
    /// required. Calls _forward() or _backward() to perform the step
    virtual void step(void);

private:
    /// The current absolution position in steps.
    long _currentPos; // Steps

    /// The target position in steps. The DvG_Stepper library will move the
    /// motor from teh _currentPos to the _targetPos, taking into account the
    /// max speed, acceleration and deceleration
    long _targetPos; // Steps

    /// The current motos speed in steps per second
    /// Positive is clockwise
    float _speed; // Steps per second

    /// The current interval between steps in milliseconds.
    unsigned long _stepInterval;

    /// The last step time in milliseconds
    unsigned long _lastStepTime;

    // The pointer to a forward-step procedure
    void (*_forward)();

    // The pointer to a backward-step procedure
    void (*_backward)();

    // NEWLY ADDED
    // -----------

    void set_trig_step_LO();
    void set_trig_step_HI();
    void set_trig_beat_LO();
    void set_trig_beat_HI();
    void toggle_trig_step();
    void toggle_trig_beat();

    Adafruit_StepperMotor *_stepper;
    uint8_t _style;          // SINGLE, DOUBLE, INTERLEAVE or MICROSTEP
    int16_t _speed_rpm;      // [revolutions / min]
    float _speed_rps;        // [revolutions / sec]
    uint16_t _steps_per_rev; // Steps per revolution as specified by the stepper motor

    // For direct port manipulation, instead of the slower 'digitalWrite()'
    uint32_t _mask_trig_step;
    uint32_t _mask_trig_beat;
    volatile uint32_t *_port_trig_step;
    volatile uint32_t *_port_trig_beat;
    uint8_t _substep = 0;
};

#endif
