add_subdirectory(extern/glfw)
add_subdirectory(extern/glm)
add_subdirectory(extern/meshopt)

add_library(slang INTERFACE)
target_include_directories(slang INTERFACE extern/slang/include)
target_link_libraries(slang INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/extern/slang/lib/slang-compiler.lib)

add_library(tinygltf INTERFACE)
target_compile_definitions(tinygltf INTERFACE TINYGLTF_NO_STB_IMAGE_WRITE)
target_include_directories(tinygltf INTERFACE extern/tinygltf)

add_library(mikktspace)
target_include_directories(mikktspace PUBLIC extern/mikktspace)
target_sources(mikktspace PUBLIC extern/mikktspace/mikktspace.c)

add_library(imgui)
target_link_libraries(imgui PUBLIC Microsoft::DirectX-Headers)
target_include_directories(imgui PUBLIC extern/imgui extern/glfw/include)
file(GLOB imgui_sources ${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/*.cpp)
target_sources(imgui PUBLIC ${imgui_sources})

add_library(utility)
target_sources(utility
        PUBLIC
        utility/window.cpp
        utility/importer.cpp
        utility/shader_compiler.cpp
        utility/camera.cpp
        utility/input.cpp
        utility/imgui.cpp)
target_link_libraries(utility PUBLIC glfw Swift imgui tinygltf mikktspace slang glm::glm meshoptimizer )
target_compile_definitions(utility PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_include_directories(utility PUBLIC utility extern/glfw/include)

include(cmake/functions.cmake)
add_example(hello_triangle ${CMAKE_CURRENT_SOURCE_DIR})
add_example(hello_model ${CMAKE_CURRENT_SOURCE_DIR})
add_example(hello_grass ${CMAKE_CURRENT_SOURCE_DIR})
copy_dirs(hello_model ${CMAKE_CURRENT_SOURCE_DIR}/assets)