#include "swift.hpp"
#include "d3d12/d3d12_context.hpp"

Swift::IContext* Swift::CreateContext(const ContextCreateInfo& create_info)
{
    return new D3D12::Context(create_info);
}

void Swift::DestroyContext(const IContext* context)
{
    delete context;
}

