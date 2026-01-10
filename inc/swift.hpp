#pragma once
#include "swift_context.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    std::shared_ptr<IContext> CreateContext(const ContextCreateInfo& create_info);
}
