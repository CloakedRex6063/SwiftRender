#pragma once
#include "swift_macros.hpp"

namespace Swift
{
    class ISampler
    {
    public:
        SWIFT_DESTRUCT(ISampler);
        SWIFT_NO_MOVE(ISampler);
        SWIFT_NO_COPY(ISampler);
        [[nodiscard]] virtual uint32_t GetDescriptorIndex() const = 0;

    protected:
        SWIFT_CONSTRUCT(ISampler);
    };
}