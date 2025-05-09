#include "keypad.h"
#include <Arduino.h>

// Cấu hình bàn phím
#define KEYPAD_NUMBER_OF_ROWS 4
#define KEYPAD_NUMBER_OF_COLS 4
const int keypadRowPins[KEYPAD_NUMBER_OF_ROWS] = {PB3, PB5, PC7, PA15};
const int keypadColPins[KEYPAD_NUMBER_OF_COLS] = {PB12, PB13, PB15, PC6};

char keypad[KEYPAD_NUMBER_OF_ROWS][KEYPAD_NUMBER_OF_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

char scanKeypad() {
    for (int row = 0; row < KEYPAD_NUMBER_OF_ROWS; row++) {
        for (int r = 0; r < KEYPAD_NUMBER_OF_ROWS; r++) {
            digitalWrite(keypadRowPins[r], HIGH);
        }
        digitalWrite(keypadRowPins[row], LOW);
        for (int col = 0; col < KEYPAD_NUMBER_OF_COLS; col++) {
            if (digitalRead(keypadColPins[col]) == LOW) {
                char key = keypad[row][col];
                while (digitalRead(keypadColPins[col]) == LOW) delay(10);
                return key;
            }
        }
    }
    return '\0';
}