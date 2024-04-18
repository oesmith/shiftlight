# Pico SDK includes required by can2040:
# https://github.com/KevinOConnor/can2040/blob/master/docs/API.md#compiling
set(PICO_CMSIS_PATH "${PICO_SDK_PATH}/src/rp2_common/cmsis/stub/CMSIS")
include_directories(
  "${PICO_CMSIS_PATH}/Device/RaspberryPi/RP2040/Include"
  "${PICO_CMSIS_PATH}/Core/Include"
)

add_library(can2040 STATIC can2040.c)

target_link_libraries(can2040 pico_stdlib)

# WS2812 LED strip library, derived from the Pico SDK example at
# https://github.com/raspberrypi/pico-examples/tree/eca13acf57916a0bd5961028314006983894fc84/pio/ws2812
add_library(ws2812 STATIC ws2812.c)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(ws2812 ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_link_libraries(ws2812 PRIVATE pico_stdlib hardware_pio)
