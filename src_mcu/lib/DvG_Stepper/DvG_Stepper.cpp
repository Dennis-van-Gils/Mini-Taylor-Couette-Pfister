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
    _style = SINGLE;
    _running = false;

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

    _beatstep = 0;
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
    _stepper->currentstep = 0;
    _style = style;
    _beatstep = 0;
    _set_trig_step_LO();
    _set_trig_beat_LO();
    setSpeed(_speed_rev_per_sec);
}

uint8_t DvG_Stepper::style()
{
    return _style;
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

void DvG_Stepper::setSpeed(float rev_per_sec)
{
    _speed_rev_per_sec = rev_per_sec;

    switch (_style)
    {
    case SINGLE:
    case DOUBLE:
    default:
        _speed_steps_per_sec = _speed_rev_per_sec * _steps_per_rev;
        break;
    case INTERLEAVE:
        _speed_steps_per_sec = _speed_rev_per_sec * _steps_per_rev * 2;
        break;
    case MICROSTEP:
        _speed_steps_per_sec = _speed_rev_per_sec * _steps_per_rev * MICROSTEPS;
        break;
    }

    _stepInterval = abs(1000000.0 / _speed_steps_per_sec);
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
    _process_beat();
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
        if (_speed_rev_per_sec > 0)
        {
            _currentPos += 1;
        }
        else if (_speed_rev_per_sec < 0)
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

void DvG_Stepper::_set_trig_step_LO() { *_port_trig_step &= ~_mask_trig_step; }
void DvG_Stepper::_set_trig_step_HI() { *_port_trig_step |= _mask_trig_step; }
void DvG_Stepper::_toggle_trig_step() { *_port_trig_step ^= _mask_trig_step; }
void DvG_Stepper::_set_trig_beat_LO() { *_port_trig_beat &= ~_mask_trig_beat; }
void DvG_Stepper::_set_trig_beat_HI() { *_port_trig_beat |= _mask_trig_beat; }
void DvG_Stepper::_toggle_trig_beat() { *_port_trig_beat ^= _mask_trig_beat; }

void DvG_Stepper::_process_beat()
{
    uint8_t N;

    switch (_style)
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

    if (_beatstep == 0)
    {
        _toggle_trig_beat();
    }

    _beatstep++;
    if (_beatstep == N)
    {
        _beatstep = 0;
    }
}