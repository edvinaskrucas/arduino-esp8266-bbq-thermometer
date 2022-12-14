#include <LiquidCrystal.h>
#include <SerialTransfer.h>

const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;

SerialTransfer myTransfer;

struct __attribute__((packed)) STRUCT {
  float p1a; // probe 1 analog reading
  float p1r; // probe 1 resistance
  float p1t; // probe 1 temperature
  float p2a; // probe 2 analog reading
  float p2r; // probe 2 resistance
  float p2t; // probe 2 temperature
} dataStruct;

// External lib config
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);

// Generic config
#define TEMP_READ_DELAY 10000 // 10 seconds between temp reading
#define BAUD_RATE 9600 // Baudrate for serial monitor and ESP

// Temperature Probes
#define TEMPERATURENOMINAL 25
#define NUMSAMPLES 10 // Number of samples to smooth input

#define PROBE1_PIN 0
#define PROBE2_PIN 1
#define THERMISTORNOMINAL 1000000 // resistance at 25 degrees C
#define BCOEFFICIENT 4000 // The beta coefficient of the thermistor
#define SERIESRESISTOR 1000000 // the value of the 'other' resistor

float samples[NUMSAMPLES];

void setup() {
  Serial.begin(BAUD_RATE);
  myTransfer.begin(Serial);
  lcd.begin(16, 2);
}

void loop() {
  float probe1Average = sampleTempData(PROBE1_PIN, NUMSAMPLES);
  float probe1Resistance = convertAnalogToResistance(probe1Average, SERIESRESISTOR);
  float probe1DegC = calculateCFromResistance(probe1Resistance, THERMISTORNOMINAL, BCOEFFICIENT, TEMPERATURENOMINAL);

  float probe2Average = sampleTempData(PROBE2_PIN, NUMSAMPLES);
  float probe2Resistance = convertAnalogToResistance(probe2Average, SERIESRESISTOR);
  float probe2DegC = calculateCFromResistance(probe2Resistance, THERMISTORNOMINAL, BCOEFFICIENT, TEMPERATURENOMINAL);

  dataStruct.p1a = probe1Average;
  dataStruct.p1r = probe1Resistance;
  dataStruct.p1t = probe1DegC;
  dataStruct.p2a = probe2Average;
  dataStruct.p2r = probe2Resistance;
  dataStruct.p2t = probe2DegC;

  // Update temps on display
  writeTempToDisplay(probe1DegC, probe2DegC);

  myTransfer.sendDatum(dataStruct);

  delay(TEMP_READ_DELAY);
}

/**
 * Samples the analog input numSamples number of times to smooth out the final value
 */
float sampleTempData(int pin, int numSamples){
  uint8_t i;
  float average;

  analogRead(pin); // read and discard residue voltage
  delay(10);

  // take N samples in a row, with a slight delay
  for (i=0; i< numSamples; i++) {
   samples[i] = analogRead(pin);
   delay(10);
  }

  // average all the samples out
  average = 0;
  for (i=0; i< numSamples; i++) {
     average += samples[i];
  }
  average /= numSamples;

  return average;
}

/**
 * Helper method to convert analog data to resistance. Taken from https://learn.adafruit.com/thermistor/using-a-thermistor
 */
float convertAnalogToResistance(float avg, long resistor){
  avg = 1023 / avg - 1;
  avg = resistor / avg;
  return avg;
}

/**
 * Helper method to calculate C from a resistance. This taken from https://learn.adafruit.com/thermistor/using-a-thermistor
 */
float calculateCFromResistance(float resistance, long thermNominal, int coefficient, int tempNominal){
  float steinhart;
  steinhart = resistance / thermNominal;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= coefficient;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (tempNominal + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
  return steinhart;
}

void writeTempToDisplay(float probe1, float probe2) {
  lcd.setCursor(0, 0);
  lcd.print("Grill   ");

  lcd.setCursor(0, 1);
  lcd.print("Food    ");

  lcd.setCursor(8, 0);
  if (probe1 <= -99.99) {
    lcd.print("   --   ");
  } else {
    lcd.print(lcdValueRight(probe1, String(" C")));
  }

  lcd.setCursor(8, 1);
  if (probe2 <= -99.99) {
    lcd.print("   --   ");
  } else {
    lcd.print(lcdValueRight(probe2, String(" C")));
  }
}

String lcdValueRight(float value, String suffix) {
  String str = String(value, 1) + suffix;

  while (str.length() < 8) {
    str = String(" ") + str;
  }

  return str;
}