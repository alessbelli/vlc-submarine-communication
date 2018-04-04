
const int photoDiodePort = 12;
const int blueDiodePort = 13; //output

const int okLedPort = 3;
const int failLedPort = 2;

uint8_t outBit = 0;
uint8_t outPort = 0; //pas nécéssaire

volatile uint8_t *out = NULL;

unsigned long previousMicros = micros();
unsigned long currentMicros;
unsigned long nextSendInstant;
int ledState = 0;
long startPacketMicros;
long ticks;
int tickDuration = 1000; // in microseconds

const int PACKET_SIZE = 64;
byte packet[PACKET_SIZE];
byte curPacketNumber = 0;
byte curByteNumber = 0;
boolean hasAcknowledgedPacket = true;
int bytesAvailable;
int packetCount = 0;
byte junk;
byte lastPacketSize;
byte parity = 0;
byte ledMask = (1 << (blueDiodePort - 8));

void setup() {
  pinMode(blueDiodePort, OUTPUT);
  pinMode(okLedPort, OUTPUT);
  pinMode(failLedPort, OUTPUT);
  blinker(3, failLedPort, 100);

  //for testing, commenting otherwise
  Serial.begin(115200);
  while (!Serial) {
    ; //wait for serial
  }
  establishContact();
}

void loop() {

  if (Serial.available()) {

    if (Serial.available() == 1 && packetCount == 0) { // first time fetching data
      packetCount = Serial.read(); // Processing sends packet count
      while (Serial.available()) {
        junk = Serial.read();  //there shouldn't be anything here but we never know so clear queue
      }
      Serial.write(curPacketNumber); // we ask for first packet
      Serial.flush();
    } else { // receiving actual packets

      curByteNumber = 0;
      bytesAvailable = Serial.available();
      while (Serial.available()) {
        packet[curByteNumber] = Serial.read();
        curByteNumber++;
      }
      // packet contains all the data.
      if (curPacketNumber == packetCount - 1 ) {
        lastPacketSize = curByteNumber; // lastPacket will be smaller in reality
      }

      while (curByteNumber < PACKET_SIZE) { // fill last packet with 0s
        packet[curByteNumber] = 0x00;
        curByteNumber++;
      }

      // packet is ready for sending!
      startPacketMicros = micros();
      ticks = 0;
      sendByte(0xff);
      sendByte(0x9A);
      sendByte(curPacketNumber);
      for (int i = 0; i < PACKET_SIZE; i++) {
        parity ^= packet[i];
        sendByte(packet[i]);
      }
      sendByte(parity);


      // ready for next packet, or to send something else
      if (curPacketNumber < packetCount - 1) {
        curPacketNumber++;
        // blinker(curPacketNumber, okLedPort, 1000);
        Serial.write(curPacketNumber);
        Serial.flush();
      }
    }

  }
}

void sendByte(byte byteToSend) {
  uint16_t modulatedByte = moduler(byteToSend);

  for (int j = 0; j < 16; j++) {
    long nextSendInstant = startPacketMicros + ticks * tickDuration;
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

void blinker(int times, int port, int period) { //mostly used for debug and to distinguish the two arduinos
  for (int i = 0; i < times; i++) {
    digitalWrite(port, HIGH);
    delay(period / 2);
    digitalWrite(port, LOW);
    delay(period / 2);
  }
}

void establishContact() {
  while (Serial.available() <= 0) {
    Serial.print('A');   // send a capital A to handshake, could be something else!
    Serial.flush();
    delay(100);
  }
}

