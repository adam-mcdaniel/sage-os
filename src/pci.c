#include <pci.h>

void pci_init(void)
{
    // Initialize and enumerate all PCI bridges and devices.

    // This should forward all virtio devices to the virtio drivers.
}

void pci_dispatch_irq(int irq)
{
    (void)irq;

    // An IRQ came from the PLIC, but recall PCI devices
    // share IRQs. So, you need to check the ISR register
    // of potential virtio devices.
}

