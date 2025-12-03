cmake_minimum_required(VERSION 3.21)

project(graph)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # to generate compile_commands.json

add_subdirectory(flags)
add_subdirectory(third_party)

add_executable(graph.x)

add_subdirectory(src/hlp)

target_sources(graph.x
    PRIVATE
        src/main.cpp
)

target_include_directories(graph.x
    PRIVATE
        include/
)

target_link_libraries(graph.x
    PRIVATE 
        graph-flags
        hlp

        spdlog
)

# enable_testing()
# add_subdirectory(tests)
