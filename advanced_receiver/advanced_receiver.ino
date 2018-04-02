#include <LowPower.h>

const byte adcPin = 0; // A0
const int photoDiodePort = 12;
const int blueDiodePort = 13;
const int vPlusPort = 11;
const int okLedPort = 3;
const int failLedPort = 2;

volatile int adcReading;
volatile boolean adcDone;
boolean adcStarted;

void setup() {
  pinMode(photoDiodePort, INPUT);
  pinMode(blueDiodePort, OUTPUT);
  pinMode(okLedPort, OUTPUT);
  pinMode(failLedPort, OUTPUT);
  pinMode(vPlusPort, OUTPUT);
  digitalWrite(vPlusPort, HIGH);
  blink(4,okLedPort,200);
    
  Serial.begin (115200);
  ADCSRA =  bit (ADEN);                      // turn ADC on
//  ADCSRA |= bit (ADPS0);                               //   2  
//  ADCSRA |= bit (ADPS1);                               //   4  
  ADCSRA |= bit (ADPS0) | bit (ADPS1);                 //   8  
//  ADCSRA |= bit (ADPS2);                               //  16 
//  ADCSRA |= bit (ADPS0) | bit (ADPS2);                 //  32 
//  ADCSRA |= bit (ADPS1) | bit (ADPS2);                 //  64 
//  ADCSRA |= bit (ADPS0) | bit (ADPS1) | bit (ADPS2);   // 128
  ADMUX  =  bit (REFS0) | (adcPin & 0x07);    // AVcc and select input port
//  ADMUX = bit (REFS0) | bit (REFS1);  // Internal 1.1V reference

  analogReference(INTERNAL);
  analogRead(adcPin);

}

// ADC complete ISR
ISR (ADC_vect) {
  adcReading = ADC;
  adcDone = true;  
}  // end of ADC_vect
  
int output;
int timeElapsed;
long newTime;
long oldTime;
void loop() {
  //float transmissionInfo[] = { 0.0, 0.0, 0.0, 0.0 };
  //Serial.println("Begin searching for receiver");
  /*
  checkIfTransmission(transmissionInfo);
  if (transmissionInfo[0] == 0) {
    goBackToSleep();
  } else {
    digitalWrite(okLedPort, HIGH);
    sendStuff();
    digitalWrite(okLedPort, LOW);
    goBackToSleep();
    
  }*/
//    digitalWrite(vPlusPort, HIGH);
//
//  output = analogRead(analogPin);
//  Serial.println(output);
//  delay(1);

// if last reading finished, process it
  if (adcDone)
    {
    adcStarted = false;

    // do something with the reading, for example, print it
    newTime=micros();
    timeElapsed=newTime - oldTime;
    Serial.println (adcReading);
//    Serial.println (timeElapsed);
    timeElapsed=0;
    oldTime=newTime;
    
    // delay (1);

    adcDone = false;
    }
    
  // if we aren't taking a reading, start a new one
  if (!adcStarted)
    {
    adcStarted = true;
    // start the conversion
    ADCSRA |= bit (ADSC) | bit (ADIE);
    }   
}

/*
  The first step is to determine whether there is a transmitter nearby, how stable the connection is and how noisy the clock rate is
  The basic idea is to have several tries, and on each try to measure the variance of the duration between n = 10 amplitude jumps greater than noise power. If no jump is detected after a timeout, the trial is considered failed.
  (to be calibrated according to the resistor plugged to the photodiode. Under current conditions, R = 1MOhm, therefore in approximate darkness even a V+ * 1/1024 threshold is quite stable.
  More ambient light produces more noise and forces the amplitude difference to be greater, interior lighting has a noise amplitude of around 8, therefore, a good threshold would be 10 to filter it out.
  This, of course, requires more light power, therefore given the conditions, it is better to increase the sensitivity, and to have an appropriate threshold.).
  If the variance is under a threshold to be determined, then we both save the measured average timing between two jumps, and the average amplitude. Only the timing will be used in the first implementation,
  while the amplitude difference could be used to give an idea about distance/calibration/etc.
  Takes an array (of zeroes, preferentially, which will contain the value 1 in 1st position if transmission can begin, and in that case fills useful information for the rest of the transmission.
*/
void checkIfTransmission(float transmissionInfo[4]) {
  digitalWrite(photoDiodePort, HIGH);
  
  long timeOut = 200000; // micro seconds based on sender frequency, 
  int totalTries = 10; // is useful for several attempts, in case a sudden obstacle hampers signal. can therfore detect jumps at frequencies as low as 1/timeOut
  int nbJumps = 20; // for more or less accuracy in the clock determination.
  int minJumpAmplitude = 10; //100; // with currentsetup, 1 only works in darkness, at least 100 with ambient light. could change dynamically to fit this pattern. higher threshold means less sensitive to noise, but requires shorter distance. 
  float maxAmplitudeVariance = 1000000; // irrelevant for now, could add meaningful threshold later.
  float maxIntervalVarianceMultiplier = 0.2; // the max acceptable variance will be maxIntervalVarianceMultiplier * measuredAverage^2

  float averageAmplitude = 0;
  float averageInterval = 0;
  float averageAmplitudeDeviation = 0;
  float averageIntervalDeviation = 0;
  int successes = 0;

  long period;
  int jumpAmplitude, sensorValue, currentValue;
  float jumpAmplitudes[nbJumps];
  float jumpIntervals[nbJumps];

  float var_amplitudes;
  float mean_amplitudes;
  float var_intervals;
  float mean_intervals;

  // give 10 tries and enforce 50% success
  for (int currentTry = 0; currentTry < totalTries; currentTry++) {
    Serial.println(currentTry);
    var_amplitudes = 0.0;
    mean_amplitudes = 0.0;
    var_intervals = 0.0;
    mean_intervals = 0.0;

    int jump = 0;
    long currentMicroSeconds = micros();
    long lastJumpMicroSeconds = micros();
    //long n = 1;
    while (jump < nbJumps && (currentMicroSeconds - lastJumpMicroSeconds) < timeOut) {
      //n = currentMicroSeconds - lastJumpMicroSeconds;
      //Serial.println(n);
      currentMicroSeconds = micros();
      sensorValue = analogRead(A0);
      jumpAmplitude = abs(sensorValue - currentValue);
      if (jumpAmplitude >= minJumpAmplitude) { // would have to change the logic to min-max instead of pure variation, to be more resilient to reflections/noise/slow jumps
        currentValue = sensorValue;
        period = currentMicroSeconds - lastJumpMicroSeconds;
        jumpAmplitudes[jump] = jumpAmplitude;
        jumpIntervals[jump] = period;
        lastJumpMicroSeconds = currentMicroSeconds;
        jump++;
      }
    }
    if (jump == nbJumps) {
      float tempTotalJumpAmplitudes = 0.0;
      float tempTotalJumpIntervals = 0.0;

      for (int i=0; i<nbJumps; i++){
        tempTotalJumpAmplitudes += jumpAmplitudes[i];
        tempTotalJumpIntervals += jumpIntervals[i];
      }

      mean_amplitudes = tempTotalJumpAmplitudes / (float)jump;
      mean_intervals = tempTotalJumpIntervals / (float)jump;
      tempTotalJumpAmplitudes = 0.0;
      tempTotalJumpIntervals = 0.0;

      for (int i=0; i<nbJumps; i++) {
        tempTotalJumpAmplitudes += (jumpAmplitudes[i]- mean_amplitudes) * (jumpAmplitudes[i] - mean_amplitudes);
        tempTotalJumpIntervals += (jumpIntervals[i] - mean_intervals) * (jumpIntervals[i] - mean_intervals);
      }
      var_amplitudes = tempTotalJumpAmplitudes / ((float)jump - 1.0);
      var_intervals = tempTotalJumpIntervals / ((float)jump - 1.0);
      Serial.println(var_intervals);
      Serial.println(mean_intervals);
      if (var_intervals < (maxIntervalVarianceMultiplier *  mean_intervals * mean_intervals) && var_amplitudes < maxAmplitudeVariance) {
        averageAmplitude = (successes * averageAmplitude + mean_amplitudes)/((float)successes + 1.0);
        averageInterval = (successes * averageInterval + mean_intervals)/((float)successes + 1.0);
        averageAmplitudeDeviation = (successes * averageAmplitudeDeviation + sqrt(var_amplitudes))/((float)successes + 1.0);
        averageIntervalDeviation = (successes * averageIntervalDeviation + sqrt(var_intervals))/((float)successes + 1.0);
        successes++;
        Serial.println("success!");
      }
    }
  }
  
  if (1.0 * successes/(float)totalTries > 0.5){
    transmissionInfo[0] = 1;
    transmissionInfo[1] = averageInterval;
    transmissionInfo[2] = averageIntervalDeviation;
    transmissionInfo[3] = averageAmplitude;
  }
  
  for(int i =0; i<4; i++){
    Serial.println(transmissionInfo[i]);
  }
  digitalWrite(photoDiodePort, LOW);
}

/*
 * Here, we would add the code for the low-power consumption, from github. 
 * Different measurements for average power consumption should be computed to deduce the average lifetime of the underwater sensor
*/
void goBackToSleep() {
  Serial.println("Going back to sleep");
  // LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);  // SLEEP_8S also works
  delay(3000);
  Serial.println("Woke up");
}

void sendStuff(){
  Serial.println("Sending Stuff...");
  delay(3000);
}

void blink(int times, int port, int period){
  for (int i=0; i<times; i++) {
    digitalWrite(port, HIGH);
    delay(period/2);
    digitalWrite(port,LOW);
    delay(period/2);
  }
}

