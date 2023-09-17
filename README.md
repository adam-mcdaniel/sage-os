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
