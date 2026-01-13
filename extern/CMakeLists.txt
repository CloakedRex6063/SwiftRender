CPMAddPackage(
        NAME DirectX
        GITHUB_REPOSITORY microsoft/DirectX-Headers
        GIT_TAG v1.618.2
)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectX-Headers)
target_link_libraries(${PROJECT_NAME} PUBLIC "dxgi.lib" "d3d12.lib" "Ole32.lib")