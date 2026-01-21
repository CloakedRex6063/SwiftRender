#pragma once
#include "swift_context.hpp"
#include "swift_structs.hpp"

namespace Swift
{
    IContext* CreateContext(const ContextCreateInfo& create_info);
    void DestroyContext(const IContext* context);
}
