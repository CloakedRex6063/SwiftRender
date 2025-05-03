FILE(GLOB_RECURSE UTILS_SOURCES src/*.cpp)
add_library(Utils ${UTILS_SOURCES})
target_include_directories(Utils PUBLIC include)

function(compile_shaders TARGET DEPENDS INPUT ENTRY_POINT OUTPUT)
    add_custom_target(${TARGET} COMMAND slangc ${INPUT} -profile glsl_450 -target spirv -o ${OUTPUT} -entry ${ENTRY_POINT})
    add_dependencies(${DEPENDS} ${TARGET})
endfunction()