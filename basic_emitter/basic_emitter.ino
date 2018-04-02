
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
  blinker(3, failLedPort, 1000);
  
  
  outBit = digitalPinToBitMask(blueDiodePort);
  outPort = digitalPinToPort(blueDiodePort);
  out = portOutputRegister(outPort); //pourrait remplacer outPort par digitalPintToPort directement....
}

void loop() {
  currentMicros = micros();
  
  if( 2*(currentMicros-previousMicros) > blinkPeriod) {
    ledState = ~ledState;
    
    
    if (ledState) {
      *out |= outBit;
    } 
    else {
      *out &= ~outBit;
    }
    
    previousMicros = currentMicros;
  }

}

void blinker(int times, int port, int period){
  for (int i=0; i<times; i++) {
    digitalWrite(port, HIGH);
    delay(period/2);
    digitalWrite(port,LOW);
    delay(period/2);
  }
}

