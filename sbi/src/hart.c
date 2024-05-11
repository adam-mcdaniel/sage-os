/**
 * @file hart.c
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Hardware thread (HART) related utilities.
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <hart.h>
#include <lock.h>
#include <csr.h>
#include <clint.h>

HartData sbi_hart_data[MAX_ALLOWABLE_HARTS];
static Mutex sbi_hart_lock[MAX_ALLOWABLE_HARTS];

void park();

HartStatus hart_get_status(unsigned int hart) {
    if (hart >= MAX_ALLOWABLE_HARTS) {
        return HS_INVALID;
    }
    return sbi_hart_data[hart].status;
}

bool hart_start(unsigned int hart, unsigned long target, unsigned long scratch, unsigned long satp) {
    bool ret = true;
    if (hart >= MAX_ALLOWABLE_HARTS || !mutex_trylock(sbi_hart_lock + hart)) {
        return false;
    }

    if (sbi_hart_data[hart].status != HS_STOPPED) {
        ret = false;
    }
    else {
        sbi_hart_data[hart].status = HS_STARTING;
        sbi_hart_data[hart].target_address = target;
        sbi_hart_data[hart].scratch = scratch;
        sbi_hart_data[hart].satp = satp;
        clint_set_msip(hart);
    }
    mutex_unlock(sbi_hart_lock + hart);
    return ret;
}

bool hart_stop(unsigned int hart) {
    if (hart >= MAX_ALLOWABLE_HARTS || !mutex_trylock(sbi_hart_lock + hart)) {
        return false;
    }
    if (sbi_hart_data[hart].status != HS_STARTED) {
        mutex_unlock(sbi_hart_lock + hart);
        return false;
    }
    else {
        sbi_hart_data[hart].status = HS_STOPPED;
        CSR_WRITE("mepc", park);
        CSR_WRITE("mstatus", MSTATUS_MPP_MACHINE | MSTATUS_MPIE);
        CSR_WRITE("mie", MIE_MSIE | (hart == 0 ? MIE_MEIE : 0));
        CSR_CLEAR("satp");
        CSR_CLEAR("sscratch");
        CSR_CLEAR("stvec");
        CSR_CLEAR("sepc");
        clint_set_mtimecmp(hart, CLINT_MTIMECMP_INFINITE);
        CSR_CLEAR("mip");
        mutex_unlock(sbi_hart_lock + hart);
        MRET();
    }

    mutex_unlock(sbi_hart_lock + hart);
    return false;
}

void hart_handle_msip(unsigned int hart) {
    // We need a mutex since this can be set asynchronously from us.
    mutex_spinlock(sbi_hart_lock + hart);

    clint_clear_msip(hart);

    // We need to check if the MSIP was due to us wanting to get started.
    if (sbi_hart_data[hart].status == HS_STARTING) {
        // The parameters from sbi_hart_data[] came from the hart_start SVCALL.
        CSR_WRITE("mepc", sbi_hart_data[hart].target_address); 
        CSR_WRITE("mstatus", MSTATUS_MPP_SUPERVISOR | MSTATUS_MPIE | MSTATUS_FS_INITIAL);
        CSR_WRITE("mie", MIE_MEIE | MIE_SSIE | MIE_STIE | MIE_MTIE);
        CSR_WRITE("mideleg", SIP_SEIP | SIP_SSIP | SIP_STIP);
        CSR_WRITE("medeleg", MEDELEG_ALL);
        CSR_WRITE("sscratch", sbi_hart_data[hart].scratch);
        CSR_WRITE("satp", sbi_hart_data[hart].satp);
        sbi_hart_data[hart].status = HS_STARTED;
    }

    mutex_unlock(sbi_hart_lock + hart);
    MRET();
}

