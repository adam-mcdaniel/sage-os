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
static int init_counter = 10;
// static VirtioDevice *keyboard_device;
// static VirtioDevice *tablet_device;
static Vector *input_devices;  //we need to store multiple input devices
// static Ring *input_events;  //TODO: use the ring to buffer input events and also limit the number of events
// const int event_limit = 1000;   //limits number of events so we don't run out of memory

void input_device_init(VirtioDevice *device) {
    if(input_devices == NULL){
       input_devices = vector_new();
    }

    // init_counter++;
    // input_events = ring_new(event_limit);
    device_active_jobs = vector_new();
    // list_add(input_devices, device);
    debugf("Input device init done for device at %p\n", device->pcidev->ecam_header);
    device->ready = true;
    volatile VirtioInputConfig *config = virtio_get_input_config(device);
    debugf("Input device initialized\n");

    //Associate InputDevice with our VirtioDevice
    InputDevice *new_device;
    new_device->virtio_dev = device;
    vector_push_ptr(input_devices,new_device);

    //extra stuff just to see if we can see the input type
    debugf("Reading input config\n");
    get_input_device_config(device,VIRTIO_INPUT_CFG_ID_NAME,0,8);
    debugf("%.128s", config->string);
    get_input_device_config(device,VIRTIO_INPUT_CFG_ID_NAME,0,8);
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

//get an InputDevice, which holds the input events for that device
InputDevice *get_input_device_by_vdev(VirtioDevice *vdev){
    for(uint32_t i = 0; i < vector_size(input_devices);i++){
        InputDevice *curr_in_device = NULL;
        vector_get_ptr(input_devices, i, &curr_in_device);
        if(curr_in_device->virtio_dev == vdev){
            return curr_in_device;
        }

    }
    
    debugf("[Input] Input not found!");
    return NULL;
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
    if(init_counter != 0){//ignore first interrupts
        debugf("[INPUT HANDLER]ignore first interrupts!\n");
        init_counter--;
        return;
    }
    if (dev == NULL) {
        debugf("Input device not initialized\n");
        return;
    }
    if(!dev->ready){
        debugf("[INPUT HANDLER]Device not ready!\n");
    }
    if(virtio_get_device_id(dev) != VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_INPUT)){
        debugf("[INPUT HANDLER] Not an input device\n");
        return;
    }
    else{
        input_dev = get_input_device_by_vdev(dev);
    }

    VirtioDescriptor received_descriptors[MAX_DESCRIPTORS];
    uint16_t num_received = virtio_receive_descriptor_chain(dev, 0, received_descriptors, MAX_DESCRIPTORS, true);
    for (uint16_t i = 0; i < num_received; ++i) {
        volatile struct virtio_input_event *event_ptr = (volatile struct virtio_input_event *)received_descriptors[i].addr;
        uint32_t len = received_descriptors[i].len;

        if (len != sizeof(struct virtio_input_event)) {
            debugf("Received invalid input event size: %d\n", len);
            virtio_send_descriptor(dev, 0, received_descriptors[i], true);
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
