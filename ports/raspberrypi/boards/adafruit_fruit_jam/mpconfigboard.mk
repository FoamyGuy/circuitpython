USB_VID = 0x239A
USB_PID = 0x816C
USB_PRODUCT = "Fruit Jam"
USB_MANUFACTURER = "Adafruit"

CHIP_VARIANT = RP2350
CHIP_PACKAGE = B
CHIP_FAMILY = rp2

EXTERNAL_FLASH_DEVICES = "W25Q128JVxQ"

CIRCUITPY_SDIOIO = 1

CIRCUITPY_PICOGAME = 1
CIRCUITPY_PICOGAME_FRAMEBUFFER = 1

# USB video (UVC) cameras have configuration descriptors of several KB.
# TinyUSB skips enumerating any device whose descriptor doesn't fit here.
CFLAGS += -DCFG_TUH_ENUMERATION_BUFSIZE=4096
# Buffer isochronous IN packets (such as UVC video) between reads.
CFLAGS += -DPIO_USB_ISO_RING_SIZE=16384

# CIRCUITPY_DISPLAY_FONT = $(TOP)/tools/fonts/unifont-16.0.02-all.bdf
# CIRCUITPY_FONT_EXTRA_CHARACTERS = "🖮🖱️"
