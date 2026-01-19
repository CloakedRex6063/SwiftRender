add_subdirectory(extern)

add_library(utility)
target_sources(utility
        PUBLIC
        utility/window.cpp
        utility/importer.cpp
        utility/shader_compiler.cpp
        utility/camera.cpp
        utility/input.cpp
        utility/imgui.cpp)
target_link_libraries(utility PUBLIC glfw Swift imgui imgui-dx12 imgui-glfw tinygltf mikktspace slang glm::glm meshoptimizer)
target_compile_definitions(utility PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_include_directories(utility PUBLIC utility extern/glfw/include)

include(cmake/functions.cmake)
add_example(hello_triangle ${CMAKE_CURRENT_SOURCE_DIR})
add_example(hello_model ${CMAKE_CURRENT_SOURCE_DIR})
add_example(hello_grass ${CMAKE_CURRENT_SOURCE_DIR})
add_example(hello_pbr ${CMAKE_CURRENT_SOURCE_DIR})
copy_dirs(hello_model ${CMAKE_CURRENT_SOURCE_DIR}/assets)
copy_dirs(hello_pbr ${CMAKE_CURRENT_SOURCE_DIR}/assets)