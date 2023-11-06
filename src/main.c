#include <compiler.h>
#include <config.h>
#include <csr.h>
#include <gpu.h>
#include <kmalloc.h>
#include <list.h>
#include <lock.h>
#include <sbi.h>  // sbi_xxx()
#include <symbols.h>
#include <util.h>  // strcmp
#include <debug.h>
#include <mmu.h>
#include <page.h>
#include <csr.h>
#include <trap.h>
#include <block.h>
#include <rng.h>
#include <vfs.h>

// Global MMU table for the kernel. This is used throughout
// the kernel.
// Defined in src/include/mmu.h
struct page_table *kernel_mmu_table;

static void init_systems(void)
{
    void plic_init(void);
    plic_init();
    debugf("plic_init() done\n");
    void page_init(void);
    page_init();
    debugf("page_init() done\n");

#ifdef USE_MMU
    struct page_table *pt = mmu_table_create();
    kernel_mmu_table = pt;

    debugf("Kernel page table at %p\n", pt);
    // Map memory segments for our kernel
    debugf("Mapping kernel segments\n");
    mmu_map_range(pt, sym_start(text), sym_end(heap), sym_start(text), MMU_LEVEL_1G,
                  PB_READ | PB_WRITE | PB_EXECUTE);
    // PLIC
    debugf("Mapping PLIC\n");
    mmu_map_range(pt, 0x0C000000, 0x0C2FFFFF, 0x0C000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // PCIe ECAM
    debugf("Mapping PCIe ECAM\n");
    mmu_map_range(pt, 0x30000000, 0x3FFFFFFF, 0x30000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // PCIe MMIO
    debugf("Mapping PCIe MMIO\n");
    mmu_map_range(pt, 0x40000000, 0x5FFFFFFF, 0x40000000, MMU_LEVEL_2M, PB_READ | PB_WRITE);
    // debugf("Testing MMU translation\n");
    // debugf("0x%08lx -> 0x%08lx\n", 0x0C000000, mmu_translate(pt, 0x0C000000));
    // debugf("0x%08lx -> 0x%08lx\n", 0x30000000, mmu_translate(pt, 0x30000000));
    // debugf("0x%08lx -> 0x%08lx\n", 0x40000000, mmu_translate(pt, 0x40000000));
    // void debug_page_table(struct page_table *tab, 
    //                    uint64_t start_virt, 
    //                    uint64_t end_virt, 
    //                    uint64_t start_phys)
    // debug_page_table(pt, MMU_LEVEL_1G);

    debugf("About to set SATP to %016lx\n", SATP_KERNEL);
    // TODO: turn on the MMU when you've written the src/mmu.c functions
    CSR_WRITE("satp", SATP_KERNEL); 
    SFENCE_ALL();
    debugf("MMU enabled\n");

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
    debugf("heap_init() done\n");

    // Call kmalloc() here to ensure it works.
    void *ptr = kmalloc(1024);
    strcpy(ptr, "Hello, world!");
    debugf("kmalloc(1024) = %p\n", ptr);
    debugf("kmalloc(1024) = %s\n", ptr);
    kfree(ptr);
#endif
#ifdef USE_PCI
    pci_init();
#endif
#ifdef USE_VIRTIO
    uint64_t stvec = trampoline_trap_start;
    
	// # 0    - gpregs
    // # 256  - fpregs
    // # 512  - sepc
    // # 520  - sstatus
    // # 528  - sie
    // # 536  - satp
    // # 544  - sscratch
    // # 552  - stvec
    // # 560  - trap_satp
    // # 568  - trap_stack
    // trampoline_thread_start();

    stvec &= ~0x3;
    CSR_WRITE("stvec", trampoline_trap_start);
    debugf("STVEC: 0x%p, 0x%p\n", stvec, trampoline_trap_start);

    Trapframe *sscratch = kzalloc(sizeof(Trapframe) * 0x1000);
    
    CSR_READ(sscratch->sepc, "sepc");
    CSR_READ(sscratch->sstatus, "sstatus");
    CSR_READ(sscratch->sie, "sie");
    CSR_READ(sscratch->satp, "satp");
    CSR_READ(sscratch->stvec, "stvec");
    CSR_READ(sscratch->trap_satp, "satp");
    sscratch->trap_stack = (uint64_t)kmalloc(0x4000);
    CSR_WRITE("sscratch", sscratch);

    virtio_init();
    uint8_t buffer[16] = {0};
    debugf("RNG State Before:");
    for (uint64_t i=0; i<sizeof(buffer)/sizeof(buffer[0]); i++) {
        debugf(" %d ", buffer[i]);
    }
    debugf("\n");

    debugf("RNG init done; about to fill\n");
    rng_fill(buffer, 16);
    debugf("RNG State After:");
    for (uint64_t i=0; i<sizeof(buffer)/sizeof(buffer[0]); i++) {
        debugf(" %d ", buffer[i]);
    }
    
    rng_fill(buffer, 16);
    for (uint64_t i=0; i<sizeof(buffer)/sizeof(buffer[0]); i++) {
        debugf(" %d ", buffer[i]);
    }


    /*
    uint32_t sector[256];
    for (uint64_t i=0; i<sizeof(sector)/sizeof(sector[i]); i++) {
        sector[i] = i;
    }
    block_device_write_sectors(0, sector, 2);
    debugf("Wrote sector:\n");

    for (uint64_t i=0; i<sizeof(sector)/sizeof(sector[i]); i+=8) {
        debugf("%d %d %d %d %d %d %d %d\n",
            sector[i], sector[i+1], sector[i+2], sector[i+3],
            sector[i+4], sector[i+5], sector[i+6], sector[i+7]);
    }
    for (uint64_t i=0; i<sizeof(sector)/sizeof(sector[i]); i++) {
        sector[i] = 0;
    }

    block_device_read_sectors(0, sector, 2);
    debugf("Read sector:\n");

    for (uint64_t i=0; i<sizeof(sector)/sizeof(sector[i]); i+=8) {
        debugf("%d %d %d %d %d %d %d %d\n",
            sector[i], sector[i+1], sector[i+2], sector[i+3],
            sector[i+4], sector[i+5], sector[i+6], sector[i+7]);
    }

    uint32_t buf[64] = {0};
    memset(buf, 0xFF, sizeof(buf));
    block_device_write_bytes(sizeof(uint32_t) * 3, buf, sizeof(uint32_t) * 4);
    memset(buf, 5, sizeof(buf));
    block_device_read_bytes(0, buf, sizeof(uint32_t) * 9);

    for (uint64_t i=0; i<sizeof(buf)/sizeof(buf[i]); i+=8) {
        debugf("%d %d %d %d %d %d %d %d\n",
            buf[i], buf[i+1], buf[i+2], buf[i+3],
            buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
    }
    */
    // TEST GPU
    debugf("GPU init %s\n", gpu_test() ? "successful" : "failed");
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
    VirtioDevice *block_device = virtio_get_block_device(0);
    // minix3_init(block_device, "/");
    vfs_init();

    File *file = vfs_open("/dev/sda/root.txt", 0, O_RDONLY, VFS_TYPE_FILE);
    uint8_t *buffer = kzalloc(2048);
    vfs_read(file, buffer, 1024);
    logf(LOG_INFO, "Read from file /dev/sda/root.txt: %1024s\n", buffer);
    // vfs_print_mounted_devices();


    File *file2 = vfs_open("/home/cosc562/subdir1/subdir2/subdir3/subdir4/subdir5/book1.txt", 0, O_RDONLY, VFS_TYPE_FILE);
    vfs_read(file2, buffer, 1024);
    logf(LOG_INFO, "Read 1024 bytes from file /home/cosc562/subdir1/subdir2/subdir3/subdir4/subdir5/book1.txt: %1024s\n", buffer);
    vfs_read(file2, buffer, 1024);
    logf(LOG_INFO, "Read another 1024 bytes from file /home/cosc562/subdir1/subdir2/subdir3/subdir4/subdir5/book1.txt: %1024s\n", buffer);

    vfs_close(file);
    vfs_close(file2);

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
