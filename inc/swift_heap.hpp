#pragma once
#include "swift_macros.hpp"
#include "swift_resource.hpp"
#include "memory"

namespace Swift
{
    class IHeap
    {
    public:
        SWIFT_DESTRUCT(IHeap);
        SWIFT_NO_COPY(IHeap);
        SWIFT_NO_MOVE(IHeap);

        virtual std::shared_ptr<IResource> CreateResource(const BufferCreateInfo& info, uint64_t offset) = 0;
        virtual std::shared_ptr<IResource> CreateResource(const TextureCreateInfo& info, uint64_t offset) = 0;

    protected:
        SWIFT_CONSTRUCT(IHeap);
    };
}