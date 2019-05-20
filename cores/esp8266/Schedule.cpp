#include "Schedule.h"
#include "PolledTimeout.h"

#include "circular_queue.h"

typedef std::function<bool(void)> mFuncT;

struct scheduled_fn_t
{
    mFuncT mFunc;
    esp8266::polledTimeout::periodicFastUs callNow;

    scheduled_fn_t(): callNow(esp8266::polledTimeout::periodicFastUs::alwaysExpired) { }
};

static circular_queue_mp<scheduled_fn_t> schedule_queue(SCHEDULED_FN_MAX_COUNT);

ICACHE_RAM_ATTR // called from ISR
bool schedule_function_us(mFuncT fn, uint32_t repeat_us)
{
    scheduled_fn_t item;
    item.mFunc = fn;
    if (repeat_us) item.callNow.reset(repeat_us);
    return schedule_queue.push(item);
}

ICACHE_RAM_ATTR // called from ISR
bool schedule_function(std::function<void(void)> fn)
{
    return schedule_function_us([&fn]() { fn(); return false; }, 0);
}

void run_scheduled_functions()
{
    const auto avail = schedule_queue.available();
    for (size_t i = 0; i < avail; ++i) {
        auto item = schedule_queue.peek();
        if (item.callNow && !item.mFunc()) item = schedule_queue.pop();
        else item = schedule_queue.pop_revenant();
    }
}
