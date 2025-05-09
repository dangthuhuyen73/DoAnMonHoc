#include "light_sensor.h"

bool hasLight = false;

void lightSensorInit() {
    pinMode(LIGHT_SENSOR_PIN, INPUT);
}

void lightSensorUpdate() {
    hasLight = digitalRead(LIGHT_SENSOR_PIN) == LOW;
}