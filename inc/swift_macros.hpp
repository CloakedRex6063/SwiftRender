#pragma once

#define SWIFT_CONSTRUCT(T) T() = default;
#define SWIFT_NO_CONSTRUCT(T) T() = delete;
#define SWIFT_DESTRUCT(T) virtual ~T() = default;

#define SWIFT_NO_COPY(T)  \
    T(const T&) = delete; \
    T& operator=(const T&) = delete;

#define SWIFT_NO_MOVE(T) \
    T(T&&) = delete;     \
    T& operator=(T&&) = delete;
