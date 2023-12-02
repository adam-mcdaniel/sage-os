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

#define INPUT_DEBUG
#ifdef INPUT_DEBUG
#define debugf(...) debugf(__VA_ARGS__)
#else
#define debugf(...)
#endif

#define MAX_DESCRIPTORS 10

static Vector *device_active_jobs = NULL;
static InputDevice keyboard_dev = {0};
static InputDevice tablet_dev = {0};
// static Ring *input_events;  //TODO: use the ring to buffer input events and also limit the number of events
// const int event_limit = 1000;   //limits number of events so we don't run out of memory
static bool input_device_initialized = false;
void input_device_init(VirtioDevice *device) {
    IRQ_OFF();
    device_active_jobs = vector_new();
    volatile VirtioInputConfig *config = virtio_get_input_config(device);

    uint16_t prod_id = input_device_get_prod_id(config);
    InputDevice *input_dev = &keyboard_dev;
    switch (prod_id) {
    case EV_KEY:
        virtio_set_device_name(device, "Keyboard");
        device->is_keyboard = 1;
        device->is_tablet = 0;
        keyboard_dev.viodev = device;
        input_dev = &keyboard_dev;
        debugf("input_device_init: Input device initialized as keyboard\n");
        break;
    case EV_ABS:
        input_dev = &tablet_dev;
        virtio_set_device_name(device, "Tablet");
        device->is_keyboard = 1;
        device->is_tablet = 0;
        tablet_dev.viodev = device;
        debugf("input_device_init: Input device initialized as tablet\n");
        break;
    default:
        fatalf("input_device_init: Unsupported device\n", prod_id);
        break;
    }

    input_dev->buffer_head = 0;
    input_dev->buffer_tail = 0;
    input_dev->buffer_count = 0;
    device->ready = true;

    IRQ_ON();
    input_device_receive_buffer_init(input_dev);
    debugf("Input device init done for device at %p\n", device->pcidev->ecam_header);
}

void done_init() {
    input_device_initialized = true;
}

// // get an InputDevice, which holds the input events for that device
// InputDevice *get_input_device_by_vdev(VirtioDevice *vdev){
//     for(uint32_t i = 0; i < vector_size(input_devices);i++){
//         InputDevice *curr_in_device = NULL;
//         vector_get_ptr(input_devices, i, &curr_in_device);
//         if(curr_in_device->virtio_dev == vdev){
//             return curr_in_device;
//         }

//     }
    
//     debugf("[Input] Input not found!");
//     return NULL;
// }

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

uint16_t input_device_get_prod_id(volatile VirtioInputConfig *config) {
    config->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
    config->subsel = 0;
    return config->ids.product;
}

void input_device_isr(VirtioDevice* viodev) {
    if (!input_device_initialized) {
        warnf("input_device_isr: Input device not initialized\n");
    }
    debugf("input_device_isr: Starting ISR\n");
    
    if(!viodev->ready){
        fatalf("input_device_isr: Device not ready!\n");
    }

    volatile InputDevice *input_dev;
    if (viodev->is_keyboard) {
        input_dev = &keyboard_dev;
    } else if (viodev->is_tablet) {
        input_dev = &tablet_dev;
    } else {
        fatalf("input_device_isr: Unsupported input device\n");
    }

    uint16_t start_device_idx = viodev->device_idx;
    uint16_t num_received = 0;
    // Have to receive multiple descriptors
    // virtio_acquire_device(input_dev);
    while (virtio_has_received_descriptor(viodev, 0)) {
        // uint32_t id = viodev->device->ring[viodev->device_idx % queue_size].id;
        VirtioDescriptor received_desc;
        virtio_receive_descriptor_chain(viodev, 0, &received_desc, 1, true);
        VirtioInputEvent *event_ptr = (VirtioInputEvent *)received_desc.addr;
        debugf("About to read input event from descriptor at %p\n", received_desc.addr);

        uint32_t len = received_desc.len;
        if (len != sizeof(VirtioInputEvent)) {
            debugf("input_device_isr: Received invalid input event size: %d\n", len);
            // virtio_send_one_descriptor(viodev, 0, received_desc, true);
            continue;
        }

        // Copy event to local buffer
        // Add event to buffer
        // input_dev->event_buffer[input_dev->buffer_head] = *event_ptr;
        // input_dev->buffer_head = (input_dev->buffer_head + 1) % INPUT_EVENT_BUFFER_SIZE;
        // ++input_dev->buffer_count;

        
        debugf("input_device_isr: Input event received: type = 0x%x, code = 0x%x, value = 0x%x\n", event_ptr->type, event_ptr->code, event_ptr->value);


        if (input_dev->buffer_count < INPUT_EVENT_BUFFER_SIZE) {
            // IRQ_OFF();
            // memcpy(&input_dev->event_buffer[input_dev->buffer_tail], event_ptr, sizeof(VirtioInputEvent));
            // IRQ_ON();
            debugf("Writing to input buffer at %p\n", &input_dev->event_buffer[input_dev->buffer_tail]);
            ++input_dev->buffer_count;
            input_dev->event_buffer[input_dev->buffer_tail % INPUT_EVENT_BUFFER_SIZE] = *event_ptr;
            input_dev->buffer_head = (input_dev->buffer_head + 1) % INPUT_EVENT_BUFFER_SIZE;
            debugf("input_device_isr: Input event received: type = 0x%x, code = 0x%x, value = 0x%x\n", event_ptr->type, event_ptr->code, event_ptr->value);
        } else {
            debugf("input_device_isr: Input event buffer full, event dropped\n");
        }
        // virtio_send_one_descriptor(viodev, 0, received_desc, false);
        ++num_received;
    }
    // virtio_release_device(input_dev);

    // Sanity check
    debugf("input_device_isr: After receiving descriptors\n");
    debugf("  num_received = %d\n", num_received);
    debugf("  Starting device_idx = %d\n", start_device_idx);
    debugf("  Ending device_idx = %d\n", viodev->device_idx);
    debugf("  Ending device->idx = %d\n", viodev->device->idx);
    debugf("  input_dev->buffer_count = %d\n", input_dev->buffer_count);
}

InputDevice *input_device_get_keyboard() {
    if (!keyboard_dev.viodev || !keyboard_dev.viodev->ready) {
        warnf("input_device_get_keyboard: Invalid virtio device\n");
        return NULL;
    }

    return &keyboard_dev;
}

InputDevice *input_device_get_tablet() {
    if (!tablet_dev.viodev || !tablet_dev.viodev->ready) {
        warnf("input_device_get_tablet: Invalid virtio device\n");
        return NULL;
    }

    return &tablet_dev;
}

// Pop the input event from the event buffer ot input_dev to event.
// Return true if succeeded, false if failed.
bool input_device_buffer_pop(InputDevice *input_dev, VirtioInputEvent *event) {
    // Event buffer is empty
    if (input_dev->buffer_count == 0) {
        return false;
    }

    *event = input_dev->event_buffer[input_dev->buffer_tail];
    
    input_dev->buffer_tail = (input_dev->buffer_tail + 1) % INPUT_EVENT_BUFFER_SIZE;
    --input_dev->buffer_count;
    
    return true;
}
