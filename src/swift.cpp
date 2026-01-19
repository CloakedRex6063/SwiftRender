#include "swift.hpp"

#include "d3d12/d3d12_context.hpp"

std::shared_ptr<Swift::IContext> Swift::CreateContext(const ContextCreateInfo& create_info)
{
    switch (create_info.backend_type)
    {
#ifdef SWIFT_WINDOWS
        case BackendType::eD3D12:
            return std::make_shared<D3D12::Context>(create_info);
#endif
        case BackendType::eVulkan:
            return nullptr;
    }
    return nullptr;
}
