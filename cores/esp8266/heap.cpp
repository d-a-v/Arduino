/* heap.c - overrides of SDK heap handling functions
 * Copyright (c) 2016 Ivan Grokhotkov. All rights reserved.
 * This file is distributed under MIT license.
 */

#include <stdlib.h>
#include "umm_malloc/umm_malloc.h"
extern "C" size_t umm_umul_sat(const size_t a, const size_t b);

// z2EapFree: See wpa2_eap_patch.cpp for details
extern "C" void z2EapFree(void *ptr, const char* file, int line) __attribute__((weak, alias("vPortFree"), nothrow));
// I don't understand all the compiler noise around this alias.
// Adding "__attribute__ ((nothrow))" seems to resolve the issue.
// This may be relevant: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=81824 

// Need FORCE_ALWAYS_INLINE to put HeapSelect class constructor/deconstructor in IRAM
#define FORCE_ALWAYS_INLINE_HEAP_SELECT
#include "umm_malloc/umm_heap_select.h"

#include <c_types.h>
#include <sys/reent.h>
#include <user_interface.h>

#define __STRHELPER(x) #x
#define STR(x) __STRHELPER(x) // stringifier

extern "C" {

#if defined(UMM_POISON_CHECK) || defined(UMM_POISON_CHECK_LITE)
#define UMM_MALLOC(s)           umm_poison_malloc(s)
#define UMM_CALLOC(n,s)         umm_poison_calloc(n,s)
#define UMM_REALLOC_FL(p,s,f,l) umm_poison_realloc_fl(p,s,f,l)
#define UMM_FREE_FL(p,f,l)      umm_poison_free_fl(p,f,l)
#define STATIC_ALWAYS_INLINE

#undef realloc
#undef free

#elif defined(DEBUG_ESP_OOM) || defined(UMM_INTEGRITY_CHECK)
#define UMM_MALLOC(s)           umm_malloc(s)
#define UMM_CALLOC(n,s)         umm_calloc(n,s)
#define UMM_REALLOC_FL(p,s,f,l) umm_realloc(p,s)
#define UMM_FREE_FL(p,f,l)      umm_free(p)
#define STATIC_ALWAYS_INLINE

#undef realloc
#undef free

#else  // ! UMM_POISON_CHECK && ! DEBUG_ESP_OOM
#define UMM_MALLOC(s)           malloc(s)
#define UMM_CALLOC(n,s)         calloc(n,s)
#define UMM_REALLOC_FL(p,s,f,l) realloc(p,s)
#define UMM_FREE_FL(p,f,l)      free(p)

// STATIC_ALWAYS_INLINE only applies to the non-debug build path,
// it must not be enabled on the debug build path.
#define STATIC_ALWAYS_INLINE static ALWAYS_INLINE
#endif


#if defined(UMM_POISON_CHECK)
  #define POISON_CHECK__ABORT() \
      do { \
          if ( ! POISON_CHECK() ) \
              abort(); \
      } while(0)

  #define POISON_CHECK__PANIC_FL(file, line) \
      do { \
          if ( ! POISON_CHECK() ) \
              __panic_func(file, line, ""); \
      } while(0)

#else // No full heap poison checking.
  #define POISON_CHECK__ABORT() do {} while(0)
  #define POISON_CHECK__PANIC_FL(file, line) do { (void)file; (void)line; } while(0)
#endif

// Debugging helper, last allocation which returned NULL
void *umm_last_fail_alloc_addr = NULL;
int umm_last_fail_alloc_size = 0;
#if defined(DEBUG_ESP_OOM)
const char *umm_last_fail_alloc_file = NULL;
int umm_last_fail_alloc_line = 0;
#endif

#ifdef UMM_INTEGRITY_CHECK
#define INTEGRITY_CHECK__ABORT() \
    do { \
        if ( ! INTEGRITY_CHECK() ) \
            abort(); \
    } while(0)

#define INTEGRITY_CHECK__PANIC_FL(file, line) \
    do { \
        if ( ! INTEGRITY_CHECK() ) \
            __panic_func(file, line, ""); \
    } while(0)

#else  // ! UMM_INTEGRITY_CHECK
#define INTEGRITY_CHECK__ABORT() do {} while(0)
#define INTEGRITY_CHECK__PANIC_FL(file, line) do { (void)file; (void)line; } while(0)

#endif //   UMM_INTEGRITY_CHECK

#if defined(DEBUG_ESP_OOM)
#define PTR_CHECK__LOG_LAST_FAIL_FL(p, s, f, l) \
    if(0 != (s) && 0 == p)\
    {\
      umm_last_fail_alloc_addr = __builtin_return_address(0);\
      umm_last_fail_alloc_size = s;\
      umm_last_fail_alloc_file = f;\
      umm_last_fail_alloc_line = l;\
    }
#define PTR_CHECK__LOG_LAST_FAIL(p, s) \
    if(0 != (s) && 0 == p)\
    {\
      umm_last_fail_alloc_addr = __builtin_return_address(0);\
      umm_last_fail_alloc_size = s;\
      umm_last_fail_alloc_file = NULL;\
      umm_last_fail_alloc_line = 0;\
    }
#else
#define PTR_CHECK__LOG_LAST_FAIL_FL(p, s, f, l) \
    (void)f;\
    (void)l;\
    if(0 != (s) && 0 == p)\
    {\
      umm_last_fail_alloc_addr = __builtin_return_address(0);\
      umm_last_fail_alloc_size = s;\
    }
#define PTR_CHECK__LOG_LAST_FAIL(p, s) \
    if(0 != (s) && 0 == p)\
    {\
      umm_last_fail_alloc_addr = __builtin_return_address(0);\
      umm_last_fail_alloc_size = s;\
    }
#endif

void* _malloc_r(struct _reent* unused, size_t size)
{
    (void) unused;
    void *ret = malloc(size);
    PTR_CHECK__LOG_LAST_FAIL(ret, size);
    return ret;
}

void _free_r(struct _reent* unused, void* ptr)
{
    (void) unused;
    free(ptr);
}

void* _realloc_r(struct _reent* unused, void* ptr, size_t size)
{
    (void) unused;
    void *ret = realloc(ptr, size);
    PTR_CHECK__LOG_LAST_FAIL(ret, size);
    return ret;
}

void* _calloc_r(struct _reent* unused, size_t count, size_t size)
{
    (void) unused;
    void *ret = calloc(count, size);
    PTR_CHECK__LOG_LAST_FAIL(ret, umm_umul_sat(count, size));
    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG_ESP_OOM
#undef malloc
#undef calloc
#undef realloc

#define DEBUG_HEAP_PRINTF ets_uart_printf

struct malloc_stat_info_s
{
    const void* allocer;
};

#define MALLOC_STAT_INC sizeof(malloc_stat_info_s)

#define MALLOC_STAT_SIZE 64

struct malloc_stat_s
{
    unsigned epoch { 0 };
    unsigned count_alloc { 0 };
    unsigned count_free { 0 };
    const void* allocer { nullptr };
    unsigned sum { 0 };
    const char* file { nullptr };
    int line { -1 };
};

static malloc_stat_s malloc_stat_list [MALLOC_STAT_SIZE];
static malloc_stat_s* malloc_stat_last = nullptr;
static unsigned malloc_stat_epoch = 0;
static unsigned malloc_stat_overflow = 0;
static unsigned malloc_lost_free = 0;

void IRAM_ATTR print_file_line(const char* file, int line);

void IRAM_ATTR malloc_stat_print (const char* head, malloc_stat_s* f, int size)
{
    DEBUG_HEAP_PRINTF("%salloc-stat: epoch=%u (alloc@%p(%u) - free(%u) =) %u +sum=%u ", head, f->epoch, f->allocer, f->count_alloc, f->count_free, f->count_alloc - f->count_free, f->sum);
    if (size > 0)
        DEBUG_HEAP_PRINTF("sz=%d ", size);
    if (f->file)
        print_file_line(f->file, f->line);
    DEBUG_HEAP_PRINTF("\n");
}

void IRAM_ATTR malloc_stat_print_all ()
{
    DEBUG_HEAP_PRINTF("/======================\n");
    for (malloc_stat_s* f = malloc_stat_list; f <= malloc_stat_last; f++)
        malloc_stat_print("| ", f, -1);
    DEBUG_HEAP_PRINTF("`====================== (overflow=%u - lost-free=%u)\n", malloc_stat_overflow, malloc_lost_free);
}

void IRAM_ATTR malloc_stat (bool allocing, const void* caller, const void* ptr, const size_t sz, const char* m, const int l)
{
    if (!ptr || (allocing && (!caller || !sz)))
        return;

    const void* allocer;

    if (allocing)
    {
        allocer = caller;
        ((malloc_stat_info_s*)ptr)->allocer = caller;
    }
    else
    {
        allocer = ((malloc_stat_info_s*)ptr)->allocer;
    }

    malloc_stat_epoch++;
    bool done = false;
    for (malloc_stat_s* f = malloc_stat_list; f <= malloc_stat_last; f++)
    {
        bool found = (allocer == f->allocer);

        if (found)
        {
            f->epoch = malloc_stat_epoch;

            if (allocing)
            {
                f->count_alloc++;
                f->file = m;
                f->line = l;
                f->sum += sz;

                // bubble sort
                while (f != malloc_stat_list)
                {
                    auto prev = f;
                    prev--;
                    if (f->count_alloc - f->count_free > prev->count_alloc - prev->count_free)
                    {
                        malloc_stat_s temp = *f;
                        *f = *prev;
                        *prev = temp;
                        f = prev;
                    }
                    else
                        break;
                }
            }
            else
            {
                f->count_free++;
                if (f->count_free == f->count_alloc)
                {
                    // remove allocer entry
                    // (always true:) if (malloc_stat_last)
                    {
                        if (malloc_stat_last != f)
                            *f = *malloc_stat_last;
                        if (malloc_stat_last == malloc_stat_list)
                            malloc_stat_last = nullptr;
                        else
                            malloc_stat_last--;
                    }
                }
                else
                {
                    // bubble sort
                    while (f != malloc_stat_last)
                    {
                        auto next = f;
                        next++;
                        if (f->count_alloc - f->count_free < next->count_alloc - next->count_free)
                        {
                            malloc_stat_s temp = *f;
                            *f = *next;
                            *next = temp;
                            f = next;
                        }
                        else
                            break;
                    }
                }
            }

            done = true;
            break;
        }
    }

    if (!done)
    {
        // entry is not found, inserting

        if (allocing)
        {
            if (/*!malloc_stat_last ||*/ malloc_stat_last < malloc_stat_list + MALLOC_STAT_SIZE - 1)
            {
                // new allocer address
                if (malloc_stat_last)
                    malloc_stat_last++;
                else
                    malloc_stat_last = malloc_stat_list;
                *malloc_stat_last =
                {
                    .epoch = malloc_stat_epoch,
                    .count_alloc = 1,
                    .count_free = 0,
                    .allocer = allocer,
                    .sum = sz,
                    .file = m,
                    .line = l
                };
            }
            else
            {
                // would replace an older entry instead, but which one ?
                DEBUG_HEAP_PRINTF("(malloc stats overload " STR(MALLOC_STAT_SIZE) " - allocer=%p)\n", allocer);
                malloc_stat_overflow++;
            }
        }
        else
        {
            static unsigned tooverbose = 0;
            if (++tooverbose < 10)
                DEBUG_HEAP_PRINTF("orphaned free? allocer=%p addr=%p\n", allocer, ptr);
            malloc_lost_free++;
        }
    }
}

void IRAM_ATTR print_file_line(const char* file, int line)
{
    (void)line;
    if (system_get_os_print()) {
        bool inISR = ETS_INTR_WITHINISR();
        if (NULL == file || (inISR && (uint32_t)file >= 0x40200000)) {
            DEBUG_HEAP_PRINTF("File: %p", file);
        } else if (!inISR && (uint32_t)file >= 0x40200000) {
            char buf[strlen_P(file) + 1];
            strcpy_P(buf, file);
            DEBUG_HEAP_PRINTF(buf);
        } else {
            DEBUG_HEAP_PRINTF(file);
        }

        DEBUG_HEAP_PRINTF(":%d", line);
    }
}

void IRAM_ATTR print_loc(size_t size, const char* file, int line)
{
    (void)size;
    if (system_get_os_print()) {
        DEBUG_HEAP_PRINTF(":oom(%d)@", (int)size);
        print_file_line(file, line);
        DEBUG_HEAP_PRINTF("\n");
        malloc_stat_print_all();
    }
}

void IRAM_ATTR print_oom_size(size_t size)
{
    (void)size;
    if (system_get_os_print()) {
        DEBUG_HEAP_PRINTF(":oom(%d)@?\n", (int)size);
        malloc_stat_print_all();
    }
}

#define OOM_CHECK__PRINT_OOM(p, s) if ((s) && !(p)) print_oom_size(s)
#define OOM_CHECK__PRINT_LOC(p, s, f, l) if ((s) && !(p)) print_loc(s, f, l)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else  // ! DEBUG_ESP_OOM

#if 1
//C - to be discussed - is this what you want?
//C Skip OOM logging of last fail for malloc/... and pvPort... .
//C It cost 64 more bytes of IRAM to turn on. And was not previously enabled.
#undef PTR_CHECK__LOG_LAST_FAIL_FL
#define PTR_CHECK__LOG_LAST_FAIL_FL(p, s, f, l)
#undef PTR_CHECK__LOG_LAST_FAIL
#define PTR_CHECK__LOG_LAST_FAIL(p, s)
#endif

#define OOM_CHECK__PRINT_OOM(p, s)
#define OOM_CHECK__PRINT_LOC(p, s, f, l)
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define VOID_PLUS(p, i) ((void*)(((long)(p)) + i))

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(DEBUG_ESP_OOM) || defined(UMM_POISON_CHECK) || defined(UMM_POISON_CHECK_LITE) || defined(UMM_INTEGRITY_CHECK)
/*
  The thinking behind the ordering of Integrity Check, Full Poison Check, and
  the specific *alloc function.

  1. Integrity Check - verifies the heap management information is not corrupt.
     This allows any other testing, that walks the heap, to run safely.

  2. Place Full Poison Check before or after a specific *alloc function?
     a. After, when the *alloc function operates on an existing allocation.
     b. Before, when the *alloc function creates a new, not modified, allocation.

     In a free() or realloc() call, the focus is on their allocation. It is
     checked 1st and reported on 1ST if an error exists. Full Poison Check is
     done after.

     For malloc(), calloc(), and zalloc() Full Poison Check is done 1st since
     these functions do not modify an existing allocation.
*/
void* IRAM_ATTR malloc(size_t size)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__ABORT();
    POISON_CHECK__ABORT();
    void* ret = UMM_MALLOC(size);
    PTR_CHECK__LOG_LAST_FAIL(ret, size);
    OOM_CHECK__PRINT_OOM(ret, size);
    malloc_stat(true, __builtin_return_address(0), ret, size, nullptr, 0);
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

void* IRAM_ATTR calloc(size_t count, size_t size)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__ABORT();
    POISON_CHECK__ABORT();
    void* ret = UMM_CALLOC(count, size);
    #if defined(DEBUG_ESP_OOM)
    size_t total_size = umm_umul_sat(count, size);// For logging purposes
    #endif
    PTR_CHECK__LOG_LAST_FAIL(ret, total_size);
    OOM_CHECK__PRINT_OOM(ret, total_size);
    malloc_stat(true, __builtin_return_address(0), ret, size, nullptr, 0);
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

void* IRAM_ATTR realloc(void* ptr, size_t size)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__ABORT();
    void* ret = UMM_REALLOC_FL(ptr? VOID_PLUS(ptr, -MALLOC_STAT_INC): nullptr, size, NULL, 0);
    POISON_CHECK__ABORT();
    PTR_CHECK__LOG_LAST_FAIL(ret, size);
    OOM_CHECK__PRINT_OOM(ret, size);
    if (ret)
    {
        malloc_stat(false, __builtin_return_address(0), ptr, 0, nullptr, 0);
        malloc_stat(true, __builtin_return_address(0), ret, size, nullptr, 0);
    }
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

void IRAM_ATTR free(void* p)
{
    INTEGRITY_CHECK__ABORT();
    malloc_stat(false, __builtin_return_address(0), VOID_PLUS(p, -MALLOC_STAT_INC), 0, nullptr, 0);
    UMM_FREE_FL(VOID_PLUS(p, -MALLOC_STAT_INC), NULL, 0);
    POISON_CHECK__ABORT();
}
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC_ALWAYS_INLINE
void* IRAM_ATTR heap_pvPortMalloc(size_t size, const char* file, int line, void* caller)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__PANIC_FL(file, line);
    POISON_CHECK__PANIC_FL(file, line);
    void* ret = UMM_MALLOC(size);
    PTR_CHECK__LOG_LAST_FAIL_FL(ret, size, file, line);
    OOM_CHECK__PRINT_LOC(ret, size, file, line);
    malloc_stat(true, caller?:__builtin_return_address(0), ret, size, file, line);
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

STATIC_ALWAYS_INLINE
void* IRAM_ATTR heap_pvPortCalloc(size_t count, size_t size, const char* file, int line, void* caller)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__PANIC_FL(file, line);
    POISON_CHECK__PANIC_FL(file, line);
    void* ret = UMM_CALLOC(count, size);
    #if defined(DEBUG_ESP_OOM)
    size_t total_size = umm_umul_sat(count, size);
    #endif
    PTR_CHECK__LOG_LAST_FAIL_FL(ret, total_size, file, line);
    OOM_CHECK__PRINT_LOC(ret, total_size, file, line);
    malloc_stat(true, caller?:__builtin_return_address(0), ret, size, file, line);
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

STATIC_ALWAYS_INLINE
void* IRAM_ATTR heap_pvPortRealloc(void *ptr, size_t size, const char* file, int line, void* caller)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__PANIC_FL(file, line);
    void* ret = UMM_REALLOC_FL(ptr? VOID_PLUS(ptr, -MALLOC_STAT_INC): nullptr, size, file, line);
    POISON_CHECK__PANIC_FL(file, line);
    PTR_CHECK__LOG_LAST_FAIL_FL(ret, size, file, line);
    OOM_CHECK__PRINT_LOC(ret, size, file, line);
    if (ret)
    {
        malloc_stat(false, caller?:__builtin_return_address(0), ptr, 0, file, line);
        malloc_stat(true, caller?:__builtin_return_address(0), ret, size, file, line);
    }
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

STATIC_ALWAYS_INLINE
void* IRAM_ATTR heap_pvPortZalloc(size_t size, const char* file, int line, void* caller)
{
    size += MALLOC_STAT_INC;
    INTEGRITY_CHECK__PANIC_FL(file, line);
    POISON_CHECK__PANIC_FL(file, line);
    void* ret = UMM_CALLOC(1, size);
    PTR_CHECK__LOG_LAST_FAIL_FL(ret, size, file, line);
    OOM_CHECK__PRINT_LOC(ret, size, file, line);
    malloc_stat(true, caller?:__builtin_return_address(0), ret, size, file, line);
    return ret? VOID_PLUS(ret, MALLOC_STAT_INC): nullptr;
}

STATIC_ALWAYS_INLINE
void IRAM_ATTR heap_vPortFree(void *ptr, const char* file, int line, void* caller)
{
    INTEGRITY_CHECK__PANIC_FL(file, line);
    malloc_stat(false, caller?:__builtin_return_address(0), VOID_PLUS(ptr, -MALLOC_STAT_INC), 0, file, line);
    UMM_FREE_FL(VOID_PLUS(ptr, -MALLOC_STAT_INC), file, line);
    POISON_CHECK__PANIC_FL(file, line);
}

size_t IRAM_ATTR xPortWantedSizeAlign(size_t size)
{
    return (size + 3) & ~((size_t) 3);
}

void system_show_malloc(void)
{
    HeapSelectDram ephemeral;
    umm_info(NULL, true);
}

/*
  NONOS SDK and lwIP do not handle IRAM heap well. Since they also use portable
  malloc calls pvPortMalloc, ... we can leverage that for this solution.
  Force pvPortMalloc, ... APIs to serve DRAM only.
*/
void* IRAM_ATTR pvPortMalloc(size_t size, const char* file, int line)
{
    HeapSelectDram ephemeral;
    return heap_pvPortMalloc(size,  file, line, __builtin_return_address(0));
}

void* IRAM_ATTR pvPortCalloc(size_t count, size_t size, const char* file, int line)
{
    HeapSelectDram ephemeral;
    return heap_pvPortCalloc(count, size,  file, line, __builtin_return_address(0));
}

void* IRAM_ATTR pvPortRealloc(void *ptr, size_t size, const char* file, int line)
{
    HeapSelectDram ephemeral;
    return heap_pvPortRealloc(ptr, size,  file, line, __builtin_return_address(0));
}

void* IRAM_ATTR pvPortZalloc(size_t size, const char* file, int line)
{
    HeapSelectDram ephemeral;
    return heap_pvPortZalloc(size,  file, line, __builtin_return_address(0));
}

void IRAM_ATTR vPortFree(void *ptr, const char* file, int line)
{
#if defined(DEBUG_ESP_OOM) || defined(UMM_POISON_CHECK) || defined(UMM_POISON_CHECK_LITE) || defined(UMM_INTEGRITY_CHECK)
    // This is only needed for debug checks to ensure they are performed in
    // correct context. umm_malloc free internally determines the correct heap.
    HeapSelectDram ephemeral;
#endif
    return heap_vPortFree(ptr,  file, line, __builtin_return_address(0));
}

};
