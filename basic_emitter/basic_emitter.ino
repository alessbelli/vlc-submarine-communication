const int ledPin =  13;      // the number of the LED pin
int blinkPeriod = 25;     // in ms
unsigned long previousMillis = 0;
int ledState = 0;

void setup() {
 Serial.begin(9600);
  pinMode(13, OUTPUT);
}
void loop() {
  unsigned long currentMillis = millis();
  if((currentMillis-previousMillis)>blinkPeriod/2) {
    ledState = 1 - ledState;
    digitalWrite(ledPin, ledState);
    previousMillis = currentMillis;
  }
}
