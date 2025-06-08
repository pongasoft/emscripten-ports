cmake_minimum_required(VERSION 3.13)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(Dawn-Example)

set(target "dawn-example")

set(CMAKE_CXX_STANDARD 20)

add_executable(${target} "main.cpp")

set(port_options "--use-port=emdawnwebgpu")

target_compile_options(${target} PRIVATE
    "-fno-exceptions"
    # We use this as both a compile flag and a link flag. Here, it sets the include path.
    "${port_options}"
    "-Wall"
    "-Wextra"
    "$<$<CONFIG:Release>:-flto>"
)

set_target_properties(${target} PROPERTIES OUTPUT_NAME "index")
set_target_properties(${target} PROPERTIES
    OUTPUT_NAME "index"
    SUFFIX ".html"
)
target_link_options(${target} PRIVATE
    # We use this as both a compile flag and a link flag. Here, it links emdawnwebgpu.
    "${port_options}"
    "-sEXIT_RUNTIME=0"
    "$<$<CONFIG:Debug>:-sASSERTIONS=1>"
    "$<$<CONFIG:Debug>:-Wno-limited-postlink-optimizations>"
    "$<$<CONFIG:Release>:--closure=1>"
)

target_link_options(${target} PRIVATE
    "-fno-exceptions"
    "-sASYNCIFY=1"
    "-sASYNCIFY_STACK_SIZE=65536"

    "$<$<CONFIG:Release>:-flto>"
    "$<$<CONFIG:Debug>:-sSAFE_HEAP=1>"
)
