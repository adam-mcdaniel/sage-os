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
#include <virtio.h>
#include "input-event-codes.h"
#include <lock.h>
#include <mmu.h>
#include <virtio.h>
#include <compiler.h>
#include <csr.h>

// #define INPUT_DEBUG
#ifdef INPUT_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

#define MAX_DESCRIPTORS 100

static Vector *device_active_jobs = NULL;
static InputDevice keyboard_dev = {0};
static InputDevice tablet_dev = {0};
// static Ring *input_events;  //TODO: use the ring to buffer input events and also limit the number of events
// const int event_limit = 1000;   //limits number of events so we don't run out of memory
static int input_devices_initialized = 0;
void input_device_init(VirtioDevice *device) {
    device_active_jobs = vector_new();
    volatile VirtioInputConfig *config = virtio_get_input_config(device);

    uint16_t prod_id = input_device_get_prod_id(config, device->name);
    debugf("input_device_init: prod_id = 0x%x\n", prod_id);
    InputDevice *input_dev = NULL;
    switch (prod_id) {
    case EV_KEY:
        // virtio_set_device_name(device, "Keyboard");
        input_dev = &keyboard_dev;
        device->is_keyboard = 1;
        device->is_tablet = 0;
        keyboard_dev.viodev = device;
        break;
    case EV_ABS:
        // input_dev = &keyboard_dev;
        input_dev = &tablet_dev;
        device->is_keyboard = 0;
        device->is_tablet = 1;
        tablet_dev.viodev = device;
        break;
    default:
        fatalf("input_device_init: Unsupported input device\n");
        break;
    }

    device->ready = true;
    input_dev->buffer_head = 0;
    input_dev->buffer_tail = 0;
    input_dev->buffer_count = 0;
    input_devices_initialized++;

    input_device_receive_buffer_init(input_dev);
    debugf("Input device init done for device at %p\n", device->pcidev->ecam_header);
}

bool is_initialized() {
    return input_devices_initialized >= 2;
}

// At device init, populate the driver ring with receive buffers so we can receive events
void input_device_receive_buffer_init(InputDevice *input_dev) {
    for (int i = 0; i < INPUT_EVENT_BUFFER_SIZE; i++) {
        VirtioDescriptor recv_buf_desc;
        recv_buf_desc.addr = kernel_mmu_translate((uintptr_t)&input_dev->event_buffer[i]);
        recv_buf_desc.flags = VIRTQ_DESC_F_WRITE;
        recv_buf_desc.len = sizeof(VirtioInputEvent);
        recv_buf_desc.next = 0;
        virtio_send_one_descriptor(input_dev->viodev, 0, recv_buf_desc, true);
    }
}

void get_input_device_config(VirtioDevice *device, uint8_t select, uint8_t subsel, uint8_t size) {
    volatile VirtioInputConfig *config = virtio_get_input_config(device);
    config->select = select;
    debugf("set_input_device_config: select = 0x%x\n", config->select);
    config->subsel = subsel;
    debugf("set_input_device_config: subsel = 0x%x\n", config->subsel);
    debugf("%.128s\n", config->string);
}

uint16_t input_device_get_prod_id(volatile VirtioInputConfig *config, char *name) {
    config->select = VIRTIO_INPUT_CFG_ID_NAME;
    config->subsel = 0;
    printf("Name: %.128s", config->string);
    strncpy(name, config->string, 60);

    config->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
    config->subsel = 0;
    return config->ids.product;
}
void input_device_push_event(InputDevice *input_dev, VirtioInputEvent event) {
    if (input_dev == NULL) {
        warnf("input_device_push_event: Input device not initialized\n");
        return;
    } else if (event.type == 0 && event.code == 0 && event.value == 0) {
        debugf("input_device_push_event: Invalid input event: type = 0x%x, code = 0x%x, value = 0x%x\n", event.type, event.code, event.value);
        return;
    } else {
        debugf("Pushing event to %s\n", input_dev->viodev->name);
    }

    if (input_dev->buffer_count < INPUT_EVENT_BUFFER_SIZE) {
        ++input_dev->buffer_count;
        input_dev->event_buffer[input_dev->buffer_tail % INPUT_EVENT_BUFFER_SIZE] = event;
        input_dev->buffer_tail = (input_dev->buffer_tail + 1) % INPUT_EVENT_BUFFER_SIZE;
        debugf("input_device_isr: Input event received: type = 0x%x, code = 0x%x, value = 0x%x\n", event.type, event.code, event.value);
    } else {
        // Just wrap around the buffer
        input_dev->event_buffer[input_dev->buffer_tail % INPUT_EVENT_BUFFER_SIZE] = event;
        input_dev->buffer_tail = (input_dev->buffer_tail + 1) % INPUT_EVENT_BUFFER_SIZE;
        input_dev->buffer_head = (input_dev->buffer_head + 1) % INPUT_EVENT_BUFFER_SIZE;
        debugf("input_device_isr: Input event received: type = 0x%x, code = 0x%x, value = 0x%x\n", event.type, event.code, event.value);
    }
}

VirtioInputEvent input_device_get_next_event(InputDevice *input_dev) {
    if (!is_initialized()) {
        warnf("input_device_get_next_event: Input device not initialized\n");
        VirtioInputEvent event;
        event.type = 0;
        event.code = 0;
        event.value = 0;
        return event;
    }
    VirtioInputEvent event;
    if (input_dev == NULL) {
        fatalf("input_device_get_next_event: Input device not initialized\n");
    }

    if (input_dev->buffer_count <= 0) {
        // Get the next event from the head of the buffer
        // event = input_dev->event_buffer[input_dev->buffer_head];
        // input_dev->buffer_head = (input_dev->buffer_head + 1) % INPUT_EVENT_BUFFER_SIZE;
        // if (input_dev->buffer_count == 0) {
        //     input_dev->buffer_head = 0;
        //     input_dev->buffer_tail = 0;
        // } else {
        //     --input_dev->buffer_count;
        // }
        debugf("input_device_get_next_event: %.60s popped event: type = 0x%x, code = 0x%x, value = 0x%x\n", input_dev->viodev->name, event.type, event.code, event.value);
        memset(&event, 0, sizeof(VirtioInputEvent));
        event = input_dev->event_buffer[input_dev->buffer_head % INPUT_EVENT_BUFFER_SIZE];
        input_dev->buffer_head = 0;
        input_dev->buffer_tail = 0;
        event.type = 0;
        event.code = 0;
        event.value = 0;
        return event;
    } else {
        // Get the next event from input_device_isr: Received invalid input event: tthe head of the buffer
        event = input_dev->event_buffer[input_dev->buffer_head % INPUT_EVENT_BUFFER_SIZE];
        input_dev->buffer_head = (input_dev->buffer_head + 1) % INPUT_EVENT_BUFFER_SIZE;
        --input_dev->buffer_count;
        debugf("input_device_get_next_event: %.60s popped event: type = 0x%x, code = 0x%x, value = 0x%x\n", input_dev->viodev->name, event.type, event.code, event.value);
    }
    return event;
}

void handle_input(InputDevice *device) {
    if (device == NULL) {
        warnf("input_device_isr: Input device not initialized\n");
        return;
    }

    if (!is_initialized()) {
        warnf("input_device_isr: Input device not initialized\n");
        return;
    }
    VirtioDevice *viodev = device->viodev;
    
    if(!viodev->ready){
        warnf("input_device_isr: Device not ready!\n");
        return;
    }
    debugf("input_device_isr: Starting ISR for %.60s\n", viodev->name);
    

    uint16_t start_device_idx = viodev->device_idx;
    uint16_t num_received = 0;
    // Have to receive multiple descriptors
    // virtio_acquire_device(input_dev);
    while (virtio_has_received_descriptor(viodev, 0)) {
        // uint32_t id = viodev->device->ring[viodev->device_idx % queue_size].id;
        volatile VirtioDescriptor received_desc = virtio_receive_one_descriptor(viodev, 0, true);
        volatile VirtioInputEvent *event_ptr = (volatile VirtioInputEvent *)received_desc.addr;
        debugf("About to read input event from descriptor at %p\n", received_desc.addr);
        VirtioInputEvent event;
        if (event_ptr != NULL) {
            event = *event_ptr;
        }

        uint32_t len = received_desc.len;
        if (len != sizeof(VirtioInputEvent)) {
            // debugf("input_device_isr: Received invalid input event size: %d\n", len);
            virtio_send_one_descriptor(viodev, 0, received_desc, true);
            continue;
        }

        // if (event_ptr->type == 0 && event_ptr->code == 0 && event_ptr->value == 0) {
        //     debugf("input_device_isr: Received invalid input event: type = 0x%x, code = 0x%x, value = 0x%x\n", event_ptr->type, event_ptr->code, event_ptr->value);
        //     virtio_send_one_descriptor(viodev, 0, received_desc, true);
        //     continue;
        // }
        
        // volatile InputDevice *input_dev;
        if (viodev->is_keyboard) {
            // This is a key event
            // input_dev = &keyboard_dev;
            input_device_push_event(&keyboard_dev, event);
            // input_device_push_event(&tablet_dev, *event_ptr);
        } else {
            // This is a tablet event
            // input_dev = &tablet_dev;
            // input_device_push_event(&keyboard_dev, *event_ptr);
            input_device_push_event(&tablet_dev, event);
        }

        debugf("input_device_isr: Input %.60s event received: type = 0x%x, code = 0x%x, value = 0x%x\n", viodev->name, event_ptr->type, event_ptr->code, event_ptr->value);
        virtio_send_one_descriptor(viodev, 0, received_desc, true);
    }
}

void input_device_isr(VirtioDevice* viodev) {
    if (viodev == NULL) {
        warnf("input_device_isr: Input device not initialized\n");
        return;
    }

    if (viodev == keyboard_dev.viodev) {
        handle_input(&keyboard_dev);
    } else if (viodev == tablet_dev.viodev) {
        handle_input(&tablet_dev);
    } else {
        warnf("input_device_isr: Invalid virtio device\n");
        return;
    }
    // handle_input(&keyboard_dev);
    // handle_input(&tablet_dev);
}

// Pop the input event from the event buffer ot input_dev to event.
// Return true if succeeded, false if failed.
bool input_device_buffer_pop(InputDevice *input_dev, VirtioInputEvent *event) {
    if (!is_initialized()) {
        warnf("input_device_buffer_pop: Input device not initialized\n");
        return NULL;
    }

    if (!input_dev->viodev || !input_dev->viodev->ready) {
        warnf("input_device_buffer_pop: Invalid virtio device\n");
        return NULL;
    }

    // Event buffer is empty
    if (input_dev->buffer_count <= 0) {
        return false;
    }

    *event = input_dev->event_buffer[input_dev->buffer_tail % INPUT_EVENT_BUFFER_SIZE];    
    input_dev->buffer_tail = (input_dev->buffer_tail + 1) % INPUT_EVENT_BUFFER_SIZE;
    --input_dev->buffer_count;
    
    return true;
}


InputDevice *input_device_get_keyboard() {
    if (!is_initialized()) {
        warnf("input_device_get_tablet: Input device not initialized\n");
        return NULL;
    }
    // if (!keyboard_dev.viodev || !keyboard_dev.viodev->ready) {
    //     warnf("input_device_get_tablet: Invalid virtio device\n");
    //     return NULL;
    // }

    return &keyboard_dev;
}

InputDevice *input_device_get_tablet() {
    if (!is_initialized()) {
        warnf("input_device_get_tablet: Input device not initialized\n");
        return NULL;
    }
    // if (!tablet_dev.viodev || !tablet_dev.viodev->ready) {
    //     warnf("input_device_get_tablet: Invalid virtio device\n");
    //     return NULL;
    // }

    return &tablet_dev;
}

VirtioInputEvent keyboard_get_next_event() {
    debugf("keyboard_get_next_event: Getting next event\n");
    return input_device_get_next_event(&keyboard_dev);
}

VirtioInputEvent tablet_get_next_event() {
    debugf("tablet_get_next_event: Getting next event\n");
    return input_device_get_next_event(&tablet_dev);
}
