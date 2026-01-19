#pragma once
#include "span"
#include "swift_command.hpp"
#include "swift_macros.hpp"

namespace Swift
{
    class ICommand;
    class IQueue
    {
    public:
        SWIFT_DESTRUCT(IQueue);
        SWIFT_NO_MOVE(IQueue);
        SWIFT_NO_COPY(IQueue);

        virtual void* GetQueue() = 0;
        virtual void Wait(uint64_t fence_value) = 0;
        virtual void WaitIdle() = 0;
        virtual uint64_t Execute(const std::span<const std::shared_ptr<ICommand>>& commands) = 0;

    protected:
        SWIFT_CONSTRUCT(IQueue);
    };
}  // namespace Swift
