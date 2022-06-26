/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Kevin Matocha
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


#include <string.h>
#include <stdio.h>


#include "py/gc.h"

#include "shared-bindings/board/__init__.h"
#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/dotclockdisplay/DotClockFramebuffer.h"
#include "shared-module/dotclockdisplay/DotClockFramebuffer.h"

#include "ports/espressif/esp-idf/components/esp_lcd/include/esp_lcd_panel_ops.h"
#include "ports/espressif/esp-idf/components/esp_lcd/include/esp_lcd_panel_rgb.h"
#include "ports/espressif/esp-idf/components/esp_lcd/interface/esp_lcd_panel_interface.h"
#include "ports/espressif/esp-idf/components/esp_common/include/esp_err.h"

#include "supervisor/memory.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (10 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_BK_LIGHT       39
#define EXAMPLE_PIN_NUM_HSYNC          47
#define EXAMPLE_PIN_NUM_VSYNC          48
#define EXAMPLE_PIN_NUM_DE             45
#define EXAMPLE_PIN_NUM_PCLK           21
#define EXAMPLE_PIN_NUM_DATA0          3  // B0
#define EXAMPLE_PIN_NUM_DATA1          4  // B1
#define EXAMPLE_PIN_NUM_DATA2          5  // B2
#define EXAMPLE_PIN_NUM_DATA3          6  // B3
#define EXAMPLE_PIN_NUM_DATA4          7  // B4
#define EXAMPLE_PIN_NUM_DATA5          8  // G0
#define EXAMPLE_PIN_NUM_DATA6          9  // G1
#define EXAMPLE_PIN_NUM_DATA7          10 // G2
#define EXAMPLE_PIN_NUM_DATA8          11 // G3
#define EXAMPLE_PIN_NUM_DATA9          12 // G4
#define EXAMPLE_PIN_NUM_DATA10         13 // G5
#define EXAMPLE_PIN_NUM_DATA11         14 // R0
#define EXAMPLE_PIN_NUM_DATA12         15 // R1
#define EXAMPLE_PIN_NUM_DATA13         16 // R2
#define EXAMPLE_PIN_NUM_DATA14         17 // R3
#define EXAMPLE_PIN_NUM_DATA15         18 // R4
#define EXAMPLE_PIN_NUM_DISP_EN        -1

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              800
#define EXAMPLE_LCD_V_RES              480

void common_hal_dotclockdisplay_framebuffer_construct(dotclockdisplay_framebuffer_obj_t *self, const int width, const int height, const mcu_pin_obj_t *hsync, const mcu_pin_obj_t *vsync, const mcu_pin_obj_t *de, const mcu_pin_obj_t *pclock, const mcu_pin_obj_t **data_pins, const mcu_pin_obj_t *enable, const mcu_pin_obj_t *backlight, const int pclock_frequency, const int hsync_back_porch, const int hsync_front_porch, const int hsync_pulse_width, const int vsync_back_porch, const int vsync_front_porch, const int vsync_pulse_width, const int pclock_active_neg, const int bounce_buffer_size_px) {


    // check enable, de and backlight pins
    // * todo * backlight

    int enable_pin_num, de_pin_num;
    if (enable == NULL) {
        enable_pin_num = -1;
    } else {
        enable_pin_num = enable->number;
    }

    if (de == NULL) {
        de_pin_num = -1;
    } else {
        de_pin_num = de->number;
    }

    // common_hal_digitalio_digitalinout_construct(&self->chip_select, chip_select);
    // common_hal_digitalio_digitalinout_switch_to_output(&self->chip_select, true, DRIVE_MODE_PUSH_PULL);
    // common_hal_never_reset_pin(chip_select);

    mp_printf(&mp_plat_print, "\n\nentering framebuffer_construct\n");
    mp_printf(&mp_plat_print, "pin 0: %d\n", data_pins[0]->number);

    size_t fb_size = width * height * 16 / 8;     // 16 = data_width

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .fb = m_malloc(fb_size, true), // preallocate framebuffer in CircuitPython
        .fb_size = fb_size,
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
        .clk_src = LCD_CLK_SRC_PLL160M,
        // .clk_src = LCD_CLK_SRC_PLL240M,
        .disp_gpio_num = enable_pin_num,
        .pclk_gpio_num = pclock->number,
        .vsync_gpio_num = vsync->number,
        .hsync_gpio_num = hsync->number,
        .de_gpio_num = de_pin_num,
        .data_gpio_nums = {
            data_pins[0]->number,
            data_pins[1]->number,
            data_pins[2]->number,
            data_pins[3]->number,
            data_pins[4]->number,
            data_pins[5]->number,
            data_pins[6]->number,
            data_pins[7]->number,
            data_pins[8]->number,
            data_pins[9]->number,
            data_pins[10]->number,
            data_pins[11]->number,
            data_pins[12]->number,
            data_pins[13]->number,
            data_pins[14]->number,
            data_pins[15]->number,
        },
        .timings = {
            .pclk_hz = pclock_frequency,
            .h_res = width,
            .v_res = height,
            // The following parameters should refer to LCD spec
            .hsync_back_porch = hsync_back_porch, // 100
            .hsync_front_porch = hsync_front_porch, // 40
            .hsync_pulse_width = hsync_pulse_width,  // 5
            .vsync_back_porch = vsync_back_porch,  // 25
            .vsync_front_porch = vsync_front_porch, // 10
            .vsync_pulse_width = vsync_pulse_width,  // 1
            .flags.pclk_active_pos = !pclock_active_neg, // 1: set 1 if RGB data is clocked out on falling edge
        },
        .flags.relax_on_idle = 1, // comment this out to stream
        .flags.fb_in_psram = 1, // allocate frame buffer in PSRAM
        // .on_frame_trans_done = draw_box,
        // .on_frame_trans_done = example_notify_lvgl_flush_ready,
        // .user_ctx = &disp_drv,
        .user_ctx = NULL,
        // .bounce_buffer_size_px = 1024 * 0,
        .bounce_buffer_size_px = bounce_buffer_size_px,
    };

    mp_printf(&mp_plat_print, "\n\nconstruct: before new_rgb_panel\n");

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    mp_printf(&mp_plat_print, "2 construct\n");

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    mp_printf(&mp_plat_print, "3 construct\n");
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    // ESP_LOGI(TAG, "Turn on LCD backlight");
    // gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    mp_printf(&mp_plat_print, "4 construct\n");

    self->bufinfo.buf = rgb_panel_get_buffer(panel_handle);
    self->bufinfo.len = rgb_panel_get_buffer_size(panel_handle);
    self->bufinfo.typecode = 'B'; // Do we need to define as BYTEARRAY_TYPECODE? as per binary.h ??
    self->panel_handle = panel_handle;
    self->width = width;
    self->height = height;

    // common_hal_dotclockdisplay_framebuffer_get_bufinfo(self, NULL);

    //    int bitmap_length=2*800;

    //    uint8_t bitmap[bitmap_length];
    //    for(int i=0; i<bitmap_length; i++) {
    //          bitmap[i]=85;
    //    }

    //    mp_printf(&mp_plat_print, "5 construct\n");

    //    /// **

    //    for(int j=0; j<400; j++) {
    //          // mp_printf(&mp_plat_print, "Looping\n");
    //  esp_lcd_panel_draw_bitmap(panel_handle, 50+j, 50+j, 50+j+20, 50+j+20, bitmap);
    // }

    //    mp_printf(&mp_plat_print, "6 construct\n****\n\n");

}


void common_hal_dotclockdisplay_framebuffer_get_bufinfo(dotclockdisplay_framebuffer_obj_t *self, mp_buffer_info_t *bufinfo) {
    if (self) {
        *bufinfo = self->bufinfo;
    }
}



STATIC void common_hal_dotclockdisplay_framebuffer_swapbuffers(dotclockdisplay_framebuffer_obj_t *self, uint8_t *dirty_row_bitmask) {
    self->full_refresh = false;
    rgb_panel_flush_buffer(self->panel_handle);
    // mp_printf(&mp_plat_print, "swapbuffers called\n");
}

void common_hal_dotclockdisplay_framebuffer_deinit(dotclockdisplay_framebuffer_obj_t *self) {
    esp_lcd_panel_reset(self->panel_handle);
    esp_lcd_panel_del(self->panel_handle);
    // free_memory(self->bufinfo.buf);
}


int common_hal_dotclockdisplay_framebuffer_get_width(dotclockdisplay_framebuffer_obj_t *self) {
    return self->width;
}

int common_hal_dotclockdisplay_framebuffer_get_height(dotclockdisplay_framebuffer_obj_t *self) {
    return self->height;
}


STATIC void dotclockdisplay_framebuffer_deinit(mp_obj_t self_in) {
    dotclockdisplay_framebuffer_obj_t *self = self_in;
    common_hal_dotclockdisplay_framebuffer_deinit(self);
}

STATIC void dotclockdisplay_framebuffer_get_bufinfo(mp_obj_t self_in, mp_buffer_info_t *bufinfo) {
    dotclockdisplay_framebuffer_obj_t *self = self_in;
    common_hal_dotclockdisplay_framebuffer_get_bufinfo(self, bufinfo);
}

STATIC int dotclockdisplay_framebuffer_get_color_depth(mp_obj_t self_in) {
    return 16;
}

STATIC int dotclockdisplay_framebuffer_get_height(mp_obj_t self_in) {
    dotclockdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_dotclockdisplay_framebuffer_get_height(self);
}

STATIC int dotclockdisplay_framebuffer_get_width(mp_obj_t self_in) {
    dotclockdisplay_framebuffer_obj_t *self = self_in;
    return common_hal_dotclockdisplay_framebuffer_get_width(self);
}

// STATIC int sharpdisplay_framebuffer_get_first_pixel_offset(mp_obj_t self_in) {
//     sharpdisplay_framebuffer_obj_t *self = self_in;
//     return common_hal_sharpdisplay_framebuffer_get_first_pixel_offset(self);
// }

// STATIC bool sharpdisplay_framebuffer_get_pixels_in_byte_share_row(mp_obj_t self_in) {
//     sharpdisplay_framebuffer_obj_t *self = self_in;
//     return common_hal_sharpdisplay_framebuffer_get_pixels_in_byte_share_row(self);
// }

// STATIC bool sharpdisplay_framebuffer_get_reverse_pixels_in_byte(mp_obj_t self_in) {
//     sharpdisplay_framebuffer_obj_t *self = self_in;
//     return common_hal_sharpdisplay_framebuffer_get_reverse_pixels_in_byte(self);
// }

// STATIC int sharpdisplay_framebuffer_get_row_stride(mp_obj_t self_in) {
//     sharpdisplay_framebuffer_obj_t *self = self_in;
//     return common_hal_sharpdisplay_framebuffer_get_row_stride(self);
// }

STATIC void dotclockdisplay_framebuffer_swapbuffers(mp_obj_t self_in, uint8_t *dirty_row_bitmask) {
    dotclockdisplay_framebuffer_obj_t *self = self_in;
    common_hal_dotclockdisplay_framebuffer_swapbuffers(self, dirty_row_bitmask);

}

const framebuffer_p_t dotclockdisplay_framebuffer_proto = {
    MP_PROTO_IMPLEMENT(MP_QSTR_protocol_framebuffer)
    .deinit = dotclockdisplay_framebuffer_deinit,
    .get_bufinfo = dotclockdisplay_framebuffer_get_bufinfo,
    .get_color_depth = dotclockdisplay_framebuffer_get_color_depth,
    .get_height = dotclockdisplay_framebuffer_get_height,
    .get_width = dotclockdisplay_framebuffer_get_width,
    .swapbuffers = dotclockdisplay_framebuffer_swapbuffers,

    // .get_first_pixel_offset = sharpdisplay_framebuffer_get_first_pixel_offset,
    // .get_pixels_in_byte_share_row = sharpdisplay_framebuffer_get_pixels_in_byte_share_row,
    // .get_reverse_pixels_in_byte = sharpdisplay_framebuffer_get_reverse_pixels_in_byte,
    // .get_row_stride = sharpdisplay_framebuffer_get_row_stride,
};

void common_hal_dotclockdisplay_framebuffer_collect_ptrs(dotclockdisplay_framebuffer_obj_t *self) {
    // *todo


    // gc_collect_ptr(self->bus);
}
