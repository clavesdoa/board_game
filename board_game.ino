int ledPins[] = {
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};  // an array of pin numbers to which LEDs are attached

void setup() {
  // loop over the pin array and set them all to output:
  for (int thisLed = 0; thisLed < 10; thisLed++) {
    pinMode(ledPins[thisLed], OUTPUT);
  }
}

void loop() {
  // loop over the LED array:
  for (int thisLed = 0; thisLed < 10; thisLed++) {
    digitalWrite(ledPins[thisLed], HIGH);
    delay(500);
    digitalWrite(ledPins[thisLed], LOW);
  }
}
