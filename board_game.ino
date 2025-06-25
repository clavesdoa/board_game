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

char buffer[17];
void makeRow(const String& msg) {
  snprintf(buffer, 17, "%-16s", msg.c_str());
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
                   lcd.setCursor(0, row);
                   makeRow(message);
                   lcd.print(buffer);
                 }),
                 10 });
  } else {
    for (int idx = 0; idx <= segmentSize; idx++) {
      // schedule the print of shifted substrings of the message to be displayed one after another
      events.add({ mkCb([message, row, idx]() {
                     String segment = message.substring(idx, idx + displayWidth);
                     lcd.setCursor(0, row);
                     makeRow(segment);
                     lcd.print(buffer);
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
    case IDLE:
      // Idle state. Used to park quietly while waiting for a timer or button event
      break;
    case INTRO:
      setState(LIGHT_DEMO);
      runIntro();
      break;
    case LIGHT_DEMO:
      setState(READY);
      lightDemo();
      break;
    case START:
      setState(IDLE);
      scrollText("How many players?");
      asyncDelay(events, 1000);
      scrollText("Push the button to select.", 1);
      // Adding the change to next state to the event queue, since we must wait to change state after the above text is displayed.
      // This is necessary because of the cancellable timeout set in the next step, which cannot be started from a timeout itself
      events.add({ mkCb([]() {
                     setState(SELECT_PLAYERS);
                   }),
                   10 });
      break;
    case SELECT_PLAYERS:
      setState(WAIT_SELECTION);
      cancellableTimer([]() {
        if (numPlayers > 0) {
          setState(CONFIRM_PLAYERS);
        } else {
          setState(NO_PLAYERS);
        }
      });
      break;
    case NO_PLAYERS:
      setState(IDLE);
      scrollText("No players");
      scrollText("selected.", 1);
      asyncDelay(events, 3000);
      events.add({ mkCb([]() {
                     setState(START);
                   }),
                   10 });
      break;
    case CONFIRM_PLAYERS:
      setState(IDLE);
      String confirm = " Confirm ";
      confirm.concat(numPlayers);
      confirm.concat(" players?");
      scrollText(confirm, 0, 10);
      scrollText("Push the button.", 1);
      events.add({ mkCb([]() {
                     setState(PLAYERS_CONFIRMED);
                   }),
                   10 });
      cancellableTimer([]() {
        // if not confirmed before 5 seconds, start all over again
        numPlayers = 0;
        setState(START);
      });
      break;
    case PLAYERS_CONFIRMED:
      // do nothing, button required
      break;
    case NEXT_PLAYER:
      setState(THROW_DICE);
      scrollText("Player n. 1 ready!");
      break;
    case THROW_DICE:
      // do nothing for now
      break;
    default:
      if (verbose) {
        Serial.print("nextState - no action for state: ");
        Serial.println(stateName(currentState));
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
        scrollText(String(numPlayers), 1);
        break;
      case PLAYERS_CONFIRMED:
        cancelTimer();
        setState(NEXT_PLAYER);
        break;
      default:
        if (verbose) {
          Serial.print("Button pressed no action for state: ");
          Serial.println(stateName(currentState));
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
