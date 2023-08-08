#include <compiler.h>
#include <config.h>
#include <lock.h>
#include <mmu.h>
#include <page.h>
#include <util.h>

#define PTE_PPN0_BIT 10
#define PTE_PPN1_BIT 19
#define PTE_PPN2_BIT 28

#define ADDR_0_BIT   12
#define ADDR_1_BIT   21
#define ADDR_2_BIT   30

struct page_table *mmu_table_create(void)
{
    return page_zalloc();
}

bool mmu_map(struct page_table *tab, uint64_t vaddr, uint64_t paddr, uint8_t lvl, uint64_t bits)
{
    const uint64_t vpn[] = {(vaddr >> ADDR_0_BIT) & 0x1FF, (vaddr >> ADDR_1_BIT) & 0x1FF,
                            (vaddr >> ADDR_2_BIT) & 0x1FF};
    const uint64_t ppn[] = {(paddr >> ADDR_0_BIT) & 0x1FF, (paddr >> ADDR_1_BIT) & 0x1FF,
                            (paddr >> ADDR_2_BIT) & 0x3FFFFFF};

    // Delete the following lines. These are here just to avoid
    // "unused" warnings.
    (void)vpn;
    (void)ppn;
    (void)bits;
    (void)tab;

    if (lvl > MMU_LEVEL_1G) {
        return false;
    }

    return false;
}

void mmu_free(struct page_table *tab)
{
    if (tab == NULL) {
        return;
    }
    // Unmap all pages and free pages.
}

uint64_t mmu_translate(const struct page_table *tab, uint64_t vaddr)
{
    uint64_t vpn[] = {(vaddr >> ADDR_0_BIT) & 0x1FF, (vaddr >> ADDR_1_BIT) & 0x1FF,
                      (vaddr >> ADDR_2_BIT) & 0x1FF};


    // Delete the following line. This is to get rid of the "unused" warning.
    (void)vpn;
    // Can't translate without a table.
    if (tab == NULL) {
        return MMU_TRANSLATE_PAGE_FAULT;
    }
    return MMU_TRANSLATE_PAGE_FAULT;
}

uint64_t mmu_map_range(struct page_table *tab, 
                       uint64_t start_virt, 
                       uint64_t end_virt, 
                       uint64_t start_phys,
                       uint8_t lvl, 
                       uint64_t bits)
{
    start_virt            = ALIGN_DOWN_POT(start_virt, PAGE_SIZE_AT_LVL(lvl));
    end_virt              = ALIGN_UP_POT(end_virt, PAGE_SIZE_AT_LVL(lvl));
    uint64_t num_bytes    = end_virt - start_virt;
    uint64_t pages_mapped = 0;

    uint64_t i;
    for (i = 0; i < num_bytes; i += PAGE_SIZE_AT_LVL(lvl)) {
        if (!mmu_map(tab, start_virt + i, start_phys + i, lvl, bits)) {
            break;
        }
        pages_mapped += 1;
    }
    return pages_mapped;
}
