#include <input.h>
#include <virtio.h>
#include <debug.h>
#include <kmalloc.h>
#include <string.h>
#include <syscall.h>
#include <compiler.h>
#include <ring.h>
#include <list.h>

#define MAX_DESCRIPTORS 10

//use this like a queue

static Vector *device_active_jobs;
// static List *input_devices;  //we need to store multiple input devices
// static Ring *input_events;  //TODO: use the ring to buffer input events and also limit the number of events
// const int event_limit = 1000;   //limits number of events so we don't run out of memory

void input_device_init(VirtioDevice *device) {
    // input_events = ring_new(event_limit);
    device_active_jobs = vector_new();
    // list_add(input_devices, device);
    debugf("Input device init done for device at %p\n", device->pcidev->ecam_header);
    device->ready = true;
    volatile VirtioInputConfig *config = virtio_get_input_config(device);
    debugf("Input device initialized\n");

    debugf("Reading input config\n");
    // set_input_device_config(device,VIRTIO_INPUT_CFG_ID_NAME,0,8);
    // debugf("%.128s", config->string);
    // set_input_device_config(device,VIRTIO_INPUT_CFG_ID_NAME,0,8);
    if (config->ids.product == EV_KEY) {
        debugf("Found keyboard input device.\n");
    }
    else if (config->ids.product = EV_ABS) {
        debugf("Found tablet input device.\n");
    }
    else {
        debugf("Found an input device product id %d\n", config->ids.product);
    }  

}

void set_input_device_config(VirtioDevice *device, uint8_t select, uint8_t subsel, uint8_t size) {
    volatile VirtioInputConfig *config = virtio_get_input_config(device);
    config->select = select;
    config->subsel = subsel;
    config->size   = size;
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
