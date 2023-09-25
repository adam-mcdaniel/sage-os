**#NetID : ttahmid**
**#NetID : amcdan23**
**#NetID : gmorale1**
**#NetID : jpark78**


# 4's Journal

Every week, you will need to write entries in this journal. Include brief information about:

* any complications you've encountered.
* any interesting information you've learned.
* any complications that you've fixed and how you did it.

Sort your entries in descending order (newest entries at the top).

# 24-September-2023
- `amcdan23`: Added setting up type1 config for pci_init_bridge, also added a function for getting the PCI ecam for a given bus and device, also added to `pci_dispatch_irq`. Merged PCI into master.

- `ttahmid`: `pci-ttahmid` branch - After the last issue was resolved, we were having problem with the BARs for devices being `0xffffffffffffffff`. The issue was, after querying the BAR size by writing 0xFFFFFFFF, we were immediately writing the allocated address to the BAR which resulted in incorrect BAR configurations. Solved the issue by placing a `MEMORY_BARRIER` after writing 0xFFFFFFFF to the BAR and before reading it back, which ensured that the write operation fully completed (and the device had a chance to update the BAR) before the subsequent read operation.

## 22-September-2023
- `amcdan23`: Fixed the aforementioned `FATAL` debug_page_table error and got the ECAM memory allocated.
- `gmorale1`: I've put in the loop and logic for the first steps of enumerating devices. As of writing, when I run `info pci` in the emulator I see 4 bridges and 2 devices, the tablet and keyboard. However, all of the subordinate bus numbers show as zero to qemu. Yet when I use debugf on the structure from the loop, it returns the number I wrote into it. Also, the loop thinks everything (when the header is not 0xFFFF) is a bus. 

## 21-September-2023
- `ttahmid`: Implemented PCI BUS Enumerating in the `pci-ttahmid` branch. Right now it hangs when trying to read the memory at the `ECAM_ADDR_START` which is set to `0x30000000`. Might need to check if MMU maps that address correctly or not. But right now stuck here.
- `ttahmid`: Figured out the previous problem with `ECAM_ADDR_START`: the `mmu_map_range` for PCIe ECAM in `main.c` was commented out. Now after uncommenting this line, that address (`0x30000000`) is being used but running into another problem: `[DEBUG]: debug_page_table: expected 0x30000000, got 0xffffffffffffffff` 
`[FATAL]: debug_page_table: entry 0x180 in page table at 0x80028000 is invalid`
Implemented `pci_enumerate_bus`, `pci_configure_bridge`, `pci_configure_device` in `pci-ttahmid` branch. Need to solve the page_table issue now.

## 17-September-2023
- `amcdan23`: We can now use the MMU and `kmalloc` + `kfree`! Fixed some bugs on how `mmu_map`, `mmu_translate`, `mmu_free` navigate page table entries. It didn't perform the 2 bit shift before using it as a physical pointer. Added many more debug prints. Additionally added some debugging functions for recursively printing the page table entries for all the mapped memory. Eliminated bug that caused physical memory to not map 1:1 with virtual memory in `mmu_map_range`. Fixed physical address offset calculation; previously it was always assuming the page size was 4K in this calculation.

## 15-September-2023
- `jpark78`: Move kernel page table creation and mapping to happen after `page_init()` in `main.c`. Fixed bugs in mmu_map. Fixed a missing conditional in `page_znalloc`. With this update, uncommeing USE_MMU causes a freeze at `CSR_WRITE and SFENCE_ALL`.

## 12-September-2023
- `ttahmid`: Right now with the current mmu implementation, when I uncomment Use_MMU in `config.h` and `make run`, I get this output `Invalid read at addr 0x0, size 8, region '(null)', reason: rejected` in infinite loop. Trying to figure it out.

## 11-September-2023
- `ttahmid`: fixed the warnings in `mmu_map` function in `mmu.c`. Implemented `mmu_free` and `mmu_translate` functions.

## 09-September-2023
- `amcdan23`: fixed bugs in uaccess.c from gaddi's copy_to and copy_from functions that had some bugs in which address indexes they fed to `mmu_translate`, and how it handled the offsets for the data copied from/to the first and last physical pages.

## 08-September-2023
- `jpark78`: implemented mmu_map.

## 02-September-2023
- `amcdan23`: changed the macros to static functions (I think the fact that they use the variable name "index" messed with other bits of code). Fixed that the OS in the master branch wouldn't boot. Rewrote `page_init` and some of `page_nalloc` to be much more readable and to work properly. Implemented `page_free`, `page_count_free`, `page_count_taken`, and created some constant macros for the bookkeeping area's size (with names including memory units like bytes vs. pages).
- `ttahmid`: updated page.c with bookkeeping calculations, page init, page nalloc, page zalloc

## 11-May-2022

EXAMPLE

## 09-May-2022

EXAMPLE
