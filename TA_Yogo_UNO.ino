/**
 * @file TA_Yogo_UNO.ino
 * @author Yogo (yogo@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-04-14
 * 
 * @copyright Copyright (c) 2023
 * 
 */

/**
 * @brief Relay
 */
#define RELAY_1_Pin     14
#define RELAY_2_Pin     15
#define RELAY_3_Pin     16
#define RELAY_4_Pin     17

#define RELAY_CHARGE_SUPPLY     RELAY_1_Pin
#define RELAY_CHARGE_BATT       RELAY_2_Pin
#define RELAY_BATT              RELAY_3_Pin
#define RELAY_LOAD              RELAY_4_Pin

#define RELAY_ON                LOW
#define RELAY_OFF               HIGH

/**
 * @brief LCD 16x2 
 * @library LiquidCrystal by Arduino Vesion 1.0.7
 * LCD RS pin to digital pin 12
 * LCD R/W pin to ground
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 */
#include <LiquidCrystal.h>
#define LCD_RS_Pin  12
#define LCD_RW_Pin  11
#define LCD_EN_Pin  10
#define LCD_D4_Pin  9
#define LCD_D5_Pin  8
#define LCD_D6_Pin  7
#define LCD_D7_Pin  6
#define LCD_BL_PIN  5
LiquidCrystal lcd(LCD_RS_Pin, LCD_EN_Pin, LCD_D4_Pin, LCD_D5_Pin, LCD_D6_Pin, LCD_D7_Pin);

/**
 * @brief INA219 
 * @library Adafruit INA219 by Adafruit Version 1.2.1
 *          Adafruit NeoPixel by Adafruit Version 1.11.0
 */
#include <Wire.h>
#include <Adafruit_INA219.h>
Adafruit_INA219 energy;

/**
 * @brief Temperature Sensor DS18B20
 * @library DallasTemperature by Miles Burton Version 3.9.0
 *          OneWire by Paul Stoffregen Version 2.3.7
 */
#include <OneWire.h>
#include <DallasTemperature.h>
#define DS18B20_DATA_Pin   2

OneWire oneWire(DS18B20_DATA_Pin);
DallasTemperature temperature(&oneWire);

/**
 * @brief SD Card
 * @library SD by Arduino Version 1.2.4
 */
#include <SPI.h>
#include <SD.h>
File myFile;

void setup(){
    Serial.begin(115200);

    /* LCD Setup */
    pinMode(LCD_RW_Pin, OUTPUT);
    digitalWrite(LCD_RW_Pin, LOW);

    pinMode(LCD_BL_PIN, OUTPUT);
    digitalWrite(LCD_BL_PIN, LOW);

    lcd.begin(16,2);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Tugas Akhir");
    lcd.setCursor(0,1);
    lcd.print("Yogo");

    /* Energy Meter INA219 Setup */
    energy.begin();

    /* Temperature DS18B20 Setup */
    temperature.begin();

    /* Relay Setup */
    pinMode(RELAY_1_Pin, OUTPUT);
    pinMode(RELAY_2_Pin, OUTPUT);
    pinMode(RELAY_3_Pin, OUTPUT);
    pinMode(RELAY_4_Pin, OUTPUT);

    digitalWrite(RELAY_1_Pin, RELAY_OFF);
    digitalWrite(RELAY_2_Pin, RELAY_OFF);
    digitalWrite(RELAY_3_Pin, RELAY_OFF);
    digitalWrite(RELAY_4_Pin, RELAY_OFF);

    delay(5000);
}

void loop(){
    digitalWrite(RELAY_1_Pin, RELAY_ON);
    digitalWrite(RELAY_2_Pin, RELAY_OFF);
    digitalWrite(RELAY_3_Pin, RELAY_OFF);
    digitalWrite(RELAY_4_Pin, RELAY_OFF);
    float busVoltage = 0;
    busVoltage = sensor219.getBusVoltage_V();

    
    delay(1000);

    digitalWrite(RELAY_1_Pin, RELAY_OFF);
    digitalWrite(RELAY_2_Pin, RELAY_ON);
    digitalWrite(RELAY_3_Pin, RELAY_OFF);
    digitalWrite(RELAY_4_Pin, RELAY_OFF);
    float current = 0; // Measure in milli amps
    busVoltage = sensor219.getBusVoltage_V();
    delay(1000);

    digitalWrite(RELAY_1_Pin, RELAY_OFF);
    digitalWrite(RELAY_2_Pin, RELAY_OFF);
    digitalWrite(RELAY_3_Pin, RELAY_ON);
    digitalWrite(RELAY_4_Pin, RELAY_OFF);
    float power = 0;
    power = busVoltage * (current/1000); // Calculate the Power
    delay(1000);

    digitalWrite(RELAY_1_Pin, RELAY_OFF);
    digitalWrite(RELAY_2_Pin, RELAY_OFF);
    digitalWrite(RELAY_3_Pin, RELAY_OFF);
    digitalWrite(RELAY_4_Pin, RELAY_ON);
    delay(1000);
}