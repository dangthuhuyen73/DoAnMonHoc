#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include <Arduino.h>

// Remove duplicate enum definitions
enum fanState_t { FAN_OFF, FAN_ON };
enum fanControlMode_t { MANUAL_MODE, SENSOR_MODE, SETUP_MODE };

// Pin definitions
#define relayPin         PE6  
#define fanOnButton      PF15
#define fanOffButton     PE13

extern volatile fanState_t fanState;
extern fanState_t lastFanState;
extern fanControlMode_t fanControlMode;
extern float dhtTempC;
extern const float TEMP_THRESHOLD;

// Add missing variables
extern int fanOnTime;
extern int fanOffTime;
extern bool fanCycleState;
extern unsigned long lastFanSwitchTime;
extern bool buttonPressed;

// Function declarations
void fanControlInit();
void fanStateWrite(fanState_t state);
void fanControlUpdate();

// Add callback declarations
void fanOnButtonCallback();
void fanOffButtonCallback();

#endif // FAN_CONTROL_H