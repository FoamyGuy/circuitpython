USB_VID = 0x1209
USB_PID = 0x2017
USB_PRODUCT = "Mini SAM M4"
USB_MANUFACTURER = "Benjamin Shockley"

CHIP_VARIANT = SAMD51G19A
CHIP_FAMILY = samd51

QSPI_FLASH_FILESYSTEM = 1
EXTERNAL_FLASH_DEVICES = "W25Q16JVxM, W25Q16JVxQ"
LONGINT_IMPL = MPZ

CIRCUITPY_FLOPPYIO = 0
CIRCUITPY_JPEGIO = 0
CIRCUITPY_SYNTHIO = 0
CIRCUITPY_VECTORIO = 0

CIRCUITPY_BITBANG_APA102 = 1

#Include these Python libraries in firmware.
FROZEN_MPY_DIRS += $(TOP)/frozen/Adafruit_CircuitPython_DotStar
