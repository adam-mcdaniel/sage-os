#include <pci.h>

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

            *bar =  VIRTIO_START + address_space;
            VIRTIO_START += address_space;

        }
    }

    ecam->command_reg |= 1 << 1;
}

static void pci_init_bridge(struct pci_ecam *ecam, int bus){
    static uint8_t sbn = 1;
    ecam->type1.primary_bus_no = bus;
    ecam->type1.secondary_bus_no = sbn;
    ecam->type1.subordinate_bus_no = sbn;

    
    sbn += 1;
    // ecam->type1.memory_base = ;
}

void pci_init(void)
{
    // Initialize and enumerate all PCI bridges and devices.
    debugf("ECAM_START = 0x%lx \n",ECAM_START);
    debugf("ecam struct size: %d\n", sizeof(struct pci_ecam));
    uint32_t bus;
    uint32_t device;
    for(bus = 0; bus < 255; bus++){
        for(device = 0; device < 32; device++){
            unsigned long cur_addr = ECAM_START + ((bus + device) * sizeof(struct pci_ecam));
            struct pci_ecam *ecam = cur_addr;
            if(ecam->vendor_id != 0xFFFF){  //something is connected
                
                // debugf("checking addr 0x%d\n",cur_addr);
                //initialize based on device type
                if(ecam->header_type == 0){ 
                    pci_init_bridge(ecam,bus);
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

void pci_dispatch_irq(int irq)
{
    (void)irq;

    // An IRQ came from the PLIC, but recall PCI devices
    // share IRQs. So, you need to check the ISR register
    // of potential virtio devices.
}

