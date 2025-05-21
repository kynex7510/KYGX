#ifndef _CTRGX_DEFS_H
#define _CTRGX_DEFS_H

#ifdef CTRGX_BAREMETAL
// TODO
#else
#include <3ds.h>
#endif // CTRGX_BAREMETAL

#define CTRGX_PACKED __attribute__((packed))
#define CTRGX_LIKELY(x) (bool)__builtin_expect((bool)(x), true)

#ifdef __cplusplus
#define CTRGX_EXTERN extern "C"
#define CTRGX_INLINE inline
#else
#define CTRGX_EXTERN
#define CTRGX_INLINE static inline
#endif // __cplusplus

#define CTRGX_AS_STRING(x) #x
#define CTRGX_STRINGIFY(x) CTRGX_AS_STRING(x)

#define CTRGX_ABORT() ctrgx_platform_abort()
#define CTRGX_BREAK() ctrgx_platform_break()

#define CTRGX_BREAK_UNLESS(cond) \
    if (!CTRGX_LIKELY(cond)) {   \
        CTRGX_BREAK();           \
    }

#ifndef NDEBUG

#define CTRGX_LOG(s) ctrgx_platform_log((s), sizeof(s) - 1)

#define CTRGX_ASSERT(cond)                                         \
    do {                                                           \
        if (!CTRGX_LIKELY(cond)) {                                 \
            CTRGX_LOG("Assertion failed: " CTRGX_STRINGIFY(cond)); \
            CTRGX_LOG("In file: " CTRGX_STRINGIFY(__FILE__));      \
            CTRGX_LOG("On line: " CTRGX_STRINGIFY(__LINE__));      \
            CTRGX_ABORT();                                         \
        }                                                          \
    } while (false)

#else
#define CTRGX_LOG(s) (void)(s)
#define CTRGX_ASSERT(cond)
#endif // !NDEBUG

#define CTRGX_UNREACHABLE(s) \
    do {                     \
        CTRGX_LOG(s);        \
        CTRGX_ABORT();       \
    } while (false)

CTRGX_EXTERN __attribute__((noreturn)) void ctrgx_platform_break(void);
CTRGX_EXTERN __attribute__((noreturn)) void ctrgx_platform_abort(void);
CTRGX_EXTERN void ctrgx_platform_log(const char* s, size_t size);

#endif /* _CTRGX_DEFS_H */