#ifndef MAIN_CPP
#define MAIN_CPP

#include <Arduino.h>
#include <ctype.h>
#include "modules/motor_control/motor_control.h"
#include "modules/fan_control/fan_control.h"
#include "modules/temperature_sensor/temperature_sensor.h"
#include "modules/keypad/keypad.h"
#include "modules/display/display.h"
#include "modules/buzzer/buzzer.h"
#include "modules/light_sensor/light_sensor.h"
#include "modules/uart_task/uart_task.h"

// Pin definitions
#define MOTOR_RUN_TIME_MS      10000
#define TIME_INCREMENT_MS      50
#define DEBOUNCE_DELAY        100
#define BUZZER_DURATION_MS    5000
#define LOCKOUT_DURATION_MS   5000
#define MAX_WRONG_ATTEMPTS     5

// Static variables
bool buttonPressed = false;
static unsigned long lastDebounceTime = 0;
static char lastKeyPressed = '\0';

char password[17] = "";
int passwordLength = 0;
char correctPassword[17] = "0703";
static bool passwordCorrect = false;

static char newPassword[17] = "";
static int newPasswordLength = 0;
int wrongPasswordAttempts = 0;
unsigned long lockoutStartTime = 0;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;

bool fanOnState = false;
bool fanOffState = false;
bool motorRunState = false;
bool motorReverseState = false;
bool motorStopState = false;
bool lastFanOnState = false;
bool lastFanOffState = false;
bool lastMotorRunState = false;
bool lastMotorReverseState = false;
bool lastMotorStopState = false;
bool passwordChecked = false;

// Global variable definitions
fanControlMode_t fanControlMode = MANUAL_MODE;
fanState_t lastFanState = FAN_OFF;
const float TEMP_THRESHOLD = 30.0;
int lastDisplayedMotorDirection = STOPPED;
motorDirection_t motorDirection = STOPPED;
motorControlMode_t motorControlMode = BUTTON_MODE;
motorDirection_t motorState = STOPPED;
bool motorRunning = false;
bool motorStoppedPermanently = false;
bool lastDisplayedLight = false;
bool inKeypadTestMode = false;  // Định nghĩa biến tại đây
bool passwordEntryAllowed = false;
bool isLockedOut = false;
bool inChangePasswordMode = false;
bool inModeSelection = false;
unsigned long lastFanSwitchTime = 0;
unsigned long lastMotorSwitchTime = 0;
unsigned long lastMotorStartTime = 0;
bool lastLightState = false;
int fanOnTime = 0;
int fanOffTime = 0;
int motorForwardTime = 0;
int motorReverseTime = 0;
int motorStopTime = 0;
bool fanCycleState = false;
int motorCycleState = 0;
int inputState = 0;
float lastDisplayedTemp = -999.0;
String onTimeInput = "";
String offTimeInput = "";
String forwardTimeInput = "";
String reverseTimeInput = "";
String stopTimeInput = "";

void debounceHandler();
void handleKeypadInput();
void displayHome();
void displayMode();
void displayPasswordResult(bool isCorrect);
void displayLcd();
void temperatureUpdate();
void motorControlUpdate();
void fanControlUpdate();
void motorRunButtonCallback();
void motorReverseButtonCallback(); 
void motorStopButtonCallback();
void fanOnButtonCallback();
void fanOffButtonCallback();
void motorDirectionWrite(motorDirection_t direction);
void fanStateWrite(fanState_t state);
void buzzerControl(bool state);
char scanKeypad();

void setup() {
    Serial.begin(115200);
    while (!Serial);

    motorControlInit();
    fanControlInit();
    temperatureInit();
    lightSensorInit();  // Khởi tạo cảm biến ánh sáng

    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    buzzerControl(false);

    Wire.setSDA(PB9);
    Wire.setSCL(PB8);
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();

    displayHome();
    delay(5000);

    pinMode(motorRunButton, INPUT_PULLUP);
    pinMode(motorReverseButton, INPUT_PULLUP);
    pinMode(motorStopButton, INPUT_PULLUP);
    pinMode(fanOnButton, INPUT_PULLUP);
    pinMode(fanOffButton, INPUT_PULLUP);

    for (int i = 0; i < KEYPAD_NUMBER_OF_ROWS; i++) {
        pinMode(keypadRowPins[i], OUTPUT);
        digitalWrite(keypadRowPins[i], HIGH);
    }
    for (int i = 0; i < KEYPAD_NUMBER_OF_COLS; i++) {
        pinMode(keypadColPins[i], INPUT_PULLUP);
    }

    attachInterrupt(digitalPinToInterrupt(motorRunButton), motorRunButtonCallback, FALLING);
    attachInterrupt(digitalPinToInterrupt(motorReverseButton), motorReverseButtonCallback, FALLING);
    attachInterrupt(digitalPinToInterrupt(motorStopButton), motorStopButtonCallback, FALLING);
    attachInterrupt(digitalPinToInterrupt(fanOnButton), fanOnButtonCallback, FALLING);
    attachInterrupt(digitalPinToInterrupt(fanOffButton), fanOffButtonCallback, FALLING);

    displayMode();
    displayLcd();
}

void loop() {
    int fanOnState = digitalRead(fanOnButton);
    int fanOffState = digitalRead(fanOffButton);
    int motorRunState = digitalRead(motorRunButton);
    int motorReverseState = digitalRead(motorReverseButton);
    int motorStopState = digitalRead(motorStopButton);
    lightSensorUpdate();  // Cập nhật trạng thái cảm biến ánh sáng

    if (buzzerActive && (millis() - buzzerStartTime >= BUZZER_DURATION_MS)) {
        buzzerControl(false);
    }
    if (isLockedOut && (millis() - lockoutStartTime >= BUZZER_DURATION_MS + LOCKOUT_DURATION_MS)) {
        isLockedOut = false;
        passwordEntryAllowed = false;
        wrongPasswordAttempts = 0;
        lcd.clear();
        lastDisplayedTemp = -999.0;
        displayLcd();
    } else if (isLockedOut && (millis() - lockoutStartTime >= BUZZER_DURATION_MS)) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Vui long cho 5 giay");
    }

    temperatureUpdate();
    motorControlUpdate();
    fanControlUpdate();
    debounceHandler();

    if (!isLockedOut) {
        handleKeypadInput();
    }

    displayLcd();

    uartTask();  // Gọi hàm uartTask từ module

    lastFanOnState = fanOnState;
    lastFanOffState = fanOffState;
    lastMotorRunState = motorRunState;
    lastMotorReverseState = motorReverseState;
    lastMotorStopState = motorStopState;

    delay(TIME_INCREMENT_MS);
}

void debounceHandler() {
    if (millis() - lastDebounceTime >= DEBOUNCE_DELAY) {
        buttonPressed = false;
        lastDebounceTime = millis();
    }
}

void handleKeypadInput() {
    char key = scanKeypad();
    if (key == '\0') return;

    if (inKeypadTestMode) {
        if (key == '#') {
            inKeypadTestMode = false;
            Serial.println("\nThoat che do kiem tra ban phim");
        } else {
            Serial.print("Phim nhan: ");
            Serial.println(key);
        }
    } else if (inChangePasswordMode) {
        if (key == '#' && newPasswordLength > 0) {
            newPassword[newPasswordLength] = '\0';
            strncpy(correctPassword, newPassword, sizeof(correctPassword) - 1);
            correctPassword[sizeof(correctPassword) - 1] = '\0';
            inChangePasswordMode = false;
            newPasswordLength = 0;
            newPassword[0] = '\0';
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Doi MK thanh cong");
            Serial.println("Doi mat khau thanh cong");
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("MAT KHAU DUNG");
            lcd.setCursor(0, 1);
            lcd.print("Chon che do: ");
        } else if (key != '#' && key != '*' && newPasswordLength < 4) {
            newPassword[newPasswordLength++] = key;
            newPassword[newPasswordLength] = '\0';
            lcd.setCursor(8 + newPasswordLength - 1, 3);
            lcd.print("*");
            Serial.print("*");
        }
    } else if (inModeSelection) {
        if (fanControlMode == SETUP_MODE && (inputState == 1 || inputState == 2)) {
            if (inputState == 1 && isdigit(key)) {
                onTimeInput += key;
                lcd.setCursor(12, 3);
                lcd.print(onTimeInput);
            } else if (inputState == 1 && key == '#') {
                fanOnTime = onTimeInput.toInt();
                inputState = 2;
                onTimeInput = "";
                lcd.setCursor(0, 3);
                lcd.print("Nhap tg OFF: ");
            } else if (inputState == 2 && isdigit(key)) {
                offTimeInput += key;
                lcd.setCursor(12, 3);
                lcd.print(offTimeInput);
            } else if (inputState == 2 && key == '#') {
                fanOffTime = offTimeInput.toInt();
                if (fanOffTime > 99) fanOffTime = 99; // Giới hạn giá trị tối đa
                inputState = 0;
                offTimeInput = "";
                lastFanSwitchTime = millis();
                fanCycleState = true;
                fanStateWrite(FAN_ON);
                // Hiển thị thời gian bật/tắt quạt trên dòng 4 của LCD
                lcd.setCursor(0, 3);
                lcd.print("Fan-OPEN:");
                lcd.print(fanOnTime);
                lcd.print("s OFF:");
                lcd.print(fanOffTime);
                lcd.print("s");
            }
        } else if (motorControlMode == SETUP_MODE_MOTOR && (inputState == 3 || inputState == 4 || inputState == 5)) {
            if (inputState == 3 && isdigit(key)) {
                forwardTimeInput += key;
                lcd.setCursor(14, 3);
                lcd.print(forwardTimeInput);
            } else if (inputState == 3 && key == '#') {
                motorForwardTime = forwardTimeInput.toInt();
                if (motorForwardTime > 99) motorForwardTime = 99; // Giới hạn giá trị tối đa
                inputState = 4;
                forwardTimeInput = "";
                lcd.setCursor(0, 3);
                lcd.print("Nhap tg OPEN: ");
                lcd.print("        ");
            } else if (inputState == 4 && isdigit(key)) {
                reverseTimeInput += key;
                lcd.setCursor(13, 3);
                lcd.print(reverseTimeInput);
            } else if (inputState == 4 && key == '#') {
                motorReverseTime = reverseTimeInput.toInt();
                if (motorReverseTime > 99) motorReverseTime = 99; // Giới hạn giá trị tối đa
                inputState = 5;
                reverseTimeInput = "";
                lcd.setCursor(0, 3);
                lcd.print("Nhap tg OFF: ");
                lcd.print("        ");
            } else if (inputState == 5 && isdigit(key)) {
                stopTimeInput += key;
                lcd.setCursor(14, 3);
                lcd.print(stopTimeInput);
            } else if (inputState == 5 && key == '#') {
                motorStopTime = stopTimeInput.toInt();
                if (motorStopTime > 99) motorStopTime = 99; // Giới hạn giá trị tối đa
                inputState = 0;
                stopTimeInput = "";
                lastMotorSwitchTime = millis();
                motorCycleState = 1;
                motorDirectionWrite(DIRECTION_1);
                
                lcd.setCursor(0, 3);
                lcd.print("CL:");
                lcd.print(motorForwardTime);
                lcd.print("s OP:");
                lcd.print(motorReverseTime);
                lcd.print("s OFF:");
                lcd.print(motorStopTime);
                lcd.print("s");
            }
        } else {
            lastKeyPressed = key;
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("MAT KHAU DUNG");
            lcd.setCursor(0, 1);
            lcd.print("Chon che do: ");
            lcd.print(key);
            switch (key) {
                case 'A':
                    inChangePasswordMode = true;
                    newPasswordLength = 0;
                    newPassword[0] = '\0';
                    lcd.setCursor(0, 2);
                    lcd.print("Nhap MK moi");
                    lcd.setCursor(0, 3);
                    lcd.print("Nhap MK: ");
                    Serial.println("Nhap mat khau moi (ket thuc bang #):");
                    break;
                case '1':
                    fanControlMode = MANUAL_MODE;
                    fanOnTime = 0;
                    fanOffTime = 0;
                    Serial.println("Che do Thu cong (Quat)");
                    lcd.setCursor(0, 2);
                    lcd.print("Thu cong (Quat)");
                    lcd.setCursor(0, 3);
                    lcd.print("Fan:");
                    lcd.print(fanState == FAN_ON ? "ON " : "OFF");
                    lcd.print("  Rem:");
                    lcd.print(motorDirection == DIRECTION_1 ? "CLOSE" : motorDirection == DIRECTION_2 ? "OPEN" : "OFF");
                    break;
                case '2':
                    fanControlMode = SENSOR_MODE;
                    fanOnTime = 0;
                    fanOffTime = 0;
                    Serial.println("Che do Cam bien (Quat)");
                    displayMode();
                    lcd.setCursor(0, 2);
                    lcd.print("Cam bien (Quat)");
                    lcd.setCursor(0, 3);
                    lcd.print("Fan:");
                    lcd.print(fanState == FAN_ON ? "ON " : "OFF");
                    lcd.print("  Rem:");
                    lcd.print(motorDirection == DIRECTION_1 ? "CLOSE" : motorDirection == DIRECTION_2 ? "OPEN" : "OFF");
                    break;
                case '3':
                    fanControlMode = SETUP_MODE;
                    inputState = 1;
                    onTimeInput = "";
                    offTimeInput = "";
                    Serial.println("Che do Cai dat tg (Quat)");
                    lcd.setCursor(0, 2);
                    lcd.print("Cai dat (Quat)");
                    lcd.setCursor(0, 3);
                    lcd.print("Nhap tg ON: ");
                    break;
                case '4':
                    motorControlMode = BUTTON_MODE;
                    motorDirection = STOPPED;
                    Serial.println("Che do Thu cong (Rem)");
                    lcd.setCursor(0, 2);
                    lcd.print("Thu cong (Rem)");
                    lcd.setCursor(0, 3);
                    lcd.print("Fan:");
                    lcd.print(fanState == FAN_ON ? "ON " : "OFF");
                    lcd.print("  Rem:");
                    lcd.print(motorDirection == DIRECTION_1 ? "CLOSE" : motorDirection == DIRECTION_2 ? "OPEN" : "OFF");
                    break;
                case '5':
                    motorControlMode = SENSOR_MODE1;
                    motorDirection = STOPPED;
                    Serial.println("Che do Cam bien Anh sang (Rem)");
                    lcd.setCursor(0, 2);
                    lcd.print("Cam bien (Rem)");
                    lcd.setCursor(0, 3);
                    lcd.print("Fan:");
                    lcd.print(fanState == FAN_ON ? "ON " : "OFF");
                    lcd.print("  Rem:");
                    lcd.print(motorDirection == DIRECTION_1 ? "CLOSE" : motorDirection == DIRECTION_2 ? "OPEN" : "OFF");
                    break;
                case '6':
                    motorControlMode = SETUP_MODE_MOTOR;
                    inputState = 3;
                    forwardTimeInput = "";
                    reverseTimeInput = "";
                    stopTimeInput = "";
                    motorForwardTime = 0;
                    motorReverseTime = 0;
                    motorStopTime = 0;
                    motorCycleState = 0;
                    Serial.println("Che do Cai dat tg (Rem)");
                    lcd.setCursor(0, 2);
                    lcd.print("Cai dat tg (Rem)");
                    lcd.setCursor(0, 3);
                    lcd.print("Nhap tg Close: ");
                    break;
                case '9':
                    inModeSelection = false;
                    passwordEntryAllowed = false;
                    passwordLength = 0;
                    password[0] = '\0';
                    passwordChecked = false;
                    lastDisplayedTemp = -999.0;
                    lastDisplayedLight = !lastDisplayedLight;
                    displayLcd();
                    availableCommands();
                    break;
                default:
                    Serial.println("Phim khong hop le!");
                    lcd.setCursor(0, 2);
                    lcd.print("Phim khong hop le!");
                    break;
            }
        }
    } else if (!passwordEntryAllowed && key == '*') {
        passwordEntryAllowed = true;
        passwordLength = 0;
        password[0] = '\0';
        lcd.setCursor(0, 3);
        lcd.print("Nhap MK: ");
        Serial.println("Nhap mat khau (ket thuc bang #):");
    } else if (passwordEntryAllowed && key == '#' && passwordLength > 0) {
        passwordChecked = true;
        passwordCorrect = (strcmp(password, correctPassword) == 0);
        displayPasswordResult(passwordCorrect);
    } else if (passwordEntryAllowed && key != '#' && key != '*' && passwordLength < 4) {
        password[passwordLength++] = key;
        password[passwordLength] = '\0';
        lcd.setCursor(8 + passwordLength - 1, 3);
        lcd.print("*");
        Serial.print("*");
    }
}

#endif // MAIN_CPP