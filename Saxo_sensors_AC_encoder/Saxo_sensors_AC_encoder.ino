#include <math.h>
#include <Wire.h>
#include "i2c.h"
#include "U8glib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SimpleRotary.h>
#include "icons.h"
#include <EEPROM.h>
#define buttonPin   3
#define encoderOutA 4 // CLK pin of Rotary Enocoder
#define encoderOutB 5 // DT pin of Rotary Enocoder
#define AC_PIN      7

Adafruit_BME280 bme280;   // front seat
Adafruit_BME280 bme280_;  // back seat
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0);
SimpleRotary rotary(encoderOutB,encoderOutA,buttonPin);

float temperature, pascal, temperature2, pascal2;
int humidity, humidity2, set_temp;
int fan_animation = 1;
byte i,ii;
int f_ = 1;
int isfirst = 1;
int AC_mode = 0; // 0 for off, 1 for auto, 2 for on
int AC_state = 0;
int state = 0; // 0 for splash screen
int screen_mode = 0; // 0 for simple, 1 for when i push button, rotate pot

int buttonState = 1;
int lastButtonState = 1;
int sensorOk = 0;
unsigned long currentMillis = 0;
unsigned long previousMillisSensors = 0;
unsigned long previousMillisOLED = 0;
unsigned long previousMillisButton = 0;
int intervalSensors = 5000;
int intervalOLED = 1000;
const int intervalButton = 50;
int test = false;

void setup() {
  if (test) Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(AC_PIN, OUTPUT);
  digitalWrite(AC_PIN, HIGH);

  // Read saved data
  set_temp = EEPROM.read(0);
  AC_mode = EEPROM.read(1);
  
  if(set_temp>40) { set_temp=40; }
  if(set_temp<15) { set_temp=15; }
  EEPROM.update(0, int(set_temp));

  //u8g.setRot180();

  bool status;
  status = bme280.begin(0x77);
  delay(10);
  bme280_.begin(0x76);
  delay(10);
  
  if (status) {
    if (test) Serial.println("Sensor found");
    sensorOk = 1;
  }
  else {
    if (test) Serial.println("Sensor missing");
    sensorOk = 0;
    //while (1) {}
  }

  climaControl();
  updateScreen();
}
void loop() {
  currentMillis = millis();
  i = rotary.rotate(); 

  if ( i == 2 ) {
    set_temp++;
    if(set_temp>40) { set_temp = 40; }

    EEPROM.update(0, int(set_temp));

    delayUpdate();
      state = 2;
      climaControl();
      updateScreen();
      if (test) Serial.println(set_temp);
  }
  if ( i == 1 ) {
    set_temp--;
    if(set_temp<15) { set_temp = 15; }
    EEPROM.update(0, int(set_temp));
    delayUpdate();
      state = 2;
      climaControl();
      updateScreen();
      if (test) Serial.println(set_temp);
  }
 
  if (currentMillis - previousMillisButton >= intervalButton) {
    previousMillisButton = currentMillis;
    checkButton();
  }
  currentMillis = millis();
  if (currentMillis - previousMillisSensors >= intervalSensors) {
    previousMillisSensors = currentMillis;
    if(test) Serial.println("Checking sensors");
    
    state = 1;
    if (sensorOk == 1) {
      temperature = bme280.readTemperature();
      pascal = bme280.readPressure() / 100.0F;
      humidity = bme280.readHumidity();

      temperature2 = bme280_.readTemperature();
      pascal2 = bme280_.readPressure() / 100.0F;
      humidity2 = bme280_.readHumidity();
    }
    else {
      temperature = 25.75;
      pascal = 970.00;
      humidity = 53;

      temperature2 = 26.75;
      pascal2 = 971.00;
      humidity2 = 54;
    }
    climaControl();
  }
  currentMillis = millis();
  if (currentMillis - previousMillisOLED >= intervalOLED) {
    Serial.print("intervalOLED ");
    Serial.println(intervalOLED);
    previousMillisOLED = currentMillis;

    if (isfirst == 1) {
      isfirst = 0;
      //u8g.setFontRefHeightExtendedText();
      u8g.setDefaultForegroundColor();
      u8g.setContrast(100);
      u8g.setFontPosTop();
    }

    updateScreen();
    
    if (state == 0) {
      delay(3000);
      state = 1;
      previousMillisOLED = currentMillis;
      previousMillisButton = currentMillis;
      previousMillisSensors = currentMillis;
    } 
  }
}
void climaControl() {
  if (AC_mode == 0) // AC is off
  {
    digitalWrite(AC_PIN, HIGH);
    AC_state = 0;
  }
  else if (AC_mode == 1) // AC is auto
  {
    if ( (temperature > set_temp + 1) && (temperature > 15) ) {
      digitalWrite(AC_PIN, LOW);
      AC_state = 1;
    }
    else {
      digitalWrite(AC_PIN, HIGH);
      AC_state = 0;
    }
  }
  else if (AC_mode == 2) // AC is on
  {
    digitalWrite(AC_PIN, LOW);
    AC_state = 1;
  }
  if (test) Serial.print("Clima control: AC is ");
  if (test) Serial.print(AC_mode);
  if (test) Serial.println(AC_state);
}
void updateScreen() {
  u8g.firstPage();
  do {
    if(test) { Serial.println("Draw to oled"); }
    if (state == 0) {
      u8g.drawBitmapP( (128 - 5 * 8) / 2, (64 - 5 * 8) / 2, 5, 40, citroenIcon);
    }
    else if (state == 1) {
      showMeasurements();
    }
    else if (state == 2) {
      showAC();
    }
  }
  while ( u8g.nextPage() );
}
void showMeasurements() {
  if(test) { Serial.println("showMeasurements"); }
  int pos = 0;
  int posY = 18;
  int offsetX, offsetY, a, b;
  char a_[8];
  char b_[8];

  offsetX = 13;
  offsetY = 8;

  if (AC_mode == 0) {
    //u8g.drawStr(46, offsetY, "AC OFF");
    //intervalOLED = intervalSensors;
  }
  else if (AC_mode == 1) {
    a = (int)set_temp;
    itoa(a, a_, 10);

    u8g.setFont(u8g_font_profont22);
    pos = 46;
    pos += u8g.drawStr(pos, offsetY+8, a_);
    //pos = u8g.drawStr(58, offsetY, " AUTO");
    
    u8g.setFont(u8g_font_profont12);
    
    if(AC_state==1) {
      u8g.drawStr(pos + 2, offsetY+8, "ON");
      //u8g.drawStr(pos + 2, offsetY+9, "ON");
      //if(f_ == 0) { f_ = 1; u8g.drawBitmapP( 128/2-8, 0, 2, 16, fan1); }
      //else if(f_ == 1)  { f_ = 0; u8g.drawBitmapP( 128/2-8, 0, 2, 16, fan2); }

      if(test) Serial.println(f_);
      //intervalOLED = 1000;
     }
    else if(AC_state==0) {
      u8g.drawStr(pos + 2, offsetY+8, "OFF");
      //u8g.drawStr(pos + 2, offsetY+9, "OFF");
      //u8g.drawBitmapP( 128/2-8, 0, 2, 16, fan1);
      //intervalOLED = intervalSensors;
     }
  }
  else if (AC_mode == 2) {
    //u8g.setFont(u8g_font_profont22);
    //u8g.drawStr(35, offsetY+6, "AC ON");
    u8g.setFont(u8g_font_profont12);

    u8g.setColorIndex(0);
    u8g.drawBox((128 - 2 * 8) / 2, offsetY-8, 16, 16);
    u8g.setColorIndex(1);
    
    if(fan_animation == 1) {
      fan_animation = 2;
      u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY-8, 2, 16, fan2);
    }
    else {
      fan_animation = 1;
      u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY-8, 2, 16, fan1);
    }
    
    //intervalOLED = intervalSensors;
  }

  offsetY = 10;
  u8g.drawStr(13, offsetY + 6, "FRONT");

  // Temperature
  a = (int)temperature;
  b = (temperature - a) * 100;
  itoa(a, a_, 10);
  itoa(b, b_, 10);

  u8g.setFont(u8g_font_profont22); //);
  pos = offsetX + u8g.drawStr(offsetX, offsetY + 24, a_);
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(pos + 1, offsetY + 22, b_);
  u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY + 9, 2, 16, tempIcon);

  // Humidity
  a = (int)humidity;
  itoa(a, a_, 10);
  u8g.setFont(u8g_font_profont22);
  pos = offsetX + u8g.drawStr(offsetX, offsetY + 41, a_);
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(pos + 1, offsetY + 41, "%");
  u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY + 26, 2, 16, humIcon);

  // Pressure
  a = (int)pascal;
  itoa(a, a_, 10);
  u8g.setFont(u8g_font_profont12);
  pos = offsetX + u8g.drawStr(offsetX, offsetY + 52, a_);
  u8g.drawStr(pos + 1, offsetY + 52, "hPa");
  u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY + 41, 2, 16, presIcon);

  // ROUND 2
  offsetX = (128 - 2 * 8) / 2 + 25;
  if(AC_mode == 0) { u8g.drawStr(offsetX, offsetY + 6, "BACK"); }
  else { u8g.drawStr(offsetX+13, offsetY + 6, "BACK"); }

  // Temperature
  a = (int)temperature2;
  b = (temperature2 - a) * 100;
  itoa(a, a_, 10);
  itoa(b, b_, 10);
  u8g.setFont(u8g_font_profont22); //);
  pos = offsetX + u8g.drawStr(offsetX, offsetY + 24, a_);
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(pos + 1, offsetY + 22, b_);

  // Humidity
  a = (int)humidity2;
  itoa(a, a_, 10);
  u8g.setFont(u8g_font_profont22); // u8g_font_7x14
  pos = offsetX + u8g.drawStr(offsetX, offsetY + 41, a_);
  u8g.setFont(u8g_font_profont12);
  u8g.drawStr(pos + 1, offsetY + 41, "%");
  u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY + 26, 2, 16, humIcon);

  // Pressure
  a = (int)pascal2;
  itoa(a, a_, 10);
  u8g.setFont(u8g_font_profont12); // u8g_font_6x10
  pos = offsetX + u8g.drawStr(offsetX, offsetY + 52, a_);
  u8g.drawStr(pos + 1, offsetY + 52, "hPa");
  u8g.drawBitmapP( (128 - 2 * 8) / 2, offsetY + 41, 2, 16, presIcon);
}
void showAC() {
  int pos = 0;
  int posY = 18;
  int offsetX, offsetY, a, b;
  char a_[8];
  char b_[8];

  offsetX = 50;
  offsetY = 20;

  a = (int)set_temp;
  itoa(a, a_, 10);

  u8g.setFont(u8g_font_profont22);
  u8g.setColorIndex(1);
  pos = offsetX + u8g.drawStr(offsetX, offsetY, a_);

  // Temperature
  a = (int)temperature;
  b = (temperature - a) * 100;
  itoa(a, a_, 10);
  itoa(b, b_, 10);

  u8g.setFont(u8g_font_profont12); //);
  pos += u8g.drawStr(pos + 5, offsetY, a_);
  pos += u8g.drawStr(pos + 3, offsetY, ".");
  u8g.drawStr(pos + 3, offsetY, b_);

  u8g.setFont(u8g_font_profont22);
  offsetY = offsetY + 20;
  
  if (AC_mode == 0) {
    u8g.drawStr(44, offsetY, "OFF");
  }
  else if (AC_mode == 1) {
    u8g.drawStr(26, offsetY, " AUTO");
  }
  else if (AC_mode == 2) {
    u8g.drawStr(50, offsetY, "ON");
  }

  //
  offsetY = offsetY + 14;
  u8g.setFont(u8g_font_profont12);
  if (AC_mode == 1 && AC_state == 0) {
    u8g.drawStr(offsetX + 4, offsetY, "OFF");
  }
  else if (AC_mode == 1 && AC_state == 1)  {
    u8g.drawStr(offsetX + 6, offsetY, "ON");
  }

}
void checkButton() {
  buttonState = digitalRead(buttonPin);

  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      if (AC_mode == 0) {
        AC_mode = 1;
      }
      else if (AC_mode == 1) {
        AC_mode = 2;
        AC_state = 1;
      }
      else {
        AC_mode = 0;
        AC_state = 0;
      }
      EEPROM.update(1, AC_mode);
      state = 2;
      if (test) Serial.print("Button Clicked: AC ");
      if (test) Serial.println(AC_mode);
      if (test) Serial.println(AC_state);
      delayUpdate();
      climaControl();
      updateScreen();
    }
  }
  lastButtonState = buttonState;
}
void delayUpdate() {
  previousMillisOLED = millis();
}
