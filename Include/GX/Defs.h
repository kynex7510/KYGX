#ifndef _CTRGX_DEFS_H
#define _CTRGX_DEFS_H

#ifdef CTRGX_BAREMETAL
#include <arm.h> // ldrex, strex et al
#include <stdbool.h>
#define CTRGX_EXMON_CAST(v) (u32*)(v)
#else
#include <3ds.h>
#define CTRGX_EXMON_CAST(v) (s32*)(v)
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

#define CTRGX_BREAK(reason) ctrgx_platform_break((reason), sizeof(reason) - 1)

#define CTRGX_BREAK_UNLESS(cond)            \
    if (!CTRGX_LIKELY(cond)) {              \
        CTRGX_BREAK(CTRGX_STRINGIFY(cond)); \
    }

#define CTRGX_UNREACHABLE(s)                                  \
    do {                                                      \
        CTRGX_BREAK("Unreachable point reached: " s           \
                    "\nIn file: " CTRGX_STRINGIFY(__FILE__)   \
                    "\nOn line: " CTRGX_STRINGIFY(__LINE__)); \
    } while (false)

#ifndef NDEBUG

#define CTRGX_ASSERT(cond)                                         \
    do {                                                           \
        if (!CTRGX_LIKELY(cond)) {                                 \
            CTRGX_BREAK("Assertion failed: " CTRGX_STRINGIFY(cond) \
                        "\nIn file: " CTRGX_STRINGIFY(__FILE__)    \
                        "\nOn line: " CTRGX_STRINGIFY(__LINE__));  \
        }                                                          \
    } while (false)

#else
#define CTRGX_ASSERT(cond)
#endif // !NDEBUG

CTRGX_EXTERN __attribute__((noreturn, cold)) void ctrgx_platform_break(const char*, size_t);

#endif /* _CTRGX_DEFS_H */