#include <Arduino.h>
#include <ctype.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//=====[Định nghĩa kiểu dữ liệu]===================================
enum motorDirection_t { STOPPED, DIRECTION_1, DIRECTION_2 };
enum fanState_t { FAN_OFF, FAN_ON };
enum fanControlMode_t { MANUAL_MODE, SENSOR_MODE, SETUP_MODE };
enum motorControlMode_t { BUTTON_MODE, SENSOR_MODE1, SETUP_MODE_MOTOR };

//=====[Hằng số]======================================================
#define TEMP_THRESHOLD         30.0
#define TIME_INCREMENT_MS      50
#define DEBOUNCE_DELAY         100
#define DHT_READ_INTERVAL      2000
#define MOTOR_RUN_TIME_MS      10000
#define BUZZER_DURATION_MS     5000
#define LOCKOUT_DURATION_MS    5000
#define MAX_WRONG_ATTEMPTS     5
#define DHT_READINGS           5      // Số lần đọc để lấy trung bình
#define DHT_CALIBRATION_OFFSET -4.0  

//=====[Định nghĩa chân]===============================================
#define motorM1Pin       PF2  
#define motorM2Pin       PE3  
#define motorRunButton   PE11  
#define motorReverseButton  PE9
#define motorStopButton  PF14  
#define relayPin         PE6  
#define fanOnButton      PF15   
#define fanOffButton     PE13   
#define dhtPin           PF13
#define buzzerPin        PE10
#define DHTTYPE          DHT11
#define LIGHT_SENSOR_PIN PG9

// Cấu hình bàn phím
#define KEYPAD_NUMBER_OF_ROWS 4
#define KEYPAD_NUMBER_OF_COLS 4
const int keypadRowPins[KEYPAD_NUMBER_OF_ROWS] = {PB3, PB5, PC7, PA15};
const int keypadColPins[KEYPAD_NUMBER_OF_COLS] = {PB12, PB13, PB15, PC6};

// Bố cục bàn phím
char keypad[KEYPAD_NUMBER_OF_ROWS][KEYPAD_NUMBER_OF_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

//=====[Biến toàn cục]==========================
static motorDirection_t motorDirection = STOPPED;
static motorDirection_t motorState = STOPPED;
static fanState_t fanState = FAN_OFF;
static fanState_t lastFanState = FAN_OFF;
static fanControlMode_t fanControlMode = MANUAL_MODE;
static motorControlMode_t motorControlMode = BUTTON_MODE;
static bool buttonPressed = false;
static unsigned long lastDebounceTime = 0;
static float dhtTempC = 0.0;
static unsigned long lastDhtReadTime = 0;
static bool hasLight = false;
static bool lastLightState = false;
static unsigned long lastMotorStartTime = 0;
static bool motorRunning = false;
static bool motorStoppedPermanently = false;
static bool inKeypadTestMode = false;
static char lastKeyPressed = '\0';

// Lưu trữ mật khẩu
static char password[17] = "";
static int passwordLength = 0;
static char correctPassword[17] = "0703"; // Thay const char[] bằng char[] để có thể thay đổi
static bool passwordCorrect = false;
static bool passwordChecked = false;
static bool passwordEntryAllowed = false;
static bool inModeSelection = false;
static bool inChangePasswordMode = false; // Biến mới để theo dõi chế độ đổi mật khẩu
static char newPassword[17] = ""; // Lưu mật khẩu mới tạm thời
static int newPasswordLength = 0;

// Quản lý số lần nhập sai và khóa
static int wrongPasswordAttempts = 0;
static bool isLockedOut = false;
static unsigned long lockoutStartTime = 0;

// Quản lý còi
static bool buzzerActive = false;
static unsigned long buzzerStartTime = 0;

// Trạng thái nút
static int lastFanOnState = HIGH;
static int lastFanOffState = HIGH;
static int lastMotorRunState = HIGH;
static int lastMotorReverseState = HIGH;
static int lastMotorStopState = HIGH;

// Giá trị hiển thị trước đó cho LCD
static float lastDisplayedTemp = -999.0;
static fanState_t lastDisplayedFanState = FAN_OFF;
static motorDirection_t lastDisplayedMotorDirection = STOPPED;
static bool lastDisplayedLight = false;

// Đối tượng DHT11
static DHT dht(dhtPin, DHTTYPE);

// LCD I2C
static LiquidCrystal_I2C lcd(0x27, 20, 4);

// Biến cho chế độ cài đặt thời gian bật/tắt quạt
static int fanOnTime = 0;
static int fanOffTime = 0;
static bool fanCycleState = false;
static unsigned long lastFanSwitchTime = 0;
static int inputState = 0; // 0: không chờ, 1: chờ fan onTime, 2: chờ fan offTime

// Biến để lưu trữ chuỗi nhập thời gian từ bàn phím
static String onTimeInput = "";
static String offTimeInput = "";
static String forwardTimeInput = "";
static String reverseTimeInput = "";
static String stopTimeInput = "";

// Biến cho chế độ cài đặt thời gian động cơ
static int motorForwardTime = 0;
static int motorReverseTime = 0;
static int motorStopTime = 0;
static unsigned long lastMotorSwitchTime = 0;
static int motorCycleState = 0;

//=====[Khai báo hàm]============================
void motorControlInit();
void motorControlUpdate();
void motorDirectionWrite(motorDirection_t direction);
motorDirection_t motorDirectionRead();
void motorRunButtonCallback();
void motorReverseButtonCallback();
void motorStopButtonCallback();
void fanControlInit();
void fanControlUpdate();
void fanStateWrite(fanState_t state);
void fanOnButtonCallback();
void fanOffButtonCallback();
void debounceHandler();
void temperatureUpdate();
void uartTask();
void availableCommands();
void displayMode();
void displayLcd();
void displayPasswordResult(bool correct);
char scanKeypad();
void buzzerControl(bool activate);
void handleKeypadInput();
void displayHome();

//=====[Hàm setup]============================
void setup() {
    Serial.begin(115200);
    while (!Serial);

    motorControlInit();
    fanControlInit();
    dht.begin();

    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    buzzerControl(false);

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
    pinMode(LIGHT_SENSOR_PIN, INPUT);

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

//=====[Hàm loop]==========================================
void loop() {
    // Đọc trạng thái nút và cảm biến ánh sáng
    int fanOnState = digitalRead(fanOnButton);
    int fanOffState = digitalRead(fanOffButton);
    int motorRunState = digitalRead(motorRunButton);
    int motorReverseState = digitalRead(motorReverseButton);
    int motorStopState = digitalRead(motorStopButton);
    hasLight = digitalRead(LIGHT_SENSOR_PIN) == LOW;

    // Quản lý còi và khóa
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

    // Cập nhật các thành phần
    temperatureUpdate();
    motorControlUpdate();
    fanControlUpdate();
    debounceHandler();

    // Xử lý nhập liệu bàn phím nếu không bị khóa
    if (!isLockedOut) {
        handleKeypadInput();
    }

    // Cập nhật hiển thị LCD
    displayLcd();

    // Xử lý giao tiếp UART
    uartTask();

    // Cập nhật trạng thái cuối cùng
    lastFanOnState = fanOnState;
    lastFanOffState = fanOffState;
    lastMotorRunState = motorRunState;
    lastMotorReverseState = motorReverseState;
    lastMotorStopState = motorStopState;

    delay(TIME_INCREMENT_MS);
}

//=====[Điều khiển động cơ]===============
void motorControlInit() {
    pinMode(motorM1Pin, OUTPUT);
    pinMode(motorM2Pin, OUTPUT);
    digitalWrite(motorM1Pin, HIGH);
    digitalWrite(motorM2Pin, HIGH);
}

void motorControlUpdate() {
    static motorDirection_t lastMotorDirection = STOPPED;

    if (motorControlMode == SENSOR_MODE1) {
        if (hasLight != lastLightState) {
            lastLightState = hasLight;
            motorStoppedPermanently = false;
            motorRunning = false;
            lastMotorStartTime = 0;
        }
        if (!motorStoppedPermanently && !motorRunning) {
            motorDirection = hasLight ? DIRECTION_2 : DIRECTION_1;
            motorRunning = true;
            lastMotorStartTime = millis();
        }
        if (motorRunning && (millis() - lastMotorStartTime >= MOTOR_RUN_TIME_MS)) {
            motorDirection = STOPPED;
            motorRunning = false;
            motorStoppedPermanently = true;
        }
    } else if (motorControlMode == SETUP_MODE_MOTOR) {
        if (motorForwardTime > 0 && motorReverseTime > 0 && motorStopTime > 0) {
            unsigned long currentTime = millis();
            if (motorCycleState == 1 && currentTime - lastMotorSwitchTime >= (unsigned long)motorForwardTime * 1000) {
                motorDirectionWrite(DIRECTION_2);
                motorCycleState = 2;
                lastMotorSwitchTime = currentTime;
            } else if (motorCycleState == 2 && currentTime - lastMotorSwitchTime >= (unsigned long)motorReverseTime * 1000) {
                motorDirectionWrite(STOPPED);
                motorCycleState = 0;
                lastMotorSwitchTime = currentTime;
            } else if (motorCycleState == 0 && currentTime - lastMotorSwitchTime >= (unsigned long)motorStopTime * 1000) {
                motorDirectionWrite(DIRECTION_1);
                motorCycleState = 1;
                lastMotorSwitchTime = currentTime;
            }
        } else if (motorDirection != STOPPED) {
            motorDirectionWrite(STOPPED);
        }
    }

    if (motorDirection != motorState) {
        switch (motorDirection) {
            case DIRECTION_1:
                digitalWrite(motorM1Pin, LOW);
                digitalWrite(motorM2Pin, HIGH);
                break;
            case DIRECTION_2:
                digitalWrite(motorM1Pin, HIGH);
                digitalWrite(motorM2Pin, LOW);
                break;
            case STOPPED:
            default:
                digitalWrite(motorM1Pin, HIGH);
                digitalWrite(motorM2Pin, HIGH);
                break;
        }
        motorState = motorDirection;
        if (motorState != lastMotorDirection) {
            displayMode();
            lastMotorDirection = motorState;
        }
    }
}

void motorDirectionWrite(motorDirection_t direction) {
    motorDirection = direction;
}

motorDirection_t motorDirectionRead() {
    return motorDirection;
}

void motorRunButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY && motorControlMode == BUTTON_MODE) {
        motorDirectionWrite(DIRECTION_1);
        lastInterruptTime = millis();
    }
}

void motorReverseButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY && motorControlMode == BUTTON_MODE) {
        motorDirectionWrite(DIRECTION_2);
        lastInterruptTime = millis();
    }
}

void motorStopButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (millis() - lastInterruptTime > DEBOUNCE_DELAY && motorControlMode == BUTTON_MODE) {
        motorDirectionWrite(STOPPED);
        lastInterruptTime = millis();
    }
}

//=====[Điều khiển quạt]==================
void fanControlInit() {
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    fanState = FAN_OFF;
}

void fanControlUpdate() {
    if (fanControlMode == SENSOR_MODE) {
        fanState_t newState = (dhtTempC > TEMP_THRESHOLD) ? FAN_ON : FAN_OFF;
        if (newState != fanState) fanStateWrite(newState);
    } else if (fanControlMode == SETUP_MODE) {
        if (fanOnTime > 0 && fanOffTime > 0) {
            unsigned long currentTime = millis();
            if (fanCycleState && currentTime - lastFanSwitchTime >= (unsigned long)fanOnTime * 1000) {
                fanStateWrite(FAN_OFF);
                fanCycleState = false;
                lastFanSwitchTime = currentTime;
            } else if (!fanCycleState && currentTime - lastFanSwitchTime >= (unsigned long)fanOffTime * 1000) {
                fanStateWrite(FAN_ON);
                fanCycleState = true;
                lastFanSwitchTime = currentTime;
            }
        } else if (fanState != FAN_OFF) {
            fanStateWrite(FAN_OFF);
        }
    }
    digitalWrite(relayPin, (fanState == FAN_ON) ? HIGH : LOW);
}

void fanStateWrite(fanState_t state) {
    if (state != lastFanState) {
        fanState = state;
        displayMode();
        lastFanState = state;
    } else {
        fanState = state;
    }
}

void fanOnButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (!buttonPressed && fanControlMode == MANUAL_MODE && millis() - lastInterruptTime > DEBOUNCE_DELAY) {
        buttonPressed = true;
        lastInterruptTime = millis();
        fanStateWrite(FAN_ON);
    }
}

void fanOffButtonCallback() {
    static unsigned long lastInterruptTime = 0;
    if (!buttonPressed && fanControlMode == MANUAL_MODE && millis() - lastInterruptTime > DEBOUNCE_DELAY) {
        buttonPressed = true;
        lastInterruptTime = millis();
        fanStateWrite(FAN_OFF);
    }
}

void debounceHandler() {
    if (millis() - lastDebounceTime >= DEBOUNCE_DELAY) {
        buttonPressed = false;
        lastDebounceTime = millis();
    }
}

//=====[Cảm biến nhiệt độ DHT11]===================
void temperatureUpdate() {
    if (millis() - lastDhtReadTime >= DHT_READ_INTERVAL) {
        float tempSum = 0.0;
        int validReadings = 0;

        // Lấy nhiều phép đo để tính trung bình, giảm nhiễu
        for (int i = 0; i < DHT_READINGS; i++) {
            float temp = dht.readTemperature();
            if (!isnan(temp)) {
                tempSum += temp + DHT_CALIBRATION_OFFSET; // Áp dụng bù hiệu chỉnh
                validReadings++;
            }
            delay(100); // Độ trễ nhỏ giữa các lần đọc để ổn định cảm biến
        }

        if (validReadings > 0) {
            dhtTempC = tempSum / validReadings; // Tính trung bình
        } else {
            Serial.println("Lỗi đọc cảm biến DHT11!");
        }
        lastDhtReadTime = millis();
    }
}
//=====[Bàn phím ma trận]===================
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
        }else if (motorControlMode == SETUP_MODE_MOTOR && (inputState == 3 || inputState == 4 || inputState == 5)) {
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
                    displayLcd(); // Hiển thị màn hình chính với thông báo nhập mật khẩu
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
            do { key = scanKeypad(); } while (key == '\0' || key != 'B');
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

//=====[Quản lý còi]===================
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

//=====[Giao tiếp UART]============================
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
    } else if (!passwordEntryAllowed && !isLockedOut && !inChangePasswordMode && (dhtTempC != lastDisplayedTemp || fanState != lastDisplayedFanState || hasLight != lastDisplayedLight)) {
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        if (!isnan(dhtTempC)) {
            lcd.print(dhtTempC, 1);
            lcd.print(" C            ");
        } else {
            lcd.print("Error          ");
        }

        lcd.setCursor(0, 1);
        lcd.print("                    ");
        lcd.setCursor(0, 1);
        lcd.print(hasLight ? "Co anh sang     " : "Khong co anh sang");

        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 2);
        lcd.print("Nhan * de nhap MK");

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