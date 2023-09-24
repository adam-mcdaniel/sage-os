#include <pci.h>
#include <debug.h>
#include <mmu.h>

#define ECAM_ADDR_START 0x30000000
#define ECAM_ADDR_END   0x30FFFFFF

static void pci_configure_device(struct pci_ecam *device);
static void pci_configure_bridge(struct pci_ecam *bridge);


static uint8_t next_bus_number = 1;


static uint32_t next_mmio_address = 0x40000000;

static inline uint32_t pci_get_config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t addr = ECAM_ADDR_START | (bus << 20) | (device << 15) | (function << 12) | (offset & 0xFC);
    if (addr >= ECAM_ADDR_START && addr <= ECAM_ADDR_END) {
        return addr;
    } else {
        debugf("Warning: PCI address out of bounds!\n");
        return 0; 
    }
}

static inline bool pci_device_exists(uint16_t vendor_id) {
    return vendor_id != 0xFFFF;
}



static void pci_enumerate_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        for (uint8_t function = 0; function < 8; function++) {
            //debugf("Checking device at bus %d, device %d, function %d\n", bus, device, function);
            
            uint32_t config_addr = pci_get_config_address(bus, device, function, 0);
            //debugf("Memory content at config address 0x%08lx: 0x%08lx\n", config_addr, *(volatile uint32_t*)(uintptr_t)config_addr);
            volatile struct pci_ecam *header = (volatile struct pci_ecam *)(uintptr_t)config_addr;
            //debugf("Vendor ID read from config address 0x%08lx: 0x%04x\n", config_addr, header->vendor_id);

            //debugf("Config address: 0x%08lx\n", config_addr);
            //debugf("Vendor ID: 0x%04x\n", header->vendor_id);

            if (!pci_device_exists(header->vendor_id)) {
              //  debugf("No device found at bus %d, device %d, function %d\n", bus, device, function);
                continue;
            }

            //debugf("Device found with vendor ID: 0x%04x, header type: 0x%02x\n", header->vendor_id, header->header_type);


            if ((header->header_type & 0x7F) == 1) {
              //  debugf("Handle type 1 bridge\n");
                pci_configure_bridge(header);
            } else if ((header->header_type & 0x7F) == 0) {
                //debugf("Handle type 0 device\n");
                pci_configure_device(header);
                //print_vendor_specific_capabilities(header);
            }

        }
    }
}

static void pci_configure_bridge(struct pci_ecam *bridge) {
    //debugf("configuring bridge\n");
    bridge->type1.primary_bus_no = next_bus_number;
    bridge->type1.secondary_bus_no = ++next_bus_number;  
    bridge->type1.subordinate_bus_no = next_bus_number;  

    debugf("Set the memory address range\n"); 
    uint16_t base_address = (next_mmio_address & 0xFFFF0000) >> 16;
    bridge->type1.memory_base = base_address;
    bridge->type1.memory_limit = base_address + (0x01000000 >> 16);

    next_mmio_address += 0x01000000; 

    debugf("checking next_mmio_address value : 0x%08lx\n", next_mmio_address);
    pci_enumerate_bus(bridge->type1.secondary_bus_no);  
}


static void pci_configure_device(struct pci_ecam *device) {
    for (int i = 0; i < 6; i++) {
        //debugf("configuring device\n");
        
        
        device->type0.bar[i] = 0xFFFFFFFF;
        
       
        uint32_t bar_value = device->type0.bar[i];

        if (bar_value == 0) {
            continue;
        }

        
        uint32_t size = ~(bar_value & ~0xF) + 1;

        
        next_mmio_address = (next_mmio_address + size - 1) & ~(size - 1);

        
        device->type0.bar[i] = next_mmio_address;  
        next_mmio_address += size;

        
        if ((bar_value & 0x6) == 0x4) {
            i++;
            device->type0.bar[i] = next_mmio_address >> 32; 
            next_mmio_address += 4;  
        }
    }
}


/*void print_vendor_specific_capabilities(struct pci_ecam* header) {
    if (header->vendor_id != 0x1AF4) return;  

    uint8_t cap_pointer = header->type0.capes_pointer;  

    while (cap_pointer) {
        struct pci_cape* cape = (struct pci_cape*)((uintptr_t)header + cap_pointer);

        if (cape->id == 0x09) {  
            debugf("Found vendor-specific capability at offset: 0x%02x\n", cap_pointer);
        }

        cap_pointer = cape->next;  
    }
}*/


void pci_init(void) {
    next_bus_number = 0;
    next_mmio_address = 0x40000000;  

    pci_enumerate_bus(0);  
}




void pci_dispatch_irq(int irq)
{
    (void)irq;

    // An IRQ came from the PLIC, but recall PCI devices
    // share IRQs. So, you need to check the ISR register
    // of potential virtio devices.
}

