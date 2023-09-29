#include <debug.h>
#include <mmu.h>
#include <pci.h>
#include <virtio.h>
#include <vector.h>

#define MMIO_ECAM_BASE 0x30000000
#define MMIO_ECAM_END   0x30FFFFFF
#define MEMORY_BARRIER() __asm__ volatile("" ::: "memory")

static uint8_t next_bus_number = 1;
static uint64_t next_mmio_address = 0x41000000;

struct Vector *all_pci_devices, *irq_pci_devices[4];

static void pci_configure_device(struct pci_ecam *device);
static void pci_configure_bridge(struct pci_ecam *bridge);

static volatile struct pci_ecam *pci_get_ecam(uint8_t bus,
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

static inline uint32_t pci_get_config_address(uint8_t bus, uint8_t device, uint8_t offset)
{
    uint32_t addr = MMIO_ECAM_BASE | (bus << 20) | (device << 15) | (offset & 0xFC);
    if (addr >= MMIO_ECAM_BASE && addr <= MMIO_ECAM_END) {
        return addr;
    } else {
        debugf("Warning: PCI address out of bounds!\n");
        return 0; 
    }
}

static inline bool pci_device_exists(uint16_t vendor_id)
{
    return vendor_id != 0xFFFF;
}

static void pci_enumerate_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        //debugf("Checking device at bus %d, device %d, function %d\n", bus, device, function);
            
        uint32_t config_addr = pci_get_config_address(bus, device, 0);
        //debugf("Memory content at config address 0x%08lx: 0x%08lx\n", config_addr, *(volatile uint32_t*)(uintptr_t)config_addr);
        volatile struct pci_ecam *header = (volatile struct pci_ecam *)(uintptr_t)config_addr;
        //debugf("Vendor ID read from config address 0x%08lx: 0x%04x\n", config_addr, header->vendor_id);

        //debugf("Config address: 0x%08lx\n", config_addr);
        //debugf("Vendor ID: 0x%04x\n", header->vendor_id);

        if (!pci_device_exists(header->vendor_id)) {
            // debugf("No device found at bus %d, device %d, function %d\n", bus, device, function);
            continue;
        }

        //debugf("Device found with vendor ID: 0x%04x, header type: 0x%02x\n", header->vendor_id, header->header_type);


        if ((header->header_type & 0x7F) == 1) {
            pci_configure_bridge(header);
        } else if ((header->header_type & 0x7F) == 0) {
            pci_configure_device(header);
            print_vendor_specific_capabilities(header);
        }
    }
}

static void pci_configure_bridge(struct pci_ecam *bridge)
{
    bridge->type1.primary_bus_no = next_bus_number;
    bridge->type1.secondary_bus_no = ++next_bus_number;  
    bridge->type1.subordinate_bus_no = next_bus_number;  

    uint32_t base_address = next_mmio_address;
    uint32_t end_address = base_address + 0x01000000 - 1;  // Adjust for 16MB

    bridge->type1.memory_base = base_address >> 16;
    bridge->type1.memory_limit = end_address >> 16;
    bridge->type1.prefetch_memory_base = base_address >> 16;
    bridge->type1.prefetch_memory_limit = end_address >> 16;

    pci_enumerate_bus(bridge->type1.secondary_bus_no);  
    next_mmio_address += 0x01000000;
}

static void pci_configure_device(struct pci_ecam *device)
{
    // Push the device into the appropriate vector.
    // The appropriate vector is the (bus + slot) % 4 for the device.
    // This is to simplify IRQ handling.
    // Get the bus number from the device address.
    uint8_t bus = ((uintptr_t)device >> 20) & 0xF;
    // Get the slot number from the device address.
    uint8_t slot = ((uintptr_t)device >> 15) & 0x1F;
    // The vector index is the sum of the bus and slot numbers, modulo 4.
    uint32_t vector_idx = (bus + slot) % 4;
    debugf("Pushing device at bus %d, slot %d into vector %d\n", bus, slot, vector_idx);
    vector_push(all_pci_devices, (uint64_t)device);
    vector_push(irq_pci_devices[vector_idx], (uint64_t)device);

    for (int i = 0; i < 6; i++) {
        // Disable the device before modifying the BAR
        device->command_reg &= ~(1 << 1);  // Clear Memory Space bit
        MEMORY_BARRIER();

        device->type0.bar[i] = 0xFFFFFFFF;
        MEMORY_BARRIER();
        
        uint32_t bar_value = device->type0.bar[i];

        // BAR not writable
        if (bar_value == 0) {
            continue;
        }
        
        uint32_t size = ~(bar_value & ~0xF) + 1;
        next_mmio_address = (next_mmio_address + size - 1) & ~(size - 1);
    
        device->type0.bar[i] = next_mmio_address;  
        next_mmio_address += size;

        // 64-bit BAR
        if ((bar_value & 0x6) == 0x4) {
            i++;
            device->type0.bar[i] = next_mmio_address >> 32; 
            next_mmio_address += 4;  
        }

        // Re-enable the device after modifying the BAR
        device->command_reg |= (1 << 1);
        MEMORY_BARRIER();
    }
}

void print_vendor_specific_capabilities(struct pci_ecam* header)
{
    if (header->vendor_id != 0x1AF4) return;  

    uint8_t cap_pointer = header->type0.capes_pointer;  
    debugf("Vendor specific capabilities pointer: 0x%02x\n", cap_pointer);
    while (cap_pointer) {
        struct pci_cape* cape = (struct pci_cape*)((uintptr_t)header + cap_pointer);

        if (cape->id == 0x09) {  

            struct VirtioCapability* virtio_cap = (struct VirtioCapability*)cape;
            switch (virtio_cap->type) {
            case VIRTIO_PCI_CAP_COMMON_CFG: /* 1 */
                debugf("  Common configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_NOTIFY_CFG: /* 2 */
                debugf("  Notify configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_ISR_CFG:    /* 3 */
                debugf("  ISR configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_DEVICE_CFG: /* 4 */
                debugf("  Device configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_PCI_CFG:    /* 5 */
                debugf("  PCI configuration access capability at offset: 0x%02x\n", cap_pointer);
                break;
            default:
                fatalf("Unknown virtio capability %d at offset: 0x%02x\n", virtio_cap->type, cap_pointer);
                break;
            }
        }

        cap_pointer = cape->next;  
    }
}

void pci_init(void)
{
    next_bus_number = 0;
    next_mmio_address = 0x41000000;
    all_pci_devices = vector_new();
    for (int i=0; i<4; i++) {
        irq_pci_devices[i] = vector_new();
    }

    pci_enumerate_bus(0);  
}

/**
 * @brief Dispatch an interrupt to the PCI subsystem
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

    // IRQ#=32+(bus+slot)mod4
    uint32_t vector_idx = irq - 32;

    

    // Check all devices in the vector
    for (int i=0; i<vector_size(irq_pci_devices[vector_idx]); i++) {
        struct pci_ecam *device;
        vector_get_ptr(irq_pci_devices[vector_idx], i, device);
        // Check if the device is a virtio device
        
        if (!pci_device_exists(device->vendor_id)) {
            // debugf("No device found at bus %d, device %d, function %d\n", bus, device, function);
            continue;
        }

        //debugf("Device found with vendor ID: 0x%04x, header type: 0x%02x\n", header->vendor_id, header->header_type);

        // if (device->vendor_id != 0x1AF4) return;  

        uint8_t cap_pointer = device->type0.capes_pointer;  
        debugf("Vendor specific capabilities pointer: 0x%02x\n", cap_pointer);
        while (cap_pointer) {
            struct pci_cape* cape = (struct pci_cape*)((uintptr_t)device + cap_pointer);

            if (cape->id == 0x09) {  

                struct VirtioCapability* virtio_cap = (struct VirtioCapability*)cape;
                switch (virtio_cap->type) {
                case VIRTIO_PCI_CAP_COMMON_CFG: /* 1 */
                    break;
                case VIRTIO_PCI_CAP_NOTIFY_CFG: /* 2 */
                    break;
                case VIRTIO_PCI_CAP_ISR_CFG:    /* 3 */
                    debugf("  ISR configuration capability at offset: 0x%02x\n", cap_pointer);
                    struct VirtioPciIsrCap* isr_cap = (struct VirtioPciIsrCap*)virtio_cap;
                    debugf("    ISR capability: 0x%08x\n", isr_cap->isr_cap);
                    if (isr_cap->queue_interrupt) {
                        debugf("    Queue interrupt\n");
                    }
                    if (isr_cap->device_cfg_interrupt) {
                        debugf("    Device configuration interrupt\n");
                    }
                    break;
                case VIRTIO_PCI_CAP_DEVICE_CFG: /* 4 */
                    break;
                case VIRTIO_PCI_CAP_PCI_CFG:    /* 5 */
                    break;
                default:
                    fatalf("Unknown virtio capability %d at offset: 0x%02x\n", virtio_cap->type, cap_pointer);
                    break;
                }
            }

            cap_pointer = cape->next;  
        }
    }
}
