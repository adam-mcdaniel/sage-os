/**
 * @file pci.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief PCI structures and routines.
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vector.h>


// PciEcam follows the structure of the PCI ECAM in the
// PCIe manual.
struct pci_ecam {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command_reg;
    uint16_t status_reg;
    uint8_t revision_id;
    uint8_t prog_if;
    union {
        uint16_t class_code;
        struct {
            uint8_t class_subcode;
            uint8_t class_basecode;
        };
    };
    uint8_t cacheline_size;
    uint8_t latency_timer;
    // The type of the header (type 0 - device, type 1 - pci-to-pci bridge, type 2 - cardbus bridge)
    uint8_t header_type;
    uint8_t bist;
    union {
        struct {
            uint32_t bar[6];
            uint32_t cardbus_cis_pointer;
            uint16_t sub_vendor_id;
            uint16_t sub_device_id;
            uint32_t expansion_rom_addr;
            uint8_t capes_pointer;
            uint8_t reserved0[3];
            uint32_t reserved1;
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint8_t min_gnt;
            uint8_t max_lat;
        } type0;
        struct {
            uint32_t bar[2];
            uint8_t primary_bus_no;
            uint8_t secondary_bus_no;
            uint8_t subordinate_bus_no;
            uint8_t secondary_latency_timer;
            uint8_t io_base;
            uint8_t io_limit;
            uint16_t secondary_status;
            uint16_t memory_base;
            uint16_t memory_limit;
            uint16_t prefetch_memory_base;
            uint16_t prefetch_memory_limit;
            uint32_t prefetch_base_upper;
            uint32_t prefetch_limit_upper;
            uint16_t io_base_upper;
            uint16_t io_limit_upper;
            uint8_t capes_pointer;
            uint8_t reserved0[3];
            uint32_t expansion_rom_addr;
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint16_t bridge_control;
        } type1;
        struct {
            uint32_t reserved0[9];
            uint8_t capes_pointer;
            uint8_t reserved1[3];
            uint32_t reserved2;
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint8_t reserved3[2];
        } common;
    };
};

typedef struct PCIDevice {
    // A pointer to the PCI device's ECAM header.
    struct pci_ecam *ecam_header;
} PCIDevice;


// Get the bus number for a given PCI device.
uint8_t pci_get_bus_number(PCIDevice *dev);

// Get the slot number for a given PCI device.
uint8_t pci_get_slot_number(PCIDevice *dev);

// Find a saved device by vendor and device ID.
PCIDevice *pci_find_saved_device(uint16_t vendor_id, uint16_t device_id);

// Save the device to the `all_pci_devices` vector. This will allow us to
// find the device later.
PCIDevice *pci_save_device(PCIDevice device);

// Get a saved device by index.
PCIDevice *pci_get_nth_saved_device(uint16_t n);

// Find the device for a given IRQ. This will check the `queue_interrupt`
// bit of the status register to see if the device is the one that
// interrupted.
PCIDevice *pci_find_device_by_irq(uint8_t irq);

// Count all the saved devices connected to the PCI port.
uint64_t pci_count_saved_devices(void);

// Count all the devices connected to PCI listening to a given IRQ.
uint64_t pci_count_irq_listeners(uint8_t irq);

// This will enumerate through the capabilities structure of the PCI device.
// This will return the nth capability of the given type. If the capability
// is not found, then NULL is returned.
struct pci_cape* pci_get_capability(PCIDevice *dev, uint8_t type, uint8_t nth);

/// Get a virtio capability for a given PCI device by the virtio capability's type.
/// If this is zero, it will get the common configuration capability. If this is
/// one, it will get the notify capability. If this is two, it will get the ISR
/// capability.
struct VirtioCapability *pci_get_virtio_capability(PCIDevice *device, uint8_t virtio_cap_type);
/// Get the common configuration structure for a given virtio device connected to PCI.
struct VirtioPciCommonCfg *pci_get_virtio_common_config(PCIDevice *device);
/// Get the notify configuration structure for a given virtio device connected to PCI.
struct VirtioPciNotifyCfg *pci_get_virtio_notify_capability(PCIDevice *device);
/// Get the interrupt service routine structure for a given virtio device connected to PCI.
struct VirtioPciISRStatus *pci_get_virtio_isr_status(PCIDevice *device);

struct pci_cape {
    uint8_t id;
    uint8_t next;
};

// #define PCI_IS_64_BIT_BAR(dev, barno) (0b100 == ((dev)->ecam->type0.bar[barno] & 0b110))

#define STATUS_REG_CAP                (1 << 4)
#define COMMAND_REG_MMIO              (1 << 1)
#define COMMAND_REG_BUSMASTER         (1 << 2)
static const uint64_t ECAM_START   = 0x30000000;
static const uint64_t VIRTIO_START = 0x40000000;
static uint64_t VIRTIO_LAST_BAR = 0x40000000;

/**
 * @brief Initialize the PCI subsystem
 *
 */
void pci_init(void);

/**
 * @brief Dispatch an interrupt to the PCI subsystem
 * @param irq - the IRQ number that interrupted
 */
void pci_dispatch_irq(int irq);

