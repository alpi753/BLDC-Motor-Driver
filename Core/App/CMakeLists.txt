add_library(app STATIC
    cdc_cbor.c
    cdc_usb_bridge.c
)

target_include_directories(app PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app PUBLIC
    stm32cubemx
    tinycbor
)
