#include <compiler.h>
#include <config.h>
#include <pci.h>
#include <plic.h>
#include <printf.h>

// PLIC is connected to 0x0c00_0000 on 'virt' machine.
#define PLIC_BASE                0x0c000000UL
#define PLIC_PRIORITY_BASE       0x4
#define PLIC_PENDING_BASE        0x1000
#define PLIC_ENABLE_BASE         0x2000
#define PLIC_ENABLE_STRIDE       0x80
#define PLIC_CONTEXT_BASE        0x200000
#define PLIC_CONTEXT_STRIDE      0x1000

#define PLIC_MODE_MACHINE        0x0
#define PLIC_MODE_SUPERVISOR     0x1

#define PLIC_PRIORITY(interrupt) (PLIC_BASE + PLIC_PRIORITY_BASE * interrupt)

#define PLIC_THRESHOLD(hart, mode) \
    (PLIC_BASE + PLIC_CONTEXT_BASE + PLIC_CONTEXT_STRIDE * (2 * hart + mode))

#define PLIC_CLAIM(hart, mode) (PLIC_THRESHOLD(hart, mode) + 4)

#define PLIC_ENABLE(hart, mode) \
    (PLIC_BASE + PLIC_ENABLE_BASE + PLIC_ENABLE_STRIDE * (2 * hart + mode))

// PCI IRQ numbers
#define PLIC_PCI_INTA 32
#define PLIC_PCI_INTB 33
#define PLIC_PCI_INTC 34
#define PLIC_PCI_INTD 35

void plic_set_priority(int interrupt_id, char priority)
{
    uint32_t *base = (uint32_t *)PLIC_PRIORITY(interrupt_id);
    *base          = priority & 0x7;
}
void plic_set_threshold(int hart, char priority)
{
    uint32_t *base = (uint32_t *)PLIC_THRESHOLD(hart, PLIC_MODE_SUPERVISOR);
    *base          = priority & 0x7;
}
void plic_enable(int hart, int interrupt_id)
{
    uint32_t *base = (uint32_t *)PLIC_ENABLE(hart, PLIC_MODE_SUPERVISOR);
    base[interrupt_id / 32] |= 1UL << (interrupt_id % 32);
}
void plic_disable(int hart, int interrupt_id)
{
    uint32_t *base = (uint32_t *)PLIC_ENABLE(hart, PLIC_MODE_SUPERVISOR);
    base[interrupt_id / 32] &= ~(1UL << (interrupt_id % 32));
}
uint32_t plic_claim(int hart)
{
    uint32_t *base = (uint32_t *)PLIC_CLAIM(hart, PLIC_MODE_SUPERVISOR);
    return *base;
}
void plic_complete(int hart, int id)
{
    uint32_t *base = (uint32_t *)PLIC_CLAIM(hart, PLIC_MODE_SUPERVISOR);
    *base          = id;
}

void plic_handle_irq(int hart)
{
    int irq = plic_claim(hart);

    switch (irq) {
        // PCI devices 32-35
        case PLIC_PCI_INTA: [[fallthrough]]
        case PLIC_PCI_INTB: [[fallthrough]]
        case PLIC_PCI_INTC: [[fallthrough]]
        case PLIC_PCI_INTD:
#ifdef USE_PCI
            debugf("PLIC: IRQ %d\n", irq);
            pci_dispatch_irq(irq);
#endif
            break;
    }

    plic_complete(hart, irq);
}

void plic_init(void)
{
    plic_enable(0, PLIC_PCI_INTA);
    plic_enable(0, PLIC_PCI_INTB);
    plic_enable(0, PLIC_PCI_INTC);
    plic_enable(0, PLIC_PCI_INTD);

    plic_set_threshold(0, 1);

    plic_set_priority(PLIC_PCI_INTA, 3);
    plic_set_priority(PLIC_PCI_INTB, 3);
    plic_set_priority(PLIC_PCI_INTC, 3);
    plic_set_priority(PLIC_PCI_INTD, 3);
}
