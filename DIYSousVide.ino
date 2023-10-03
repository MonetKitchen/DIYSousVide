//
//
//     m     m     oooo     n    n    eeeee   ttttt
//     mm   mm    o    o    nn   n    e         t
//     m m m m    o    o    n n  n    eeee      t
//     m  m  m    o    o    n  n n    e         t
//     m     m     oooo     n    n    eeeee     t
//
//
// monet_kitchen 2023/9/28 
// (instagram.com/monet_kitchen)
// SousVide Control
//   Receive termpareture from thermocouple (via MAX6675), 
//   show temperature on 4D7S display (with TM1637),
//   drive a replay to control 電湯匙, according to given rules
//

const String VERSION_CODE = "0.9.0";
const String BUILD_NUMBER = "1";
const String BUILD_DATE = "2023/9/28";

//-----------------------------------------------------------------
//                          max6675
//-----------------------------------------------------------------
#include "max6675.h"

#define PIN_thermoDO 12 //4;
#define PIN_thermoCS 15 //5;
#define PIN_thermoCLK 14 //6;

MAX6675 thermocouple(PIN_thermoCLK, PIN_thermoCS, PIN_thermoDO);

#define THRESHOLD_MAX 70
#define THRESHOLD_HIGH 66
#define THRESHOLD_LOW 64

float fTemp = -101.0;
unsigned long lTime_readTemp = 0L;

//-----------------------------------------------------------------
//                          tm1637
//-----------------------------------------------------------------
#include <TM1637Display.h>

#define PIN_1637_CLK 13 //2
#define PIN_1637_DIO 4

TM1637Display display_TM1637(PIN_1637_CLK, PIN_1637_DIO);

//-----------------------------------------------------------------
//                          general I/O
//-----------------------------------------------------------------
#define LED_green 2
//#define LED_red 4
#define BUTTON_PIN 0

#define LED_ON HIGH
#define LED_OFF LOW

boolean bGreen = false;
boolean bButton = false, bButton_old = false;

//-----------------------------------------------------------------
//                          relay
//-----------------------------------------------------------------
#define RELAY_PIN 5

#define RELAY_ON HIGH
#define RELAY_OFF LOW

boolean bRelay = false;

//-----------------------------------------------------------------
//                          setup()
//-----------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  
  Serial.println("\n\nStarted.");
  Serial.println("M's SousVide");
  Serial.println("Version: "+VERSION_CODE+", build "+BUILD_NUMBER+ ", " +BUILD_DATE);
  Serial.println("Board: 8266");  // wait for MAX chip to stabilize
  Serial.println();

  pinMode(LED_green, OUTPUT);
  #ifdef LED_blue
    pinMode(LED_blue, OUTPUT);
  #endif
  #ifdef LED_red
    pinMode(LED_red, OUTPUT);
  #endif
  pinMode(BUTTON_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);

  digitalWrite(LED_green, LED_OFF);
  #ifdef LED_blue
    digitalWrite(LED_blue, LED_OFF);
  #endif
  #ifdef LED_red
    digitalWrite(LED_red, LED_OFF);
  #endif
  digitalWrite(RELAY_PIN, RELAY_OFF);
  bGreen = false;
  bRelay = false;
  display_TM1637.setBrightness(0x0f);
  uint8_t data8888[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  uint8_t data0000[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //uint8_t data0000[] = { B00111101, B01011100, B01011100, B01011110, 0x00, 0x00};
  display_TM1637.setSegments(data0000, 6, 0);
  delay(500);


  digitalWrite(LED_green, LED_ON);
  #ifdef LED_blue
    digitalWrite(LED_blue, LED_ON);
  #endif
  #ifdef LED_red
    digitalWrite(LED_red, LED_ON);
  #endif
  delay(400);
  
  digitalWrite(LED_green, LED_OFF);
  #ifdef LED_blue
    digitalWrite(LED_blue, LED_OFF);
  #endif
  #ifdef LED_red
    digitalWrite(LED_red, LED_OFF);
  #endif
  delay(200);

  display_TM1637.setSegments(data8888, 6, 0); delay(400);
  display_TM1637.setSegments(data0000, 6, 0); delay(200);

  display_TM1637.showNumberDec(THRESHOLD_HIGH, false, 2, 0); delay(600);
  display_TM1637.setSegments(data0000, 6, 0); delay(100);
  display_TM1637.showNumberDec(THRESHOLD_LOW,  false, 2, 2); delay(600);
  display_TM1637.setSegments(data0000, 6, 0); delay(1000);

  //digitalWrite(LED_green, LED_ON);
  Serial.println("\nReady.\n");
}

//-----------------------------------------------------------------
//                          loop()
//-----------------------------------------------------------------
void loop() {
  // basic readout test, just print the current temp
  
  bButton = !digitalRead(BUTTON_PIN);
  if (bButton != bButton_old) {
    if (bButton) {
      if (bRelay) {
        digitalWrite(LED_green, LED_OFF);
        digitalWrite(RELAY_PIN, RELAY_OFF);
        bRelay = false;
        Serial.println("Relay off");
      } else {
        digitalWrite(LED_green, LED_ON);
        digitalWrite(RELAY_PIN, RELAY_ON);
        bRelay = true;
        Serial.println("Relay ON");
      }
    }
    bButton_old = bButton;
  }

  if (millis() - lTime_readTemp > 2000L) {
    fTemp = thermocouple.readCelsius();
    Serial.print("C = "+String(fTemp));
    set_temperature_display_C(fTemp);
    //Serial.print("F = ");
    //Serial.println(thermocouple.readFahrenheit());
  
    if (fTemp >= THRESHOLD_MAX && bRelay) {
      digitalWrite(LED_green, LED_OFF);
      digitalWrite(RELAY_PIN, RELAY_OFF);
      bRelay = false;
      Serial.print(" off (force off)");
    } else if (fTemp <= THRESHOLD_LOW && !bRelay) {
      //#ifdef LED_red
      //  digitalWrite(LED_red, LED_ON);
      //#endif
      digitalWrite(LED_green, LED_ON);
      digitalWrite(RELAY_PIN, RELAY_ON);
      bRelay = true;
      Serial.print(" ON");
    } else if (fTemp >= THRESHOLD_HIGH && bRelay) {
      //#ifdef LED_red
      //  digitalWrite(LED_red, LED_OFF);
      //#endif
      digitalWrite(LED_green, LED_OFF);
      digitalWrite(RELAY_PIN, RELAY_OFF);
      bRelay = false;
      Serial.print(" off");
    }

    Serial.println();
    // For the MAX6675 to update, you must delay AT LEAST 250ms between reads!
    //delay(2000);
    lTime_readTemp = millis();
  }

} // loop()


void set_temperature_display_C(float _temperature) {
    //if (DEBUG) { Serial.println("set_pm25_display(), value="+String(_pm25)); }
    //int nTemperature = (_temperature > 0.0 ? int(_temperature+0.5) : int(_temperature-0.5)) ;
    int nTemperature = int(_temperature);
    
    //Serial.println("tempC="+String(nTemperature));
    if (nTemperature < -998) {  // initial value is -999
      //-- no value
      uint8_t dataP000[] = { B01000000, B00000000, B01100011, B00111001}; // "- 'C"
      display_TM1637.setSegments(dataP000);
    } else if (nTemperature > 99 || nTemperature < -99) {
      //-- value too big or too small
      uint8_t dataP000[] = { B01000000, B01000000, B01100011, B00111001}; // "--'C"
      display_TM1637.setSegments(dataP000);
    } else if (nTemperature < -9) {
      uint8_t dataP000[] = { B01000000, B00000000, B00000000, B01100011}; // "-nn'"
      display_TM1637.setSegments(dataP000);
      display_TM1637.showNumberDec(-1*nTemperature, false, 2, 1);
    } else if (nTemperature < 0) {
      uint8_t dataP000[] = { B01000000, B00000000, B01100011, B00111001}; // "-n'C"
      display_TM1637.setSegments(dataP000);
      display_TM1637.showNumberDec(-1*nTemperature, false, 1, 1);
    } else {
      uint8_t dataP000[] = { B00000000, B00000000, B01100011, B00111001}; // "nn'C"
      display_TM1637.setSegments(dataP000);
      display_TM1637.showNumberDec(nTemperature, false, 2, 0);
    }
    //nDisplay_Status = DISPLAY_STATUS__NORMAL_TEMP;
}  // set_temperature_display_C()
