#include <virtio.h>

void virtio_init(void);
void virtio_notify(VirtioDevice *viodev, uint16_t which_queue);
