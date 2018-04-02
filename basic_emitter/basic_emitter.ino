
const int photoDiodePort = 12;
const int blueDiodePort = 13;

const int okLedPort = 3;
const int failLedPort = 2;

int blinkPeriod = 1;     // in ms
unsigned long previousMillis = 0;
int ledState = 0;

void setup() {
  pinMode(blueDiodePort, OUTPUT);
  pinMode(okLedPort, OUTPUT);
  pinMode(failLedPort, OUTPUT);
  blinker(3, failLedPort, 1000); 
}
void loop() {
  unsigned long currentMillis = millis();
  if((currentMillis-previousMillis)>blinkPeriod/2) {
    ledState = 1 - ledState;
    digitalWrite(blueDiodePort, ledState);
    previousMillis = currentMillis;
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
