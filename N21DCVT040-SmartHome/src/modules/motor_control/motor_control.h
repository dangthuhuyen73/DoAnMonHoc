#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>

enum motorDirection_t { STOPPED, DIRECTION_1, DIRECTION_2 };
enum motorControlMode_t { BUTTON_MODE, SENSOR_MODE1, SETUP_MODE_MOTOR };

#define motorM1Pin       PF2  
#define motorM2Pin       PE3  
#define motorRunButton   PE11  
#define motorReverseButton  PE9
#define motorStopButton  PF14  

// Define motor states if not already defined
typedef enum {
    MOTOR_STOPPED,
    MOTOR_RUNNING,
    MOTOR_REVERSING
} motorState_t;

extern motorDirection_t motorDirection;
extern motorDirection_t motorState;
extern bool hasLight;
extern bool lastLightState;
extern unsigned long lastMotorStartTime;
extern bool motorRunning;
extern bool motorStoppedPermanently;
extern motorControlMode_t motorControlMode;
extern int motorForwardTime;
extern int motorReverseTime;
extern int motorStopTime;
extern unsigned long lastMotorSwitchTime;
extern int motorCycleState;

void motorControlInit();
void motorControlUpdate();
void motorDirectionWrite(motorDirection_t direction);
motorDirection_t motorDirectionRead();
void motorRunButtonCallback();
void motorReverseButtonCallback();
void motorStopButtonCallback();

#endif // MOTOR_CONTROL_H