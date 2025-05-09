#include "modules/display/display.h"

extern char scanKeypad();
extern void buzzerControl(bool);
// Define LCD object
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Define constant
const int MAX_WRONG_ATTEMPTS = 5;

// Define variables declared as extern in display.h
fanState_t lastDisplayedFanState = FAN_OFF;

void displayMode() {
    String modeText;
    switch (fanControlMode) {
        case MANUAL_MODE:
            modeText = "Che do Quat: Thu cong";
            Serial.print(modeText);
            Serial.print(" | Quat: ");
            Serial.println(fanState == FAN_ON ? "ON" : "OFF");
            break;
        case SENSOR_MODE:
            modeText = "Che do Quat: Cam bien";
            Serial.print(modeText);
            Serial.print(" | ");
            if (dhtTempC > TEMP_THRESHOLD) {
                Serial.println(">30 do C, Quat: ON");
            } else {
                Serial.println("<30 do C, Quat: OFF");
            }
            break;
        case SETUP_MODE:
            modeText = "Che do Quat: Cai dat Thoi gian";
            Serial.print(modeText);
            if (fanOnTime > 0 && fanOffTime > 0) {
                Serial.print(" | ON: ");
                Serial.print(fanOnTime);
                Serial.print("s, OFF: ");
                Serial.print(fanOffTime);
                Serial.println("s");
            } else {
                Serial.println(" | Dang cho nhap thoi gian");
            }
            break;
    }

    switch (motorControlMode) {
        case BUTTON_MODE:
            modeText = "Che do Rem Cua: Thu cong";
            Serial.print(modeText);
            Serial.print(" | Rem Cua: ");
            switch (motorDirection) {
                case DIRECTION_1:
                    Serial.println("CLOSE");
                    break;
                case DIRECTION_2:
                    Serial.println("OPEN");
                    break;
                case STOPPED:
                    Serial.println("OFF");
                    break;
            }
            break;
        case SENSOR_MODE1:
            modeText = "Che do Rem Cua: Cam bien Anh sang";
            Serial.print(modeText);
            Serial.print(" | ");
            if (motorDirection == STOPPED) {
                Serial.println("Rem Cua OFF");
            } else {
                Serial.print(hasLight ? "Co Anh sang" : "Khong Anh sang");
                Serial.print(" | Rem Cua: ");
                Serial.println(hasLight ? "OPEN" : "CLOSE");
            }
            break;
        case SETUP_MODE_MOTOR:
            modeText = "Che do Rem Cua: Cai dat Thoi gian";
            Serial.print(modeText);
            if (motorForwardTime > 0 && motorReverseTime > 0 && motorStopTime > 0) {
                Serial.print(" | CLOSE: ");
                Serial.print(motorForwardTime);
                Serial.print("s, OPEN: ");
                Serial.print(motorReverseTime);
                Serial.print("s, OFF: ");
                Serial.print(motorStopTime);
                Serial.println("s");
            } else {
                Serial.println(" | Dang cho nhap thoi gian");
            }
            break;
    }
}
void displayPasswordResult(bool correct) {
    lcd.clear();
    if (correct) {
        inModeSelection = true;
        lcd.setCursor(0, 0);
        lcd.print("MAT KHAU DUNG");
        lcd.setCursor(0, 1);
        lcd.print("Chon che do: ");
        wrongPasswordAttempts = 0;
    } else {
        wrongPasswordAttempts++;
        if (wrongPasswordAttempts >= MAX_WRONG_ATTEMPTS) {
            isLockedOut = true;
            lockoutStartTime = millis();
            lcd.setCursor(0, 0);
            lcd.print("NHAP SAI 5 LAN");
            lcd.setCursor(0, 1);
            lcd.print("KHOA 5 GIAY");
            buzzerControl(true);
            Serial.println("Nhap sai 5 lan, khoa 5 giay");
        } else {
            lcd.setCursor(0, 0);
            lcd.print("MAT KHAU SAI");
            lcd.setCursor(0, 1);
            lcd.print("Nhan B de quay lai");
            lcd.setCursor(0, 2);
            lcd.print("Sai: ");
            lcd.print(wrongPasswordAttempts);
            lcd.print("/5 lan");
            Serial.print("Nhap sai, lan thu: ");
            Serial.println(wrongPasswordAttempts);
            char key;
            do {
                key = scanKeypad();
            } while (key == '\0' || key != 'B');
            lastDisplayedTemp = -999.0;
            lastDisplayedLight = !lastDisplayedLight;
        }
    }
    if (!correct && !isLockedOut) {
        passwordLength = 0;
        password[0] = '\0';
        passwordChecked = false;
        passwordEntryAllowed = false;
        //lcd.clear();
        displayLcd();
    }
}

void displayLcd() {
    
    if (inModeSelection && !inChangePasswordMode) {
        if (fanControlMode != SETUP_MODE && fanState != lastDisplayedFanState) {
            lcd.setCursor(0, 3);
            String fanStr = (fanState == FAN_ON) ? "ON" : "OFF";
            String fanDisplay = "Fan:" + fanStr;
            while (fanDisplay.length() < 9) {
                fanDisplay += " ";
            }
            lcd.print(fanDisplay);
            lastDisplayedFanState = fanState;
        }
        if (motorControlMode != SETUP_MODE_MOTOR && motorDirection != lastDisplayedMotorDirection) {
            lcd.setCursor(9, 3);
            String motorStr;
            if (motorDirection == DIRECTION_1) {
                motorStr = "CLOSE";
            } else if (motorDirection == DIRECTION_2) {
                motorStr = "OPEN";
            } else {
                motorStr = "OFF";
            }
            String motorDisplay = "Rem:" + motorStr;
            while (motorDisplay.length() < 11) {
                motorDisplay += " ";
            }
            lcd.print(motorDisplay);
            lastDisplayedMotorDirection = motorDirection;
        }
    } else if (!passwordEntryAllowed && !isLockedOut && !inChangePasswordMode && (dhtTempC != lastDisplayedTemp || hasLight != lastDisplayedLight)) {
        // Luôn cập nhật màn hình khi ở trạng thái bình thường
        // Xóa và cập nhật dòng 0 (Nhiệt độ)
        lcd.setCursor(0, 0);
        lcd.print("                    ");
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        if (!isnan(dhtTempC)) {
            lcd.print(dhtTempC, 1);
            lcd.print(" C");
        } else {
            lcd.print("Error");
        }

        // Xóa và cập nhật dòng 1 (Trạng thái ánh sáng)
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 1);
        lcd.print(hasLight ? "Co anh sang" : "Khong co anh sang");

        // Xóa và cập nhật dòng 2 (Yêu cầu nhập mật khẩu)
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("Nhan * de nhap MK");

        // Xóa dòng 3 để tránh nội dung cũ
        lcd.setCursor(0, 3);
        lcd.print("                    ");

        // Cập nhật các giá trị hiển thị cuối cùng
        lastDisplayedTemp = dhtTempC;
        lastDisplayedFanState = fanState;
        lastDisplayedLight = hasLight;
    }
}

void displayHome() {
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("- SMART HOME -");
    lcd.setCursor(3, 1);
    lcd.print("DANG THU HUYEN");
    lcd.setCursor(2, 2);
    lcd.print("MSSV: N21DCVT040");
}