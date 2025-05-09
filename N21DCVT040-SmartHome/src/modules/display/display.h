#pragma once

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "../fan_control/fan_control.h"
#include "../motor_control/motor_control.h"

extern LiquidCrystal_I2C lcd;

// Variable declarations (remove duplicate enums)
extern fanControlMode_t fanControlMode;
extern volatile fanState_t fanState;
extern fanState_t lastFanState;
extern fanState_t lastDisplayedFanState;
extern float dhtTempC;
extern const float TEMP_THRESHOLD;
extern int lastDisplayedMotorDirection;
extern motorDirection_t motorDirection;
extern motorControlMode_t motorControlMode;
extern float lastDisplayedTemp;
extern bool lastDisplayedLight; // Khai báo biến, không định nghĩa

extern bool hasLight;
extern bool passwordEntryAllowed;
extern bool isLockedOut;
extern bool inChangePasswordMode;
extern int wrongPasswordAttempts;

// Add missing variables
extern bool inModeSelection;
extern unsigned long lockoutStartTime;
extern char password[17];
extern int passwordLength;
extern char correctPassword[17];
extern int fanOnTime;
extern int fanOffTime;
extern bool fanCycleState;
extern unsigned long lastFanSwitchTime;
extern String onTimeInput;
extern String offTimeInput;
extern String forwardTimeInput;
extern String reverseTimeInput;
extern String stopTimeInput;
extern int inputState;

// State tracking variables
extern bool fanOnState;
extern bool fanOffState;
extern bool motorRunState;
extern bool motorReverseState;
extern bool motorStopState;
extern bool lastFanOnState;
extern bool lastFanOffState;
extern bool lastMotorRunState;
extern bool lastMotorReverseState;
extern bool lastMotorStopState;
extern bool passwordChecked;

// Function declarations
void displayHome();
void displayMode();
void displayLcd();
void displayPasswordResult(bool isCorrect);

extern const int MAX_WRONG_ATTEMPTS;  // Changed from macro to const int declaration