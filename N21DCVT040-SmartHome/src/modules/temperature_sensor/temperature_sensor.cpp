#include "modules/temperature_sensor/temperature_sensor.h"


static DHT dht(dhtPin, DHTTYPE);
float dhtTempC = 0.0;  // Remove static to make it globally accessible
static unsigned long lastDhtReadTime = 0;

void temperatureInit() {
    dht.begin();  // Initialize the DHT sensor
}

void temperatureUpdate() {
    if (millis() - lastDhtReadTime >= DHT_READ_INTERVAL) {
        float tempSum = 0.0;
        int validReadings = 0;

        for (int i = 0; i < DHT_READINGS; i++) {
            float temp = dht.readTemperature();
            if (!isnan(temp)) {
                tempSum += temp + DHT_CALIBRATION_OFFSET;
                validReadings++;
            }
            delay(100);
        }

        if (validReadings > 0) {
            dhtTempC = tempSum / validReadings;
            
        } else {
            Serial.println("Loi doc cam bien DHT11!");
        }
        lastDhtReadTime = millis();
    }
}