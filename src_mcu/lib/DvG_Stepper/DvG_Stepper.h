/*
DvG_Stepper.cpp

Heavily modified code from:
  AccelStepper.cpp
  Copyright (C) 2009 Mike McCauley
  $Id: AccelStepper.cpp,v 1.2 2010/10/24 07:46:18 mikem Exp mikem $
*/

#ifndef DvG_Stepper_h
#define DvG_Stepper_h

#include <Arduino.h>
#include "Adafruit_MotorShield.h"

class DvG_Stepper
{
public:
    /// Constructor. You can have multiple simultaneous steppers, all moving
    /// at different speeds, provided you call their run()
    /// functions at frequent enough intervals. Current Position is set to 0, target
    /// position is set to 0.
    /// Any motor initialization should happen before hand, no pins are used or initialized.
    /// \param[in] forward void-returning procedure that will make a forward step
    /// \param[in] backward void-returning procedure that will make a backward step
    DvG_Stepper(void (*forward)(), void (*backward)());

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

    uint8_t _stepStyle; // SINGLE, DOUBLE, INTERLEAVE or MICROSTEP
    int16_t _stepRpm;
};

#endif
