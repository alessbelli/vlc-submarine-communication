
const byte adcPin = 0; //A0
const int photoDiodePort = 12;
const int blueDiodePort = 13;
const int vPlusPort = 11;
const int okLedPort = 3;
const int failLedPort = 2;

volatile int adcReading;
volatile boolean adcDone;
long startTime;
long endTime;
int currentIndex = 0;
int packetNo = 0;
int jumps = 0;
int junk;
const int DETECTION_THRESHOLD = 1; // ~ 4mV = 1.1V/256 threshold for jump detection
const int NB_JUMPS_SYNC = 5;
const int PACKET_SIZE = 64;
uint8_t dataBuffer[PACKET_SIZE + 2];
int timeTaken = 0;
int signalClock;
int elapsedClockTics;
// booleans that organize the flow in the loop
boolean sendWakeupSignal = false; // should be true in real environment
boolean measureSignalPeriod = true;
boolean computeSignalPeriod = false;
boolean receivePreamble = false;
boolean receiveData = false;
boolean receiveFirstBit = false;
boolean hasSignalClock = false;
boolean hasData = false;
boolean computeParity = false;
boolean writeReceivedData = false;
boolean adcStarted;

boolean even = false;
byte ledMask = 0x01 << (blueDiodePort - 8);
byte newVal = 0;
byte curVal = 0;
long startSyncTime;
long endSyncTime;
long nextSendInstant;
long tickDuration = 10000;
long ticks;
long startPacketMicros;
long maxSleepDuration = 10000000; // also µs
byte receivedByte = 0;
byte resultByte = 0;
int bitIndex = 0;
byte parityByte = 0;
uint16_t receivedPreamble;
uint16_t OK_PREAMBLE = 0b0101010110010110; //=0x09 modulated manchester

// Defines for clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
// Defines for setting register bits
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#define isnull 0 // null character for serial info

void setup ()
{
  Serial.begin (115200);
  pinMode(photoDiodePort, INPUT);
  pinMode(blueDiodePort, OUTPUT);
  pinMode(okLedPort, OUTPUT);
  pinMode(failLedPort, OUTPUT);
  pinMode(vPlusPort, OUTPUT);
  digitalWrite(vPlusPort, HIGH);
  blinker(4, okLedPort, 200);
  endTime = micros();
  receiveData = false;
  while(Serial.available()<=0) {//hold until Serial instruction to start
    delay(100);
  }
  while(Serial.available()) { //empty the buffer, we don't really care for this data.
    junk = Serial.read(); 
  }
}  // end of setup

// ADC complete ISR
ISR (ADC_vect)
{
  if (measureSignalPeriod) {
    byte newVal = ADCH;

    if (newVal >= DETECTION_THRESHOLD && curVal == 0) {
      jumps++;
      if (jumps == 1) {
        startSyncTime = micros();
      }
    }
    if (jumps == NB_JUMPS_SYNC) {
      endSyncTime = micros();
      cbi(ADCSRA, ADEN); // disable ADC
      computeSignalPeriod = true;
      measureSignalPeriod = false;
    }
    curVal = newVal;

  }
  if (receivePreamble) {
    receivedPreamble = (receivedPreamble << 1) | (ADCH >=DETECTION_THRESHOLD);
    if (receivedPreamble ==  OK_PREAMBLE){
      cbi(ADCSRA,ADEN);
      receivedPreamble = 0;
      receiveData = true;
    }
  }
  if (receiveData) {
    if (even) {
      resultByte |= (ADCH>=DETECTION_THRESHOLD) << bitIndex;
      bitIndex++;
    }
    even = !even;
    if(bitIndex == 8) {
      dataBuffer[currentIndex] = resultByte;
      resultByte = 0;
      currentIndex++;
    }
    if (currentIndex == PACKET_SIZE + 1) {
      cbi(ADCSRA, ADEN); // disable ADC
      endTime = micros();
      computeParity = true;
    }
  }
  adcStarted = false;
}  // end of ADC_vect


void loop ()
{
  if (sendWakeupSignal) {
    tickDuration = 10000; // microseconds
    startPacketMicros = micros();
    ticks = 0;
    while (micros() < startPacketMicros + maxSleepDuration) {
      sendByte(0x00); // send clock repeatedly so that it may wake up fixed station
    }
    sendByte(0x09); // preamble to check that it's not a fluke;
    //this should coincide with the other side sending their packets, so listen for that
    measureSignalPeriod = true;
  }

  
  if (measureSignalPeriod) {
    if (!adcStarted){
      startADC();
    } 
    // delay(1); // should go, but for now required for not crashing the software
  }
  if (computeSignalPeriod) {
    measureSignalPeriod = true;
    signalClock = (endSyncTime - startSyncTime) / (2 * (NB_JUMPS_SYNC - 1)); // int so <65536µs, so, frequency must be greater than 15Hz
    Serial.println(signalClock) ;
    jumps = 0;
    delay(100);
    computeSignalPeriod = false;
    //receiveData = true;
    //receiveFirstBit = true;
    // receivePreamble = true
  }
  if (receivePreamble) {
    hasSignalClock = true;
    if (receiveFirstBit) {
      ticks = (micros() - endSyncTime) / (2 * signalClock) + 1;
      receiveFirstBit = false;
    }
    if (micros() > endSyncTime + signalClock * ticks + signalClock / 2) {
      startADC();
      ticks++;
    }
  }

  if (receiveData) {
    if (micros() > endSyncTime + signalClock * ticks + signalClock / 2) {
      startADC();
      ticks++;
    }
  }
  
  if (computeParity) {
    parityByte = 0;
    for (int i=1; i<PACKET_SIZE+1;i++){
      parityByte ^= dataBuffer[i];
    }
    if (parityByte == dataBuffer[PACKET_SIZE+1]) {
      sendByte(parityByte);
      writeReceivedData = true;
    } else {
      delay(16*signalClock/1000); //wait for the packet to be sent again, replace function with nonblocking?
      measureSignalPeriod = true;;
    }
  }

  if (writeReceivedData) {
    timeTaken = (endTime - startTime);
    Serial.println("time taken"); // send buffer header
    Serial.println(timeTaken);
    Serial.write(dataBuffer, PACKET_SIZE+2);
    Serial.print('\n');
    Serial.flush();
    writeReceivedData = false;
    packetNo++;
    startTime = micros();
    hasData = true;
  }
  // do other stuff here
  if (hasData) {
    startADC();
  }

}  // end of loop

void startADC() {
  adcStarted = true;
  hasData = false;
  currentIndex = 0;
  writeReceivedData = false;
  currentIndex = 0;
  cli();//disable interrupts
  ADCSRB = 0;
  ADMUX = (1 << REFS0) | (1 << REFS1) | (1 << ADLAR) | (adcPin & 0x07);    // set internal ref 1.1V, output on 1 byte and select input port
  ADCSRA = (1 << ADEN) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADIE) | (1 << ADSC); // enable ADC, change clock multiplier to 1/8, and start it on interrupts
  // maybe clock divider should be increased for more stability
  startTime = micros(); // pointless by itself, but seems to delay just enough as to ensure stability without additional delay()
  sei(); //enable interrupts
}

void sendAck() {

}
void blinker(int times, int port, int period) {
  for (int i = 0; i < times; i++) {
    digitalWrite(port, HIGH);
    delay(period / 2);
    digitalWrite(port, LOW);
    delay(period / 2);
  }
}

void sendByte(byte byteToSend) {
  uint16_t modulatedByte = moduler(byteToSend);

  for (int j = 0; j < 16; j++) {
    nextSendInstant = startPacketMicros + ticks * tickDuration;
    while (micros() < nextSendInstant) {
      // digitalWrite(blueDiodePort,(0x01& (modulatedByte>>j))); // We use the faster version which is only 1 instruction
      PORTB = (PORTB & ~ledMask) | ((((0x01 & (modulatedByte >> j)) << (blueDiodePort - 8))) & ledMask);

    }
    //  delay(10);
    ticks++;
  }


}

uint16_t moduler(byte message) {
  byte val;
  int j;
  uint16_t inter = 0;
  uint16_t mask = 0;
  for (j = 0; j < 8; j++) {
    val = (message >> j) & 0x01;
    mask = 0x03 << 2 * j; // les deux bits qu'on modifie seulement
    inter = (inter & ~mask) | (0x01 << (2 * j + 1 - val) & mask ); // replace bits under mask by 01 if val = 1, 10 if val = 0
  }
  return inter;
}
