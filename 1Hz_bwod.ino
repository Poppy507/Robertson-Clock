
const int ledPin =  LED_BUILTIN;
const int pulse = 2;

int ledState = LOW;

unsigned long previousMillis = 0;

// constants won't change:
const long interval = 1000;           // blinking time period (milliseconds)

void setup() {
  // set the digital pin as output:
  pinMode(ledPin, OUTPUT);
  pinMode(pulse, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    if (ledState == LOW) {
      ledState = HIGH;
    }
    else {
      ledState = LOW;
    }

    digitalWrite(ledPin, ledState);
    digitalWrite(pulse, ledState);
  }
}
