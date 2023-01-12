#include <LiquidCrystal_PCF8574.h>
#include <RunningMedian.h>

const float refVoltage = 2.5017;
const int pinA = A0;
const int pinB = A1;
const int pinC = A2;
const int TpinA = 11;
const int TpinB = 10;
const int TpinC = 9;

const int swA = 2;
const int clkA = 3;
int clkAstate = 0;
const int dtA = 4;
int dtAstate = 0;
int clkAlaststate = 0;

const int swB = 18;
const int clkB = 19;
int clkBstate = 0;
const int dtB = 17;
int dtBstate = 0;
int clkBlaststate = 0;

// calibration table
const unsigned int nobands = 5;
/* // kalibracja u Mariusza w domu
const float Aslope[nobands] =     {  0.02086,  0.02107,  0.02112,  0.02080,  0.02116 };
const float Aintercept[nobands] = { -87.9674, -87.1648, -87.4526, -90.1923, -91.6824 };
const float Atadj[nobands] =      {     0.00,     0.00,     0.00,     0.00,     0.00 };

const float Bslope[nobands] =     {  0.02086,  0.02115,  0.02115,  0.02076,  0.02092 };
const float Bintercept[nobands] = { -85.7142, -84.6806, -85.1536, -88.0057, -90.3919 };
const float Btadj[nobands] =      {     0.00,     0.00,     0.00,     0.00,     0.00 };

const float Cslope[nobands] =     {  0.02076,  0.02095,  0.02100,  0.02070,  0.02112 };
const float Cintercept[nobands] = { -86.0308, -85.2505, -85.6190, -88.2608, -89.6306 };
const float Ctadj[nobands] =      {     0.00,     0.00,     0.00,     0.00,     0.00 };
*/

// kalibracja u Mariusza w pracy (zakres -50dB do 0dB)
const float Aslope[nobands] =     {    0.021,  0.02106,  0.02124,  0.02122,   0.0211 };
const float Aintercept[nobands] = { -87.6190, -87.4643, -87.0527, -89.1140, -91.5639 };
const float Atadj[nobands] =      {     0.00,     0.00,     0.00,     0.00,     0.00 };

const float Bslope[nobands] =     {  0.02096,   0.0211,   0.0212,  0.02118,  0.02096 };
const float Bintercept[nobands] = { -85.3053, -84.8815, -84.8113, -86.6383, -89.2652 };
const float Btadj[nobands] =      {     0.00,     0.00,     0.00,     0.00,     0.00 };

const float Cslope[nobands] =     {  0.02082,    0.021,  0.02096,  0.02106,  0.02114 };
const float Cintercept[nobands] = { -85.7829, -85.2380, -85.7824, -86.9895, -88.5525 };
const float Ctadj[nobands] =      {     0.00,     0.00,     0.00,     0.00,     0.00 };

const int bands[nobands] =        {       50,      144,      433,     1296,     2320 }; // MHz
// calibration table

volatile float tempAatt = 0;
float Aatt = 0;
float Batt = 0;
float Catt = 0;

unsigned long lastClear = millis();

volatile unsigned int band = 3;

LiquidCrystal_PCF8574 lcd(0x38);

const unsigned int getAverageNo = 5;
const unsigned int MedianNo = 100;

RunningMedian medianAin = RunningMedian(MedianNo);
RunningMedian medianBin = RunningMedian(MedianNo);
RunningMedian medianCin = RunningMedian(MedianNo);

float analogReadRef(int pin) {
  return analogRead(pin)*refVoltage/1024;
}

float vin2dbm(float in,float slope,float intercept) {
  return (in/slope)+intercept;
}

float dbm2watts(float in) {
  return pow(10,((in-30)/10));
}

String printWatts(float dBmIn) {
/*
-90 -60 = 001pW - 999pW
-60 -30 = 001nW - 999nW
-30  -0 = 001uW - 999uW
0   -30 = 001mW - 999mW
30   40 = 01.0W - 09.9W
40   70 = 0010W - 9999W
*/
  float w = dbm2watts(dBmIn);
  char wf[6];
  char r[6];
  if ( dBmIn > 70.0 )       { return "HIGH!"; }
  else if ( dBmIn > 40.0 )  { dtostrf(w, 4, 0, wf); sprintf(r, "%sW" ,wf); return r; }
  else if ( dBmIn > 30.0 )  { dtostrf(w, 4, 1, wf); sprintf(r, "%sW" ,wf); return r; }
  else if ( dBmIn > 0.0)    { w = w*1000; dtostrf(w, 3, 0, wf); sprintf(r, "%smW" ,wf); return r; }
  else if ( dBmIn > -30.0 ) { w = w*1000*1000; dtostrf(w, 3, 0, wf); sprintf(r, "%suW" ,wf); return r; }
  else if ( dBmIn > -60.1 ) { w = w*1000*1000*1000; dtostrf(w, 3, 0, wf); sprintf(r, "%snW" ,wf); return r; }
  else if ( dBmIn > -90.0 ) { w = w*1000*1000*1000*1000; dtostrf(w, 3, 0, wf); sprintf(r, "%spW" ,wf); return r; }
}

void bandChange() {
  if ( band == (nobands-1) ) {
    band = 0;    
  } else {
    band++;
  }
}

void attChange() {
  cli();
  clkAstate = digitalRead(clkA);
  dtAstate = digitalRead(dtA);
  if (clkAstate != clkAlaststate) {
    if (dtAstate != clkAstate) {
      if (tempAatt > 0) {
        tempAatt--;
      }
    } else {
        tempAatt++;
    }
  }
  clkAlaststate = clkAstate;
  tempAatt=abs(tempAatt);
  sei();
}

void attChangeB() {
  cli();
  clkBstate = digitalRead(clkB);
  dtBstate = digitalRead(dtB);
  if (clkBstate != clkBlaststate) {
    if (dtBstate != clkBstate) {
      if (tempAatt > 0) {
        tempAatt--;
      }
    } else {
        tempAatt++;
    }
  }
  clkBlaststate = clkBstate;
  tempAatt=abs(tempAatt);
  sei();
}

void setup() {
  analogReference(EXTERNAL); // must be very first command
  pinMode(swA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(swA), bandChange, RISING);
  pinMode(dtA, INPUT_PULLUP);
  pinMode(clkA, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(clkA), attChange, CHANGE);
  clkAlaststate = digitalRead(clkA);

  pinMode(swB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(swB), bandChange, RISING);
  pinMode(dtB, INPUT_PULLUP);
  pinMode(clkB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(clkB), attChangeB, CHANGE);
  clkBlaststate = digitalRead(clkB);

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

  int i=MedianNo;
  for (i; i > 0; i--) {
    medianAin.add(analogReadRef(pinA));
    medianBin.add(analogReadRef(pinB));
    medianCin.add(analogReadRef(pinC));
  }
  
  float aAin = medianAin.getAverage(getAverageNo);
  float aBin = medianBin.getAverage(getAverageNo);
  float aCin = medianCin.getAverage(getAverageNo);
  
//  float hAin = medianAin.getHighest();
//  float hBin = medianBin.getHighest();
//  float hCin = medianCin.getHighest();

  Aatt = tempAatt/10/2; // calculate real ATT based on encoder input
  if ( millis() - lastClear > 100 ) {
    float AdBm = vin2dbm(aAin,Aslope[band],Aintercept[band])+Aatt; float AWatt = dbm2watts(AdBm);
    float BdBm = vin2dbm(aBin,Bslope[band],Bintercept[band])+Batt; float BWatt = dbm2watts(BdBm);
    float CdBm = vin2dbm(aCin,Cslope[band],Cintercept[band])+Catt; float CWatt = dbm2watts(CdBm);
    lcd.setCursor(0,0); lcd.print("                    ");
    lcd.setCursor(0,0); lcd.print(bands[band]); lcd.print(" MHz  Vdc    dBm");
    lcd.setCursor(0,1); lcd.print("                    ");
    lcd.setCursor(0,1); lcd.print("A "); lcd.print(Aatt,1); lcd.print(" ");lcd.print(printWatts(AdBm)); lcd.print(" "); lcd.print(AdBm,1);
    lcd.setCursor(0,2); lcd.print("                    ");
    lcd.setCursor(0,2); lcd.print("B "); lcd.print(Batt,1); lcd.print(" ");lcd.print(printWatts(BdBm)); lcd.print(" "); lcd.print(BdBm,1);
    lcd.setCursor(0,3); lcd.print("                    ");
    lcd.setCursor(0,3); lcd.print("C "); lcd.print(Catt,1); lcd.print(" ");lcd.print(printWatts(CdBm)); lcd.print(" "); lcd.print(CdBm,1);
  
    lastClear = millis();
  }
}
