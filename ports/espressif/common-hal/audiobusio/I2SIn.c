// This file is part of the CircuitPython project: https://circuitpython.org
//
// SPDX-FileCopyrightText: Copyright (c) 2026 Tim Cocks for Adafruit Industries
//
// SPDX-License-Identifier: MIT

#include "bindings/espidf/__init__.h"

#include "common-hal/audiobusio/I2SIn.h"
#include "py/runtime.h"
#include "shared-bindings/audiobusio/I2SIn.h"
#include "shared-bindings/microcontroller/Pin.h"

#include "driver/i2s_std.h"

#if CIRCUITPY_AUDIOBUSIO_I2SIN

void common_hal_audiobusio_i2sin_construct(audiobusio_i2sin_obj_t *self,
    const mcu_pin_obj_t *bit_clock, const mcu_pin_obj_t *word_select,
    const mcu_pin_obj_t *data, const mcu_pin_obj_t *main_clock,
    uint32_t sample_rate, uint8_t bit_depth, bool mono, bool left_justified) {

    if (bit_depth != 8 && bit_depth != 16 && bit_depth != 24 && bit_depth != 32) {
        mp_raise_ValueError(MP_ERROR_TEXT("bit_depth must be 8, 16, 24, or 32."));
    }

    i2s_data_bit_width_t bit_width = (i2s_data_bit_width_t)bit_depth;
    i2s_slot_mode_t slot_mode = mono ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &self->rx_chan);
    if (err == ESP_ERR_NOT_FOUND) {
        mp_raise_RuntimeError(MP_ERROR_TEXT("Peripheral in use"));
    }
    CHECK_ESP_RESULT(err);

    i2s_std_slot_config_t slot_cfg = left_justified
        ? (i2s_std_slot_config_t)I2S_STD_MSB_SLOT_DEFAULT_CONFIG(bit_width, slot_mode)
        : (i2s_std_slot_config_t)I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bit_width, slot_mode);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .mclk = main_clock != NULL ? main_clock->number : I2S_GPIO_UNUSED,
            .bclk = bit_clock->number,
            .ws = word_select->number,
            .dout = I2S_GPIO_UNUSED,
            .din = data->number,
        },
    };
    CHECK_ESP_RESULT(i2s_channel_init_std_mode(self->rx_chan, &std_cfg));
    CHECK_ESP_RESULT(i2s_channel_enable(self->rx_chan));

    self->bit_clock = bit_clock;
    self->word_select = word_select;
    self->data = data;
    self->mclk = main_clock;
    self->sample_rate = sample_rate;
    self->bit_depth = bit_depth;

    claim_pin(bit_clock);
    claim_pin(word_select);
    claim_pin(data);
    if (main_clock) {
        claim_pin(main_clock);
    }
}

bool common_hal_audiobusio_i2sin_deinited(audiobusio_i2sin_obj_t *self) {
    return self->data == NULL;
}

void common_hal_audiobusio_i2sin_deinit(audiobusio_i2sin_obj_t *self) {
    if (common_hal_audiobusio_i2sin_deinited(self)) {
        return;
    }

    if (self->rx_chan) {
        i2s_channel_disable(self->rx_chan);
        i2s_del_channel(self->rx_chan);
        self->rx_chan = NULL;
    }

    if (self->bit_clock) {
        reset_pin_number(self->bit_clock->number);
    }
    self->bit_clock = NULL;

    if (self->word_select) {
        reset_pin_number(self->word_select->number);
    }
    self->word_select = NULL;

    if (self->data) {
        reset_pin_number(self->data->number);
    }
    self->data = NULL;

    if (self->mclk) {
        reset_pin_number(self->mclk->number);
    }
    self->mclk = NULL;
}

uint32_t common_hal_audiobusio_i2sin_record_to_buffer(audiobusio_i2sin_obj_t *self,
    void *buffer, uint32_t length) {
    size_t result = 0;
    size_t element_size = self->bit_depth / 8;
    // 24-bit samples occupy a 32-bit slot on the I2S bus.
    if (self->bit_depth == 24) {
        element_size = 4;
    }
    esp_err_t err = i2s_channel_read(self->rx_chan, buffer, length * element_size, &result, portMAX_DELAY);
    CHECK_ESP_RESULT(err);
    return result / element_size;
}

uint8_t common_hal_audiobusio_i2sin_get_bit_depth(audiobusio_i2sin_obj_t *self) {
    return self->bit_depth;
}

uint32_t common_hal_audiobusio_i2sin_get_sample_rate(audiobusio_i2sin_obj_t *self) {
    return self->sample_rate;
}

#endif // CIRCUITPY_AUDIOBUSIO_I2SIN
