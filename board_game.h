#ifndef BOARD_GAME_H
#define BOARD_GAME_H

// we use this implementation, install it in your Arduino IDE
// https://github.com/Francis-Magallanes/CircularQueue
#include <Queue.h>

// debug
const bool verbose = false;

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

// event queue pointer (must be preallocated with enough storage)
using EventQueue = Queue<Event, 35>;
// utility queue to manage generic events
EventQueue events;

// represents a led array (basically a strip of leds connected to pins)
struct LedArray {
  LedArray(const int* _pins, int _size)
    : pins(_pins), size(_size){};
  const int* pins;
  int size;
  EventQueue ledEvents;
};

// state "machine". It is actually implemented in the methods nextState() and buttonState() 
enum State {
  INTRO,
  LIGHT_DEMO,
  READY,
  START,
  SELECT_PLAYERS,
  WAIT_SELECTION,
  NO_PLAYERS,
  PLAYERS_SELECTED,
  CONFIRM_PLAYERS,
  PLAYERS_CONFIRMED,
  NEXT_PLAYER,
  THROW_DICE
};
State currentState = INTRO;

const char* stateName(State s) {
  switch (s) {
    case INTRO: return "INTRO";
    case LIGHT_DEMO: return "LIGHT_DEMO";
    case READY: return "READY";
    case START: return "START";
    case SELECT_PLAYERS: return "SELECT_PLAYERS";
    case WAIT_SELECTION: return "WAIT_SELECTION";
    case NO_PLAYERS: return "NO_PLAYERS";
    case PLAYERS_SELECTED: return "PLAYERS_SELECTED";
    case CONFIRM_PLAYERS: return "CONFIRM_PLAYERS";
    case PLAYERS_CONFIRMED: return "PLAYERS_CONFIRMED";
    case NEXT_PLAYER: return "NEXT_PLAYER";
    case THROW_DICE: return "THROW_DICE";
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

#endif  // BOARD_GAME_H
