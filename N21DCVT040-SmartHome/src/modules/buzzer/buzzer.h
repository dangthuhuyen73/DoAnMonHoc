#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

#define buzzerPin PE10

extern bool buzzerActive;
extern unsigned long buzzerStartTime;

void buzzerControl(bool activate);

#endif // BUZZER_H