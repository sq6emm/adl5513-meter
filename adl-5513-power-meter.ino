#include <LiquidCrystal_PCF8574.h>
#include <RunningMedian.h>

const float refVoltage = 2.5017;
const int pinA = A0;
const int pinB = A1;
const int pinC = A2;
const int TpinA = 11;
const int TpinB = 10;
const int TpinC = 9;

const float Aslope[6] =     { 0.021, 0.021, 0.021, 0.021, 0.021, 0.021 };
const float Aintercept[6] = { -87.0, -87.0, -88.0, -88.0, -88.0, -88.0 };
const float Atadj[6] =      {   0.0,   0.0,   0.0,   0.0,   0.0,   0.0 };

const float Bslope[6] =     { 0.021, 0.021, 0.021, 0.021, 0.021, 0.021 };
const float Bintercept[6] = { -87.0, -87.0, -88.0, -88.0, -88.0, -88.0 };
const float Btadj[6] =      {   0.0,   0.0,   0.0,   0.0,   0.0,   0.0 };

const float Cslope[6] =     { 0.021, 0.021, 0.021, 0.021, 0.021, 0.021 };
const float Cintercept[6] = { -87.0, -87.0, -88.0, -88.0, -88.0, -88.0 };
const float Ctadj[6] =      {   0.0,   0.0,   0.0,   0.0,   0.0,   0.0 };

const int bands[6] = { 30, 50, 144, 430, 900, 1296 };

unsigned int band = 4;

LiquidCrystal_PCF8574 lcd(0x38);
RunningMedian medianAin = RunningMedian(100);
RunningMedian medianBin = RunningMedian(100);
RunningMedian medianCin = RunningMedian(100);

float analogReadRef(int pin) {
  return analogRead(pin)*refVoltage/1024;
}

float vin2dbm(float in,float slope,float intercept) {
  return (in/slope)+intercept;
}

void setup() {
  analogReference(EXTERNAL);
  pinMode(TpinA, OUTPUT);
  pinMode(TpinB, OUTPUT);
  pinMode(TpinC, OUTPUT);
  lcd.begin(20, 4);
  lcd.setBacklight(255);
}

void loop() {
  analogWrite(TpinA,int(255/5*Atadj[band]));
  analogWrite(TpinB,int(255/5*Btadj[band]));
  analogWrite(TpinC,int(255/5*Ctadj[band]));
  int i=100;
  
  for (i; i > 0; i--) {
    medianAin.add(analogReadRef(pinA));
    medianBin.add(analogReadRef(pinB));
    medianCin.add(analogReadRef(pinC));
  }
  
  float aAin = medianAin.getAverage(3);
  float aBin = medianBin.getAverage(3);
  float aCin = medianCin.getAverage(3);
//  float hAin = medianAin.getHighest();
//  float hBin = medianBin.getHighest();
//  float hCin = medianCin.getHighest();
  
  lcd.setCursor(0,0); lcd.print("    Vdc    dBm");
  lcd.setCursor(0,1); lcd.print("A  "); lcd.print(aAin,3); lcd.print("  "); lcd.print(vin2dbm(aAin,Aslope[band],Aintercept[band]),1);
  lcd.setCursor(0,2); lcd.print("B  "); lcd.print(aBin,3); lcd.print("  "); lcd.print(vin2dbm(aBin,Bslope[band],Bintercept[band]),1);
  lcd.setCursor(0,3); lcd.print("C  "); lcd.print(aCin,3); lcd.print("  "); lcd.print(vin2dbm(aCin,Cslope[band],Cintercept[band]),1);
  delay(150);
}
