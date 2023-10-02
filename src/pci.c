#include <debug.h>
#include <mmu.h>
#include <pci.h>
#include <virtio.h>
#include <vector.h>
#include <kmalloc.h>

#define MMIO_ECAM_BASE 0x30000000
#define MMIO_ECAM_END  0x30FFFFFF

// These contain pointers to the common configurations for each device.
// The `all_pci_devices` vector contains all devices, while the
// `irq_pci_devices` vector contains devices that share an IRQ number (32, 33, 34, and 35).
// These vectors contain the pointers to the devices in the ECAM address space.
static struct Vector *all_pci_devices, *irq_pci_devices[4];

static inline bool pci_device_exists(uint16_t vendor_id)
{
    return vendor_id != 0xFFFF;
}

// Is this a virtio device?
bool pci_is_virtio_device(PCIDevice *dev) {
    return dev->ecam_header->vendor_id == 0x1AF4;
}

// Find the saved PCI device with the given vendor and device ID.
// This will retrieve the bookkeeping structure for the PCI device
// maintained by the OS.
PCIDevice *pci_find_saved_device(uint16_t vendor_id, uint16_t device_id) {
    debugf("Searching for device with vendor ID: 0x%04x, device ID: 0x%04x\n", vendor_id, device_id);
    // Iterate through the devices
    for (uint32_t i=0; i<vector_size(all_pci_devices); i++) {
        // Check if the device has the given vendor and device ID
        PCIDevice *pcidev = pci_get_nth_saved_device(i);
        debugf("Checking device with vendor ID: 0x%04x, device ID: 0x%04x\n", pcidev->ecam_header->vendor_id, pcidev->ecam_header->device_id);
        if (pcidev->ecam_header->vendor_id == vendor_id && pcidev->ecam_header->device_id == device_id) {
            return pcidev;
        }
    }
    // If we get here, we didn't find the device
    debugf("No device found with vendor ID: 0x%04x, device ID: 0x%04x\n", vendor_id, device_id);
    return NULL;
}

// Get the nth PCI capability for the PCI device. This is used with `0x9` as the type
// to enumerate all of the several Virtio capabilities for a PCI device with the Virtio
// vendor ID.
volatile struct pci_cape *pci_get_capability(PCIDevice *device, uint8_t type, uint8_t nth) {
    // Get the header for the device
    volatile struct pci_ecam *header = device->ecam_header;
    // Get the offset of the first capability
    uint8_t cap_pointer = header->type0.capes_pointer;
    // Count the number of capabilities we've seen
    uint8_t count = 0;
    
    // While the capability pointer is not zero
    while (cap_pointer) {
        // Get the capability at the offset
        volatile struct pci_cape* cape = (struct pci_cape*)((uintptr_t)header + cap_pointer);
        // If the capability ID matches the type we're looking for
        if (cape->id == type) {
            // If we've seen the nth capability, return it
            if (count++ == nth) {
                return cape;
            }
        }
        // Otherwise, continue to the next capability
        cap_pointer = cape->next;  
    }
    return NULL;
}

// Get a virtio capability for a given device by the virtio capability's type.
// For the common configuration capability, use `VIRTIO_PCI_CAP_COMMON_CFG`.
// For the notify capability, use `VIRTIO_PCI_CAP_NOTIFY_CFG`.
// For the ISR capability, use `VIRTIO_PCI_CAP_ISR_CFG`.
// For the device configuration capability, use `VIRTIO_PCI_CAP_DEVICE_CFG`.
// For the PCI configuration access capability, use `VIRTIO_PCI_CAP_PCI_CFG`.
volatile struct VirtioCapability *pci_get_virtio_capability(PCIDevice *device, uint8_t virtio_cap_type) {
    // Iterate through the first 10 capabilities
    for (uint8_t i=0; i<10; i++) {
        // Get the capability
        volatile struct pci_cape *cape = pci_get_capability(device, 0x09, i);
        volatile struct VirtioCapability *virtio_cap = (struct VirtioCapability *)cape;
        // If the capability isnt NULL and the type matches, return it
        if (virtio_cap && virtio_cap->type == virtio_cap_type) {
            return virtio_cap;
        }
    }
    // If we get here, we didn't find the capability
    debugf("No virtio capability found with type %d\n", virtio_cap_type);
    return NULL;
}

// Return the number of bookkeeping PCI devices saved by the OS.
uint64_t pci_count_saved_devices(void) {
    return vector_size(all_pci_devices);
}

// Count how many devices are listening for the given IRQ.
uint64_t pci_count_irq_listeners(uint8_t irq) {
    uint32_t vector_idx = irq - 32;
    return vector_size(irq_pci_devices[vector_idx]);
}

// Get the bus number for the given PCI device.
uint8_t pci_get_bus_number(PCIDevice *dev) {
    return ((uintptr_t)dev->ecam_header >> 20) & 0xF;
}

// Get the slot number for the given PCI device.
uint8_t pci_get_slot_number(PCIDevice *dev) {
    return ((uintptr_t)dev->ecam_header >> 15) & 0x1F;
}

// Save the PCI device for bookkeeping. This will save some
// information about the device for quick access later.
PCIDevice *pci_save_device(PCIDevice device) {
    // Allocate some memory for the device's bookkeeping structure
    PCIDevice *pcidev = (PCIDevice *)kmalloc(sizeof(PCIDevice));
    // Record the device's ECAM header
    pcidev->ecam_header = device.ecam_header;
    // Store the device in the all devices vector
    vector_push_ptr(all_pci_devices, pcidev);
    // Store the device in the appropriate IRQ vector
    uint8_t bus = pci_get_bus_number(pcidev);
    uint8_t slot = pci_get_slot_number(pcidev);
    debugf("Saving device with vendor ID: 0x%04x, device ID: 0x%04x\n", device.ecam_header->vendor_id, device.ecam_header->device_id);
    debugf("  Bus: %d, slot: %d\n", bus, slot);
    uint32_t vector_idx = (bus + slot) % 4;
    vector_push_ptr(irq_pci_devices[vector_idx], pcidev);
    // Return the device's bookkeeping structure in memory
    return pcidev;
}

// Get the nth saved PCI device structure kept by the OS.
PCIDevice *pci_get_nth_saved_device(uint16_t n) {
    PCIDevice *pcidev;
    vector_get_ptr(all_pci_devices, n, &pcidev);
    return pcidev;
}

// Get the device responsible for a given IRQ.
PCIDevice *pci_find_device_by_irq(uint8_t irq) {
    uint32_t vector_idx = irq - 32;

    // Check all devices in the vector
    for (uint32_t i=0; i<vector_size(irq_pci_devices[vector_idx]); i++) {
        // Get the nth PCI device listening for the IRQ
        PCIDevice *device;
        vector_get_ptr(irq_pci_devices[vector_idx], i, &device);
        // If the device is a Virtio device, check the Virtio ISR status
        if (!pci_is_virtio_device(device)) continue;  
        
        // Confirm that the device exists
        if (!pci_device_exists(device->ecam_header->vendor_id)) {
            continue;
        }

        // Get the Virtio ISR status
        volatile struct VirtioPciIsrCap *isr = pci_get_virtio_isr_status(device);

        // Check if the device's configuration has changed
        if (isr->device_cfg_interrupt) {
            debugf("Device configuration interrupt from device 0x%04x\n", device->ecam_header->device_id);
            return device;
        }

        // Check if the device's queue has an interrupt
        if (isr->queue_interrupt) {
            debugf("Device queue interrupt from device 0x%04x\n", device->ecam_header->device_id);
            return device;
        }
    }
    debugf("No device found with IRQ %d\n", irq);
    return NULL;
}
// Get the common configuration capability for the given virtio device.
volatile struct VirtioPciCommonCfg *pci_get_virtio_common_config(PCIDevice *device) {
    return (struct VirtioPciCommonCfg *)pci_get_virtio_capability(device, VIRTIO_PCI_CAP_COMMON_CFG);
}
// Get the notify capability for the given virtio device.
volatile struct VirtioPciNotifyCap *pci_get_virtio_notify_capability(PCIDevice *device) {
    return (struct VirtioPciNotifyCap *)pci_get_virtio_capability(device, VIRTIO_PCI_CAP_NOTIFY_CFG);
}
// Get the ISR capability for the given virtio device.
volatile struct VirtioPciIsrCap *pci_get_virtio_isr_status(PCIDevice *device) {
    return (struct VirtioPciIsrCap *)pci_get_virtio_capability(device, VIRTIO_PCI_CAP_ISR_CFG);
}

static uint8_t next_bus_number = 1;
static uint64_t next_mmio_address = 0x41000000;

static void pci_configure_device(volatile struct pci_ecam *device);
static void pci_configure_bridge(volatile struct pci_ecam *bridge);

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
static void pci_enumerate_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        volatile struct pci_ecam *header = (volatile struct pci_ecam *)(uintptr_t)pci_get_config_address(bus, device, 0);

        if (!pci_device_exists(header->vendor_id)) {
            // debugf("No device found at bus %d, device %d, function %d\n", bus, device, function);
            continue;
        }


        if ((header->header_type & 0x7F) == 1) {
            pci_configure_bridge(header);
        } else if ((header->header_type & 0x7F) == 0) {
            pci_configure_device(header);
            // print_vendor_specific_capabilities(PCIDevice);
            PCIDevice *device = pci_find_saved_device(header->vendor_id, header->device_id);
            print_vendor_specific_capabilities(device);
        }
    }
}

static void pci_configure_bridge(volatile struct pci_ecam *bridge)
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

static void pci_configure_device(volatile struct pci_ecam *device)
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
    // debugf("Pushing device at bus %d, slot %d into vector %d\n", bus, slot, vector_idx);
    // vector_push(all_pci_devices, (uint64_t)device);
    // vector_push(irq_pci_devices[vector_idx], (uint64_t)device);
    PCIDevice pcidev;
    pcidev.ecam_header = device;
    pci_save_device(pcidev);

    for (int i = 0; i < 6; i++) {
        // Disable the device before modifying the BAR
        device->command_reg &= ~(1 << 1);  // Clear Memory Space bit

        device->type0.bar[i] = 0xFFFFFFFF;
        
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
    }
}

void print_vendor_specific_capabilities(PCIDevice *pcidevice)
{
    if (!pci_is_virtio_device(pcidevice)) return;  
    struct pci_ecam *header = pcidevice->ecam_header;

    uint8_t cap_pointer = header->type0.capes_pointer;  
    debugf("Vendor specific capabilities with offset 0x%02x\n", cap_pointer);
    debugf("  Common configuration capability at: 0x%08x\n", pci_get_virtio_common_config(pcidevice));
    debugf("  Notify configuration capability at: 0x%08x\n", pci_get_virtio_notify_capability(pcidevice));
    debugf("  ISR configuration capability at: 0x%08x\n", pci_get_virtio_isr_status(pcidevice));
    debugf("  Device configuration capability at: 0x%08x\n", pci_get_virtio_capability(pcidevice, 0x04));
    debugf("  PCI configuration access capability at: 0x%08x\n", pci_get_virtio_capability(pcidevice, 0x05));
    /*
    // Old method of printing capabilities
    if (header->vendor_id != 0x1AF4) return;  

    uint8_t cap_pointer = header->type0.capes_pointer;  
    debugf("Vendor specific capabilities pointer: 0x%02x\n", cap_pointer);
    while (cap_pointer) {
        struct pci_cape* cape = (struct pci_cape*)((uintptr_t)header + cap_pointer);

        if (cape->id == 0x09) {  

            struct VirtioCapability* virtio_cap = (struct VirtioCapability*)cape;
            switch (virtio_cap->type) {
            case VIRTIO_PCI_CAP_COMMON_CFG:
                debugf("  Common configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_NOTIFY_CFG:
                debugf("  Notify configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_ISR_CFG:
                debugf("  ISR configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_DEVICE_CFG:
                debugf("  Device configuration capability at offset: 0x%02x\n", cap_pointer);
                break;
            case VIRTIO_PCI_CAP_PCI_CFG:
                debugf("  PCI configuration access capability at offset: 0x%02x\n", cap_pointer);
                break;
            default:
                fatalf("Unknown virtio capability %d at offset: 0x%02x\n", virtio_cap->type, cap_pointer);
                break;
            }
        }

        cap_pointer = cape->next;  
    }
    */
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

    debugf("PCI devices: %d\n", pci_count_saved_devices());
    debugf("PCI devices sharing IRQ 32: %d\n", pci_count_irq_listeners(32));
    debugf("PCI devices sharing IRQ 33: %d\n", pci_count_irq_listeners(33));
    debugf("PCI devices sharing IRQ 34: %d\n", pci_count_irq_listeners(34));
    debugf("PCI devices sharing IRQ 35: %d\n", pci_count_irq_listeners(35));
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
    // uint32_t vector_idx = irq - 32;
    PCIDevice *pcidevice = pci_find_device_by_irq(irq);
    debugf("PCI device with IRQ %d: 0x%04x\n", irq, pcidevice->ecam_header->device_id);
    if (pcidevice == NULL) {
        debugf("No PCI device found with IRQ %d\n", irq);
        return;
    }
    // Is this a virtio device?
    if (pci_is_virtio_device(pcidevice)) { 
        // Access through ecam_header
        VirtioDevice *virtdevice = virtio_get_by_device(pcidevice);

        //rng device
        if(virtdevice->pcidev->ecam_header->device_id == VIRTIO_PCI_DEVICE_ID(VIRTIO_PCI_DEVICE_ENTROPY)){
            rng_job_done(virtdevice);
        }
        
    }
}
