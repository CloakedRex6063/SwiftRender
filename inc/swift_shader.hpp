#pragma once
#include "swift_macros.hpp"

namespace Swift
{
    class IShader
    {
    public:
        SWIFT_DESTRUCT(IShader);
        SWIFT_NO_MOVE(IShader);
        SWIFT_NO_COPY(IShader);
        [[nodiscard]] virtual void* GetPipeline() const = 0;

    protected:
        SWIFT_CONSTRUCT(IShader);
    };
}
