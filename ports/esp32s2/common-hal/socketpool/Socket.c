/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Scott Shawcroft for Adafruit Industries
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

#include "shared-bindings/socketpool/Socket.h"

#include "bindings/espidf/__init__.h"
#include "lib/utils/interrupt_char.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "supervisor/shared/tick.h"

void common_hal_socketpool_socket_settimeout(socketpool_socket_obj_t* self, mp_uint_t timeout_ms) {
    self->timeout_ms = timeout_ms;
}

bool common_hal_socketpool_socket_connect(socketpool_socket_obj_t* self, const char* host, mp_uint_t hostlen, mp_int_t port) {
    // For simplicity we use esp_tls for all TCP connections. If it's not SSL, ssl_context will be
    // NULL and should still work. This makes regular TCP connections more memory expensive but TLS
    // should become more and more common. Therefore, we optimize for the TLS case.

    esp_tls_cfg_t* tls_config = NULL;
    if (self->ssl_context != NULL) {
        tls_config = &self->ssl_context->ssl_config;
    }
    int result = esp_tls_conn_new_sync(host, hostlen, port, tls_config, self->tcp);
    self->connected = result >= 0;
    if (result < 0) {
        int esp_tls_code;
        int flags;
        esp_err_t err = esp_tls_get_and_clear_last_error(self->tcp->error_handle, &esp_tls_code, &flags);

        if (err == ESP_ERR_MBEDTLS_SSL_SETUP_FAILED) {
            mp_raise_espidf_MemoryError();
        } else if (ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED) {
            mp_raise_OSError_msg_varg(translate("Failed SSL handshake"));
        } else {
            mp_raise_OSError_msg_varg(translate("Unhandled ESP TLS error %d %d %x %d"), esp_tls_code, flags, err, result);
        }
    }

    return self->connected;
}

bool common_hal_socketpool_socket_get_connected(socketpool_socket_obj_t* self) {
    return self->connected;
}

mp_uint_t common_hal_socketpool_socket_send(socketpool_socket_obj_t* self, const uint8_t* buf, mp_uint_t len) {
    size_t sent = esp_tls_conn_write(self->tcp, buf, len);

    if (sent < 0) {
        mp_raise_OSError(MP_ENOTCONN);
    }
    return sent;
}

mp_uint_t common_hal_socketpool_socket_recv_into(socketpool_socket_obj_t* self, const uint8_t* buf, mp_uint_t len) {
    size_t received = 0;
    int status = 0;
    uint64_t start_ticks = supervisor_ticks_ms64();
    int sockfd;
    esp_err_t err = esp_tls_get_conn_sockfd(self->tcp, &sockfd);
    if (err != ESP_OK) {
        mp_raise_OSError(MP_EBADF);
    }
    while (received == 0 &&
           status >= 0 &&
           (self->timeout_ms == 0 || supervisor_ticks_ms64() - start_ticks <= self->timeout_ms) &&
           !mp_hal_is_interrupted()) {
        RUN_BACKGROUND_TASKS;
        size_t available = esp_tls_get_bytes_avail(self->tcp);
        if (available == 0) {
            // This reads the raw socket buffer and is used for non-TLS connections
            // and between encrypted TLS blocks.
            status = lwip_ioctl(sockfd, FIONREAD, &available);
        }
        size_t remaining = len - received;
        if (available > remaining) {
            available = remaining;
        }
        if (available > 0) {
            status = esp_tls_conn_read(self->tcp, (void*) buf + received, available);
            if (status > 0) {
                received += status;
            }
        }
    }

    if (received == 0) {
        // socket closed
        common_hal_socketpool_socket_close(self);
    }
    if (status < 0) {
        mp_raise_BrokenPipeError();
    }
    return received;
}

void common_hal_socketpool_socket_close(socketpool_socket_obj_t* self) {
    self->connected = false;
    if (self->tcp != NULL) {
        esp_tls_conn_destroy(self->tcp);
        self->tcp = NULL;
    }
}

bool common_hal_socketpool_socket_get_closed(socketpool_socket_obj_t* self) {
    return self->tcp == NULL;
}


mp_uint_t common_hal_socketpool_socket_get_hash(socketpool_socket_obj_t* self) {
    return self->num;
}
