#include <compiler.h>
#include <config.h>
#include <debug.h>
#include <stdint.h>
#include <stddef.h>
#include <lock.h>
#include <mmu.h>
#include <page.h>
#include <csr.h>
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
        // debugf("mmu_map: invalid argument");
        return false;
    }

    // debugf("mmu_map: page table at 0x%08lx\n", tab);
    // debugf("mmu_map: vaddr == 0x%08lx\n", vaddr);
    // debugf("mmu_map: paddr == 0x%08lx\n", paddr);

    const uint64_t vpn[] = {(vaddr >> ADDR_0_BIT) & 0x1FF, (vaddr >> ADDR_1_BIT) & 0x1FF,
                            (vaddr >> ADDR_2_BIT) & 0x1FF};
    // debugf("mmu_map: vpn = {%d, %d, %d}\n", vpn[0], vpn[1], vpn[2]);
    const uint64_t ppn[] = {(paddr >> ADDR_0_BIT) & 0x1FF, (paddr >> ADDR_1_BIT) & 0x1FF,
                            (paddr >> ADDR_2_BIT) & 0x3FFFFFF};

    int i;
    struct page_table *pt = tab;

    for (i = MMU_LEVEL_1G; i > lvl; i--) {
        unsigned long pte = pt->entries[vpn[i]];

        if (!is_valid(pte)) {
            // debugf("mmu_map: entry %d in page table at 0x%08lx is invalid\n", vpn[i], pt);
            struct page_table *new_pt = mmu_table_create();
            if (new_pt == NULL) {
                // debugf("mmu_map: mmu_table_create returned null");
                return false;
            }
            // debugf("mmu_map: create a new page table at 0x%08lx\n", new_pt);
            pt->entries[vpn[i]] = (unsigned long)new_pt >> 2 | PB_VALID;
            // debugf("mmu_map: set entry %d in page table at 0x%08lx as lvl %d branch to 0x%08lx\n", vpn[i], pt, i, new_pt);
        } else {
            // debugf("mmu_map: entry %d in page table at 0x%08lx is valid\n", vpn[i], pt);
        }
        pt = (struct page_table*)((pt->entries[vpn[i]] & ~0x3FF) << 2);
    }

    unsigned long ppn_leaf = ppn[2] << PTE_PPN2_BIT |
                             ppn[1] << PTE_PPN1_BIT |
                             ppn[0] << PTE_PPN0_BIT;
    
    // debugf("mmu_map: ppn_leaf == 0x%x\n", (ppn_leaf << 2));
    pt->entries[vpn[i]] = ppn_leaf | bits | PB_VALID;

    // debugf("mmu_map: set bits of address 0x%08lx to 0x%08lx\n", &pt->entries[vpn[i]], ppn_leaf | bits | PB_VALID);

    // debugf("mmu_map: set entry %d in page table at 0x%08lx as lvl %d leaf to 0x%08lx\n", vpn[i], pt, i, ppn_leaf << 2);
    return true;
}

void mmu_free(struct page_table *tab) 
{ 
    uint64_t entry; 
    int i; 

    if (tab == NULL) { 
        return; 
    } 

    for (i = 0; i < (PAGE_SIZE / 8); i++) { 
        entry = tab->entries[i]; 
        if (entry & PB_VALID) {
            mmu_free((struct page_table *)((entry & ~0x3FF) << 2)); // Recurse into the next level
        }
        tab->entries[i] = 0; 
    } 

    page_free(tab); 
    SFENCE_ALL();
}

uint64_t mmu_translate(const struct page_table *tab, uint64_t vaddr) 
{ 

    // debugf("mmu_translate: page table at 0x%016lx\n", tab);
    // debugf("mmu_translate: vaddr == 0x%016lx\n", vaddr);

    if (tab == NULL) { 
        // debugf("mmu_translate: tab == NULL\n");
        return MMU_TRANSLATE_PAGE_FAULT; 
    } 

    // Extract the virtual page numbers
    uint64_t vpn[] = {(vaddr >> ADDR_0_BIT) & 0x1FF, 
                      (vaddr >> ADDR_1_BIT) & 0x1FF, 
                      (vaddr >> ADDR_2_BIT) & 0x1FF};
    // debugf("mmu_translate: vpn[0] == 0x%03lx\n", vpn[0]);
    // debugf("mmu_translate: vpn[1] == 0x%03lx\n", vpn[1]);
    // debugf("mmu_translate: vpn[2] == 0x%03lx\n", vpn[2]);

    uint64_t lvl = MMU_LEVEL_1G;
    // Traverse the page table hierarchy using the virtual page numbers
    for (int i = MMU_LEVEL_1G; i >= MMU_LEVEL_4K; i--) {
        // Iterate through and print the page table entries
        // debugf("mmu_translate: tab->entries == 0x%08lx\n", tab->entries);
        for (int j = 0; j < (PAGE_SIZE / 8); j++) {
            if (tab->entries[j] & PB_VALID) {
                // debugf("mmu_translate: tab->entries[%x] == 0x%0lx\n", j, tab->entries[j]);
            }
        }

        if (!(tab->entries[vpn[i]] & PB_VALID)) {
            // debugf("mmu_translate: entry %x in page table at 0x%08lx is invalid\n", vpn[i], tab);
            return MMU_TRANSLATE_PAGE_FAULT; // Entry is not valid
        } else if (!is_leaf(tab->entries[vpn[i]])) {
            // debugf("mmu_translate: entry %x in page table at 0x%08lx is a branch to 0x%08lx\n", vpn[i], tab, (tab->entries[vpn[i]] & ~0x3FF) << 2);
            tab = (struct page_table *)((tab->entries[vpn[i]] & ~0x3FF) << 2);
        } else {
            // debugf("mmu_translate: entry %x in page table at 0x%08lx is a leaf\n", vpn[i], tab);
            lvl = i;
            break; // Entry is a leaf
        }
    }

    // debugf("mmu_translate: vaddr == 0x%08lx\n", vaddr);

    uint64_t page_mask = PAGE_SIZE_AT_LVL(lvl) - 1;
    
    // Extract the physical address from the final page table entry
    uint64_t paddr = ((tab->entries[vpn[lvl]] & ~0x3FF) << 2) & ~page_mask;

    uint64_t result = paddr | (vaddr & page_mask);
    // debugf("mmu_translate: paddr == 0x%08lx\n", result);

    return result; // Combine with the offset within the page
}

uint64_t kernel_mmu_translate(uint64_t vaddr) 
{ 
    return mmu_translate(kernel_mmu_table, vaddr); 
}

uint64_t mmu_map_range(struct page_table *tab, 
                       uint64_t start_virt, 
                       uint64_t end_virt, 
                       uint64_t start_phys,
                       uint8_t lvl, 
                       uint64_t bits)
{
    // debugf("mmu_map_range: page table at 0x%08lx\n", tab);
    start_virt            = ALIGN_DOWN_POT(start_virt, PAGE_SIZE_AT_LVL(lvl));
    start_phys            = ALIGN_DOWN_POT(start_phys, PAGE_SIZE_AT_LVL(lvl));
    end_virt              = ALIGN_UP_POT(end_virt, PAGE_SIZE_AT_LVL(lvl));
    uint64_t num_bytes    = end_virt - start_virt;
    // debugf("mmu_map_range: start_virt = 0x%08lx\n", start_virt);
    // debugf("mmu_map_range: start_phys = 0x%08lx\n", start_phys);
    // debugf("mmu_map_range: mapping = %d bytes\n", num_bytes);
    uint64_t pages_mapped = 0;

    uint64_t i;
    for (i = 0; i < num_bytes; i += PAGE_SIZE_AT_LVL(lvl)) {
        // debugf("mmu_map_range: mapping %d bytes for page %d\n", PAGE_SIZE_AT_LVL(lvl), i / PAGE_SIZE_AT_LVL(lvl));
        if (!mmu_map(tab, start_virt + i, start_phys + i, lvl, bits)) {
            break;
        }
        pages_mapped += 1;
    }
    // debugf("mmu_map_range: mapped %d pages\n", pages_mapped);
    SFENCE_ALL();
    return pages_mapped;
} 

// This function performs some basic sanity checks on the page table.
// For each level of the page table, it prints out the entries that are valid.
void debug_page_table(struct page_table *tab, uint8_t lvl) {
    // debugf("debug_page_table: debugging page table at 0x%016lx\n", tab);
    uint64_t page_mask = PAGE_SIZE_AT_LVL(lvl) - 1;

    for (uint64_t i=0; i < 512; i++) {
        // Is the entry a leaf?
        bool is_leaf = (tab->entries[i] & 0xE) != 0;

        // Is the entry valid?
        bool is_valid = tab->entries[i] & PB_VALID;

        // Is the entry a branch?
        bool is_branch = is_valid && !is_leaf;

        uint64_t paddr = ((tab->entries[i] & ~0x3FF) << 2) & ~page_mask;
        if (paddr == (uint64_t)tab) {
            continue;
        }

        uint64_t vaddr = paddr;
        if (is_valid && is_leaf) {
            // Confirm that we can translate the address
            uint64_t translated = mmu_translate(tab, vaddr);
            if (translated != paddr) {
                // debugf("debug_page_table: page table at 0x%08lx is invalid\n", tab);
                // debugf("debug_page_table: expected 0x%08lx, got 0x%08lx\n", paddr, translated);
                fatalf("debug_page_table: entry 0x%x in page table at 0x%08lx is invalid\n", i, tab);
            } else {
                // debugf("debug_page_table: page table at 0x%08lx is valid\n", tab);
            }
        } else if (is_branch && lvl > MMU_LEVEL_4K) {
            // Recurse into the next level
            // debugf("debug_page_table: entry %d in page table at 0x%08lx is a branch to 0x%08lx\n", i, tab, (tab->entries[i] & ~0x3FF) << 2);
            // debug_page_table((struct page_table *)((tab->entries[i] & ~0x3FF) << 2), lvl - 1);
        } else {
            // Invalid entry, confirm that it's all zeroes
            if (tab->entries[i] != 0) {
                // debugf("debug_page_table: page table at 0x%08lx is invalid\n", tab);
                // debugf("debug_page_table: expected all zeroes, got 0x%08lx\n", tab->entries[i]);
                fatalf("debug_page_table: entry 0x%x in page table at 0x%08lx is invalid\n", i, tab);
            }
        }
    }

    // debugf("debug_page_table: page table at 0x%08lx is valid\n", tab);
}
