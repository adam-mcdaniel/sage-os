#include <pci.h>
#include <virtio.h>

#define MMIO_ECAM_BASE 0x30000000
static volatile struct pci_ecam *pcie_get_ecam(uint8_t bus,
                                                 uint8_t device,
                                                 uint8_t function,
                                                 uint16_t reg) 
{
    // Since we're shifting, we need to make sure we
    // have enough space to shift into.
    uint64_t bus64 = bus & 0xff;
    uint64_t device64 = device & 0x1f;
    uint64_t function64 = function & 0x7;
    uint64_t reg64 = reg & 0x3ff; 
    // Finally, put the address together
    return (struct pci_ecam *)
                 (MMIO_ECAM_BASE |     // base 0x3000_0000
                 (bus64 << 20) |       // bus number A[(20+n-1):20] (up to 8 bits)
                 (device64 << 15) |    // device number A[19:15]
                 (function64 << 12) |  // function number A[14:12]
                 (reg64 << 2));        // register number A[11:2]
}

void pci_init_device(struct pci_ecam *ecam, int bus){
    uint32_t i;
    for (i = 0; i < 6;i++){
        if((ecam->type0.bar[i] >> 1 ) & 3){
            //not 00
            uint64_t *bar = ecam->type0.bar + i;
            i += 1;

            *bar = -1UL;
            if (*bar = 0){
                continue;
            }

            uint64_t address_space = *bar & ~0xF;
            address_space = ~address_space;

            *bar =  VIRTIO_LAST_BAR + address_space;
            VIRTIO_LAST_BAR += address_space;

        }
    }

    ecam->command_reg |= 1 << 1;
}

static void pci_init_bridge(struct pci_ecam *ecam, int bus){
    static uint8_t sbn = 1;

    uint64_t addr_start = 0x40000000 | ((uint64_t)sbn << 20);
    uint64_t addr_end = addr_start + ((1 << 20) - 1);
    
    ecam->command_reg = COMMAND_REG_MMIO;
    ecam->type1.memory_base = addr_start >> 16;
    ecam->type1.memory_limit = addr_end >> 16;
    ecam->type1.prefetch_memory_base = addr_start >> 16;
    ecam->type1.prefetch_memory_limit = addr_end >> 16;
    ecam->type1.primary_bus_no = bus;
    ecam->type1.secondary_bus_no = sbn;
    ecam->type1.subordinate_bus_no = sbn;

    sbn += 1;
}

void pci_init(void)
{
    // Initialize and enumerate all PCI bridges and devices.
    debugf("ECAM_START = 0x%lx \n",ECAM_START);
    debugf("ecam struct size: %d\n", sizeof(struct pci_ecam));
    uint32_t bus;
    uint32_t device;
    for(bus = 0; bus < 256; bus++){
        for(device = 0; device < 32; device++){
            unsigned long cur_addr = ECAM_START + (((bus * 32) + device) * sizeof(struct pci_ecam));
            struct pci_ecam *ecam = cur_addr;
            // debugf("checking addr 0x%lx\n",cur_addr);
            if(ecam->vendor_id != 0xFFFF){  //something is connected
                //initialize based on device type
                if(ecam->header_type == 0){ 
                    pci_init_bridge(ecam,bus);
                    debugf("Bus loop idx is %d and device idx is %d\n",bus,device);
                    debugf("Found bridge at 0x%lx (calculated addr)\n",cur_addr);
                    debugf("Found bridge at 0x%p (pointer addr)\n",ecam);
                    debugf("secondary bus no. %d\n" , ecam->type1.secondary_bus_no);
                    
                }
                if(ecam->header_type == 1){
                    pci_init_device(ecam,bus);
                    debugf("Found device at 0x%lx\n",cur_addr);
                }
            }
        }
    }
    debugf("Busses enumerated!\n");
    // This should forward all virtio devices to the virtio drivers.
}

/**
 * @brief Dispatch an interrup to the PCI subsystem
 * @param irq - the IRQ number that interrupted
 */
void pci_dispatch_irq(int irq)
{
    // An IRQ came from the PLIC, but recall PCI devices
    // share IRQs. So, you need to check the ISR register
    // of potential virtio devices.

    // To determine which device did it, we have to look at
    // one of these two fields. If queue_interrupt is 1, that
    // means that the interrupt was caused by the device responding
    // to the VirtQ. If device_cfg_interrupt is 1, that means the
    // device changed its configuration, and that was the reason
    // the interrupt occurred.
    uint32_t bus;
    uint32_t device;
    // There are a MAXIMUM of 256 busses
    // although some implementations allow for fewer.
    // Minimum # of busses is 1
    for (bus = 0;bus < 256;bus++) {
        for (device = 0;device < 32;device++) {
            // EcamHeader is defined below
            unsigned long cur_addr = ECAM_START + (((bus * 32) + device) * sizeof(struct pci_ecam));
            struct pci_ecam *ecam = cur_addr;
            // Vendor ID 0xffff means "invalid"
            if (ecam->vendor_id == 0xffff) continue;
            // If we get here, we have a device.
            printf("Device at bus %d, device %d (MMIO @ 0x%08lx), class: 0x%04x\n",
                    bus, device, ecam, ecam->class_code);
            printf("   Device ID    : 0x%04x, Vendor ID    : 0x%04x\n",
                    ecam->device_id, ecam->vendor_id);



            // Make sure there are capabilities (bit 4 of the status register).
            if (0 != (ecam->status_reg & (1 << 4))) {
                unsigned char capes_next = ecam->common.capes_pointer;
                while (capes_next != 0) {
                    unsigned long cap_addr = (unsigned long)pcie_get_ecam(bus, device, 0, 0) + capes_next;
                    struct VirtioCapability *cap = (struct VirtioCapability *)cap_addr;
                    switch (cap->id) {
                        case 0x09: /* Vendor Specific */
                        {
                            /* ... */
                        }
                        break;
                        case 0x10: /* PCI-express */
                        {
                        }
                        break;
                        default:
                            logf("Unknown capability ID 0x%02x (next: 0x%02x)\n", cap->id, cap->next);
                        break;
                    }
                    capes_next = cap->next;
                }
            }
        }
    }


}
