#include <compiler.h>
#include <config.h>
#include <csr.h>
#include <kmalloc.h>
#include <list.h>
#include <lock.h>
#include <sbi.h>  // sbi_xxx()
#include <symbols.h>
#include <util.h>  // strcmp
#include <debug.h>
#include <mmu.h>
#include <page.h>

// Global MMU table for the kernel. This is used throughout
// the kernel.
// Defined in src/include/mmu.h
struct page_table *kernel_mmu_table;

static void init_systems(void)
{
    void plic_init(void);
    plic_init();
    void page_init(void);
    page_init();

#ifdef USE_MMU
    struct page_table *pt = mmu_table_create();
    kernel_mmu_table = pt;
    // Map memory segments for our kernel
    mmu_map_range(pt, sym_start(text), sym_end(heap), sym_start(text), MMU_LEVEL_1G,
                  PB_READ | PB_WRITE | PB_EXECUTE);
    // PLIC
    mmu_map_range(pt, 0x0C000000, 0x0C2FFFFF, 0x0C000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // PCIe ECAM
    mmu_map_range(pt, 0x30000000, 0x30FFFFFF, 0x30000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // PCIe MMIO
    mmu_map_range(pt, 0x40000000, 0x4FFFFFFF, 0x40000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);

    // TODO: turn on the MMU when you've written the src/mmu.c functions
    CSR_WRITE("satp", SATP_KERNEL); 
    SFENCE_ALL();
#endif

#ifdef USE_HEAP
    void heap_init(void);
    void sched_init(void);
    void *kmalloc(uint64_t size);
    void *kcalloc(uint64_t elem, uint64_t size);
    void kfree(void *ptr);
    void util_connect_galloc(void *(*malloc)(uint64_t size),
                             void *(*calloc)(uint64_t elem, uint64_t size),
                             void (*free)(void *ptr));
    util_connect_galloc(kmalloc, kcalloc, kfree);
    heap_init();
#endif
#ifdef USE_PCI
    pci_init();
#endif
#ifdef USE_VIRTIO
    virtio_init();
#endif
}

static const char *hart_status_values[] = {"NOT PRESENT", "STOPPED", "STARTING", "RUNNING"};
#ifdef RUN_INTERNAL_CONSOLE
static void console(void);
#endif
void main(unsigned int hart)
{
    // Initialize the page allocator
    // Allocate and zero the kernel's page table.

    // Kind of neat to see our memory mappings to ensure they make sense.
    logf(LOG_INFO, "[[ MEMORY MAPPINGS ]]\n");
    logf(LOG_INFO, "  [TEXT]  : 0x%08lx -> 0x%08lx\n", sym_start(text), sym_end(text));
    logf(LOG_INFO, "  [BSS]   : 0x%08lx -> 0x%08lx\n", sym_start(bss), sym_end(bss));
    logf(LOG_INFO, "  [RODATA]: 0x%08lx -> 0x%08lx\n", sym_start(rodata), sym_end(rodata));
    logf(LOG_INFO, "  [DATA]  : 0x%08lx -> 0x%08lx\n", sym_start(data), sym_end(data));
    logf(LOG_INFO, "  [STACK] : 0x%08lx -> 0x%08lx\n", sym_start(stack), sym_end(stack));
    logf(LOG_INFO, "  [HEAP]  : 0x%08lx -> 0x%08lx\n", sym_start(heap), sym_end(heap));

    logf(LOG_INFO, "[[ HART MAPPINGS ]]\n");
    for (unsigned int i = 0; i < MAX_ALLOWABLE_HARTS; i++) {
        if (i == hart) {
            logf(LOG_INFO, "  [HART#%d]: %s (this HART).\n", i, hart_status_values[sbi_hart_get_status(i)]);
        }
        else {
            logf(LOG_INFO, "  [HART#%d]: %s.\n", i, hart_status_values[sbi_hart_get_status(i)]);
        }
    }

    // Initialize all submodules here, including PCI, VirtIO, Heap, etc.
    // Many will require the MMU, so write those functions first.
    init_systems();

    // Now that all submodules are initialized, you need to schedule the init process
    // and the idle processes for each HART.
    logf(LOG_INFO, "Congratulations! You made it to the OS! Going back to sleep.\n");
    logf(LOG_INFO, 
        "The logf function in the OS uses sbi_putchar(), so this means ECALLs from S-mode are "
        "working!\n");
    logf(LOG_INFO, 
        "If you don't remember, type CTRL-a, followed by x to exit. Make sure your CAPS LOCK is "
        "off.\n");
    // Below is just a little shell that demonstrates the sbi_getchar and
    // how the console works.

    // This is defined above main()
#ifdef RUN_INTERNAL_CONSOLE
    console();
#else
    extern uint32_t *elfcon;
    Process *con = process_new(PM_USER);
    if (!elf_load(con, elfcon)) {
        logf(LOG_INFO, "PANIC: Could not load init.\n");
        WFI_LOOP();
    }
    sched_add(con);
    con->state = PS_RUNNING;
    sched_invoke(0);
#endif
}

#ifdef RUN_INTERNAL_CONSOLE
ATTR_NORET static void console(void)
{
    const int BUFFER_SIZE = 56;
    int at                = 0;
    char input[BUFFER_SIZE];
    logf(LOG_TEXT, "> ");
    do {
        char c;
        // Recall that sbi_getchar() will return -1, 0xff, 255
        // if the receiver is empty.
        if ((c = sbi_getchar()) != 0xff) {
            if (c == '\r' || c == '\n') {
                if (at > 0) {
                    input[at] = '\0';
                    if (!strcmp(input, "quit")) {
                        logf(LOG_TEXT, "\nShutting down...\n\n");
                        sbi_poweroff();
                    }
                    else if (!strcmp(input, "fatal")) {
                        logf(LOG_TEXT, "\n");
                        fatalf("Testing fatal error @ %lu.\nHanging HART...\n", sbi_rtc_get_time());
                        logf(LOG_ERROR, "If I get here, fatal didn't work :'(.\n");
                    }
                    else if (!strcmp(input, "heap")) {
                        logf(LOG_TEXT, "\n");
                        void heap_print_stats(void);
                        heap_print_stats();
                    }
                    else {
                        logf(LOG_TEXT, "\nUnknown command '%s'\n", input);
                    }
                    at = 0;
                }
                logf(LOG_TEXT, "\n> ");
            }
            else if (c == 127) {
                // BACKSPACE
                if (at > 0) {
                    logf(LOG_TEXT, "\b \b");
                    at -= 1;
                }
            }
            else if (c == 0x1B) {
                // Escape sequence
                char esc1 = sbi_getchar();
                char esc2 = sbi_getchar();
                if (esc1 == 0x5B) {
                    switch (esc2) {
                        case 0x41:
                            logf(LOG_INFO, "UP\n");
                            break;
                        case 0x42:
                            logf(LOG_INFO, "DOWN\n");
                            break;
                        case 0x43:
                            logf(LOG_INFO, "RIGHT\n");
                            break;
                        case 0x44:
                            logf(LOG_INFO, "LEFT\n");
                            break;
                    }
                }
            }
            else {
                if (at < (BUFFER_SIZE - 1)) {
                    input[at++] = c;
                    logf(LOG_TEXT, "%c", c);
                }
            }
        }
        else {
            // We can WFI here since interrupts are enabled
            // for the UART.
            WFI();
        }
    } while (1);
}
#endif
