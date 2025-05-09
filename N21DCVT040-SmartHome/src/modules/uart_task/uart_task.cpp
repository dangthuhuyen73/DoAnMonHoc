#include "uart_task.h"
#include "modules/display/display.h"
#include "modules/fan_control/fan_control.h"
#include "modules/motor_control/motor_control.h"
#include "modules/temperature_sensor/temperature_sensor.h"

extern fanControlMode_t fanControlMode;
extern motorControlMode_t motorControlMode;
extern int fanOnTime;
extern int fanOffTime;
extern int motorForwardTime;
extern int motorReverseTime;
extern int motorStopTime;
extern int inputState;
extern bool inChangePasswordMode;
extern char correctPassword[17];
extern bool inKeypadTestMode;
extern float dhtTempC;
extern unsigned long lastFanSwitchTime;
extern bool fanCycleState;
extern unsigned long lastMotorSwitchTime;
extern int motorCycleState;
extern bool inModeSelection;

void uartTask() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();

        if (inChangePasswordMode) {
            if (input.length() > 0 && input.length() <= 16) {
                strncpy(correctPassword, input.c_str(), sizeof(correctPassword) - 1);
                correctPassword[sizeof(correctPassword) - 1] = '\0';
                inChangePasswordMode = false;
                Serial.println("Doi mat khau thanh cong");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Doi MK thanh cong");
                delay(2000);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("MAT KHAU DUNG");
                lcd.setCursor(0, 1);
                lcd.print("Chon che do: ");
            } else {
                Serial.println("Mat khau moi khong hop le (1-16 ky tu)");
            }
        } else if (inputState == 1) {
            fanOnTime = input.toInt();
            inputState = 2;
            Serial.println("Nhap thoi gian OFF (giay, vi du: 30) roi nhan Enter:");
        } else if (inputState == 2) {
            fanOffTime = input.toInt();
            inputState = 0;
            Serial.println("Dang thuc hien.....");
            lastFanSwitchTime = millis();
            fanCycleState = true;
            fanStateWrite(FAN_ON);
        } else if (inputState == 3) {
            motorForwardTime = input.toInt();
            inputState = 4;
            Serial.println("Nhap thoi gian chay OPEN (giay, vi du: 30) roi nhan Enter:");
        } else if (inputState == 4) {
            motorReverseTime = input.toInt();
            inputState = 5;
            Serial.println("Nhap thoi gian OFF (giay, vi du: 30) roi nhan Enter:");
        } else if (inputState == 5) {
            motorStopTime = input.toInt();
            inputState = 0;
            Serial.println("Dang thuc hien.....");
            lastMotorSwitchTime = millis();
            motorCycleState = 1;
            motorDirectionWrite(DIRECTION_1);
        } else {
            char receivedChar = input.charAt(0);
            switch (receivedChar) {
                case '0':
                    availableCommands();
                    break;
                case '1':
                    fanControlMode = MANUAL_MODE;
                    fanOnTime = 0;
                    fanOffTime = 0;
                    Serial.println("Chuyen sang Che do Thu cong (Quat)");
                    displayMode();
                    break;
                case '2':
                    fanControlMode = SENSOR_MODE;
                    fanOnTime = 0;
                    fanOffTime = 0;
                    Serial.println("Chuyen sang Che do Cam bien (Quat)");
                    displayMode();
                    break;
                case '3':
                    fanControlMode = SETUP_MODE;    
                    Serial.println("Chuyen sang Che do Cai dat tg (Quat)");
                    Serial.println("Nhap thoi gian ON (giay, vi du: 30) roi nhan Enter:");
                    inputState = 1;
                    break;
                case '4':
                    motorControlMode = BUTTON_MODE;
                    motorDirection = STOPPED;
                    motorState = STOPPED;
                    motorRunning = false;
                    motorStoppedPermanently = false;
                    Serial.println("Chuyen sang Che do Thu cong (Rem)");
                    displayMode();
                    break;
                case '5':
                    motorControlMode = SENSOR_MODE1;
                    motorDirection = STOPPED;
                    motorState = STOPPED;
                    motorRunning = false;
                    motorStoppedPermanently = false;
                    Serial.println("Chuyen sang Che do Cam bien Anh sang (Rem)");
                    displayMode();
                    break;
                case '6':
                    motorControlMode = SETUP_MODE_MOTOR;
                    motorForwardTime = 0;
                    motorReverseTime = 0;
                    motorStopTime = 0;
                    motorCycleState = 0;
                    Serial.println("Chuyen sang Che do Cai dat tg (Rem)");
                    displayMode();
                    Serial.println("Nhap thoi gian chay CLOSE (giay, vi du: 30) roi nhan Enter:");    
                    inputState = 3;
                    break;
                case '7':
                    inKeypadTestMode = true;
                    Serial.println("Kiem tra ban phim: nhan phim tren keypad, nhan '#' de thoat");
                    break;
                case 'c':
                case 'C':
                    Serial.print("Nhiet do: ");
                    if (!isnan(dhtTempC)) {
                        Serial.print(dhtTempC);
                        Serial.println(" do C");
                    } else {
                        Serial.println("Khong co du lieu nhiet do hop le!");
                    }
                    break;
                case 'f':
                case 'F':
                    Serial.print("Nhiet do: ");
                    if (!isnan(dhtTempC)) {
                        Serial.print(dhtTempC * 9.0 / 5.0 + 32.0);
                        Serial.println(" do F");
                    } else {
                        Serial.println("Khong co du lieu nhiet do hop le!");
                    }
                    break;
                case 'A':
                case 'a':
                    if (inModeSelection) {
                        inChangePasswordMode = true;
                        Serial.println("Nhap mat khau moi (1-4 ky tu, ket thuc bang Enter):");
                    } else {
                        Serial.println("Can nhap mat khau dung truoc khi doi mat khau!");
                    }
                    break;
                default:
                    Serial.println("Lenh khong hop le! Nhan '0' de xem menu lenh.");
                    break;
            }
        }
    }
}

void availableCommands() {
    Serial.println("=== MENU LENH ===");
    Serial.println("Nhan '0' de hien thi menu nay");
    Serial.println("Nhan '1' de chuyen sang Che do Thu cong (Quat)");
    Serial.println("Nhan '2' de chuyen sang Che do Cam bien (Quat)");
    Serial.println("Nhan '3' de chuyen sang Che do Cai dat (Quat)");
    Serial.println("Nhan '4' de chuyen sang Che do Thu cong (Rem Cua)");
    Serial.println("Nhan '5' de chuyen sang Che do Cam bien Anh sang (Rem Cua)");
    Serial.println("Nhan '6' de chuyen sang Che do Cai dat tg (Rem Cua)");
    Serial.println("Nhan '7' Kiem tra ban phim: nhan phim tren keypad");
    Serial.println("Nhan 'f' hoac 'F' de xem nhiet do theo do Fahrenheit");
    Serial.println("Nhan 'c' hoac 'C' de xem nhiet do theo do Celsius");
    Serial.println("Nhan 'A' hoac 'a' de doi mat khau");
    Serial.println("================");
}