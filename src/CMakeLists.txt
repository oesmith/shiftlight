add_executable(shift main.c mbe_can.c)

pico_enable_stdio_usb(shift 1)
pico_enable_stdio_uart(shift 0)
pico_add_extra_outputs(shift)

target_link_libraries(shift
  pico_stdlib
  hardware_pio
  can2040
  ws2812
)
