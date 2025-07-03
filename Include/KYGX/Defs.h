#ifndef _KYGX_DEFS_H
#define _KYGX_DEFS_H

#ifdef KYGX_BAREMETAL
#include <arm.h> // ldrex, strex et al
#include <stdbool.h>
#define KYGX_EXMON_CAST(v) (u32*)(v)
#else
#include <3ds.h>
#define KYGX_EXMON_CAST(v) (s32*)(v)
#endif // KYGX_BAREMETAL

#define KYGX_PACKED __attribute__((packed))
#define KYGX_LIKELY(x) (bool)__builtin_expect((bool)(x), true)

#ifdef __cplusplus
#define KYGX_INLINE inline
#else
#define KYGX_INLINE static inline
#endif // __cplusplus

#define KYGX_AS_STRING(x) #x
#define KYGX_STRINGIFY(x) KYGX_AS_STRING(x)

#define KYGX_YIELD() kygx_platform_yield()

#ifndef NDEBUG
#define KYGX_BREAK(reason) kygx_platform_break((reason))

#define KYGX_ASSERT(cond)                                        \
    do {                                                         \
        if (!KYGX_LIKELY(cond)) {                                \
            KYGX_BREAK("Assertion failed: " KYGX_STRINGIFY(cond) \
                        "\nIn file: " KYGX_STRINGIFY(__FILE__)   \
                        "\nOn line: " KYGX_STRINGIFY(__LINE__)); \
        }                                                        \
    } while (false)
#else
#define KYGX_BREAK(reason) kygx_platform_break(NULL)
#define KYGX_ASSERT(cond) (void)((cond))
#endif // !NDEBUG

#define KYGX_BREAK_UNLESS(cond)           \
    if (!KYGX_LIKELY(cond)) {             \
        KYGX_BREAK(KYGX_STRINGIFY(cond)); \
    }

#define KYGX_UNREACHABLE(s)                                  \
    do {                                                     \
        KYGX_BREAK("Unreachable point reached: " s           \
                    "\nIn file: " KYGX_STRINGIFY(__FILE__)   \
                    "\nOn line: " KYGX_STRINGIFY(__LINE__)); \
    } while (false)

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

void kygx_platform_yield(void);
__attribute__((noreturn, cold)) void kygx_platform_break(const char*);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* _KYGX_DEFS_H */