#include "modules/buzzer/buzzer.h"

extern bool buzzerActive;
extern unsigned long buzzerStartTime;

void buzzerControl(bool activate) {
    if (activate && !buzzerActive) {
        buzzerActive = true;
        buzzerStartTime = millis();
        digitalWrite(buzzerPin, HIGH);
    } else if (!activate && buzzerActive) {
        buzzerActive = false;
        digitalWrite(buzzerPin, LOW);
    }
}