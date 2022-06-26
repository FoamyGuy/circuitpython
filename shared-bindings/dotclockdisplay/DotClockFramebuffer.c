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

#include "py/objarray.h"
#include "py/runtime.h"

#include "shared-bindings/microcontroller/Pin.h"
#include "shared-bindings/dotclockdisplay/DotClockFramebuffer.h"
#include "shared-module/displayio/__init__.h"
#include "shared-module/dotclockdisplay/DotClockFramebuffer.h"

STATIC mp_obj_t dotclockdisplay_framebuffer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

// ** Todo
// * List of data pins
// * other connection pins
// * pixel width, height
// * Verify there are 16 data pins listed

    enum { ARG_width, ARG_height, ARG_hsync, ARG_vsync, ARG_de, ARG_pclock, ARG_data_pins, ARG_enable, ARG_backlight, ARG_pclock_frequency, ARG_hsync_back_porch, ARG_hsync_front_porch, ARG_hsync_pulse_width, ARG_vsync_back_porch, ARG_vsync_front_porch, ARG_vsync_pulse_width, ARG_pclock_active_neg, ARG_bounce_buffer_size_px, NUM_ARGS };
    static const mp_arg_t allowed_args[] = {

        // default pixel clock frequencyis 10 MHz (10 000 000)
        // { MP_QSTR_chip_select, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_hsync, MP_ARG_OBJ | MP_ARG_REQUIRED | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_vsync, MP_ARG_OBJ | MP_ARG_REQUIRED | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_de, MP_ARG_OBJ | MP_ARG_REQUIRED | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_pclock, MP_ARG_OBJ | MP_ARG_REQUIRED | MP_ARG_KW_ONLY, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_data_pins, MP_ARG_OBJ | MP_ARG_REQUIRED | MP_ARG_KW_ONLY, {.u_obj = mp_const_none } },
        { MP_QSTR_enable, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_none} },
        { MP_QSTR_backlight, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_obj = mp_const_none} },
        { MP_QSTR_pclock_frequency, MP_ARG_INT, {.u_int = 10000000} }, // default pixel clock frequency: 10 MHz
        { MP_QSTR_hsync_back_porch, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_hsync_front_porch, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_hsync_pulse_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_vsync_back_porch, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_vsync_front_porch, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_vsync_pulse_width, MP_ARG_INT | MP_ARG_REQUIRED, {.u_int = 0} },
        { MP_QSTR_pclock_active_neg, MP_ARG_INT, {.u_int = 1} }, // set 1 if RGB data is clocked out on falling edge
        { MP_QSTR_bounce_buffer_size_px, MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    MP_STATIC_ASSERT(MP_ARRAY_SIZE(allowed_args) == NUM_ARGS);

    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // if a pin is "None", then send -1
    // Only bk_light and disp_en can be None.

    // data_pins: list of 16 pin names

    mp_printf(&mp_plat_print, "setup pins\n");

    const mcu_pin_obj_t *hsync = validate_obj_is_free_pin(args[ARG_hsync].u_obj);
    mp_printf(&mp_plat_print, "setup pins1\n");
    const mcu_pin_obj_t *vsync = validate_obj_is_free_pin(args[ARG_vsync].u_obj);
    mp_printf(&mp_plat_print, "setup pins2\n");
    const mcu_pin_obj_t *de = validate_obj_is_free_pin(args[ARG_de].u_obj);
    mp_printf(&mp_plat_print, "setup pins3\n");
    const mcu_pin_obj_t *pclock = validate_obj_is_free_pin(args[ARG_pclock].u_obj);
    mp_printf(&mp_plat_print, "setup pins4\n");

    const mcu_pin_obj_t *enable = validate_obj_is_free_pin_or_none(args[ARG_enable].u_obj);
    mp_printf(&mp_plat_print, "setup pins5\n");
    const mcu_pin_obj_t *backlight = validate_obj_is_free_pin_or_none(args[ARG_backlight].u_obj);
    mp_printf(&mp_plat_print, "setup pins6\n");

    uint8_t num_pins;
    mp_printf(&mp_plat_print, "setup pins7\n");
    const mcu_pin_obj_t *data_pins[16];
    mp_printf(&mp_plat_print, "setup pins8\n");
    mp_printf(&mp_plat_print, "validate data_pins\n");
    validate_list_is_free_pins(MP_QSTR_data_pins, data_pins, (mp_int_t)MP_ARRAY_SIZE(data_pins), args[ARG_data_pins].u_obj, &num_pins);

    if (num_pins != 16) {
        mp_raise_ValueError(translate("Specify exactly 16 data_pins."));
    }

    mp_printf(&mp_plat_print, "pins are validated\n");
    dotclockdisplay_framebuffer_obj_t *self = &allocate_display_bus_or_raise()->dotclockdisplay;
    self->base.type = &dotclockdisplay_framebuffer_type;

    mp_printf(&mp_plat_print, "shared-bindings: before construct\n");

    common_hal_dotclockdisplay_framebuffer_construct(self, args[ARG_width].u_int, args[ARG_height].u_int, hsync, vsync, de, pclock, data_pins, enable, backlight, args[ARG_pclock_frequency].u_int, args[ARG_hsync_back_porch].u_int, args[ARG_hsync_front_porch].u_int, args[ARG_hsync_pulse_width].u_int, args[ARG_vsync_back_porch].u_int, args[ARG_vsync_front_porch].u_int, args[ARG_vsync_pulse_width].u_int, args[ARG_pclock_active_neg].u_int, args[ARG_bounce_buffer_size_px].u_int);

    mp_printf(&mp_plat_print, "shared-bindings: after construct\n");

    return MP_OBJ_FROM_PTR(self);
}


STATIC mp_int_t dotclockdisplay_framebuffer_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {

    dotclockdisplay_framebuffer_obj_t *self = (dotclockdisplay_framebuffer_obj_t *)self_in;
    // a readonly framebuffer would be unusual but not impossible
    if ((flags & MP_BUFFER_WRITE) && !(self->bufinfo.typecode & MP_OBJ_ARRAY_TYPECODE_FLAG_RW)) {
        return 1;
    }
    *bufinfo = self->bufinfo;
    return 0;
}

STATIC mp_obj_t dotclockdisplay_framebuffer_deinit(mp_obj_t self_in) {

    dotclockdisplay_framebuffer_obj_t *self = (dotclockdisplay_framebuffer_obj_t *)self_in;
    common_hal_dotclockdisplay_framebuffer_deinit(self);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(dotclockdisplay_framebuffer_deinit_obj, dotclockdisplay_framebuffer_deinit);

STATIC const mp_rom_map_elem_t dotclockdisplay_framebuffer_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&dotclockdisplay_framebuffer_deinit_obj) },
};
STATIC MP_DEFINE_CONST_DICT(dotclockdisplay_framebuffer_locals_dict, dotclockdisplay_framebuffer_locals_dict_table);


const mp_obj_type_t dotclockdisplay_framebuffer_type = {
    { &mp_type_type },
    .name = MP_QSTR_DotClockFramebuffer,
    .flags = MP_TYPE_FLAG_EXTENDED,
    .make_new = dotclockdisplay_framebuffer_make_new,
    .locals_dict = (mp_obj_dict_t *)&dotclockdisplay_framebuffer_locals_dict,
    MP_TYPE_EXTENDED_FIELDS(
        .buffer_p = { .get_buffer = dotclockdisplay_framebuffer_get_buffer, },
        .protocol = &dotclockdisplay_framebuffer_proto
        ),
};
