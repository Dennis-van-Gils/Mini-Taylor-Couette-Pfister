/*
DvG_Stepper.cpp

Heavily modified code from:
  AccelStepper.cpp
  Copyright (C) 2009 Mike McCauley
  $Id: AccelStepper.cpp,v 1.2 2010/10/24 07:46:18 mikem Exp mikem $

  Major changes:
  * Internal step timer is based on micros() instead of millis()
*/

#include "DvG_Stepper.h"

void DvG_Stepper::moveTo(long absolute)
{
    _targetPos = absolute;
    computeNewSpeed();
}

void DvG_Stepper::move(long relative)
{
    moveTo(_currentPos + relative);
}

// Implements steps according to the current speed
// You must call this at least once per step
// returns true if a step occurred
bool DvG_Stepper::runSpeed()
{
    unsigned long time = micros();

    if (time > _lastStepTime + _stepInterval)
    {
        if (_speed > 0)
        {
            _currentPos += 1;
        }
        else if (_speed < 0)
        {
            _currentPos -= 1;
        }
        step();

        _lastStepTime = time;
        return true;
    }
    else
        return false;
}

long DvG_Stepper::distanceToGo()
{
    return _targetPos - _currentPos;
}

long DvG_Stepper::targetPosition()
{
    return _targetPos;
}

long DvG_Stepper::currentPosition()
{
    return _currentPos;
}

// Useful during initialisations or after initial positioning
void DvG_Stepper::setCurrentPosition(long position)
{
    _currentPos = position;
}

void DvG_Stepper::computeNewSpeed()
{
    setSpeed(desiredSpeed());
}

// Work out and return a new speed.
// Subclasses can override if they want
// Implement acceleration, deceleration and max speed
// Negative speed is anticlockwise
// This is called:
//  after each step
//  after user changes:
//   maxSpeed
//   acceleration
//   target position (relative or absolute)
float DvG_Stepper::desiredSpeed()
{
    long distanceTo = distanceToGo();

    // Max possible speed that can still decelerate in the available distance
    float requiredSpeed;

    if (distanceTo == 0)
        return 0.0;
    else if (distanceTo > 0)
        requiredSpeed = sqrt(2.0 * distanceTo * _acceleration);
    else
        requiredSpeed = -sqrt(2.0 * -distanceTo * _acceleration);

    if (requiredSpeed > _speed)
    {
        // Accelerate
        if (_speed == 0)
            requiredSpeed = sqrt(2.0 * _acceleration);
        else
            requiredSpeed = _speed + abs(_acceleration / _speed);
        if (requiredSpeed > _maxSpeed)
            requiredSpeed = _maxSpeed;
    }
    else if (requiredSpeed < _speed)
    {
        // Decelerate
        if (_speed == 0)
            requiredSpeed = -sqrt(2.0 * _acceleration);
        else
            requiredSpeed = _speed - abs(_acceleration / _speed);
        if (requiredSpeed < -_maxSpeed)
            requiredSpeed = -_maxSpeed;
    }
    return requiredSpeed;
}

// Run the motor to implement speed and acceleration in order to proceed to the target position
// You must call this at least once per step, preferably in your main loop
// If the motor is in the desired position, the cost is very small
// returns true if we are still running to position
bool DvG_Stepper::run()
{
    if (_targetPos == _currentPos)
        return false;

    if (runSpeed())
        computeNewSpeed();
    return true;
}

DvG_Stepper::DvG_Stepper(void (*forward)(), void (*backward)())
{
    _currentPos = 0;
    _targetPos = 0;
    _speed = 0.0;
    _maxSpeed = 1.0;
    _acceleration = 1.0;
    _stepInterval = 0;
    _lastStepTime = 0;
    _forward = forward;
    _backward = backward;
}

void DvG_Stepper::setMaxSpeed(float speed)
{
    _maxSpeed = speed;
    computeNewSpeed();
}

void DvG_Stepper::setAcceleration(float acceleration)
{
    _acceleration = acceleration;
    computeNewSpeed();
}

void DvG_Stepper::setSpeed(float speed)
{
    _speed = speed;
    _stepInterval = abs(1000000.0 / _speed);
}

float DvG_Stepper::speed()
{
    return _speed;
}

void DvG_Stepper::step()
{
    if (_speed > 0)
    {
        _forward();
    }
    else
    {
        _backward();
    }
}

// Blocks until the target position is reached
void DvG_Stepper::runToPosition()
{
    while (run())
        ;
}

bool DvG_Stepper::runSpeedToPosition()
{
    return _targetPos != _currentPos ? DvG_Stepper::runSpeed() : false;
}

// Blocks until the new target position is reached
void DvG_Stepper::runToNewPosition(long position)
{
    moveTo(position);
    runToPosition();
}
