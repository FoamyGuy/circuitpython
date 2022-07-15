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

#pragma once

#include "py/obj.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-module/framebufferio/FramebufferDisplay.h"
#include "ports/espressif/esp-idf/components/esp_lcd/include/esp_lcd_panel_ops.h"
#include "ports/espressif/esp-idf/components/esp_lcd/include/esp_lcd_panel_rgb.h"

// * Todo  *

typedef struct {
    mp_obj_base_t base;

    // *** update this with pin information for setup
    mp_buffer_info_t bufinfo;

    uint16_t width, height;
    bool full_refresh : 1;
    esp_lcd_panel_handle_t panel_handle;
} dotclockdisplay_framebuffer_obj_t;

// * required
void common_hal_dotclockdisplay_framebuffer_construct(dotclockdisplay_framebuffer_obj_t *self, const int width, const int height, const mcu_pin_obj_t *hsync, const mcu_pin_obj_t *vsync, const mcu_pin_obj_t *de, const mcu_pin_obj_t *pclock, const mcu_pin_obj_t **data_pins, const mcu_pin_obj_t *enable, const mcu_pin_obj_t *backlight, const int pclock_frequency, const int hsync_back_porch, const int hsync_front_porch, const int hsync_pulse_width, const int vsync_back_porch, const int vsync_front_porch, const int vsync_pulse_width, const int pclk_active_neg, const int bounce_buffer_size_px);
void common_hal_dotclockdisplay_framebuffer_get_bufinfo(dotclockdisplay_framebuffer_obj_t *self, mp_buffer_info_t *bufinfo);
void common_hal_dotclockdisplay_framebuffer_swap_buffers(dotclockdisplay_framebuffer_obj_t *self, uint8_t *dirty_row_bitmask);
void common_hal_dotclockdisplay_framebuffer_deinit(dotclockdisplay_framebuffer_obj_t *self);
int common_hal_dotclockdisplay_framebuffer_get_width(dotclockdisplay_framebuffer_obj_t *self);
int common_hal_dotclockdisplay_framebuffer_get_height(dotclockdisplay_framebuffer_obj_t *self);

// * optional * todo - do we need these?
// void common_hal_dotclockdisplay_framebuffer_reset(dotclockdisplay_framebuffer_obj_t *self);
// void common_hal_dotclockdisplay_framebuffer_reconstruct(dotclockdisplay_framebuffer_obj_t *self);

extern const framebuffer_p_t dotclockdisplay_framebuffer_proto;


// ** Todo do we need a collect ptrs object here?
void common_hal_dotclockdisplay_framebuffer_collect_ptrs(dotclockdisplay_framebuffer_obj_t *);
