#ifndef BOARD_GAME_H
#define BOARD_GAME_H

// we use this implementation, install it in your Arduino IDE
// https://github.com/Francis-Magallanes/CircularQueue
#include <Queue.h>

// To represents an event and being able to easily enqueue it using capturing lambdas, we must use a heap-allocated,
// object-oriented wrapper to a function pointer, since more modern approaches are not fully supported across Arduinos

// polymorphic callback interface
struct Callback {
  virtual void call() = 0;
  virtual ~Callback() {}
};

// template wrapper for all (including capturing) lambda
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
  // deallocate memory (must be called manually after all calls are completed)
  void cleanUp() {
    delete cb;
  }
};

// event queue pointer (must be preallocated with enough storage)
using EventQueue = Queue<Event, 35>;

// represents a led array (basically a strip of leds connected to pins)
struct LedArray {
  LedArray(const int* _pins, int _size)
    : pins(_pins), size(_size){};
  const int* pins;
  int size;
  EventQueue ledEvents;
};

#endif // BOARD_GAME_H