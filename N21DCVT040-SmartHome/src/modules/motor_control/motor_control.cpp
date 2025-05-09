#include "modules/motor_control/motor_control.h"

#define MOTOR_RUN_TIME_MS      10000
#define DEBOUNCE_DELAY         100

void motorControlInit() {
    pinMode(motorM1Pin, OUTPUT);
    pinMode(motorM2Pin, OUTPUT);
    digitalWrite(motorM1Pin, HIGH);
    digitalWrite(motorM2Pin, HIGH);
}

void motorControlUpdate() {
    static motorDirection_t lastMotorDirection = STOPPED;

    if (motorControlMode == SENSOR_MODE1) {
        if (hasLight != lastLightState) {
            lastLightState = hasLight;
            motorStoppedPermanently = false;
            motorRunning = false;
            lastMotorStartTime = 0;
        }
        if (!motorStoppedPermanently && !motorRunning) {
            motorDirection = hasLight ? DIRECTION_2 : DIRECTION_1;
            motorRunning = true;
            lastMotorStartTime = millis();
        }
        if (motorRunning && (millis() - lastMotorStartTime >= MOTOR_RUN_TIME_MS)) {
            motorDirection = STOPPED;
            motorRunning = false;
            motorStoppedPermanently = true;
        }
    } else if (motorControlMode == SETUP_MODE_MOTOR) {
        if (motorForwardTime > 0 && motorReverseTime > 0 && motorStopTime > 0) {
            unsigned long currentTime = millis();
            if (motorCycleState == 1 && currentTime - lastMotorSwitchTime >= (unsigned long)motorForwardTime * 1000) {
                motorDirectionWrite(DIRECTION_2);
                motorCycleState = 2;
                lastMotorSwitchTime = currentTime;
            } else if (motorCycleState == 2 && currentTime - lastMotorSwitchTime >= (unsigned long)motorReverseTime * 1000) {
                motorDirectionWrite(STOPPED);
                motorCycleState = 0;
                lastMotorSwitchTime = currentTime;
            } else if (motorCycleState == 0 && currentTime - lastMotorSwitchTime >= (unsigned long)motorStopTime * 1000) {
                motorDirectionWrite(DIRECTION_1);
                motorCycleState = 1;
                lastMotorSwitchTime = currentTime;
            }
        } else if (motorDirection != STOPPED) {
            motorDirectionWrite(STOPPED);
        }
    }

    if (motorDirection != motorState) {
        switch (motorDirection) {
            case DIRECTION_1:
                digitalWrite(motorM1Pin, LOW);
                digitalWrite(motorM2Pin, HIGH);
                break;
            case DIRECTION_2:
                digitalWrite(motorM1Pin, HIGH);
                digitalWrite(motorM2Pin, LOW);
                break;
            case STOPPED:
            default:
                digitalWrite(motorM1Pin, HIGH);
                digitalWrite(motorM2Pin, HIGH);
                break;
        }
        motorState = motorDirection;
        if (motorState != lastMotorDirection) {
            extern void displayMode();
            displayMode();
            lastMotorDirection = motorState;
        }
    }
}

void motorDirectionWrite(motorDirection_t direction) {
    motorDirection = direction;
}

motorDirection_t motorDirectionRead() {
    return motorDirection;
}

void motorRunButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY && motorControlMode == BUTTON_MODE) {
        motorDirectionWrite(DIRECTION_1);
        lastInterruptTime = millis();
    }
}

void motorReverseButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY && motorControlMode == BUTTON_MODE) {
        motorDirectionWrite(DIRECTION_2);
        lastInterruptTime = millis();
    }
}

void motorStopButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY && motorControlMode == BUTTON_MODE) {
        motorDirectionWrite(STOPPED);
        lastInterruptTime = millis();
    }
}