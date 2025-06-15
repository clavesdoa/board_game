#include <LiquidCrystal.h>
// we use this implementation, install it in your Arduino IDE
// https://github.com/Aasim-A/AsyncTimer
#include <AsyncTimer.h>
#include "board_game.h"

const int BUTTON_PIN = 3;
const int displayWidth = 16;

// timer
AsyncTimer t;
// schedule function calls so that the events run in a chain
unsigned short schedule(const EventQueue& queue) {
  if (queue.isEmpty()) {
    return 0;
  }
  return t.setTimeout([&queue]() {
    // call the lambda
    queue.front().cb->call();
    // clean up the lambda
    queue.front().cleanUp();
    // schedule next event after this completed
    if (queue.dequeue()) {
      schedule(queue);
    }
  },
                      queue.front().delay);
}

// number of players
unsigned int numPlayers = 0;

// state "machine"
enum Stage {
  INTRO,
  LIGHT_DEMO,
  READY,
  START,
  SELECT_PLAYERS,
  NUM_PLAYERS,
  NEXT_PLAYER,
  THROW_DICE
};
Stage currentStage = INTRO;

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
volatile unsigned long lastInterruptTime = 0;
const unsigned long debounceDelay = 50;
volatile byte buttonReleased = false;
void buttonReleasedInterrupt() {
  unsigned long now = millis();
  if (now - lastInterruptTime > debounceDelay) {
    buttonReleased = true;
    lastInterruptTime = now;
  }
}

void setUpPins(const LedArray& leds) {
  // loop over the pin array and set them all to output:
  for (int idx = 0; idx < leds.size; idx++) {
    pinMode(leds.pins[idx], OUTPUT);
  }
}

void putOnOffLed(const LedArray& leds, int index, bool putOn = true, unsigned long delayMillis = 300) {
  leds.ledEvents.enqueue({ mkCb([index, putOn, &leds]() {
                             digitalWrite(leds.pins[index], putOn ? HIGH : LOW);
                           }),
                           delayMillis });
}

// put on/off leds of a led array in sequence
void lightSequence(const LedArray& leds, bool on = true) {
  for (int idx = 0; idx < leds.size; idx++) {
    putOnOffLed(leds, idx, on);
  }
}

void asyncDelay(const EventQueue& queue, unsigned long delayMillis) {
  queue.enqueue({ mkCb([]() {
                    // do nothing
                  }),
                  delayMillis });
}

void blinkText(const EventQueue& queue, int repetitions = 2, unsigned long delayMillis = 800) {
  for (int idx = 0; idx < repetitions; idx++) {
    queue.enqueue({ mkCb([]() {
                      lcd.noDisplay();
                    }),
                    delayMillis });
    queue.enqueue({ mkCb([]() {
                      lcd.display();
                    }),
                    delayMillis });
  }
}

void scrollText(const EventQueue& queue, const String& message, int row = 0, unsigned long delayMillis = 300) {
  int segmentSize = message.length() - displayWidth;
  if (segmentSize <= 0) {
    // no need to scroll (the message is short enough)
    queue.enqueue({ mkCb([message, row]() {
                      if (row == 0) {
                        lcd.clear();
                      }
                      lcd.setCursor(0, row);
                      lcd.print(message);
                    }),
                    delayMillis });
  } else {
    for (int idx = 0; idx <= segmentSize; idx++) {
      // schedule the print of shifted substrings of the message to be displayed one after another
      queue.enqueue({ mkCb([message, row, idx]() {
                        String segment = message.substring(idx, idx + displayWidth);
                        lcd.setCursor(0, row);
                        lcd.print(segment);
                      }),
                      delayMillis });
    }
  }
}

void runIntro() {
  // initial greetings
  events.enqueue({ mkCb([]() {
                     lcd.clear();
                     lcd.setCursor(0, 0);
                     lcd.print(" Hello Players!");
                   }),
                   10 });
  // blink the writing on the display
  blinkText(events);
  asyncDelay(events, 500);
  // scroll text with more greetings
  scrollText(events, " Hello Players!  Welcome to the Game!");
  asyncDelay(events, 1000);
  scrollText(events, "Ready to play?");
  asyncDelay(events, 1000);
  scrollText(events, "Push the button!", 1);
  schedule(events);
}

void ledsDemo(const LedArray& leds) {
  lightSequence(leds);
  lightSequence(leds, false);
  // loop demo until players start the game
  leds.ledEvents.enqueue({ mkCb([&leds]() {
                             if (currentStage == READY) {
                               ledsDemo(leds);
                             }
                           }),
                           10 });
}

void lightDemo() {
  // boss energy light demo
  ledsDemo(bossEnergyLeds);
  schedule(bossEnergyLeds.ledEvents);
  // player points demo
  ledsDemo(playerPointsLeds);
  schedule(playerPointsLeds.ledEvents);
  // path leds demo
  ledsDemo(pathLeds);
  schedule(pathLeds.ledEvents);
}

void nextStage() {
  switch (currentStage) {
    case INTRO:
      currentStage = LIGHT_DEMO;
      runIntro();
      break;
    case LIGHT_DEMO:
      currentStage = READY;
      lightDemo();
      break;
    case START:
      currentStage = SELECT_PLAYERS;
      lcd.clear();
      scrollText(events, "  How many players? Push the button to select.");
      schedule(events);
      break;
    case NUM_PLAYERS:
      currentStage = NEXT_PLAYER;
      scrollText(events, "Player n. 1 ready!");
      break;
    default:
      //
      break;
  }
}

void setup() {
  // setup led pins
  setUpPins(bossEnergyLeds);
  setUpPins(playerPointsLeds);
  setUpPins(pathLeds);
  // setup serial
  Serial.begin(9600);
  // setup button
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN),
                  buttonReleasedInterrupt,
                  FALLING);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
}

// event to cancel
unsigned short timerId = 0;

void loop() {
  // hadle timer
  t.handle();
  // proceed to next stage
  nextStage();
  // handle button
  if (buttonReleased) {
    buttonReleased = false;
    Serial.println("Button released");
    switch (currentStage) {
      case READY:
        currentStage = START;
        break;
      case SELECT_PLAYERS:
        // cancel current timeout
        t.cancel(timerId);
        numPlayers++;
        lcd.setCursor(0, 1);
        lcd.print(numPlayers);
        // create new timeout
        events.enqueue({ mkCb([]() {
                           currentStage = NUM_PLAYERS;
                         }),
                         5000 });
        timerId = schedule(events);
        break;
      default:
        //
        break;
    }
  }
  //delay(100);
  // fillUpEnergy(bossEnergyPins, BOSS_ENERGY);
  // loopPins(playerPointsPins, PLAYER_POINTS);
  // loopPins(pathLightPins, PATH_LIGHTS);

  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  // lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  // lcd.print(millis() / 1000);
}
