#pragma once

#include <Arduino.h>

#define KEYPAD_NUMBER_OF_ROWS 4
#define KEYPAD_NUMBER_OF_COLS 4  // Changed to 4 columns

extern const int keypadRowPins[KEYPAD_NUMBER_OF_ROWS];
extern const int keypadColPins[KEYPAD_NUMBER_OF_COLS];
extern char keypad[KEYPAD_NUMBER_OF_ROWS][KEYPAD_NUMBER_OF_COLS];

char scanKeypad();