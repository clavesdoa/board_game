#include <LiquidCrystal.h>

const int BOSS_ENERGY = 10;
const int PLAYER_ENERGY = 10;
const int PATH_LIGHTS = 10;
const int BUTTON_PIN = 3;

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// an array of pin numbers to which bos LEDs are attached
int bossEnergyPins[] = {
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31
};

// an array of pin numbers to which player LEDs are attached
int playerEnergyPins[] = {
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41
};

// an array of pin numbers to which path LEDs are attached
int pathLightPins[] = {
  42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

volatile byte buttonReleased = false;
void buttonReleasedInterrupt() {
  buttonReleased = true;
}

void setUpPins(const int* pins, int size) {
  // loop over the pin array and set them all to output:
  for (int idx = 0; idx < size; idx++) {
    pinMode(pins[idx], OUTPUT);
  }
}

void loopPins(const int* pins, int size) {
  // loop over the LED array:
  for (int idx = 0; idx < size; idx++) {
    digitalWrite(pins[idx], HIGH);
    delay(500);
    digitalWrite(pins[idx], LOW);
  }
}

void setup() {
  // setup led pins
  setUpPins(bossEnergyPins, BOSS_ENERGY);
  setUpPins(playerEnergyPins, PLAYER_ENERGY);
  setUpPins(pathLightPins, PATH_LIGHTS);

  Serial.begin(9600);
  // setup button
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  buttonReleasedInterrupt,
                  FALLING);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
}

void loop() {
  if (buttonReleased) {
    buttonReleased = false;
    // do something here, for example print on Serial
    Serial.println("Button released");
  }
  delay(100);
  // loopPins(bossEnergyPins, BOSS_ENERGY);
  // loopPins(playerEnergyPins, PLAYER_ENERGY);
  // loopPins(pathLightPins, PATH_LIGHTS);

  // Turn off the display:
  lcd.noDisplay();
  delay(500);
  // Turn on the display:
  lcd.display();
  delay(500);
}
