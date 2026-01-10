function(add_example EXAMPLE_NAME SOURCE_DIR)
    add_executable(${EXAMPLE_NAME})
    file(GLOB_RECURSE EXAMPLE_SOURCES ${SOURCE_DIR}/${EXAMPLE_NAME}/*.cpp)
    target_sources(${EXAMPLE_NAME} PUBLIC ${EXAMPLE_SOURCES})
    target_link_libraries(${EXAMPLE_NAME} PUBLIC utility)

    set_target_properties(${EXAMPLE_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME}"
    )

    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SOURCE_DIR}/extern/slang/bin/slang-compiler.dll"
            "${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME}"
            COMMENT "Copying slang-compiler.dll"
    )

    add_custom_command(TARGET ${EXAMPLE_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SOURCE_DIR}/${EXAMPLE_NAME}/shaders"
            "${CMAKE_BINARY_DIR}/examples/${EXAMPLE_NAME}"
            COMMENT "Copying ${EXAMPLE_NAME} shaders"
    )
endfunction()

function(copy_dirs TARGET_NAME)
    foreach(DIR ${ARGN})
        cmake_path(GET DIR STEM DIR_NAME)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${DIR}"
                "${CMAKE_BINARY_DIR}/examples/${TARGET_NAME}/${DIR_NAME}"
                COMMENT "Copying ${DIR}"
        )
    endforeach()
endfunction()