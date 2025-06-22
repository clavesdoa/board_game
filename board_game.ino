#include <LiquidCrystal.h>
#include "board_game.h"

const int BUTTON_PIN = 3;
const int displayWidth = 16;

// number of players
unsigned int numPlayers = 0;

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
// we add a debounce mechanism since the physical button can bounce causing mutiple unwanted triggers
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
  leds.ledEvents.add({ mkCb([index, putOn, &leds]() {
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

// non blocking delay added to the event queue
void asyncDelay(const Scheduler& sched, unsigned long delayMillis) {
  sched.add({ mkCb([]() {
                // do nothing
              }),
              delayMillis });
}

// the blinking is done putting on/off the display
void blinkText(int repetitions = 2, unsigned long delayMillis = 800) {
  for (int idx = 0; idx < repetitions; idx++) {
    events.add({ mkCb([]() {
                   lcd.noDisplay();
                 }),
                 delayMillis });
    events.add({ mkCb([]() {
                   lcd.display();
                 }),
                 delayMillis });
  }
}

// creates a non blocking scrolling effect
void scrollText(const String& message, int row = 0, unsigned long delayMillis = 300) {
  if (row > 1) {
    Serial.println("*** Wrong Row ***");
    Serial.println(row);
  }
  int segmentSize = message.length() - displayWidth;
  if (segmentSize <= 0) {
    // no need to scroll (the message is short enough to fit in the LCD display)
    events.add({ mkCb([message, row]() {
                   if (row == 0) {
                     lcd.clear();
                   }
                   lcd.setCursor(0, row);
                   lcd.print(message);
                 }),
                 10 });
  } else {
    for (int idx = 0; idx <= segmentSize; idx++) {
      // schedule the print of shifted substrings of the message to be displayed one after another
      events.add({ mkCb([message, row, idx]() {
                     if (row == 0 && idx == 0) {
                       lcd.clear();
                     }
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
  events.add({ mkCb([]() {
                 lcd.clear();
                 lcd.setCursor(0, 0);
                 lcd.print(" Hello Players!");
               }),
               10 });
  // blink the writing on the display
  blinkText();
  asyncDelay(events, 500);
  // scroll text with more greetings
  scrollText(" Hello Players!  Welcome to the Game!");
  asyncDelay(events, 1000);
  scrollText("Ready to play?");
  asyncDelay(events, 1000);
  scrollText("Push the button!", 1);
}

// a few light effects
void ledsDemo(const LedArray& leds) {
  lightSequence(leds);
  lightSequence(leds, false);
  // loop demo until players start the game
  leds.ledEvents.add({ mkCb([&leds]() {
                         if (currentState == READY) {
                           ledsDemo(leds);
                         }
                       }),
                       10 });
}

void lightDemo() {
  // boss energy light demo
  ledsDemo(bossEnergyLeds);
  // player points demo
  ledsDemo(playerPointsLeds);
  // path leds demo
  ledsDemo(pathLeds);
}

// state machine implementation
void nextState() {
  switch (currentState) {
    case INTRO:
      setState(LIGHT_DEMO);
      runIntro();
      break;
    case LIGHT_DEMO:
      setState(READY);
      lightDemo();
      break;
    case START:
      setState(SELECT_PLAYERS);
      scrollText(" How many players?", 0, 10);
      scrollText(" How many players? Push the button to select.");
      break;
    case SELECT_PLAYERS:
      setState(WAIT_SELECTION);
      events.add({ mkCb([]() {
                     cancellableTimer([]() {
                       if (numPlayers > 0) {
                         setState(PLAYERS_SELECTED);
                       } else {
                         setState(NO_PLAYERS);
                       }
                     });
                   }),
                   10 });
      break;
    case PLAYERS_SELECTED:
      setState(CONFIRM_PLAYERS);
      events.add({ mkCb([]() {
                     cancellableTimer([]() {
                       if (currentState != NEXT_PLAYER) {
                         setState(START);
                       }
                     });
                   }),
                   10 });
      break;
    case NO_PLAYERS:
      setState(START);
      scrollText("No players selected");
      break;
    case CONFIRM_PLAYERS:
      setState(PLAYERS_CONFIRMED);
      String confirm = " Confirm ";
      confirm.concat(numPlayers);
      confirm.concat(" players?");
      scrollText(confirm, 0, 10);
      scrollText("Push the button.", 1);
      break;
    case PLAYERS_CONFIRMED:
      // do nothing, button required
      break;
    case NEXT_PLAYER:
      setState(THROW_DICE);
      scrollText("Player n. 1 ready!");
      break;
    default:
      if (verbose) {
        Serial.println("nextState: default");
        delay(500);
      }
      break;
  }
}

// state machine implementation
void buttonState() {
  if (buttonReleased) {
    buttonReleased = false;
    if (verbose) {
      Serial.print("Button released - State: ");
      Serial.print(stateName(currentState));
      Serial.print(", num players: ");
      Serial.println(numPlayers);
    }
    switch (currentState) {
      case READY:
        setState(START);
        break;
      case WAIT_SELECTION:
        cancelTimer();
        setState(SELECT_PLAYERS);
        numPlayers++;
        lcd.setCursor(0, 1);
        lcd.print(numPlayers);
        break;
      case PLAYERS_CONFIRMED:
        setState(NEXT_PLAYER);
        break;
      default:
        if (verbose) {
          Serial.print("Button pressed no action for state: ");
          Serial.println(currentState);
        }
        break;
    }
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

void loop() {
  // hadle timer
  t.handle();
  // proceed to next State
  nextState();
  // handle button
  buttonState();
}
