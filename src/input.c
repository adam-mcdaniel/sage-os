/*
Input Device Driver
*/

#include <input.h>

//use this like a queue
static Vector *input_active_jobs;
static VirtioDevice *input_device_keyboard, *input_device_tablet;
static Ring *input_events;
const int event_limit = 1000;   //limits number of events so we don't run out of memory


void input_device_init() {
    input_events = ring_new(1000);
    input_active_jobs = vector_new();
    VirtioDevice *input_device = virtio_get_input_device();
    debugf("input init done for device at %p\n", input_device->pcidev->ecam_header);
    input_device->ready = true;

    // volatile virtio_input_config *ic = (virtio_input_config *)input_device->common_cfg;
    volatile struct virtio_input_config *ic = get_input_config(input_device);
    debugf("Reading input config\n");
    ic->select = VIRTIO_INPUT_CFG_ID_NAME;
    ic->subsel = 0;
    debugf("%.128s", ic->string);
    ic->select = VIRTIO_INPUT_CFG_ID_DEVIDS;
    ic->subsel = 0;
    if (ic->ids.product == EV_KEY) {
        debugf("Found keyboard input device.\n");
        input_device_keyboard = input_device;
    }
    else if (ic->ids.product = EV_ABS) {
        debugf("Found tablet input device.\n");
        input_device_tablet = input_device;
    }
    else {
        debugf("Found an input device product id %d\n", ic->ids.product);
    }   
}

/*
Creates an event struct and gives it to the event device so it can fill it with an event
*/
uint32_t input_handle(VirtioDevice *dev) 
{
    if (!virtio_is_rng_device(dev)) {
        fatalf("[Input] Incorrect device provided\n");
    }

    if (!dev->ready) {
        fatalf("Input is not ready\n");
        return;
    }

    //create event struct
    virtio_input_event *new_in_event;
    ring_push(input_events,new_in_event);

    //give descriptor with our event struct in it
    VirtioDescriptor desc;
    desc.addr = kernel_mmu_translate((uintptr_t)new_in_event);
    desc.len = 8;   //size of event struct
    desc.flags = VIRTQ_DESC_F_WRITE;
    desc.next = 0;

    virtio_send_descriptor(dev, 0, desc, true);

    return ;
}

volatile struct virtio_input_config *get_input_config(VirtioDevice *device) {
    return (volatile struct virtio_input_config *)pci_get_device_specific_config(device->pcidev);
};