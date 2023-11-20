# Group \#4's Journal

![OS](OS.png)

- **NetID**: ttahmid
- **NetID**: amcdan23
- **NetID**: gmorale1
- **NetID**: jpark78

Every week, you will need to write entries in this journal. Include brief information about:

* any complications you've encountered.
* any interesting information you've learned.
* any complications that you've fixed and how you did it.

Sort your entries in descending order (newest entries at the top).
# 19-Nov-2023
- `jpark78`: Debugged running the program loaded by the ELF load. Added trap_stack allocation and mapped the process's trap_frame to the user's page table. Also, fixed how the mutex was being used in sched.c. Made sure we were storing the entry point in the process's SEPC. Fixed miscellaneous compiler warnings on types and unused functions.

# 15-Nov-2023

- `gmorale1`: Changed scheduler code so only hart 0 is put in a WFI loop if no more processes are ready to run. Other harts will be stopped.
- `amcdan23`: Read an ELF file from the disk and map its sections into memory with its own page table. Added functions for getting each section for the program from the bytes of an ELF file. Added initialization for the RCB, and added virtual and physical pointers to each of the sections in the `Process` struct.

# 14-Nov-2023
- `jpark78`: Added additional data structures to store processes and keep track of which process is on which hart.

# 13-Nov-2023
- `jpark78`: Added additional fields to RCB. Fixed `generate_unique_pid`.

# 11-Nov-2023

- `amcdan23`: Fixed small bug with the kernel trap frame allocation; fixed compilation errors and changed type names to be consistent with our styling.

# 8-Nov-2023

- `amcdan23`: Created `elf.c` and created functions for parsing the headers and getting the segment data. Added functions for printing out all the information about a file.

# 7-Nov-2023
- `ttahmid`: Added `Resource Control Block` and unique id generation to processes. Added more functions for scheduling like function to choose the next process to run based on CFS, `sched_choose_next`; Function to update the vruntime of a process, `sched_update_vruntime`; Function to handle the timer interrupt for context switching, `sched_handle_timer_interrupt`; as well as template functions for `context_switch`.
- `gmorale1`: added Process-and-Scheduling branch, added `sched.c` and `sched.h`. Implemented methods for the completely fair scheduler, which use a red-black tree. Methods include the scheduler init, adding a process, removing a process, and selecting the next process.

# 5-Nov-2023
- `amcdan23`: Expanded the block driver to work with any block device connected to the machine. Added functionality for: mounting devices on the VFS, opening `File*` objects on arbitrary mounted devices on the VFS (multiple open files on multiple drives simultaneously), reading/writing files on VFS, and automatically populating mounted drives in `/dev` folder on the base drive \#0 at `/`. Wrote `vfs_read`, `vfs_write`, `vfs_init`, `vfs_open`, `vfs_close`, `vfs_mount`, `vfs_print_mounted_devices`, `vfs_print_open_files`, many more private VFS helper functions in `vfs.c`, and rewrote all the block driver functions with devices as parameters in `block.c`. Added a PCI function for getting the Nth device of a given type.

# 4-Nov-2023
- `amcdan23`: Fixed bug where indirect zones are read incorrectly (I accidentally wrote `(uint8_t)*indirect_zone` instead of `(uint8_t*)indirect_zone`, and it still typechecked!). Added callbacks for mapping the filesystem into two maps of inodes and paths. Now we can read the book and the poem into a buffer just by using their `const char *path` string, manipulate them, and write them back! Added struct for convenient representation of a `File`. Added optimization to reduce the number of requests made by the block device to the operating system by saving the inode data after retrieval.
- `jpark78`: Added functions to support stat(2) and link(2). Also added functions to allocate inode and find the next free directory entry. Also added get_parent option to `minix3_get_inode_from_path` to more easily find the inode of the parent.

# 3-Nov-2023
- `amcdan23`: Added functions for enumerating all directory entries, and finding an entry in a directory by its name and parent inode. Created function for writing files based on the function for reading files. Fixed bug where reading/writing to files on zone boundaries had wrong buffer arithmetic. Added functions for traversing file tree with a callback function and some persistent state. Added warnings to log messages.
- `jpark78`: Added functions for finding free inode and zone. Also added function to mark the inode/zone bitmap.

# 2-Nov-2023
- `amcdan23`: Used zone manipulating functions to implement reading files. Only works with direct zones. Added functions for getting file information from inodes (is directory, is file). Also added functions for getting individual directory entries.

# 1-Nov-2023
- `amcdan23`: Created infrastructure for scheduling jobs to VirtioDevices. Added reading in/writing Inodes, fixed the number of sectors calculated to be read/written in block device submodule. Fixed where packet request could sometimes result in an instruction page fault or a regular page fault. Added functions for manipulating zones. Added debug enable/disable flags for the different submodules. 
- `jpark78`: Fix input device init and ISR to receive keyboard, tablet events but traps with instruction page fault when the incoming input is too fast. Fixed a bug in virtio_receive_decriptor_chain where we were setting our internal device_idx equal to the device->idx regardless of the number of descriptors read instead of incrementing it by one. Added is_keybaord and is_tablet to VirtioDevice to differentiate between a keyboard and a tablet. This was prefered over querying the device specific config since we are not able to handle interrupts inside input_device_isr.

# 29-October-2023
- `amcdan23`: Added the minix3 harddrive to the repo, Rewrote `virtio_send_one_descriptor` to be in terms of `virtio_send_descriptor_chain`. Added functions to read in the inode and zone bitmaps in the filesystem.
- `jpark78`: Fixed an issue where `gpu_init` had intermittent failures. Fixed it by turning off IRQ and spinlocking while sending the descriptor, by adding a while loop to wait for the device_idx to catch up, and by removing the error checking code. Even though this fixes a lot of the issues, we still want to figure out why the error checking code is causing it to fail. Also helped with input device initialization by making sure we are setting STVEC before `virtio_init`.
- `gmorale1`: Added input interrupt handler, refactored input driver to allow multiple input devices.

# 28-October-2023
- `ttahmid`: Wrote `set_input_device_config` function for setting up select and subsel. 

# 27-October-2023
- `jpark78`: Got a test graphics to draw succesfully on `gpu_init`. One of the issues was XQuartz not working well with the hydra machine. Another was not setting the offset correctly when trasnferring.

# 26-October-2023
- `jpark78`: Implemented GPU initialization along with helper functions `gpu_send_command` and `gpu_get_display_config`. Was able to get the correct size window.

# 25-October-2023

- `amcdan23`: Got implementation of block device working! Added functions for block device initialization, setting up and sending request packets, writing to and reading from sectors. Added function for chaining virtio descriptors which performs the packet request. Fixed all the warnings! Wrote `block_device_init`, `block_device_send_request`, `block_device_read_sector`, `block_device_write_sector`, `block_device_read_sectors`, `block_device_write_sectors`, `virtio_send_descriptor_chain`, and `virtio_send_descriptor`. Added basic skeleton for filesystem. Went back and added `virtio_receive_descriptor_chain` + related functions for reading linked list of descriptors from device into a buffer.
- `ttahmid`: Wrote `input.c` and `input.h`. Added functions for input device, `input_device_init`, `input_device_interrupt_handler`. Now, need to query the input device through device configuration registers using type 4 capability.

# 24-October-2023

- `amcdan23`: Got RNG working! Added SRETs to trap handler to return correctly instead of instruction faults, added catches for important causes, and changed PLIC threshold to properly catch these. (<--- this was another bug I introduced which I caught later today) Fixed issue where RNG device was using the descriptor index for the queue to notify in successive queries. Added fix where `sscratch` did not point to kernel trap frame; set that up. Then, called the trampolines stvec handler instead of `os_trap_handler`. This fixed the instruction page faults and the trap handler sees that the RNG sent the interrupt every time! Added abstractions for interracting with all the different devices, getting device specific configurations, and sending / receiving descriptors with a device without interacting with the rings directly.

- `jpark78`: Helped with debugging the `os_trap_handler` issue.

- `ttahmid`: Implemented all the functions for block device, `virtio_setup_block_request`, `block_read`, `block_write`. `virtio_submit_and_wait` needs to worked on. Testing needs to be done after RNG is working properly.
- `gmorale`: added initial GPU outline for `gpu.c` and `gpu.h`

# 23-October-2023

- `amcdan23`: Fixed notify register address calculations, fixed how bars were stored in bookkeeping. Successfully got the ISR to change for our RNG device, but now PC gets zeroed when we write here. Fixed issue where notification did not trigger trap handler; now trap handler is triggered when we notify a device.

# 18-October-2023
- `amcdan23`: Added bar pointers directly to the PCI bookkeeping structures, and the virtio bookkeeping structures. Changed how bookkeeping structures were copied so we can add new fields without breaking the implementation. Added reporting for the enumerated PCI device's bookkeeping. Fixed bug where wrong virtio device was being used for the RNG requests.
- `jpark78`: Fixed how we were retrieving the PCI common config, notification structure, and ISR status. We were not using the BAR number to to get those configs.

# 17-October-2023
- `amcdan23`: Fixed bug where bar calculations were including the last 4 bits of the bar (*not used*).
- `jpark78`: In an effort to fix virtio notify, fixed a bug in PCI enumeration where we go down the first bridge we see instead of enumerating all the devices first. Fixed the wrong `PCI_ECAM_END` value. Set the correct bits for the command register during PCI configuration.

# 16-October-2023
- `amcdan23`: Fixed mapped ECAM memory address range. Found that the mapped addresses for the PCI device enumeration was wrong: the devices and bridges were being configured in the wrong order, and the bus + slot were being stored in the wrong part of the address. This fixed the bug where notifying the RNG (writing to the notify register) freezed the OS. Fixed these bugs.

# 15-October-2023
- `amcdan23`: Added checks in `virtio.c` to confirm the descriptors of the device are set; also found that they were not being written properly. This led to the discovery that the capability pointers we were storing in our virtio bookkeeping were wrong. =

# 14-October-2023
- `amcdan23`: Added functions for identifying the virtio devices like the RNG, added fixes to `rng_get_numbers` to store the jobs. Discovered bug with notifying RNG devices and started fix. Added initialization of the virtio devices in `virtio_init`.

# 30-September-2023
- `gmorale1`: Added some initial code for the rng device which should use the driver ring when called.  Also added a getter funciton for getting VirtIO devices from a pci device pointer. Used when we need to find an interrupt from a pci device.
- `amcdan23`: Added `PCIDevice` infrastructure for setting up VirtIO. Created functions for quickly getting and setting vital info about each PCI device and VirtIO config info from the device. Added functions for performing all the `PCIDevice`'s lookups with the static tables. Added this structure to the `VirtioDevice` structure. Changed PCI functions to use these methods for enumerating devices and printing capabilities. Also added methods for managing the `VirtioDevice`s saved by the operating system when initialized. Fixed bug where rings not set properly in `virtio_init`.

- `ttahmid`: Implemented `virtio_init` to set up the `virtio_devices` from `PCIDevice`. Allocated memory for each identified virtio device. Made an initial memory allocation for descriptor table, driver ring, and device ring. Need to set up qsize properly.

- `jpark78`: Implemented `virtio_notify`.
# 29-September-2023
- `amcdan23`: Added some changes suggested by Dr. Marz. Use vectors to store address to configuration structures for each of the devices connected to PCI (one vector that contains all the devices, another 4 for each of the shared IRQ numbers). Added ISR checking to `pci_irq_dispatch`. Now when `pci_irq_dispatch` is called, it only looks through the smaller shared vector , and now it checks the ISR register's `queue_interrupt` and `device_cfg_interrupt` for each device. -- Also added some copy-pasted comments from lab into the `virtio.h` include file for future reference.

# 24-September-2023
- `jpark78`: Fixed a bug with 64b BAR allocation and removed unneeded looping of the function bits in `pci_enumerate_bus`.

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
