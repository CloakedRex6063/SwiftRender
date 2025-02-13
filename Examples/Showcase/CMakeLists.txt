add_executable(Showcase Source/App.cpp)
set(SHADER_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/Shaders" CACHE PATH "")
set(DESTINATION_FOLDER "${CMAKE_CURRENT_BINARY_DIR}/Resources" CACHE PATH "")
target_link_libraries(Showcase PUBLIC SwiftRender fastgltf glfw imgui Utility)