/*
DvG_Stepper.cpp

Heavily modified code from:
  AccelStepper.cpp
  Copyright (C) 2009 Mike McCauley
  $Id: AccelStepper.cpp,v 1.2 2010/10/24 07:46:18 mikem Exp mikem $
*/

#include "DvG_Stepper.h"

DvG_Stepper::DvG_Stepper(
    Adafruit_StepperMotor *stepper,
    uint16_t steps_per_rev)
{
    _stepper = stepper;
    _steps_per_rev = steps_per_rev;
    _running = false;
    _style = SINGLE;
    _steps_per_beat = 1;
    _beatstep = 0;

    _currentPos = 0;
    _targetPos = 0;
    _speed_rev_per_sec = 0.0;
    _speed_steps_per_sec = 0.0;
    _stepInterval = 0;
    _lastStepTime = 0;

    // Set up direct port manipulation for the trigger-out signals
    volatile uint32_t *mode;

    _mask_trig_step = digitalPinToBitMask(PIN_TRIG_STEP);
    _port_trig_step = portOutputRegister(digitalPinToPort(PIN_TRIG_STEP));
    mode = portModeRegister(digitalPinToPort(PIN_TRIG_STEP));
    *mode |= _mask_trig_step; // Set pin to ouput
    _set_trig_step_LO();      // Set pin to low

    _mask_trig_beat = digitalPinToBitMask(PIN_TRIG_BEAT);
    _port_trig_beat = portOutputRegister(digitalPinToPort(PIN_TRIG_BEAT));
    mode = portModeRegister(digitalPinToPort(PIN_TRIG_BEAT));
    *mode |= _mask_trig_beat; // Set pin to ouput
    _set_trig_beat_LO();      // Set pin to low
}

void DvG_Stepper::turn_on()
{
    _running = true;
}

void DvG_Stepper::turn_off()
{
    _running = false;
    _stepper->release();
}

bool DvG_Stepper::running()
{
    return _running;
}

void DvG_Stepper::setStyle(uint8_t style)
{
    _style = style;
    switch (style)
    {
    case SINGLE:
    case DOUBLE:
    default:
        _steps_per_beat = 2;
        break;
    case INTERLEAVE:
        _steps_per_beat = 4;
        break;
    case MICROSTEP:
        _steps_per_beat = MICROSTEPS * 2;
        break;
    }

    // Reset the steps and the beat trigger to maintain a correct sync between
    // the beat trigger and the coil voltage.
    // May cause a little motor stutter. Don't care.
    _stepper->reset_currentstep();
    _beatstep = 0;
    _set_trig_step_LO();
    _set_trig_beat_LO();

    // Must recalculate the steps per second
    setSpeed(_speed_rev_per_sec);
}

uint8_t DvG_Stepper::style()
{
    return _style;
}

void DvG_Stepper::moveTo(int32_t absolute)
{
    _targetPos = absolute;
}

void DvG_Stepper::move(int32_t relative)
{
    moveTo(_currentPos + relative);
}

int32_t DvG_Stepper::distanceToGo()
{
    return _targetPos - _currentPos;
}

int32_t DvG_Stepper::targetPosition()
{
    return _targetPos;
}

int32_t DvG_Stepper::currentPosition()
{
    return _currentPos;
}

void DvG_Stepper::setCurrentPosition(int32_t position)
{
    // Useful during initialisations or after initial positioning
    _currentPos = position;
}

void DvG_Stepper::setSpeed(float rev_per_sec)
{
    _speed_rev_per_sec = rev_per_sec;
    _speed_steps_per_sec = rev_per_sec * (_steps_per_rev * _steps_per_beat / 2);
    _stepInterval = abs(1000000. / _speed_steps_per_sec);

    // Account for overhead I2C communication
    _stepInterval -= 3; // 3 usec
}

float DvG_Stepper::speed()
{
    return _speed_rev_per_sec;
}

float DvG_Stepper::speed_steps_per_sec()
{
    return _speed_steps_per_sec;
}

void DvG_Stepper::step()
{
    _stepper->onestep(_speed_rev_per_sec > 0 ? FORWARD : BACKWARD, _style);
    _toggle_trig_step();

    // Process beat
    if (_beatstep == 0)
    {
        _toggle_trig_beat();
    }
    _beatstep++;

    if (_beatstep >= _steps_per_beat)
    {
        _beatstep = 0;
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

    uint32_t time = micros();

    if (time > _lastStepTime + _stepInterval)
    {
        if (_speed_rev_per_sec > 0)
        {
            _currentPos += 1;
            step();
        }
        else if (_speed_rev_per_sec < 0)
        {
            _currentPos -= 1;
            step();
        }

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

void DvG_Stepper::runToNewPosition(int32_t position)
{
    // Blocks until the new target position is reached
    moveTo(position);
    runToPosition();
}

void DvG_Stepper::_set_trig_step_LO() { *_port_trig_step &= ~_mask_trig_step; }
void DvG_Stepper::_set_trig_step_HI() { *_port_trig_step |= _mask_trig_step; }
void DvG_Stepper::_toggle_trig_step() { *_port_trig_step ^= _mask_trig_step; }
void DvG_Stepper::_set_trig_beat_LO() { *_port_trig_beat &= ~_mask_trig_beat; }
void DvG_Stepper::_set_trig_beat_HI() { *_port_trig_beat |= _mask_trig_beat; }
void DvG_Stepper::_toggle_trig_beat() { *_port_trig_beat ^= _mask_trig_beat; }