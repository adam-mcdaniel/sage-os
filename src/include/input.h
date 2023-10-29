/*
* Input Driver header
*/

#include <input-event-codes.h>
#include <stdint.h>
#include <virtio.h>
#include <pci.h>
#include <vector.h>
#include <ring.h>

/* structures */
enum virtio_input_config_select {
    VIRTIO_INPUT_CFG_UNSET     = 0x00,
    VIRTIO_INPUT_CFG_ID_NAME   = 0x01,
    VIRTIO_INPUT_CFG_ID_SERIAL = 0x02,
    VIRTIO_INPUT_CFG_ID_DEVIDS = 0x03,
    VIRTIO_INPUT_CFG_PROP_BITS = 0x10,
    VIRTIO_INPUT_CFG_EV_BITS   = 0x11,
    VIRTIO_INPUT_CFG_ABS_INFO  = 0x12,
};

typedef struct virtio_input_absinfo {
    uint32_t min;
    uint32_t max;
    uint32_t fuzz;
    uint32_t flat;
    uint32_t res;
}virtio_input_absinfo;

typedef struct virtio_input_devids {
    uint16_t bustype;
    uint16_t vendor;
    uint16_t product;
    uint16_t version;
}virtio_input_devids;

typedef struct virtio_input_config {
    uint8_t select;
    uint8_t subsel;
    uint8_t size;
    uint8_t reserved[5];
    union {
        char string[128];
        uint8_t bitmap[128];
        struct virtio_input_absinfo abs;
        struct virtio_input_devids ids;
    };
}virtio_input_config;

typedef struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
}virtio_input_event;

/* methods */
volatile struct virtio_input_config *get_input_config(VirtioDevice *device);

void input_device_init();