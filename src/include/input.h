#pragma once

#include "virtio.h"

#define INPUT_EVENT_BUFFER_SIZE 64

enum virtio_input_config_select {
    VIRTIO_INPUT_CFG_UNSET     = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME   = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS   = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO  = 0x12,
};

struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
};

typedef struct InputDevice {
    VirtioDevice *virtio_dev;
    struct virtio_input_event event_buffer[INPUT_EVENT_BUFFER_SIZE];
    int head;
    int tail;
    int size;
} InputDevice;

void input_device_init(void);

int input_device_read_event(InputDevice *input_device, struct virtio_input_event *event);

void input_device_interrupt_handler(void);

InputDevice *get_input_device(void);