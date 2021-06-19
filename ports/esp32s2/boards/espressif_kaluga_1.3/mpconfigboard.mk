USB_VID = 0x239A
USB_PID = 0x80C8
USB_PRODUCT = "Kaluga 1"
USB_MANUFACTURER = "Espressif"

INTERNAL_FLASH_FILESYSTEM = 1
LONGINT_IMPL = MPZ

# The default queue depth of 16 overflows on release builds,
# so increase it to 32.
CFLAGS += -DCFG_TUD_TASK_QUEUE_SZ=32

CIRCUITPY_ESP_FLASH_MODE=dio
CIRCUITPY_ESP_FLASH_FREQ=80m
CIRCUITPY_ESP_FLASH_SIZE=4MB

# We only have enough endpoints available in hardware to
# enable ONE of these at a time.
CIRCUITPY_USB_MIDI = 1
CIRCUITPY_USB_HID = 0
CIRCUITPY_USB_VENDOR = 0

CIRCUITPY_MODULE=wrover
