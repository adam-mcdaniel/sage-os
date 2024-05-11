/**
 * @file hart.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief HART related macros, structures, and utilities.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <config.h>
#include <stdbool.h>

/**
 * @brief Describes the status of a given HART.
 * 
 */
typedef enum HartStatus {
    HS_INVALID,  // A HART that doesn't exist
    HS_STOPPED,  // The HART is parked and ready to awaken.
    HS_STARTING, // The HART has received the MSIP, but hasn't actually returned yet.
    HS_STARTED   // The HART is currently running instructions and is not parked.
} HartStatus;

/**
 * @brief The privilege mode to awaken a HART to.
 * 
 */
typedef enum HartPrivMode {
    HPM_USER,
    HPM_SUPERVISOR,
    HPM_HYPERVISOR,
    HPM_MACHINE
} HartPrivMode;

/**
 * @brief The data we need to store to read when an MSIP is generated.
 * 
 */
typedef struct HartData {
    HartStatus status;
    HartPrivMode priv_mode;
    unsigned long target_address;
    unsigned long scratch;
    unsigned long satp;
} HartData;

// HART data to store for the hart_start SVCALL.
extern HartData sbi_hart_data[MAX_ALLOWABLE_HARTS];

/**
 * @brief Get the status of a given HART.
 * 
 * @param hart - the HART to check the value.
 * @return HartStatus - returns the status of the HART or HS_INVALID if the HART isn't present.
 */
HartStatus hart_get_status(unsigned int hart);

/**
 * @brief Start a HART by writing the HartData and sending an MSIP.
 * 
 * @param hart - the HART to awken. The HART must be in STOPPED state, otherwise this will fail.
 * @param target - The target *physical* address to jump to. MEPC is set to this value.
 * @param scratch - The value of the scratch register.
 * @param satp - The value to put into the SATP register (may be 0).
 * @return true - the HART was successfully started
 * @return false - something went wrong.
 */
bool hart_start(unsigned int hart, unsigned long target, unsigned long scratch, unsigned long satp);

/**
 * @brief Only a HART can stop itself, since it won't hear MSIPs in the STARTED state.
 * This function basically resets the HartData structure.
 * 
 * @param hart - The HART to stop.
 * @return true - The HART should not return if it successfully stopped.
 * @return false - The HART could not stop for some reason.
 */
bool hart_stop(unsigned int hart);

/**
 * @brief When MSIPs occur in ctrap.c, they are fowarded here.
 * 
 * @param hart - the hart that received an MSIP
 */
void hart_handle_msip(unsigned int hart);
