#ifndef ESP_SCHEDULE_H
#define ESP_SCHEDULE_H

#include <functional>

#define SCHEDULED_FN_MAX_COUNT 32
#define SCHEDULED_FN_INITIAL_COUNT 4

// This API was not considered stable but is now fossilizing.
// Function signatures may change, queue must stay FIFO.
// You have been warned.

// Run given function ONCE next time `loop` function returns,
// or `yield` is called,
// or `run_scheduled_functions` is called.
// Use std::bind to pass arguments to a function, or call a class member function.
// Note: there is no mechanism for cancelling scheduled functions.
// Keep that in mind when binding functions to objects which may have short lifetime.
// Returns false if the number of scheduled functions exceeds SCHEDULED_FN_MAX_COUNT.
//bool schedule_function(std::function<void(void)>&& fn);
bool schedule_function(std::function<void(void)> fn);

// Run given function every at least <repeat_us> microseconds until it returns false
//bool schedule_function_us(std::function<bool(void)>&& fn, uint32_t repeat_us);
bool schedule_function_us(std::function<bool(void)> fn, uint32_t repeat_us);

// Run all scheduled functions.
// Use this function if your are not using `loop`, or `loop` does not return
// on a regular basis.
void run_scheduled_functions();

#endif //ESP_SCHEDULE_H
