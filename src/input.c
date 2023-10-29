#include <debug.h>
#include <csr.h>
#include <input.h>
#include <kmalloc.h>
#include <string.h>
#include <syscall.h>
#include <stdint.h>
#include <compiler.h>
#include <ring.h>
#include <list.h>
#include "input-event-codes.h"

#define MAX_DESCRIPTORS 10

//use this like a queue

static Vector *device_active_jobs;
static VirtioDevice *keyboard_device;
static VirtioDevice *tablet_device;
// static List *input_devices;  //we need to store multiple input devices
// static Ring *input_events;  //TODO: use the ring to buffer input events and also limit the number of events
// const int event_limit = 1000;   //limits number of events so we don't run out of memory

void input_device_init() {
    // input_events = ring_new(event_limit);
    device_active_jobs = vector_new();
    // list_add(input_devices, device);
    IRQ_OFF();
    VirtioDevice *input_device[2] = { virtio_get_nth_saved_device(0), virtio_get_nth_saved_device(1) };
    debugf("input_device_init: %d\n", virtio_is_input_device(input_device[0]));
    debugf("input_device_init: %d\n", virtio_is_input_device(input_device[1]));
    keyboard_device = input_device[0];
    tablet_device = input_device[1];
    keyboard_device->ready = true;
    tablet_device->ready = true;

    volatile VirtioInputConfig *keyboard_config = virtio_get_input_config(keyboard_device);
    volatile VirtioInputConfig *tablet_config = virtio_get_input_config(tablet_device);

    debugf("input_device_init:\n");
    debugf("  Keyboard\n");
    keyboard_config->select = VIRTIO_INPUT_CFG_ID_NAME;
    keyboard_config->subsel = 0;
    if (keyboard_config->size != 0) {
        debugf("    VIRTIO_INPUT_CFG_ID_NAME: %.128s\n", keyboard_config->string);
    } else {
        debugf("    VIRTIO_INPUT_CFG_ID_NAME: N/A\n");
    }
    keyboard_config->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
    keyboard_config->subsel = 0;
    if (keyboard_config->ids.product == EV_KEY) {
        debugf("    Product ID is EV_KEY\n");
    } else {
        debugf("    Product ID is %d\n", keyboard_config->ids.product);
    }


    debugf("  Tablet\n");
    tablet_config->select = VIRTIO_INPUT_CFG_ID_NAME;
    tablet_config->subsel = 0;
    if (tablet_config->size != 0) {
        debugf("    VIRTIO_INPUT_CFG_ID_NAME: %.128s\n", tablet_config->string);
    } else {
        debugf("    VIRTIO_INPUT_CFG_ID_NAME: N/A\n");
    }
    tablet_config->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
    tablet_config->subsel = 0;
    if (tablet_config->ids.product == EV_ABS) {
        debugf("    Product ID is EV_ABS\n");
    } else {
        debugf("    Product ID is %d\n", tablet_config->ids.product);
    }

    // IRQ_ON();
}

void get_input_device_config(VirtioDevice *device, uint8_t select, uint8_t subsel, uint8_t size) {
    volatile VirtioInputConfig *config = virtio_get_input_config(device);
    config->select = select;
    debugf("set_input_device_config: select = 0x%x\n", config->select);
    config->subsel = subsel;
    debugf("set_input_device_config: subsel = 0x%x\n", config->subsel);
    // config->size = size;
    // debugf("set_input_device_config: size = 0x%x\n", config->size);
    debugf("%.128s\n", config->string);
}

void input_device_interrupt_handler(VirtioDevice* dev) {
    InputDevice *input_dev = NULL;
    if (dev == NULL) {
        debugf("Input device not initialized\n");
        return;
    }
    if(virtio_get_device_id(dev) != VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_INPUT)){
        debugf("[INPUT HANDLER] Not an input device\n");
        return;
    }
    else{
        input_dev = (InputDevice *)dev;
    }

    VirtioDescriptor received_descriptors[MAX_DESCRIPTORS];
    uint16_t num_received = virtio_receive_descriptor_chain(input_dev, 0, received_descriptors, MAX_DESCRIPTORS, true);

    for (uint16_t i = 0; i < num_received; ++i) {
        volatile struct virtio_input_event *event_ptr = (volatile struct virtio_input_event *)received_descriptors[i].addr;
        uint32_t len = received_descriptors[i].len;

        if (len != sizeof(struct virtio_input_event)) {
            debugf("Received invalid input event size: %d\n", len);
            continue;
        }

        // Copy event to local buffer
        if (input_dev->size < INPUT_EVENT_BUFFER_SIZE) {
            memcpy(&input_dev->event_buffer[input_dev->tail], event_ptr, sizeof(struct virtio_input_event));
            input_dev->tail = (input_dev->tail + 1) % INPUT_EVENT_BUFFER_SIZE;
            input_dev->size++;
            debugf("Input event received: type=%d, code=%d, value=%d\n", event_ptr->type, event_ptr->code, event_ptr->value);
        } else {
            debugf("Input event buffer full, event dropped\n");
        }

        // Send the descriptor back to the device
        virtio_send_descriptor(dev, 0, received_descriptors[i], true);
    }
}

// InputDevice *get_input_device(void) {
//     return input_device;
// }
