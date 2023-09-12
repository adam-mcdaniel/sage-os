#include <compiler.h>
#include <config.h>
#include <debug.h>
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

// Check the valid bit of a page table entry.
static inline bool is_valid(unsigned long pte)
{
    return pte & 1UL;
}

// Check if a page table entry is a leaf, return false if it's a branch.
static inline bool is_leaf(unsigned long pte)
{
    return (pte & 0xE) != 0;
}

bool mmu_map(struct page_table *tab, uint64_t vaddr, uint64_t paddr, uint8_t lvl, uint64_t bits)
{
    if (tab == NULL || lvl > MMU_LEVEL_1G || (bits & 0xE) == 0) {
        return false;
    }

    const uint64_t vpn[] = {(vaddr >> ADDR_0_BIT) & 0x1FF, (vaddr >> ADDR_1_BIT) & 0x1FF,
                            (vaddr >> ADDR_2_BIT) & 0x1FF};
    const uint64_t ppn[] = {(paddr >> ADDR_0_BIT) & 0x1FF, (paddr >> ADDR_1_BIT) & 0x1FF,
                            (paddr >> ADDR_2_BIT) & 0x3FFFFFF};

    int i;
    struct page_table *pt = tab;

    for (i = MMU_LEVEL_1G; i > lvl; i -= 1) {
        unsigned long pte = pt->entries[vpn[i]];

        if (!is_valid(pte)) {
            debugf("mmu_map: entry %d in page table at 0x%08lx is invalid\n", vpn[i], pt);
            pt = mmu_table_create();
            if (pt == NULL) {
                debugf("mmu_map: mmu_table_create returned null");
                return false;
            }
            pt->entries[vpn[i]] = (unsigned long) pt >> 2 | PB_VALID;
            debugf("mmu_map: create a new page table at 0x%08lx\n", pt);
            debugf("mmu_map: set entry %d as lvl %d branch in new page table", vpn[i], i);
        }
        
        pt = (struct page_table *)((pt->entries[vpn[i]] & 0x3FF) << 2);
        debugf("mmu_map: lvl %d page table is at 0x%08lx\n", i - 1, pt);
    }

    pt->entries[vpn[i]] = ppn[2] << PTE_PPN2_BIT |
                          ppn[1] << PTE_PPN1_BIT |
                          ppn[0] << PTE_PPN0_BIT |
                          bits |
                          PB_VALID;

    debugf("mmu_map: set entry %d as lvl %d leaf in page table at 0x%08lx\n", vpn[i], i, pt);

    return true;
}

void mmu_free(struct page_table *tab) 
{ 
    uint64_t entry; 
    int i; 

    if (tab == NULL) { 
        return; 
    } 

    for (i = 0; i < (PAGE_SIZE / 8); i += 1) { 
        entry = tab->entries[i]; 
        if (entry & PB_VALID) {
            mmu_free((struct page_table *)(entry & ~0xFFF)); // Recurse into the next level
        }
        tab->entries[i] = 0; 
    } 

    page_free(tab); 
}

uint64_t mmu_translate(const struct page_table *tab, uint64_t vaddr) 
{ 
    int i; 

    if (tab == NULL) { 
        return MMU_TRANSLATE_PAGE_FAULT; 
    } 

    // Extract the virtual page numbers
    uint64_t vpn[] = {(vaddr >> ADDR_0_BIT) & 0x1FF, 
                      (vaddr >> ADDR_1_BIT) & 0x1FF, 
                      (vaddr >> ADDR_2_BIT) & 0x1FF};

    // Traverse the page table hierarchy using the virtual page numbers
    for (i = MMU_LEVEL_1G; i >= MMU_LEVEL_4K; i--) {
        if (!(tab->entries[vpn[i]] & PB_VALID)) {
            return MMU_TRANSLATE_PAGE_FAULT; // Entry is not valid
        }
        tab = (struct page_table *)(tab->entries[vpn[i]] & ~0xFFF);
    }

    // Extract the physical address from the final page table entry
    uint64_t paddr = tab->entries[vpn[MMU_LEVEL_4K]] & ~0xFFF;
    return paddr | (vaddr & (PAGE_SIZE - 1)); // Combine with the offset within the page
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

