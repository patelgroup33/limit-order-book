# ---------------------------------------------------------------------------
# lob_sim: the order-flow simulator + JSON exporter that feeds the visualizer.
# Links only the core library; the JSON writer is hand-rolled (no dependency).
# ---------------------------------------------------------------------------
add_executable(lob_sim
    main.cpp
)

target_link_libraries(lob_sim PRIVATE lob)

# Headers live alongside main.cpp in this directory.
target_include_directories(lob_sim PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

if(LOB_ENABLE_WARNINGS)
    target_compile_options(lob_sim PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
endif()
