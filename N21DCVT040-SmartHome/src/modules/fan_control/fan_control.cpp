#include "modules/fan_control/fan_control.h"

extern bool buttonPressed;

#define DEBOUNCE_DELAY         100

volatile fanState_t fanState = FAN_OFF;
extern fanControlMode_t fanControlMode;
extern float dhtTempC;
extern fanState_t lastFanState;

void fanControlInit() {
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    fanState = FAN_OFF;
}

void fanControlUpdate() {
    if (fanControlMode == SENSOR_MODE) {
        fanState_t newState = (dhtTempC > TEMP_THRESHOLD) ? FAN_ON : FAN_OFF;
        if (newState != fanState) fanStateWrite(newState);
    } else if (fanControlMode == SETUP_MODE) {
        if (fanOnTime > 0 && fanOffTime > 0) {
            unsigned long currentTime = millis();
            if (fanCycleState && currentTime - lastFanSwitchTime >= (unsigned long)fanOnTime * 1000) {
                fanStateWrite(FAN_OFF);
                fanCycleState = false;
                lastFanSwitchTime = currentTime;
            } else if (!fanCycleState && currentTime - lastFanSwitchTime >= (unsigned long)fanOffTime * 1000) {
                fanStateWrite(FAN_ON);
                fanCycleState = true;
                lastFanSwitchTime = currentTime;
            }
        } else if (fanState != FAN_OFF) {
            fanStateWrite(FAN_OFF);
        }
    }
    digitalWrite(relayPin, (fanState == FAN_ON) ? HIGH : LOW);
}

void fanStateWrite(fanState_t state) {
    if (state != lastFanState) {
        fanState = state;
        extern void displayMode();
        displayMode();
        lastFanState = state;
    } else {
        fanState = state;
    }
}

void fanOnButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (!buttonPressed && fanControlMode == MANUAL_MODE && millis() - lastInterruptTime > DEBOUNCE_DELAY) {
        buttonPressed = true;
        lastInterruptTime = millis();
        fanStateWrite(FAN_ON);
    }
}

void fanOffButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (!buttonPressed && fanControlMode == MANUAL_MODE && millis() - lastInterruptTime > DEBOUNCE_DELAY) {
        buttonPressed = true;
        lastInterruptTime = millis();
        fanStateWrite(FAN_OFF);
    }
}