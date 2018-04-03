
#define messageSize  2

const int photoDiodePort = 12;
const int blueDiodePort = 13; //output

const int okLedPort = 3;
const int failLedPort = 2;


uint8_t outBit = 0;
uint8_t outPort = 0; //pas nécéssaire

volatile uint8_t *out = NULL;


int blinkPeriod = 1000;     // in µs
unsigned long previousMicros = micros();
unsigned long currentMicros;

int ledState = 0;

void setup() {
  pinMode(blueDiodePort, OUTPUT);
  pinMode(okLedPort, OUTPUT);
  pinMode(failLedPort, OUTPUT);
  blinker(3, failLedPort, 100);

  //for testing, commenting otherwise
  Serial.begin(9600);

  byte test1[messageSize];
  test1[0] = 0x0;
  test1[1] = 0xAA;

  uint16_t test2[messageSize];

  moduler (test1, test2);

  Serial.print(test1[0], BIN);
  Serial.print("\t");
  Serial.print(test1[1], BIN);
  Serial.print("\n");

  Serial.print(test2[0], BIN);
  Serial.print("\t");
  Serial.print(test2[1], BIN);
  Serial.print("\n");


  outBit = digitalPinToBitMask(blueDiodePort);
  outPort = digitalPinToPort(blueDiodePort);
  out = portOutputRegister(outPort); //pourrait remplacer outPort par digitalPintToPort directement....
}

void loop() {
  currentMicros = micros();

  if ( 2 * (currentMicros - previousMicros) > blinkPeriod) {
    ledState ^= 0x01; //toggle


    if (ledState) {
      *out |= outBit;
    }
    else {
      *out &= ~outBit;
    }

    previousMicros = currentMicros;
  }
}


void moduler(byte message[messageSize], uint16_t moduledMsg[messageSize])
{
  byte val;
  int i, j;
  uint16_t inter = 0;
  uint16_t mask = 0;
  for (i = 0; i < messageSize; i++)
  {
    for (j = 0; j < 8; j++)
    {
      val = (message[i] >> j) & 0x01;
      mask = 0x03 << 2 * j; // les deux bits qu'on modifie seulement
      inter = (inter & ~mask) | (0x01 << (2 * j + 1 - val) & mask ); // replace bits under mask by 01 if val = 1, 10 if val = 0
    }
    moduledMsg[i] = inter;
  }
}


void blinker(int times, int port, int period) {
  for (int i = 0; i < times; i++) {
    digitalWrite(port, HIGH);
    delay(period / 2);
    digitalWrite(port, LOW);
    delay(period / 2);
  }
}

