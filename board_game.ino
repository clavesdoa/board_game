#include <LiquidCrystal.h>
#include <AsyncTimer.h>
#include <Queue.h>

const int BUTTON_PIN = 3;
const int displayWidth = 16;

// represents a led array
struct LedArray {
  LedArray(const int* _pins, int _size)
    : pins(_pins), size(_size){};
  // set index, returns the current setting
  int setIndex(int newIndex) {
    int current = index;
    index = newIndex % size;
    return current;
  }
  const int* pins;
  int size;
  int index = 0;
};

// represents an event. We use a type parameter instead of std::function since the latter is not fully supported across Arduinos
struct Event {
  void run() {
    if (callback) {
      Serial.print("a");
      callback();
      Serial.print("b");
    }
  }
  void (*callback)();
  unsigned long delay;
};

// event queue pointer
using EventQueue = Queue<Event, 10>;
EventQueue events;

// timer
AsyncTimer t;
// schedule function calls
void schedule() {
  Serial.print("Scheduling: ");
  Serial.println(events.size());
  if (events.isEmpty()) {
    return;
  }
  Event next = events.front();
  t.setTimeout([&]() {
    Serial.print("x");
    // call the lambda
    next.run();
    Serial.print("y");
    // schedule next event
    if (events.dequeue()) {
      schedule();
    }
  },
               next.delay);
}

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 12, en = 11, d4 = 10, d5 = 9, d6 = 8, d7 = 7;
const LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// energy leds
const int energyPins[] = { 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
const LedArray bossEnergyLeds = LedArray(energyPins, sizeof(energyPins) / sizeof(energyPins[0]));
// player leds
const int playerPins[] = { 32, 33, 34, 35, 36, 37, 38, 39, 40, 41 };
const LedArray playerPointsLeds = LedArray(playerPins, sizeof(playerPins) / sizeof(playerPins[0]));
// path leds
const int pathPins[] = { 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
const LedArray pathLeds = LedArray(pathPins, sizeof(pathPins) / sizeof(pathPins[0]));

// manages the button events
volatile byte buttonReleased = false;
void buttonReleasedInterrupt() {
  buttonReleased = true;
}

void setUpPins(const LedArray& leds) {
  // loop over the pin array and set them all to output:
  for (int idx = 0; idx < leds.size; idx++) {
    pinMode(leds.pins[idx], OUTPUT);
  }
}

void putOnLed(const LedArray& leds, int index, bool putOffPrev = true) {
  if (putOffPrev) {
    digitalWrite(leds.pins[leds.setIndex(index)], LOW);
  }
  digitalWrite(leds.pins[leds.index], HIGH);
}

// void blinkText(unsigned long delayMillis = 1000) {

//   schedule([]() {
//     // Turn off the display:
//     lcd.noDisplay();
//   },
//            delayMillis * idx);
//   schedule([]() {
//     // Turn on the display:
//     lcd.display();
//   },
//            delayMillis * idx);
// }

// void scrollText(const String& message, int row = 0, unsigned long delayMillis = 300) {
//   if (row == 0) {
//     lcd.clear();
//   }
//   int segmentSize = message.length() - displayWidth;
//   // Print a message to the LCD.
//   if (segmentSize <= 0) {
//     lcd.setCursor(0, row);
//     lcd.print(message);
//   } else {
//     for (int idx = 0; idx <= segmentSize; idx++) {
//       schedule([&]() {
//         String segment = message.substring(idx, idx + displayWidth);
//         lcd.setCursor(0, row);
//         lcd.print(segment);
//       },
//                delayMillis * (idx + 1));
//     }
//   }
// }

void runIntro() {
  events.enqueue({ []() {
                    Serial.print("f");
                  },
                   1000 });
  events.enqueue({ []() {
                    Serial.print("u");
                  },
                   1000 });
  events.enqueue({ []() {
                    Serial.print("f");
                  },
                   1000 });
  events.enqueue({ []() {
                    Serial.print("f");
                  },
                   1000 });
  // events.enqueue({ []() {
  //                   lcd.clear();
  //                   lcd.setCursor(0, 0);
  //                   lcd.print("Hello Players!");
  //                 },
  //                  1000 });
  // events.enqueue({ []() {
  //                   lcd.noDisplay();
  //                 },
  //                  1000 });
  // events.enqueue({ []() {
  //                   lcd.display();
  //                 },
  //                  1000 });
  // scrollText("Hello Players!  Welcome to the Game!");
  // delay(1000);
  // scrollText("Ready to play?");
  // delay(1000);
  // scrollText("Push the button", 1);
  schedule();
}

unsigned int stage = 0;

void nextStage() {
  switch (stage) {
    case 0:
      runIntro();
      stage++;
      break;
  }
}

void setup() {
  // setup led pins
  setUpPins(bossEnergyLeds);
  setUpPins(playerPointsLeds);
  setUpPins(pathLeds);

  Serial.begin(9600);
  // setup button
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  buttonReleasedInterrupt,
                  FALLING);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
}

void loop() {
  // hadle timer
  t.handle();
  // proceed to next stage
  nextStage();
  // handle button
  if (buttonReleased) {
    buttonReleased = false;
    // do something here, for example print on Serial
    Serial.println("Button released");
  }
  delay(100);
  // fillUpEnergy(bossEnergyPins, BOSS_ENERGY);
  // loopPins(playerPointsPins, PLAYER_POINTS);
  // loopPins(pathLightPins, PATH_LIGHTS);

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  // lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  // lcd.print(millis() / 1000);
}
