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

bool ICACHE_RAM_ATTR schedule_function_us(std::function<bool(void)>&& fn, uint32_t repeat_us)
{
    scheduled_fn_t item;
    item.mFunc = std::move(fn);
    if (repeat_us) item.callNow.reset(repeat_us);
    return schedule_queue.push(std::move(item));
}

bool ICACHE_RAM_ATTR schedule_function_us(const std::function<bool(void)>& fn, uint32_t repeat_us)
{
    return schedule_function_us(std::function<bool(void)>(fn), repeat_us);
}

bool ICACHE_RAM_ATTR schedule_function(std::function<void(void)>&& fn)
{
    return schedule_function_us([fn = std::move(fn)]() { fn(); return false; }, 0);
}

bool ICACHE_RAM_ATTR schedule_function(const std::function<void(void)>& fn)
{
    return schedule_function(std::function<void(void)>(fn));
}

void run_scheduled_functions()
{
    schedule_queue.for_each_revenant([](const scheduled_fn_t& arg)
        {
            auto func = arg;
            return (!func.callNow || func.mFunc());
        });
}
