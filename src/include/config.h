/**
 * @file config.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Tunable parameters for the SBI
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

// When you create files and make it work, uncomment the following.

#define DEFAULT_LOG_LEVEL 3

#define RUN_INTERNAL_CONSOLE
#define USE_MMU
#define USE_HEAP
#define USE_PCI
#define USE_VIRTIO


// The maximum number of HARTS we can support. The more HARTs,
// the more memory we need to support them. Changing this has
// several implications including memory footprint and MMIO
// strides. Setting this number wrong might crash the SBI.
#define MAX_ALLOWABLE_HARTS       4

// The MTIME register increments 10MHz
#define VIRT_TIMER_FREQ           10000000

// Switches per second
#define CONTEXT_SWITCHES_PER_SEC  50

#define CONTEXT_SWITCH_TIMER      (VIRT_TIMER_FREQ / CONTEXT_SWITCHES_PER_SEC)

// Search parameters for finding the OS_TARGET_MAGIC
#define OS_TARGET_START           0x80010000UL
#define OS_TARGET_END             0x80FFFFF0UL

// The key to search for when trying to find the OS for OS_TARGET_MODE_MAGIC
#define OS_TARGET_MAGIC           0xDEAD0BEEF1CAFE22UL

// Only for OS_TARGET_MODE_JUMP
#define OS_TARGET_JUMP_ADDR       0x80050000UL

#define OS_TARGET_MODE_JUMP       1
#define OS_TARGET_MODE_MAGIC      2

// You can change the way we get to our OS here. If the target mode is
// magic, it will search for the OS_TARGET_MAGIC and jump to the address
// defined by that structure. Otherwise, if this is OS_TARGET_MODE_JUMP,
// it will jump to OS_TARGET_JUMP_ADDR regardless of what is actually there.
#define OS_TARGET_MODE            OS_TARGET_MODE_MAGIC

// The smaller the alignment, the more searching needs to
// be done, and the slower it will be. However, higher alignments
// means that unless the target is properly aligned in the OS, it
// will not be found.
#define OS_TARGET_ALIGNMENT       8

// OS Target information. 
#ifdef __c 
struct os_target {
    unsigned long magic; // OS_TARGET_MAGIC
    unsigned long target_address;
};
#endif




// #define DISABLE_DEBUG
// #define DISABLE_WARNINGS
// #ifdef DISABLE_DEBUG
// #define debugf(...)
// #endif

// #ifdef DISABLE_DEBUG
// #define warnf(...)
// #endif
