#include <LiquidCrystal.h> 
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif


//pins:
const int HX711_dout = 2; //mcu > HX711 dout pin
const int HX711_sck = 3; //mcu > HX711 sck pin


const int rs = 9, en = 8, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);


const int calVal_calVal_eepromAdress = 0;
unsigned long t = 0;


const int trigPin = 10;
const int echoPin = 12;
long duration;
int distance;
int distance1;
int speakerPin = 8;


void setup() {
  Serial.begin(57600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  lcd.begin(16, 2);
  delay(10);
  Serial.println();


  float calibrationValue; // calibration value 10463.47 10324.40
  calibrationValue = 10324.47; // uncomment this if you want to set this value in the sketch
#if defined(ESP8266) || defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266 and want to fetch this value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch this value from eeprom


  LoadCell.begin();
  //LoadCell.setReverseOutput();
  unsigned long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration factor (float)
    Serial.println("Startup is complete");
  }
  while (!LoadCell.update());
  Serial.print("Calibration value: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settlingtime ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Note that the settling time may increase significantly if you use delay() in your sketch!");
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  }
  else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }
}


void loop() {
  long duration, cm;
  static boolean newDataReady = 0;
  const int serialPrintInterval = 500; //increase value to slow down serial print activity


  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Measure the response from the HC-SR04 Echo Pin
  duration = pulseIn(echoPin, HIGH);
  cm = duration / 58;
  
  // Determine distance from duration
  // Use 343 metres per second as speed of sound
  distance= duration*0.0343/2;
  distance1= 180 - distance ;


  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;


  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      float kg = i * 0.45359237;
      float bmi = (kg / ((float)distance1/100 * (float)distance1/100));
      Serial.print("Load_cell output val: ");
      Serial.print(i);
      Serial.print(" pounds   ");
      Serial.print(kg);
      Serial.println(" Kilograms");
      newDataReady = 0;


      //display to LCD
      lcd.setCursor(0, 0);
      lcd.print(distance1);
      lcd.print("cm, ");
      lcd.print(kg);
      lcd.print("kgs");


      lcd.setCursor(0, 1);
      lcd.print(" BMI: ");
      lcd.print(bmi);
      t = millis();
    }
  }


  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }


  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }


}

