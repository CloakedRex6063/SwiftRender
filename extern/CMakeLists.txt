add_subdirectory(directx-headers)
target_link_libraries(${PROJECT_NAME} PUBLIC Microsoft::DirectX-Headers)

target_link_libraries(${PROJECT_NAME} PUBLIC "dxgi.lib" "d3d12.lib" "Ole32.lib")