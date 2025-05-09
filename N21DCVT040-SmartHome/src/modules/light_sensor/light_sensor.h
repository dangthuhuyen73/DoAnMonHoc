#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <Arduino.h>

#define LIGHT_SENSOR_PIN PG9

extern bool hasLight;

void lightSensorInit();
void lightSensorUpdate();

#endif // LIGHT_SENSOR_H