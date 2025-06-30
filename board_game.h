#ifndef BOARD_GAME_H
#define BOARD_GAME_H

// we use this implementation, install it in your Arduino IDE
// https://github.com/Aasim-A/AsyncTimer
#include <AsyncTimer.h>
// we use this implementation, install it in your Arduino IDE
// https://github.com/Francis-Magallanes/CircularQueue
#include <Queue.h>

// debug
const bool verbose = true;

// state "machine". It is actually implemented in the methods nextState() and buttonState()
enum State {
  IDLE,
  INTRO,
  LIGHT_DEMO,
  READY,
  START,
  SELECT_PLAYERS,
  WAIT_SELECTION,
  NO_PLAYERS,
  CONFIRM_PLAYERS,
  WAIT_CONFIRMATION,
  PLAYERS_CONFIRMED,
  NEXT_PLAYER,
  THROW_DICE,
  PROGRESS
};
State currentState = INTRO;

const char* stateName(State s) {
  switch (s) {
    case IDLE: return "IDLE";
    case INTRO: return "INTRO";
    case LIGHT_DEMO: return "LIGHT_DEMO";
    case READY: return "READY";
    case START: return "START";
    case SELECT_PLAYERS: return "SELECT_PLAYERS";
    case WAIT_SELECTION: return "WAIT_SELECTION";
    case NO_PLAYERS: return "NO_PLAYERS";
    case CONFIRM_PLAYERS: return "CONFIRM_PLAYERS";
    case WAIT_CONFIRMATION: return "WAIT_CONFIRMATION";
    case PLAYERS_CONFIRMED: return "PLAYERS_CONFIRMED";
    case NEXT_PLAYER: return "NEXT_PLAYER";
    case THROW_DICE: return "THROW_DICE";
    case PROGRESS: return "PROGRESS";
    default: return "UNKNOWN";
  }
}

void setState(State newState) {
  if (verbose) {
    Serial.print("Transitioning from ");
    Serial.print(stateName(currentState));
    Serial.print(" to ");
    Serial.println(stateName(newState));
  }
  currentState = newState;
}

// To represents an event and being able to easily enqueue it using capturing lambdas, we must use a heap-allocated,
// object-oriented wrapper to a function pointer, since more modern approaches are not fully supported across Arduinos

// polymorphic callback interface
struct Callback {
  virtual void call() = 0;
  virtual ~Callback() {}
};

// template wrapper for all (including capturing) lambdas
template<typename Lambda>
struct LambdaWrapper : Callback {
  Lambda fn;
  LambdaWrapper(Lambda l)
    : fn(l) {}
  void call() override {
    fn();
  }
};

// factory to create the wrapper (on the heap)
template<typename Lambda>
Callback* mkCb(Lambda l) {
  return new LambdaWrapper<Lambda>(l);
}

// the Event further encapsulates the callback
struct Event {
  Callback* cb;
  unsigned long delay;
  // deallocate memory (must be called manually after calls are completed)
  void cleanUp() {
    delete cb;
  }
};
// event queue (must be preallocated with enough storage)
using EventQueue = Queue<Event, 100>;

// timer
AsyncTimer t;
// event scheduler
struct Scheduler {
  void add(Event event) {
    if (events.enqueue(event)) {
      schedule();
    } else {
      Serial.println("Add to queue failed");
    }
  }
private:
  // schedule function calls so that the events run in a chain
  void schedule() {
    if (events.isEmpty()) {
      running = false;
      return;
    }
    if (running) {
      return;
    }
    running = true;
    t.setTimeout([&]() {
      // call the lambda
      events.front().cb->call();
      // clean up the lambda
      events.front().cleanUp();
      // schedule next event after this completed
      if (events.dequeue()) {
        running = false;
        schedule();
      }
    },
                 events.front().delay);
  }
  // flag to ensure only one scheduler runs at a time
  bool running = false;
  // events queue
  EventQueue events;
};
// utility queue to manage generic events
Scheduler events;

// represents a led array (basically a strip of leds connected to pins)
struct LedArray {
  LedArray(const int* _pins, int _size)
    : pins(_pins), size(_size){};
  const int* pins;
  int size;
  Scheduler ledEvents;
};

// represents a player
const unsigned int MAX_PLAYERS = 10;
struct Player {
  unsigned int points = 0;
  unsigned int step = 0;
  unsigned int hpGold = 0;
};
// players
Player players[MAX_PLAYERS];

// cancellable timer. At the moment we can manage only one at a time (which is also all we need)
unsigned short toCancelId = 0;

void debugCancellableTimer(String context) {
  if (verbose) {
    Serial.print(context);
    Serial.print(" timer - State: ");
    Serial.print(stateName(currentState));
    Serial.print(" - timerId: ");
    Serial.println(toCancelId);
  }
}

template<typename Func>
void cancellableTimer(Func f) {
  toCancelId = t.setTimeout(f, 5000);
  debugCancellableTimer("Cancellable");
}

void cancelTimer() {
  t.cancel(toCancelId);
  debugCancellableTimer("Cancelled");
}

#endif  // BOARD_GAME_H
