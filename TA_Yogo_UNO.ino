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
 * @brief Button
 */
#define BUTTON_PIN      3

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

#define TIMEOUT_UPDATE          1000    // ms
#define TIMEOUT_DETECT_BATT     2000    // ms
#define TIMEOUT_SAVE            60000   // ms
#define TIMEOUT_DISPLAY_TITLE   3000    //
#define TIMEOUT_DISPLAY_DATA    10000    //
typedef struct{
    uint32_t update;
    uint32_t detect_batt;
    uint32_t button;
    uint32_t save;
    uint32_t display;
}SYSTEM_TimeoutTypeDef;

typedef enum{
    SYSTEM_IDLE = 0,
    SYSTEM_CHARGE,
    SYSTEM_LOAD,
    SYSTEM_FINISH,
    SYSTEM_OVERTEMP,
    SYSTEM_START,
}SYSTEM_Flag_t;

typedef struct{
    float voltage;
    float current;
    float capacity;
    float temperature;
    float voltage_full;
    float voltage_empty;
    float current_full;
    float temperature_limit;
    float temperature_release;
    float capacity_limit;
    float temperature_max;
    float capacity_percent;
    float state_of_health;
}SYSTEM_DataTypeDef;

typedef struct{
    float a;
    float b;
    float c;
    float d;
    float level;
}MEMBERSHIP_TrapesiumTypeDef;

typedef struct{
    MEMBERSHIP_TrapesiumTypeDef low;
    MEMBERSHIP_TrapesiumTypeDef medium;
    MEMBERSHIP_TrapesiumTypeDef high;
}FUZZY_KapasitasTypeDef;

typedef struct{
    MEMBERSHIP_TrapesiumTypeDef dingin;
    MEMBERSHIP_TrapesiumTypeDef normal;
    MEMBERSHIP_TrapesiumTypeDef panas;
}FUZZY_SuhuTypeDef;

typedef struct{
    MEMBERSHIP_TrapesiumTypeDef rusak;
    MEMBERSHIP_TrapesiumTypeDef lemah;
    MEMBERSHIP_TrapesiumTypeDef bagus;
}FUZZY_SoHTypeDef;

typedef struct{
    float pembilang;
    float penyebut;
}FUZZY_CoGTypeDef;

typedef struct{
    FUZZY_KapasitasTypeDef kapasitas;
    FUZZY_SuhuTypeDef suhu;
    FUZZY_SoHTypeDef soh;
    FUZZY_CoGTypeDef cog;
    float tp1;
    float tp2;
    float tp3;
    float tp4;
    float tp5;
    float tp6;
    float tp7;
    float tp8;
    float tp9;
    float tp10;
}FUZZY_TypeDef;

SYSTEM_TimeoutTypeDef timeout;
SYSTEM_Flag_t flag;
SYSTEM_DataTypeDef data;
FUZZY_TypeDef fuzzy;

char display_buffer[20];
char f_str[10];
char f_str1[10];
String log_number_str;

typedef struct{
    String str;
    bool complete;
}SYSTEM_CommandTypeDef;
SYSTEM_CommandTypeDef command;

bool led;

void ISR_BUTTON(void){
    uint8_t bounce;
    while(bounce < 100){
        if(digitalRead(BUTTON_PIN) == LOW){
            bounce++;
            delay(1);
        }
        else{
            break;
        }
    }

    if(bounce >= 100){
        Serial.println("Button Pressed");

        if(flag == SYSTEM_IDLE){
            flag = SYSTEM_START;
            timeout.display = millis();
        }
        else if(flag == SYSTEM_START){
            flag = SYSTEM_IDLE;
        }
        else if(flag == SYSTEM_FINISH || flag == SYSTEM_OVERTEMP){
            flag = SYSTEM_IDLE;
            timeout.display = millis();
        }
    }
    else{
        Serial.println("Button Bounce");
    }
}

void setup(){
    Serial.begin(115200);
    Serial1.begin(9600);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), ISR_BUTTON, FALLING);

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

    /* Fuzzy Logic */
    /* Membership functions */
    fuzzy.kapasitas.low.a = 0;
    fuzzy.kapasitas.low.b = 0;
    fuzzy.kapasitas.low.c = 30;
    fuzzy.kapasitas.low.d = 40;

    fuzzy.kapasitas.medium.a = 30;
    fuzzy.kapasitas.medium.b = 40;
    fuzzy.kapasitas.medium.c = 70;
    fuzzy.kapasitas.medium.d = 80;

    fuzzy.kapasitas.high.a = 70;
    fuzzy.kapasitas.high.b = 80;
    fuzzy.kapasitas.high.c = 100;
    fuzzy.kapasitas.high.d = 100;

    fuzzy.suhu.dingin.a = 0;
    fuzzy.suhu.dingin.b = 0;
    fuzzy.suhu.dingin.c = 20;
    fuzzy.suhu.dingin.d = 30;

    fuzzy.suhu.normal.a = 20;
    fuzzy.suhu.normal.b = 30;
    fuzzy.suhu.normal.c = 35;
    fuzzy.suhu.normal.d = 45;

    fuzzy.suhu.panas.a = 35;
    fuzzy.suhu.panas.b = 45;
    fuzzy.suhu.panas.c = 100;
    fuzzy.suhu.panas.d = 100;

    fuzzy.soh.rusak.a = 0;
    fuzzy.soh.rusak.b = 0;
    fuzzy.soh.rusak.c = 40;
    fuzzy.soh.rusak.d = 50;

    fuzzy.soh.lemah.a = 40;
    fuzzy.soh.lemah.b = 50;
    fuzzy.soh.lemah.c = 70;
    fuzzy.soh.lemah.d = 80;

    fuzzy.soh.bagus.a = 70;
    fuzzy.soh.bagus.b = 80;
    fuzzy.soh.bagus.c = 100;
    fuzzy.soh.bagus.d = 100;

    fuzzy.tp1 = 10;
    fuzzy.tp2 = 20;
    fuzzy.tp3 = 30;
    fuzzy.tp4 = 40;
    fuzzy.tp5 = 50;
    fuzzy.tp6 = 60;
    fuzzy.tp7 = 70;
    fuzzy.tp8 = 80;
    fuzzy.tp9 = 90;
    fuzzy.tp10 = 100;

    data.voltage_full = 4.0;
    data.voltage_empty = 2.8;
    data.current_full = 100;
    data.temperature_limit = 45;
    data.temperature_release = 35;
    data.capacity_limit = 2000;

    data.capacity = 239;
    data.temperature_max = 32;

    fuzzyProcess();

    dataClear();

    flag = SYSTEM_IDLE;
}

void loop(){
    if(command.complete == true){
        command.complete = false;
        Serial.println(command.str);
        processData();
    }

    /* LED Blynk */
    if(led == true){
        digitalWrite(LED_BUILTIN, LOW);
        led = false;
    }
    else{
        digitalWrite(LED_BUILTIN, HIGH);
        led = true;
    }

    /* Update Sensor */
    data.voltage = energy.getBusVoltage_V();
    data.current = energy.getCurrent_mA();
    
    if(data.current < 0){
        data.current *= -1;
    } 

    temperature.requestTemperatures();
    data.temperature = temperature.getTempCByIndex(0);

    /* Update Display */
    if((millis() - timeout.update) > TIMEOUT_UPDATE){
        timeout.update = millis();
        
        if(flag == SYSTEM_CHARGE){
            digitalWrite(RELAY_LOAD, RELAY_OFF);

            digitalWrite(RELAY_BATT, RELAY_ON);
            digitalWrite(RELAY_CHARGE_BATT, RELAY_ON);
            digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_ON);

            /* Count Capacity */
            data.capacity += data.current / 3600;

            if(data.voltage > data.voltage_full && data.current < data.current_full){
                dataClear();
                flag = SYSTEM_LOAD;
            }

            if((millis() - timeout.display) < TIMEOUT_DISPLAY_TITLE){
                displayTitle("Charge");
            }
            else if((millis() - timeout.display) < TIMEOUT_DISPLAY_DATA){
                displayData();
            }
            else{
                Serial.println("Charge");
                timeout.display = millis();
            }
        }
        else if(flag == SYSTEM_LOAD){
            digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
            digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
            
            digitalWrite(RELAY_BATT, RELAY_ON);
            digitalWrite(RELAY_LOAD, RELAY_ON);

            /* Count Capacity */
            data.capacity += data.current / 3600;

            if(data.voltage < data.voltage_empty){
                flag = SYSTEM_FINISH;
            }

            if((millis() - timeout.display) < TIMEOUT_DISPLAY_TITLE){
                displayTitle("Load");
            }
            else if((millis() - timeout.display) < TIMEOUT_DISPLAY_DATA){
                displayData();
            }
            else{
                Serial.println("Load");
                timeout.display = millis();
            }
        }
        else if(flag == SYSTEM_FINISH || flag == SYSTEM_OVERTEMP){
            digitalWrite(RELAY_BATT, RELAY_OFF);
            digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
            digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
            digitalWrite(RELAY_LOAD, RELAY_OFF);

            fuzzyProcess();

            if((millis() - timeout.display) < TIMEOUT_DISPLAY_TITLE){
                if(flag == SYSTEM_OVERTEMP){
                    displayTitle("Over Temp");

                    dtostrf(data.temperature_max, 2,1, f_str);
                    sprintf(display_buffer, "%s C", f_str);
                    lcd.setCursor((16 - strlen(display_buffer)) / 2, 1);
                    lcd.print(display_buffer);  
                }
                else{
                    displayTitle("Finish");

                    sprintf(display_buffer, "File %d", getLogNumber());    
                    lcd.setCursor((16 - strlen(display_buffer)) / 2, 1);
                    lcd.print(display_buffer);    
                }
            }
            else if((millis() - timeout.display) < TIMEOUT_DISPLAY_DATA){
                lcd.clear();
                lcd.setCursor(0,0);
                dtostrf(data.state_of_health, 2,1, f_str);
                sprintf(display_buffer, "SoH %s", f_str);
                lcd.print(display_buffer);

                lcd.setCursor(0,1);
                dtostrf(data.temperature_max, 2,1, f_str);
                dtostrf(data.capacity, 4,0, f_str1);
                sprintf(display_buffer, "%s C  %s mAh", f_str, f_str1);
                lcd.print(display_buffer);
            }
            else{
                if(flag == SYSTEM_OVERTEMP){
                    Serial.println("Over Temp");
                }
                else{
                    Serial.println("Finish");
                }
                
                timeout.display = millis();
            }
        }
        else if(flag == SYSTEM_START){
            if(data.voltage > 1.0){
                displayTitle("Baterai Terdeteksi");
                if((millis() - timeout.detect_batt) > TIMEOUT_DETECT_BATT){
                    createNewLogNumber();
                    createNewLog();
                    dataClear();
                    flag = SYSTEM_CHARGE;
                }
            }
            else{
                displayTitle("Pasang Baterai");
                timeout.detect_batt = millis();
            }
        }
        else{
            digitalWrite(RELAY_BATT, RELAY_OFF);
            digitalWrite(RELAY_CHARGE_BATT, RELAY_OFF);
            digitalWrite(RELAY_CHARGE_SUPPLY, RELAY_OFF);
            digitalWrite(RELAY_LOAD, RELAY_OFF);

            displayTitle("Press Button");
        }
    }

    if(flag == SYSTEM_CHARGE || flag == SYSTEM_LOAD){
        /* Get Temperature Max */
        if(data.temperature > data.temperature_max){
            data.temperature_max = data.temperature;
        }

        if(data.temperature > data.temperature_limit){
            if(flag != SYSTEM_OVERTEMP){
                flag = SYSTEM_OVERTEMP;
            }
        }

        if((millis() - timeout.save) > TIMEOUT_SAVE){
            timeout.save = millis();

            saveLog();
        }
    }
    else{
        timeout.save = millis();
    }
}

void displayTitle(String title){
    if(title.length() <= 16){
        uint8_t cursor = (16-title.length()) / 2;

        lcd.clear();
        lcd.setCursor(cursor,0);

        title.toCharArray(display_buffer, 16);
        lcd.print(display_buffer);
    }
}

void displayData(void){
    lcd.clear();
    lcd.setCursor(0,0);
    dtostrf(data.voltage, 4,2, f_str);
    dtostrf(data.current, 4,0, f_str1);
    sprintf(display_buffer, "%s V  %s mA", f_str, f_str1);
    lcd.print(display_buffer);

    lcd.setCursor(0,1);
    dtostrf(data.temperature, 2,1, f_str);
    dtostrf(data.capacity, 4,0, f_str1);
    sprintf(display_buffer, "%s C  %s mAh", f_str, f_str1);
    lcd.print(display_buffer);
}

void sendData(void){
    command.str = "";
    command.str = "DATA|" + String(data.temperature, 0)
                + "|" + String(data.voltage, 2) 
                + "|" + String(data.current, 0)
                + "|" + String(data.capacity, 0)
                + "|" + String(flag)
                + "|" + String(data.temperature_limit,0)
                + "|" + String(data.temperature_release,0)
                + "|" + String(data.voltage_full,2)
                + "|" + String(data.voltage_empty,2)
                + "|" + String(data.current_full,0)
                + "|" + String(data.capacity_limit,0)
                + "|" + String(data.state_of_health,1)
                ;

    Serial.println(command.str);
    Serial1.println(command.str);
}

void processData(void){
    int index, index2, index3;

    index = command.str.indexOf("|");

    if(command.str.substring(0, index) == "GET"){
        index++;
        if(command.str.substring(index) == "DATA"){
            sendData();
        }
    }
    else if(command.str.substring(0, index) == "SET"){
        index++;
        index2 = command.str.indexOf("|", index);
        if(command.str.substring(index, index2) == "PARAM"){
            index = index2 + 1;
            index2 = command.str.indexOf("|", index);
            data.temperature_limit = command.str.substring(index, index2).toFloat();

            index = index2 + 1;
            index2 = command.str.indexOf("|", index);
            data.temperature_release = command.str.substring(index, index2).toFloat();

            index = index2 + 1;
            index2 = command.str.indexOf("|", index);
            data.voltage_full = command.str.substring(index, index2).toFloat();

            index = index2 + 1;
            index2 = command.str.indexOf("|", index);
            data.voltage_empty = command.str.substring(index, index2).toFloat();

            index = index2 + 1;
            index2 = command.str.indexOf("|", index);
            data.current_full = command.str.substring(index, index2).toFloat();

            index = index2 + 1;
            index2 = command.str.indexOf("|", index);
            data.capacity_limit = command.str.substring(index, index2).toFloat();

            sendData();
        }
    }

    command.str = "";
}

void dataClear(void){
    data.temperature_max = 0;
    data.state_of_health = 0;
    data.capacity = 0;
    data.capacity_percent = 0;
}

float fuzzyDisjuntion(float level1, float level2){
    if(level1 < level2){
        return level2;
    }
    else{
        return level1;
    }
}

float fuzzyConjunction(MEMBERSHIP_TrapesiumTypeDef mf1, MEMBERSHIP_TrapesiumTypeDef mf2){
    if(mf1.level < mf2.level){
        return mf1.level;
    }
    else{
        return mf2.level;
    }
}

void fuzzyTrapesium(MEMBERSHIP_TrapesiumTypeDef *val, float setpoint){
    if(setpoint <= val->a){
        val->level = 0;
    }
    else if(val->a < setpoint && setpoint < val->b){
        val->level = (setpoint - val->a) / (val->b - val->a);
    }
    else if(val->b <= setpoint && setpoint <= val->c){
        val->level = 1;
    }
    else if(val->c < setpoint && setpoint < val->d){
        val->level = (val->d - setpoint) / (val->d - val->c);
    }
    else if(val->d <= setpoint){
        val->level = 0;
    }
}

void fuzzyProcess(void){
    Serial.println("Fuzzy");
    if(data.capacity > data.capacity_limit){
        data.capacity_percent = 100;
    }
    else{
        data.capacity_percent = (data.capacity / data.capacity_limit) * 100;
    }
    Serial.println(data.capacity_percent);
    Serial.println(data.temperature_max);

    Serial.println("Membership");
    fuzzyTrapesium(&fuzzy.kapasitas.low, data.capacity_percent);
    Serial.println(fuzzy.kapasitas.low.level);
    fuzzyTrapesium(&fuzzy.kapasitas.medium, data.capacity_percent);
    Serial.println(fuzzy.kapasitas.medium.level);
    fuzzyTrapesium(&fuzzy.kapasitas.high, data.capacity_percent);
    Serial.println(fuzzy.kapasitas.high.level);

    fuzzyTrapesium(&fuzzy.suhu.dingin, data.temperature_max);
    Serial.println(fuzzy.suhu.dingin.level);
    fuzzyTrapesium(&fuzzy.suhu.normal, data.temperature_max);
    Serial.println(fuzzy.suhu.normal.level);
    fuzzyTrapesium(&fuzzy.suhu.panas, data.temperature_max);
    Serial.println(fuzzy.suhu.panas.level);

    Serial.println("Conjunction");
    float ruleRusak1 = fuzzyConjunction(fuzzy.kapasitas.low, fuzzy.suhu.panas);
    Serial.println(ruleRusak1);
    float ruleRusak2 = fuzzyConjunction(fuzzy.kapasitas.medium, fuzzy.suhu.panas);
    Serial.println(ruleRusak2);
    float ruleRusak3 = fuzzyConjunction(fuzzy.kapasitas.high, fuzzy.suhu.panas);
    Serial.println(ruleRusak3);

    float ruleLemah1 = fuzzyConjunction(fuzzy.kapasitas.low, fuzzy.suhu.dingin);
    Serial.println(ruleLemah1);
    float ruleLemah2 = fuzzyConjunction(fuzzy.kapasitas.low, fuzzy.suhu.normal);
    Serial.println(ruleLemah2);
    float ruleLemah3 = fuzzyConjunction(fuzzy.kapasitas.medium, fuzzy.suhu.dingin);
    Serial.println(ruleLemah3);
    float ruleLemah4 = fuzzyConjunction(fuzzy.kapasitas.medium, fuzzy.suhu.normal);
    Serial.println(ruleLemah4);

    float ruleBagus1 = fuzzyConjunction(fuzzy.kapasitas.high, fuzzy.suhu.dingin);
    Serial.println(ruleBagus1);
    float ruleBagus2 = fuzzyConjunction(fuzzy.kapasitas.high, fuzzy.suhu.normal);
    Serial.println(ruleBagus2);

    Serial.println("Disjunction");
    fuzzy.soh.rusak.level = fuzzyDisjuntion(ruleRusak1, ruleRusak2);
    fuzzy.soh.rusak.level = fuzzyDisjuntion(ruleRusak2, fuzzy.soh.rusak.level);
    Serial.println(fuzzy.soh.rusak.level);

    fuzzy.soh.lemah.level = fuzzyDisjuntion(ruleLemah1, ruleLemah2);
    fuzzy.soh.lemah.level = fuzzyDisjuntion(ruleLemah2, fuzzy.soh.lemah.level);
    fuzzy.soh.lemah.level = fuzzyDisjuntion(ruleLemah3, fuzzy.soh.lemah.level);
    fuzzy.soh.lemah.level = fuzzyDisjuntion(ruleLemah4, fuzzy.soh.lemah.level);
    Serial.println(fuzzy.soh.lemah.level);

    fuzzy.soh.bagus.level = fuzzyDisjuntion(ruleBagus1, ruleBagus2);
    Serial.println(fuzzy.soh.bagus.level);

    fuzzy.cog.pembilang = 0;
    fuzzy.cog.penyebut = 0;

    fuzzy.cog.pembilang =   ((fuzzy.tp1 + fuzzy.tp2 + fuzzy.tp3 + fuzzy.tp4) * fuzzy.soh.rusak.level) 
                            + ((fuzzy.tp5 + fuzzy.tp6 + fuzzy.tp7) * fuzzy.soh.lemah.level )
                            + ((fuzzy.tp8 + fuzzy.tp9 + fuzzy.tp10) * fuzzy.soh.bagus.level);

    fuzzy.cog.penyebut =    (fuzzy.soh.rusak.level * 4) + (fuzzy.soh.lemah.level * 3) + (fuzzy.soh.bagus.level * 3);
    
    data.state_of_health = fuzzy.cog.pembilang / fuzzy.cog.penyebut;

    Serial.println("SoH");
    Serial.println(data.state_of_health);
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
            dataFile.println("voltage (V),current (mA),capacity (mAh),temperature (C),voltage full (V),voltage empty (V),current full (mA),capacity limit (mAh),temperature limit (C)");
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
            dtostrf(data.voltage, 4,2, f_str);
            dtostrf(data.current, 4,0, f_str1);
            sprintf(display_buffer, "%s,%s,", f_str, f_str1);
            dataFile.print(display_buffer);
            
            dtostrf(data.capacity, 4,0, f_str);
            dtostrf(data.temperature, 2,0, f_str1);
            sprintf(display_buffer, "%s,%s,", f_str, f_str1);
            dataFile.print(display_buffer);

            dtostrf(data.voltage_full, 4,2, f_str);
            dtostrf(data.voltage_empty, 4,2, f_str1);
            sprintf(display_buffer, "%s,%s,", f_str, f_str1);
            dataFile.print(display_buffer);

            dtostrf(data.current_full, 4,0, f_str);
            dtostrf(data.capacity_limit, 4,0, f_str1);
            sprintf(display_buffer, "%s,%s,", f_str, f_str1);
            dataFile.print(display_buffer);

            dtostrf(data.temperature_limit, 2,0, f_str);
            sprintf(display_buffer, "%s", f_str);
            dataFile.print(display_buffer);

            dataFile.println();
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

void serialEvent1(void){
    while(Serial1.available()){
        char inChar = (char) Serial1.read();

        if(inChar == '\n'){
            command.complete = true;
            break;
        }
        else if(inChar == '\r'){}
        else{   command.str += inChar;  }
    }
}