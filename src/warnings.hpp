#pragma once

#ifdef _MSC_VER
#define SWIFT_WARN_BEGIN() \
__pragma(warning(push, 0))
#elif __GNUC__
#define SWIFT_WARN_BEGIN() \
_Pragma("GCC diagnostic push") 
#else
#define SWIFT_WARN_BEGIN() \
_Pragma("clang diagnostic push") 
#endif

#ifdef _MSC_VER
#define SWIFT_WARN_DISABLE() \
_Pragma("clang diagnostic ignored \"-Weverything\"")
#elif __GNUC__
#define SWIFT_WARN_DISABLE() \
_Pragma("GCC diagnostic ignored \"-Wall\"") \
_Pragma("GCC diagnostic ignored \"-Wextra\"") \
_Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#else
#define SWIFT_WARN_DISABLE() \
_Pragma("clang diagnostic push") 
#endif

#ifdef _MSC_VER
#define SWIFT_WARN_END() \
__pragma(warning(pop))
#elif __GNUC__
#define SWIFT_WARN_END() \
_Pragma("GCC diagnostic pop") 
#else
#define SWIFT_WARN_END() \
_Pragma("clang diagnostic pop") 
#endif


