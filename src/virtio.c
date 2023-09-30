#include <debug.h>
#include <virtio.h>
#include <util.h>
#include <vector.h>
#include <pci.h>

static Vector *virtio_devices = NULL;

/**
 * @brief Initialize the virtio system
 */
void virtio_init(void) {
    virtio_devices = vector_new();

    // for (int i = 0; i < pci_count_saved_devices(); i++) {
    // }
}

/**
 * @brief Virtio notification
 * @param viodev - virtio device to notify for
 * @param which_queue - queue number to notify
 */
void virtio_notify(VirtioDevice *viodev, uint16_t which_queue)
{
    uint16_t num_queues = viodev->common_cfg->num_queues;

    if (which_queue >= viodev->common_cfg->num_queues) {
        logf(LOG_ERROR, "virtio_notify: Provided queue number is too big...\n");
        return;
    }

    // Select the queue we are looking at
    viodev->common_cfg->queue_select = which_queue;

    // Determine offset for the given queue
    uint8_t bar_num = viodev->notify_cap->cap.bar;
    uint32_t offset = viodev->notify_cap->cap.offset;
    uint16_t queue_notify_off = viodev->common_cfg->queue_notify_off;
    uint32_t notify_off_multiplier = viodev->notify_cap->notify_off_multiplier;
    uint32_t bar = viodev->pcidev->ecam_header->type0.bar[bar_num];

    // FIXME: 
    uint16_t *notify = bar + BAR_NOTIFY_CAP(offset, queue_notify_off, notify_off_multiplier);

    // Write the queue's number to the notify register
    *notify = which_queue; 

    return;
};
