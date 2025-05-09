#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <Arduino.h>
#include <DHT.h>

#define dhtPin           PF13
#define DHTTYPE          DHT11


#define DHT_READ_INTERVAL      2000
#define DHT_READINGS           5
#define DHT_CALIBRATION_OFFSET -4.0

extern float dhtTempC;  // Declare as extern for global access

void temperatureInit();  // Add initialization function
void temperatureUpdate();

#endif