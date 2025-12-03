add_library(hlp INTERFACE)

target_include_directories(hlp
    INTERFACE
        include
)

target_link_libraries(hlp
    INTERFACE
        graph-flags
        spdlog
)
