/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "supervisor/board.h"
#include "shared-bindings/displayio/I2CDisplay.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/displayio/mipi_constants.h"
#include "shared-bindings/busio/I2C.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "supervisor/board.h"
#include "supervisor/shared/board.h"
#include "shared-bindings/board/__init__.h"

/*
 _INIT_SEQUENCE = (
        0xae,0x00,  // display off, sleep mode
        0xdc,0x01,0x00,  // set display start line 0
        0x81,0x01,0x4f,  // contrast setting = 0x4f
        0x20,0x00,  // vertical (column) addressing mode (POR=0x20)
        0xa0,0x00,  // segment remap = 1 (POR=0, down rotation)
        0xc0,0x00,  // common output scan direction = 0 (0 to n-1 (POR=0))
        0xa8,0x01,0x7f,  // multiplex ratio = 128 (POR=0x7F)
        0xd3,0x01,0x60,  // set display offset mode = 0x60
        0xd9,0x01,0x22,  // pre-charge/dis-charge period mode: 2 DCLKs/2 DCLKs (POR)
        0xdb,0x01,0x35,  // VCOM deselect level = 0.770 (POR)
        0xa4,0x00,  // entire display off, retain RAM, normal status (POR)
        0xa6,0x00,  // normal (not reversed) display
        0xaf,0x00  // DISPLAY_ON
    )
 */

uint8_t display_init_sequence[] = { // SH1107
        0xae, 0,  // display off, sleep mode
        0xdc, 1, 0x00,  // set display start line 0
        0x81, 1, 0x4f,  // contrast setting = 0x4f
        0x20, 0,  // vertical (column) addressing mode (POR=0x20)
        0xa0, 0,  // segment remap = 1 (POR=0, down rotation)
        0xc0, 0,  // common output scan direction = 0 (0 to n-1 (POR=0))
        0xa8, 1, 0x7f,  // multiplex ratio = 128 (POR=0x7F)
        0xd3, 1, 0x60,  // set display offset mode = 0x60
        0xd9, 1, 0x22,  // pre-charge/dis-charge period mode: 2 DCLKs/2 DCLKs (POR)
        0xdb, 1, 0x35,  // VCOM deselect level = 0.770 (POR)
        0xa4, 0,  // entire display off, retain RAM, normal status (POR)
        0xa6, 0,  // normal (not reversed) display
        0xaf, 0  // DISPLAY_ON
};

void board_init(void) {
    busio_i2c_obj_t *i2c = common_hal_board_create_i2c(0);

    // What we would do if it wasn't the shared board I2C: (for reference)
    // busio_i2c_obj_t *i2c = &displays[0].i2cdisplay_bus.inline_bus;
    // common_hal_busio_i2c_construct(i2c, &pin_GPIO23, &pin_GPIO22, 100000, 0);
    // common_hal_busio_i2c_never_reset(i2c);

    displayio_i2cdisplay_obj_t *bus = &displays[0].i2cdisplay_bus;
    bus->base.type = &displayio_i2cdisplay_type;
    common_hal_displayio_i2cdisplay_construct(bus,
        i2c,
        0x3c,
        NULL
        );

    displayio_display_obj_t *display = &displays[0].display;
    display->base.type = &displayio_display_type;
    common_hal_displayio_display_construct(display,
        bus,
        128, // Width
        64, // Height
        0, // column start
        0, // row start
        90, // rotation
        1, // Color depth
        true, // grayscale
        false, // pixels in byte share row. Only used with depth < 8
        1, // bytes per cell. Only valid for depths < 8
        false, // reverse_pixels_in_byte. Only valid for depths < 8
        true, // reverse_pixels_in_word
        0x21, // Set column command
        0x22, // Set row command
        44, // Write ram command
        display_init_sequence,
        sizeof(display_init_sequence),
        NULL, // no backlight pin
        0x81, // brightness command
        1.0f, // brightness
        true, // single_byte_bounds
        true, // data as commands
        true, // auto_refresh
        60, // native_frames_per_second
        true, // backlight_on_high
        true, // SH1107_addressing
        0); // backlight pwm frequency
}
// Use the MP_WEAK supervisor/shared/board.c versions of routines not defined here.
