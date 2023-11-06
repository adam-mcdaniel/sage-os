#pragma once

#include <lock.h>
#include <input-event-codes.h>
#include <stdint.h>
#include <virtio.h>

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


typedef struct VirtioInputEvent {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} VirtioInputEvent;

typedef struct InputDevice {
    Mutex lock;
    VirtioDevice *viodev;
    VirtioInputEvent event_buffer[INPUT_EVENT_BUFFER_SIZE];
    int head;
    int tail;
    int buffer_size;
} InputDevice;

void input_device_init(VirtioDevice *device);
void get_input_device_config(VirtioDevice *device, uint8_t select, uint8_t subsel, uint8_t size);
void input_device_receive_buffer_init(InputDevice *input_dev);
uint16_t input_device_get_prod_id(volatile VirtioInputConfig *config);
void input_device_isr(VirtioDevice *device);
// InputDevice *get_input_device_by_vdev(VirtioDevice *vdev);
