#include <input.h>
#include <virtio.h>
#include <debug.h>
#include <kmalloc.h>
#include <sched.h>
#include <syscall.h>
#include <compiler.h>

#define MAX_DESCRIPTORS 10

static InputDevice *input_dev = NULL;

void input_device_init(InputDevice *input_dev, VirtioDevice *virtio_dev) {
    input_dev = kzalloc(sizeof(InputDevice));
    input_dev->virtio_dev = virtio_dev;

    if (input_dev->virtio_dev == NULL) {
        debugf("Failed to initialize input device: Virtio device not found\n");
        return;
    }

    // Initialize the input device here if necessary
    // ...

    debugf("Input device initialized\n");
}

void input_device_interrupt_handler(void) {
    if (input_dev == NULL) {
        debugf("Input device not initialized\n");
        return;
    }

    VirtioDescriptor received_descriptors[MAX_DESCRIPTORS];
    uint16_t num_received = virtio_receive_descriptor_chain(input_dev->virtio_dev, 0, received_descriptors, MAX_DESCRIPTORS, true);

    for (uint16_t i = 0; i < num_received; ++i) {
        volatile struct virtio_input_event *event_ptr = (volatile struct virtio_input_event *)received_descriptors[i].addr;
        uint32_t len = received_descriptors[i].len;

        if (len != sizeof(struct virtio_input_event)) {
            debugf("Received invalid input event size: %d\n", len);
            continue;
        }

        // Handle the input event here
        // ...

        // Send the descriptor back to the device
        virtio_send_descriptor(input_dev->virtio_dev, 0, received_descriptors[i], true);
    }
}

InputDevice *get_input_device(void) {
    return input_dev;
}
