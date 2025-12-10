add_library(dag INTERFACE)

target_include_directories(dag
    INTERFACE
        include
)

target_link_libraries(dag
    INTERFACE
        graph-flags
        hlp

        spdlog
)
