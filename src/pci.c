#include <pci.h>
#include <debug.h>

static volatile struct pci_ecam* pci_get_ecam(uint8_t bus, uint8_t device, uint8_t function, uint16_t reg) 
{
    debugf("Setting bus device reg...\n");
    uint64_t bus64 = bus & 0xff;
    uint64_t device64 = device & 0x1f;
    uint64_t function64 = function & 0x7;
    uint64_t reg64 = reg & 0x3ff;

    debugf("Setting address...\n");
    uint64_t address = ECAM_ADDR_START |
                        (bus64 << 20) |
                        (device64 << 15) |
                        (function64 << 12) |
                        (reg64 << 2);

    debugf("set address to: 0x%08lx\n", address);

    // Test read of the memory location
    uint32_t testValue = *((volatile uint32_t *) address);
    debugf("Value at 0x%08lx: 0x%08x\n", address, testValue);

    // Check if address is within the range
    if (address < ECAM_ADDR_START || address > ECAM_ADDR_END) {
        debugf("Invalid PCI ECAM Address: 0x%08lx\n", address);
        return NULL; 
    }

    debugf("ending pci_ecam..");
    return (struct pci_ecam *)address;
}



void pci_init(void)
{
    debugf("Starting PCI Initialization...\n");
    int bus, device, function;
    
    for (bus = 0; bus < MAX_BUSES; bus++) {
        for (device = 0; device < MAX_DEVICES; device++) {
            for (function = 0; function < MAX_FUNCTIONS; function++) {
                debugf("Checking bus %d, device %d, function %d...\n", bus, device, function);
                struct pci_ecam *ec = pci_get_ecam(bus, device, function, 0);
                if (!ec) {
                    debugf("address out of bounds?");
                    continue; // Address was out of bounds
                }
                debugf("Device at bus %d, device %d, function %d (MMIO @ 0x%08lx), class: 0x%04x\n",
                        bus, device, function, ec, ec->class_code);
                debugf("   Device ID    : 0x%04x, Vendor ID    : 0x%04x\n",
                        ec->device_id, ec->vendor_id);
            }
        }
    }
}




void pci_dispatch_irq(int irq)
{
    (void)irq;

    // An IRQ came from the PLIC, but recall PCI devices
    // share IRQs. So, you need to check the ISR register
    // of potential virtio devices.
}

