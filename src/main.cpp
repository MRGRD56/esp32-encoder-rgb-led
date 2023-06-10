#include <Arduino.h>
#include "../lib/button.h"
#include <U8g2lib.h>
#include <EncButton.h>
#include "ESP32Servo.h"
#include <cmath>

#define PIN_LED_R 25
#define PIN_LED_G 26
#define PIN_LED_B 27

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

EncButton<EB_TICK, 34, 35, 32> encoder;
Servo servo;

volatile int servoPosition = 90;

volatile byte selectedServoSetting = 0;

volatile short servoStep = 0;
volatile short servoDelay = 5;
volatile short servoMaxAngle = 180;

//volatile byte ledR = 255;
//volatile byte ledG = 170;
//volatile byte ledB = 20;
//volatile byte selectedLedPart = 0;
//volatile bool isSelectedLedPartBlink = false;

TaskHandle_t updateScreenTaskHandle;
TaskHandle_t encoderTaskHandle;
//TaskHandle_t selectedLedPartBlinkTaskHandle;

void updateScreen() {
    u8g2.firstPage();
    do {
//        u8g2.setFont(u8g2_font_10x20_t_cyrillic);
        u8g2.setFontMode(0);

        //----

        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_spleen8x16_mf);
        u8g2.setCursor(6, 16);
        u8g2.print("step");

        u8g2.setDrawColor(selectedServoSetting != 0);
//        u8g2.setFont(u8g2_font_spleen16x32_mf);
        u8g2.setCursor(66, 16);

        u8g2.print(servoStep);

        //----

        //----

        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_spleen8x16_mf);
        u8g2.setCursor(6, 16 + 20);
        u8g2.print("delay");

        u8g2.setDrawColor(selectedServoSetting != 1);
//        u8g2.setFont(u8g2_font_spleen16x32_mf);
        u8g2.setCursor(66, 16 + 20);

        u8g2.print(servoDelay);

        //----

        //----

        u8g2.setDrawColor(1);
        u8g2.setFont(u8g2_font_spleen8x16_mf);
        u8g2.setCursor(6, 16 + 20 + 20);
        u8g2.print("angle");

        u8g2.setDrawColor(selectedServoSetting != 2);
//        u8g2.setFont(u8g2_font_spleen16x32_mf);
        u8g2.setCursor(66, 16 + 20 + 20);

        u8g2.print(servoMaxAngle);

        //----


//
//        u8g2.setDrawColor(selectedLedPart != 1 && selectedLedPart != 3);
//        u8g2.printf("%02X", ledG);
//
//        u8g2.setDrawColor(selectedLedPart != 2 && selectedLedPart != 3);
//        u8g2.printf("%02X", ledB);
//        if (isPressed) {
//            u8g2.drawStr(0, 16 + 32, "BTN");
//        }
//        if (isLabelShown) {
//            u8g2.setFont(u8g2_font_streamline_interface_essential_other_t);
//            u8g2.drawGlyph(u8g2.getWidth() - 26, 16 + 22 + 22 + 2, 50);
//        }
    } while (u8g2.nextPage());
}

//void updateLed() {
//    analogWrite(PIN_LED_R, ledR);
//    analogWrite(PIN_LED_G, ledG);
//    analogWrite(PIN_LED_B, ledB);
//}

[[noreturn]]
void updateScreenTask(void* pvParam) {
    while (true) {
        if (xTaskNotifyWait(0, 0, nullptr, portMAX_DELAY) == pdTRUE) {
            updateScreen();
        }

        vTaskDelay(1);
    }
}

void updateScreenAsync() {
    xTaskNotify(updateScreenTaskHandle, 1, eSetValueWithOverwrite);
}

[[noreturn]]
void encoderTask(void* pvParam) {
    while (true) {
        encoder.tick();

        bool isUpdated = false;

        if (encoder.click()) {
            isUpdated = true;

            if (selectedServoSetting == 2) {
                selectedServoSetting = 0;
            } else {
                selectedServoSetting++;
            }
        }

        int step = 0;
        if (encoder.left()) {
            step = -1;
        } else if (encoder.right()) {
            step = 1;
        } else if (encoder.leftH()) {
            step = -5;
        } else if (encoder.rightH()) {
            step = 5;
        }

        if (step != 0) {
            isUpdated = true;

            if (selectedServoSetting == 0) {
                servoStep = constrain(servoStep + step, 0, 180);
            } else if (selectedServoSetting == 1) {
                servoDelay = constrain(servoDelay + step, 0, 1000);
            } else if (selectedServoSetting == 2) {
                servoMaxAngle = constrain(servoMaxAngle + step, 0, 180);
            }
//            for (int i = 0; i < abs(step); i++) {
//                servoPosition = constrain(servoPosition + singleStep, 0, 180);
//
//                servo.write(servoPosition);
//                vTaskDelay(pdMS_TO_TICKS(2));
//
//                updateScreenAsync();
//            }
        }

        if (isUpdated) {
            //Serial.printf("#%02X%02X%02X\n", ledR, ledG, ledB);

            updateScreenAsync();
        }

        vTaskDelay(1);
    }
}

//[[noreturn]]
//void selectedLedPartBlinkTask(void* pvParam) {
//    while (true) {
//        isSelectedLedPartBlink = !isSelectedLedPartBlink;
//        updateScreenAsync();
//
//        vTaskDelay(500);
//    }
//}

[[noreturn]]
void updateServoTask(void* pvParam) {
    while (true) {
        for (int i = (180 - servoMaxAngle) / 2; i <= (180 - (180 - servoMaxAngle) / 2); i += servoStep) {
            servoPosition = constrain(i, 0, 180);

            servo.write(servoPosition);
            vTaskDelay(pdMS_TO_TICKS(servoDelay));
        }

        for (int i = (180 - (180 - servoMaxAngle) / 2); i >= (180 - servoMaxAngle) / 2; i -= servoStep) {
            servoPosition = constrain(i, 0, 180);

            servo.write(servoPosition);
            vTaskDelay(pdMS_TO_TICKS(servoDelay));
        }
    }
}

void setup() {
    Serial.begin(9600);

//    pinMode(PIN_LED_R, OUTPUT);
//    pinMode(PIN_LED_G, OUTPUT);
//    pinMode(PIN_LED_B, OUTPUT);

    servo.attach(25);

    servo.write(servoPosition);
    delay(300);

    u8g2.begin();

    xTaskCreate(encoderTask, "encoder", CONFIG_ESP_MAIN_TASK_STACK_SIZE, nullptr, 1, &encoderTaskHandle);
    xTaskCreate(updateScreenTask, "update-screen", CONFIG_ESP_MAIN_TASK_STACK_SIZE, nullptr, 1, &updateScreenTaskHandle);
    xTaskCreate(updateServoTask, "update-servo", CONFIG_ESP_MAIN_TASK_STACK_SIZE, nullptr, 1, nullptr);

    updateScreenAsync();
}

void loop() { }