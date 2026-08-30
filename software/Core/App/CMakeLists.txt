add_library(app STATIC
    telemetry.c
    ${PROJECT_SOURCE_DIR}/protocol/bldc.pb.c
    # drv8323r.c
)

target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(app PUBLIC
    stm32cubemx
    nanopb
    m
)
