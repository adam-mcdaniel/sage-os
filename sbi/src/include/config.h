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

// Use the OLD plic or the new AIA?
#define USE_OLD_PLIC

// The maximum number of HARTS we can support. The more HARTs,
// the more memory we need to support them. Changing this has
// several implications including memory footprint and MMIO
// strides. Setting this number wrong might crash the SBI.
#define MAX_ALLOWABLE_HARTS       8

// The MTIME register increments 10MHz
#define VIRT_TIMER_FREQ           10000000UL

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
struct os_target {
    unsigned long magic; // OS_TARGET_MAGIC
    unsigned long target_address;
};
