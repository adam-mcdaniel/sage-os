#include <virtio.h>
#include <util.h>
#include <vector.h>
#include <pci.h>

static Vector *virtio_devices = NULL;

void virtio_init(void) {
    virtio_devices = vector_new();

    // for (int i = 0; i < vector_size(all_pci_devices); i++) {
    //     PCIDevice *pcidev = vector_get(all_pci_devices, i);
    //     if (pcidev->vendor_id == 0x1AF4) {
    //         VirtioDevice *viodev = (VirtioDevice *)kmalloc(sizeof(VirtioDevice));
    //         viodev->pcidev = pcidev;
    //         viodev->virtio_mmio = (VirtioMMIO *)pcidev->bar[0];
    //         vector_push(virtio_devices, viodev);
    //     }
    // }
}

void virtio_notify(VirtioDevice *viodev, uint16_t which_queue) {

}
