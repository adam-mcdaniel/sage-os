#include <input.h>
#include <virtio.h>
#include <debug.h>
#include <kmalloc.h>
#include <sched.h>
#include <syscall.h>
#include <compiler.h>

#define MAX_DESCRIPTORS 10

//use this like a queue
static Vector *device_active_jobs;
static VirtioDevice *input_device;

void input_device_init() {
    device_active_jobs = vector_new();
    input_device = virtio_get_input_device();
    debugf("Input device init done for device at %p\n", input_device->pcidev->ecam_header);
    input_device->ready = true;
    debugf("Input device initialized\n");
}



void input_device_interrupt_handler(void) {
    if (input_device == NULL) {
        debugf("Input device not initialized\n");
        return;
    }

    VirtioDescriptor received_descriptors[MAX_DESCRIPTORS];
    uint16_t num_received = virtio_receive_descriptor_chain(input_device, 0, received_descriptors, MAX_DESCRIPTORS, true);

    for (uint16_t i = 0; i < num_received; ++i) {
        volatile struct virtio_input_event *event_ptr = (volatile struct virtio_input_event *)received_descriptors[i].addr;
        uint32_t len = received_descriptors[i].len;

        if (len != sizeof(struct virtio_input_event)) {
            debugf("Received invalid input event size: %d\n", len);
            continue;
        }

        // Copy event to local buffer
        InputDevice *input_dev = get_input_device();
        if (input_dev->size < INPUT_EVENT_BUFFER_SIZE) {
            memcpy(&input_dev->event_buffer[input_dev->tail], event_ptr, sizeof(struct virtio_input_event));
            input_dev->tail = (input_dev->tail + 1) % INPUT_EVENT_BUFFER_SIZE;
            input_dev->size++;
            debugf("Input event received: type=%d, code=%d, value=%d\n", event_ptr->type, event_ptr->code, event_ptr->value);
        } else {
            debugf("Input event buffer full, event dropped\n");
        }

        // Send the descriptor back to the device
        virtio_send_descriptor(input_device, 0, received_descriptors[i], true);
    }
}

InputDevice *get_input_device(void) {
    return input_device;
}
