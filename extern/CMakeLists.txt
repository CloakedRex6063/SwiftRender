CPMAddPackage(
        NAME DirectX
        GITHUB_REPOSITORY microsoft/DirectX-Headers
        GIT_TAG v1.618.2
)
CPMAddPackage(
        NAME D3D12MA
        GITHUB_REPOSITORY GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator
        GIT_TAG v3.1.0
)
target_include_directories(D3D12MemoryAllocator BEFORE PRIVATE
        "${DirectX_SOURCE_DIR}/include"
        "${DirectX_SOURCE_DIR}/include/directx"
)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectX-Headers D3D12MemoryAllocator)
target_link_libraries(${PROJECT_NAME} PUBLIC "dxgi.lib" "d3d12.lib" "Ole32.lib" "dxguid.lib")