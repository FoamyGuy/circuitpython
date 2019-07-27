/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Dan Halbert for Adafruit Industries
 * Copyright (c) 2018 Artur Pacholec
 * Copyright (c) 2016 Glenn Ruben Bakke
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

#include "py/runtime.h"
#include "shared-bindings/bleio/__init__.h"
#include "shared-bindings/bleio/Adapter.h"
#include "shared-bindings/bleio/Central.h"
#include "shared-bindings/bleio/Characteristic.h"
#include "shared-bindings/bleio/Descriptor.h"
#include "shared-bindings/bleio/Peripheral.h"
#include "shared-bindings/bleio/Service.h"
#include "shared-bindings/bleio/UUID.h"

#include "common-hal/bleio/__init__.h"

static volatile bool m_discovery_in_process;
static volatile bool m_discovery_successful;

static bleio_service_obj_t *m_char_discovery_service;
static bleio_characteristic_obj_t *m_desc_discovery_characteristic;

// Turn off BLE on a reset or reload.
void bleio_reset() {
    if (common_hal_bleio_adapter_get_enabled()) {
        common_hal_bleio_adapter_set_enabled(false);
    }
}

// The singleton bleio.Adapter object, bound to bleio.adapter
// It currently only has properties and no state
const super_adapter_obj_t common_hal_bleio_adapter_obj = {
    .base = {
        .type = &bleio_adapter_type,
    },
};

uint16_t common_hal_bleio_device_get_conn_handle(mp_obj_t device) {
    if (MP_OBJ_IS_TYPE(device, &bleio_peripheral_type)) {
        return ((bleio_peripheral_obj_t*) MP_OBJ_TO_PTR(device))->conn_handle;
    } else if (MP_OBJ_IS_TYPE(device, &bleio_central_type)) {
        return ((bleio_central_obj_t*) MP_OBJ_TO_PTR(device))->conn_handle;
    } else {
        return 0;
    }
}

mp_obj_list_t *common_hal_bleio_device_get_remote_services_list(mp_obj_t device) {
    if (MP_OBJ_IS_TYPE(device, &bleio_peripheral_type)) {
        return ((bleio_peripheral_obj_t*) MP_OBJ_TO_PTR(device))->remote_services_list;
    } else if (MP_OBJ_IS_TYPE(device, &bleio_central_type)) {
        return ((bleio_central_obj_t*) MP_OBJ_TO_PTR(device))->remote_services_list;
    } else {
        return NULL;
    }
}

// service_uuid may be NULL, to discover all services.
STATIC bool discover_next_services(mp_obj_t device, uint16_t start_handle, ble_uuid_t *service_uuid) {
    m_discovery_successful = false;
    m_discovery_in_process = true;

    uint16_t conn_handle = common_hal_bleio_device_get_conn_handle(device);
    uint32_t err_code = sd_ble_gattc_primary_services_discover(conn_handle, start_handle, service_uuid);

    if (err_code != NRF_SUCCESS) {
        mp_raise_OSError_msg(translate("Failed to discover services"));
    }

    // Wait for a discovery event.
    while (m_discovery_in_process) {
        MICROPY_VM_HOOK_LOOP;
    }
    return m_discovery_successful;
}

STATIC bool discover_next_characteristics(mp_obj_t device, bleio_service_obj_t *service, uint16_t start_handle) {
    m_char_discovery_service = service;

    ble_gattc_handle_range_t handle_range;
    handle_range.start_handle = start_handle;
    handle_range.end_handle = service->end_handle;

    m_discovery_successful = false;
    m_discovery_in_process = true;

    uint16_t conn_handle = common_hal_bleio_device_get_conn_handle(device);
    uint32_t err_code = sd_ble_gattc_characteristics_discover(conn_handle, &handle_range);
    if (err_code != NRF_SUCCESS) {
        return false;
    }

    // Wait for a discovery event.
    while (m_discovery_in_process) {
        MICROPY_VM_HOOK_LOOP;
    }
    return m_discovery_successful;
}

STATIC bool discover_next_descriptors(mp_obj_t device, bleio_characteristic_obj_t *characteristic, uint16_t start_handle, uint16_t end_handle) {
    m_desc_discovery_characteristic = characteristic;

    ble_gattc_handle_range_t handle_range;
    handle_range.start_handle = start_handle;
    handle_range.end_handle = end_handle;

    m_discovery_successful = false;
    m_discovery_in_process = true;

    uint16_t conn_handle = common_hal_bleio_device_get_conn_handle(device);
    uint32_t err_code = sd_ble_gattc_descriptors_discover(conn_handle, &handle_range);
    if (err_code != NRF_SUCCESS) {
        return false;
    }

    // Wait for a discovery event.
    while (m_discovery_in_process) {
        MICROPY_VM_HOOK_LOOP;
    }
    return m_discovery_successful;
}

STATIC void on_primary_srv_discovery_rsp(ble_gattc_evt_prim_srvc_disc_rsp_t *response, mp_obj_t device) {
    for (size_t i = 0; i < response->count; ++i) {
        ble_gattc_service_t *gattc_service = &response->services[i];

        bleio_service_obj_t *service = m_new_obj(bleio_service_obj_t);
        service->base.type = &bleio_service_type;

        // Initialize several fields at once.
        common_hal_bleio_service_construct(service, NULL, mp_obj_new_list(0, NULL), false);

        service->device = device;
        service->is_remote = true;
        service->start_handle = gattc_service->handle_range.start_handle;
        service->end_handle = gattc_service->handle_range.end_handle;
        service->handle = gattc_service->handle_range.start_handle;

        if (gattc_service->uuid.type != BLE_UUID_TYPE_UNKNOWN) {
            // Known service UUID.
            bleio_uuid_obj_t *uuid = m_new_obj(bleio_uuid_obj_t);
            uuid->base.type = &bleio_uuid_type;
            bleio_uuid_construct_from_nrf_ble_uuid(uuid, &gattc_service->uuid);
            service->uuid = uuid;
        } else {
            // The discovery response contained a 128-bit UUID that has not yet been registered with the
            // softdevice via sd_ble_uuid_vs_add(). We need to fetch the 128-bit value and register it.
            // For now, just set the UUID to NULL.
            service->uuid = NULL;
        }

        mp_obj_list_append(common_hal_bleio_device_get_remote_services_list(device), service);
    }

    if (response->count > 0) {
        m_discovery_successful = true;
    }
    m_discovery_in_process = false;
}

STATIC void on_char_discovery_rsp(ble_gattc_evt_char_disc_rsp_t *response, mp_obj_t device) {
    for (size_t i = 0; i < response->count; ++i) {
        ble_gattc_char_t *gattc_char = &response->chars[i];

        bleio_characteristic_obj_t *characteristic = m_new_obj(bleio_characteristic_obj_t);
        characteristic->base.type = &bleio_characteristic_type;

        characteristic->descriptor_list = mp_obj_new_list(0, NULL);

        bleio_uuid_obj_t *uuid = NULL;

        if (gattc_char->uuid.type != BLE_UUID_TYPE_UNKNOWN) {
            // Known characteristic UUID.
            uuid = m_new_obj(bleio_uuid_obj_t);
            uuid->base.type = &bleio_uuid_type;
            bleio_uuid_construct_from_nrf_ble_uuid(uuid, &gattc_char->uuid);
        } else {
            // The discovery response contained a 128-bit UUID that has not yet been registered with the
            // softdevice via sd_ble_uuid_vs_add(). We need to fetch the 128-bit value and register it.
            // For now, just leave the UUID as NULL.
        }

        bleio_characteristic_properties_t props;

        props.broadcast = gattc_char->char_props.broadcast;
        props.indicate = gattc_char->char_props.indicate;
        props.notify = gattc_char->char_props.notify;
        props.read = gattc_char->char_props.read;
        props.write = gattc_char->char_props.write;
        props.write_no_response = gattc_char->char_props.write_wo_resp;

        // Call common_hal_bleio_characteristic_construct() to initalize some fields and set up evt handler.
        common_hal_bleio_characteristic_construct(characteristic, uuid, props, mp_obj_new_list(0, NULL));
        characteristic->handle = gattc_char->handle_value;
        characteristic->service = m_char_discovery_service;

        mp_obj_list_append(m_char_discovery_service->characteristic_list, MP_OBJ_FROM_PTR(characteristic));
    }

    if (response->count > 0) {
        m_discovery_successful = true;
    }
    m_discovery_in_process = false;
}

STATIC void on_desc_discovery_rsp(ble_gattc_evt_desc_disc_rsp_t *response, mp_obj_t device) {
    for (size_t i = 0; i < response->count; ++i) {
        ble_gattc_desc_t *gattc_desc = &response->descs[i];

        // Remember handles for certain well-known descriptors.
        switch (gattc_desc->uuid.uuid) {
            case DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION:
                m_desc_discovery_characteristic->cccd_handle = gattc_desc->handle;
                break;

            case DESCRIPTOR_UUID_SERVER_CHARACTERISTIC_CONFIGURATION:
                m_desc_discovery_characteristic->sccd_handle = gattc_desc->handle;
                break;

            case DESCRIPTOR_UUID_CHARACTERISTIC_USER_DESCRIPTION:
                m_desc_discovery_characteristic->user_desc_handle = gattc_desc->handle;
                break;

            default:
                // TODO: sd_ble_gattc_descriptors_discover() can return things that are not descriptors,
                // so ignore those.
                // https://devzone.nordicsemi.com/f/nordic-q-a/49500/sd_ble_gattc_descriptors_discover-is-returning-attributes-that-are-not-descriptors
                break;
        }

        bleio_descriptor_obj_t *descriptor = m_new_obj(bleio_descriptor_obj_t);
        descriptor->base.type = &bleio_descriptor_type;

        bleio_uuid_obj_t *uuid = NULL;

        if (gattc_desc->uuid.type != BLE_UUID_TYPE_UNKNOWN) {
            // Known descriptor UUID.
            uuid = m_new_obj(bleio_uuid_obj_t);
            uuid->base.type = &bleio_uuid_type;
            bleio_uuid_construct_from_nrf_ble_uuid(uuid, &gattc_desc->uuid);
        } else {
            // The discovery response contained a 128-bit UUID that has not yet been registered with the
            // softdevice via sd_ble_uuid_vs_add(). We need to fetch the 128-bit value and register it.
            // For now, just leave the UUID as NULL.
        }

        common_hal_bleio_descriptor_construct(descriptor, uuid);
        descriptor->handle = gattc_desc->handle;
        descriptor->characteristic = m_desc_discovery_characteristic;

        mp_obj_list_append(m_desc_discovery_characteristic->descriptor_list, MP_OBJ_FROM_PTR(descriptor));
    }

    if (response->count > 0) {
        m_discovery_successful = true;
    }
    m_discovery_in_process = false;
}

STATIC void discovery_on_ble_evt(ble_evt_t *ble_evt, mp_obj_t device) {
    switch (ble_evt->header.evt_id) {
        case BLE_GAP_EVT_DISCONNECTED:
            m_discovery_successful = false;
            m_discovery_in_process = false;
            break;

        case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
            on_primary_srv_discovery_rsp(&ble_evt->evt.gattc_evt.params.prim_srvc_disc_rsp, device);
            break;

        case BLE_GATTC_EVT_CHAR_DISC_RSP:
            on_char_discovery_rsp(&ble_evt->evt.gattc_evt.params.char_disc_rsp, device);
            break;

        case BLE_GATTC_EVT_DESC_DISC_RSP:
            on_desc_discovery_rsp(&ble_evt->evt.gattc_evt.params.desc_disc_rsp, device);
            break;

        default:
            // For debugging.
            // mp_printf(&mp_plat_print, "Unhandled discovery event: 0x%04x\n", ble_evt->header.evt_id);
            break;
    }
}


void common_hal_bleio_device_discover_remote_services(mp_obj_t device, mp_obj_t service_uuids_whitelist) {
    mp_obj_list_t *remote_services_list = common_hal_bleio_device_get_remote_services_list(device);

    ble_drv_add_event_handler(discovery_on_ble_evt, device);

    if (service_uuids_whitelist == mp_const_none) {
        // List of service UUID's not given, so discover all available services.

        uint16_t next_service_start_handle = BLE_GATT_HANDLE_START;

        while (discover_next_services(device, next_service_start_handle, MP_OBJ_NULL)) {
            // discover_next_services() appends to remote_services_list.

            // Get the most recently discovered service, and then ask for services
            // whose handles start after the last attribute handle inside that service.
            const bleio_service_obj_t *service =
                MP_OBJ_TO_PTR(remote_services_list->items[remote_services_list->len - 1]);
            next_service_start_handle = service->end_handle + 1;
        }
    } else {
        mp_obj_iter_buf_t iter_buf;
        mp_obj_t iterable = mp_getiter(service_uuids_whitelist, &iter_buf);
        mp_obj_t uuid_obj;
        while ((uuid_obj = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) {
            if (!MP_OBJ_IS_TYPE(uuid_obj, &bleio_uuid_type)) {
                mp_raise_ValueError(translate("non-UUID found in service_uuids_whitelist"));
            }
            bleio_uuid_obj_t *uuid = MP_OBJ_TO_PTR(uuid_obj);

            ble_uuid_t nrf_uuid;
            bleio_uuid_convert_to_nrf_ble_uuid(uuid, &nrf_uuid);

            // Service might or might not be discovered; that's ok. Caller has to check
            // Central.remote_services to find out.
            // We only need to call this once for each service to discover.
            discover_next_services(device, BLE_GATT_HANDLE_START, &nrf_uuid);
        }
    }


    for (size_t service_idx = 0; service_idx < remote_services_list->len; ++service_idx) {
        bleio_service_obj_t *service = MP_OBJ_TO_PTR(remote_services_list->items[service_idx]);

        // Skip the service if it had an unknown (unregistered) UUID.
        if (service->uuid == NULL) {
            continue;
        }

        uint16_t next_char_start_handle = service->start_handle;

        // Stop when we go past the end of the range of handles for this service or
        // discovery call returns nothing.
        // discover_next_characteristics() appends to the characteristic_list.
        while (next_char_start_handle <= service->end_handle &&
               discover_next_characteristics(device, service, next_char_start_handle)) {


            // Get the most recently discovered characteristic, and then ask for characteristics
            // whose handles start after the last attribute handle inside that characteristic.
            const bleio_characteristic_obj_t *characteristic =
                MP_OBJ_TO_PTR(service->characteristic_list->items[service->characteristic_list->len - 1]);
            next_char_start_handle = characteristic->handle + 1;
        }

        // Got characteristics for this service. Now discover descriptors for each characteristic.
        size_t char_list_len = service->characteristic_list->len;
        for (size_t char_idx = 0; char_idx < char_list_len; ++char_idx) {
            bleio_characteristic_obj_t *characteristic =
                MP_OBJ_TO_PTR(service->characteristic_list->items[char_idx]);
            const bool last_characteristic = char_idx == char_list_len - 1;
            bleio_characteristic_obj_t *next_characteristic = last_characteristic
                ? NULL
                : MP_OBJ_TO_PTR(service->characteristic_list->items[char_idx + 1]);

            // Skip the characteristic if it had an unknown (unregistered) UUID.
            if (characteristic->uuid == NULL) {
                continue;
            }

            uint16_t next_desc_start_handle = characteristic->handle + 1;

            // Don't run past the end of this service or the beginning of the next characteristic.
            uint16_t next_desc_end_handle = next_characteristic == NULL
                ? service->end_handle
                : next_characteristic->handle - 1;

            // Stop when we go past the end of the range of handles for this service or
            // discovery call returns nothing.
            // discover_next_descriptors() appends to the descriptor_list.
            while (next_desc_start_handle <= service->end_handle &&
                   next_desc_start_handle < next_desc_end_handle &&
                   discover_next_descriptors(device, characteristic,
                                             next_desc_start_handle, next_desc_end_handle)) {

                // Get the most recently discovered descriptor, and then ask for descriptors
                // whose handles start after that descriptor's handle.
                const bleio_descriptor_obj_t *descriptor =
                    MP_OBJ_TO_PTR(characteristic->descriptor_list->items[characteristic->descriptor_list->len - 1]);
                next_desc_start_handle = descriptor->handle + 1;
            }
        }
    }

    // This event handler is no longer needed.
    ble_drv_remove_event_handler(discovery_on_ble_evt, device);

}
