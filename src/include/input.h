#pragma once

#include <lock.h>
#include <virtio.h>
#include <input-event-codes.h>

#define INPUT_EVENT_BUFFER_SIZE 64

typedef enum virtio_input_config_select {
    VIRTIO_INPUT_CFG_UNSET = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO = 0x12,
} InputConfigSelect;


struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

typedef struct InputDevice {
    Mutex lock;
    VirtioDevice *virtio_dev;
    struct virtio_input_event event_buffer[INPUT_EVENT_BUFFER_SIZE];
    int head;
    int tail;
    int size;
} InputDevice;

void input_device_init();

void input_device_interrupt_handler(VirtioDevice *device);

// void set_input_device_config(VirtioDevice *device, uint8_t select, uint8_t subsel, uint8_t size);

// InputDevice *get_input_device(void);