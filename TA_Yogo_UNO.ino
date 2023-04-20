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
 *          SdFat by Bill Greimen Version 2.2.2
 */
#include <SPI.h>
#include <SD.h>
#define SD_CS_Pin           53      // pin 53 Arduino Mega
#define LOG_NUMBER_FILE     "number.txt"
File myFile;

#define TIMEOUT_UPDATE      1000    // ms
#define TIMEOUT_DETECT_BATT 2000    // ms
#define TIMEOUT_SAVE        60000   // ms
typedef struct{
    uint32_t update;
    uint32_t detect_batt;
    uint32_t button;
    uint32_t save;
}SYSTEM_TimeoutTypeDef;

typedef enum{
    SYSTEM_IDLE = 0,
    SYSTEM_CHARGE,
    SYSTEM_LOAD,
    SYSTEM_FINISH,
    SYSTEM_OVERTEMP,
}SYSTEM_Flag_t;

typedef struct{
    float voltage;
    float current;
    float capacity;
    float temperature;
}SYSTEM_DataTypeDef;

SYSTEM_TimeoutTypeDef timeout;
SYSTEM_Flag_t flag;
SYSTEM_DataTypeDef data;

char display_buffer[20];
String log_number_str;

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
    updateSensor();

    if(data.temperature > 50){
        flag = SYSTEM_OVERTEMP;
    }
    else if(data.temperature < 35){
        flag = SYSTEM_IDLE;
    }

    if((millis() - timeout.update) > TIMEOUT_UPDATE){
        timeout.update = millis();

        if(flag == SYSTEM_CHARGE){
            systemCharge();

            if(data.voltage > 4.2 && data.current < 100){
                dataClear();
                flag = SYSTEM_LOAD;
            }
        }
        else if(flag == SYSTEM_LOAD){
            systemLoad();

            if(data.voltage < 2.8){
                flag = SYSTEM_FINISH;
            }
        }
        else if(flag == SYSTEM_FINISH){
            systemFinish();
        }
        else if(flag == SYSTEM_OVERTEMP){
            systemOverTemp();
        }
        else{
            systemIdle();

            if(data.voltage > 1.0){
                if((millis() - timeout.detect_batt) > TIMEOUT_DETECT_BATT){
                    createNewLogNumber();
                    createNewLog();
                    flag = SYSTEM_CHARGE;
                }
            }
            else{
                timeout.detect_batt = millis();
            }
        }
    }

    if(flag == SYSTEM_CHARGE || flag == SYSTEM_LOAD){
        if((millis() - timeout.save) > TIMEOUT_SAVE){
            timeout.save = millis();

            saveLog();
        }
    }
    else{
        timeout.save = millis();
    }
}

void updateSensor(void){
    data.voltage = energy.getBusVoltage_V();
    Serial.print(data.voltage);     Serial.println(" V");

    data.current = energy.getCurrent_mA();       // Measure in milli amps
    Serial.print(data.current);     Serial.println(" mA");

    data.capacity += data.current / 3600;
    Serial.print(data.capacity);    Serial.println(" mAh");

    temperature.requestTemperatures();
    data.temperature = temperature.getTempCByIndex(0);
    Serial.print(data.temperature); Serial.println(" C");
}

void systemIdle(void){
    digitalWrite(RELAY_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
    digitalWrite(RELAY_LOAD, RELAY_OFF);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Insert Battery");
    lcd.setCursor(0,1);
    lcd.print("Press Button");
}

void systemCharge(void){
    digitalWrite(RELAY_LOAD, RELAY_OFF);

    digitalWrite(RELAY_BATT, RELAY_ON);
    digitalWrite(RELAY_CHARGE_BATT, RELAY_ON);
    digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_ON);

    lcd.clear();
    lcd.setCursor(0,0);
    sprintf(display_buffer, "Charge %0f C", data.temperature);
    lcd.print(display_buffer);
    lcd.setCursor(0,1);
    sprintf(display_buffer, "%.02f V  %04f mA", data.voltage, data.current);
    lcd.print(display_buffer);
}

void systemLoad(void){
    digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
    
    digitalWrite(RELAY_BATT, RELAY_ON);
    digitalWrite(RELAY_LOAD, RELAY_ON);

    lcd.clear();
    lcd.setCursor(0,0);
    sprintf(display_buffer, "Load %0f mAh", data.capacity);
    lcd.print(display_buffer);
    lcd.setCursor(0,1);
    sprintf(display_buffer, "%.02f V  %04f mA", data.voltage, data.current);
    lcd.print(display_buffer);
}

void systemFinish(){
    digitalWrite(RELAY_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
    digitalWrite(RELAY_LOAD, RELAY_OFF);

    lcd.clear();
    lcd.setCursor(0,0);
    uint16_t log = getLogNumber();
    sprintf(display_buffer, "Finish %d", log);
    lcd.print(display_buffer);
    lcd.setCursor(0,1);
    sprintf(display_buffer, "Capacity %0f mAh", data.capacity);
    lcd.print(display_buffer);
}

void systemOverTemp(){
    digitalWrite(RELAY_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
    digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
    digitalWrite(RELAY_LOAD, RELAY_OFF);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Over Temperature");
    lcd.setCursor(0,1);
    sprintf(display_buffer, "Temp %0f C", data.temperature);
    lcd.print(display_buffer);
}

void dataClear(void){
    data.capacity = 0;
}

uint16_t getLogNumber(void){
    uint16_t ret = 0;

    if (SD.begin(SD_CS_Pin)) {
        File dataFile = SD.open(LOG_NUMBER_FILE);
    
        if (dataFile) {
            log_number_str = "";

            while(dataFile.available()){
                char a = dataFile.read();
                log_number_str += a;
            }
            ret = log_number_str.toInt();
            ret++;

            dataFile.close();
        }
        else {
            Serial.print("Fail to Open");
            Serial.println();
        }   
    }
    else{
        Serial.println("SD isn't Present.");
    }

    SD.end();

    return ret;
}

uint16_t createNewLogNumber(void){
    uint16_t ret = 0;

    if (SD.begin(SD_CS_Pin)) {
        File dataFile = SD.open(LOG_NUMBER_FILE);
    
        if (dataFile) {
            log_number_str = "";

            while(dataFile.available()){
                char a = dataFile.read();
                log_number_str += a;
            }
            ret = log_number_str.toInt();
            ret++;

            dataFile.close();

            for(uint8_t i=0; i<3; i++){
                SD.remove(LOG_NUMBER_FILE);
                if(!SD.exists(LOG_NUMBER_FILE)){
                    break;
                }
            }
        }
        else {
            Serial.print("Fail to Open");
            Serial.println();
        }   

        dataFile = SD.open(LOG_NUMBER_FILE, FILE_WRITE);

        if(dataFile){
            dataFile.print(ret);
            dataFile.close();
        }
    }
    else{
        Serial.println("SD isn't Present.");
    }

    SD.end();

    return ret;
}

void createNewLog(void){
    uint16_t log = getLogNumber();
    sprintf(display_buffer, "%d.csv", log);
    
    if (SD.begin(SD_CS_Pin)) {
        if(SD.exists(display_buffer)){
            for(uint8_t i=0; i<3; i++){
                SD.remove(display_buffer);
                if(!SD.exists(display_buffer)){
                    break;
                }
            }
        }

        File dataFile = SD.open(display_buffer, FILE_WRITE);
    
        if (dataFile) {
            dataFile.println("volage (V),current (mA),capacity (mAh),temperature (C)");
            dataFile.close();
        }
        else {
            Serial.print("Fail to Open");
            Serial.println();
        }   
    }
    else{
        Serial.println("SD isn't Present.");
    }

    SD.end();
}

void saveLog(void){
    uint16_t log = getLogNumber();
    sprintf(display_buffer, "%d.csv", log);
    
    if (SD.begin(SD_CS_Pin)) {
        File dataFile = SD.open(display_buffer, FILE_WRITE);
    
        if (dataFile) {
            sprintf(display_buffer, "%.02f,%04f", data.voltage, data.current);
            dataFile.print(display_buffer);
            sprintf(display_buffer, "%04f,%02f", data.capacity, data.temperature);
            dataFile.println(display_buffer);
            dataFile.close();
        }
        else {
            Serial.print("Fail to Open");
            Serial.println();
        }   
    }
    else{
        Serial.println("SD isn't Present.");
    }

    SD.end();
}