CPMAddPackage(
        NAME DirectX
        GITHUB_REPOSITORY microsoft/DirectX-Headers
        GIT_TAG v1.618.2
)
CPMAddPackage(
        NAME VulkanHeaders
        GITHUB_REPOSITORY KhronosGroup/Vulkan-Headers
        GIT_TAG v1.4.343
)
CPMAddPackage(
        NAME VkBootstrap
        GITHUB_REPOSITORY charles-lunarg/vk-bootstrap
        GIT_TAG v1.4.342
)
CPMAddPackage(
        NAME VMA
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
        GIT_TAG v3.3.0
)
CPMAddPackage(
        NAME Volk
        GITHUB_REPOSITORY zeux/volk
        GIT_TAG 1.4.304
)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectX-Headers Vulkan-Headers vk-bootstrap::vk-bootstrap GPUOpen::VulkanMemoryAllocator volk::volk)
target_link_libraries(${PROJECT_NAME} PUBLIC "dxgi.lib" "d3d12.lib" "Ole32.lib" "dxguid.lib")