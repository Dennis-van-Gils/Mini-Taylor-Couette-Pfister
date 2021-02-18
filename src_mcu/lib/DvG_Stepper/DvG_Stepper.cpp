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

#include "DvG_Stepper.h"

DvG_Stepper::DvG_Stepper(void (*forward)(), void (*backward)())
{
    _currentPos = 0;
    _targetPos = 0;
    _speed = 0.0;
    _stepInterval = 0;
    _lastStepTime = 0;
    _forward = forward;
    _backward = backward;
}

void DvG_Stepper::moveTo(long absolute)
{
    _targetPos = absolute;
}

void DvG_Stepper::move(long relative)
{
    moveTo(_currentPos + relative);
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

void DvG_Stepper::setCurrentPosition(long position)
{
    // Useful during initialisations or after initial positioning
    _currentPos = position;
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

bool DvG_Stepper::run()
{
    // Run the motor to the target position at constant velocity.
    // You must call this at least once per step, preferably in your main loop
    // Returns true if we are still running to position
    if (_targetPos == _currentPos)
        return false;

    runSpeed();
    return true;
}

bool DvG_Stepper::runSpeed()
{
    // Implements steps according to the current speed
    // You must call this at least once per step
    // Returns true if a step occurred

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

void DvG_Stepper::runToPosition()
{
    // Blocks until the target position is reached
    while (run())
        ;
}

bool DvG_Stepper::runSpeedToPosition()
{
    return _targetPos != _currentPos ? DvG_Stepper::runSpeed() : false;
}

void DvG_Stepper::runToNewPosition(long position)
{
    // Blocks until the new target position is reached
    moveTo(position);
    runToPosition();
}
