/**
 * @file csr.h
 * @author Stephen Marz (sgm@utk.edu)
 * @brief Control and Status Registers as well as other RISC-V related macros
 * @version 0.1
 * @date 2022-05-19
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

// CSR_READ(variable, "register"). Must use quotes for the register name.
#define CSR_READ(var, csr)    __asm__ volatile("csrr %0, " csr : "=r"(var))

// CSR_WRITE("register", variable). Must use quotes for the register name.
#define CSR_WRITE(csr, var)   __asm__ volatile("csrw " csr ", %0" ::"r"(var))

// CSR_CLEAR("register"). Clears a register to 0. Must use quotes for the register name.
#define CSR_CLEAR(csr)  __asm__ volatile("csrw " csr ", zero");

// SFENCE for the SATP register (for MMU).
// Flush the entire TLB
#define SFENCE_ALL()          __asm__ volatile("sfence.vma");
// Flush only TLB entries tagged with ASID
#define SFENCE_ASID(x)        __asm__ volatile("sfence.vma zero, %0" ::"r"(x))
// Flush only TLB entries with the given virtual memory address (VMA)
#define SFENCE_VMA(x)         __asm__ volatile("sfence.vma %0, zero" ::"r"(x))
// Flush only TLB entries with the given ASID that match the given VMA
#define SFENCE(vma, asid)     __asm__ volatile("sfence.vma %0, %1" ::"r"(vma), "r"(asid))

// Return from trap (M = machine, S = supervisor)
#define MRET()                __asm__ volatile("mret")
#define SRET()                __asm__ volatile("sret")

// Waiting defines
#define WFI()                 __asm__ volatile("wfi")
#define WFI_LOOP() \
    do {           \
        WFI();     \
    } while (1)

// STATUS Previous Privilege Mode ([MS]PP)
#define MSTATUS_MPP_BIT                    11
#define MSTATUS_MPP_MACHINE                (3UL << MSTATUS_MPP_BIT)
#define MSTATUS_MPP_SUPERVISOR             (1UL << MSTATUS_MPP_BIT)
#define MSTATUS_MPP_USER                   (0UL << MSTATUS_MPP_BIT)

#define SSTATUS_SPP_BIT                    8
#define SSTATUS_SPP_SUPERVISOR             (1UL << SSTATUS_SPP_BIT)
#define SSTATUS_SPP_USER                   (0UL << SSTATUS_SPP_BIT)

// STATUS Previous Interrupt Enable ([MS]PIE)
#define MSTATUS_MPIE_BIT                   7
#define MSTATUS_MPIE                       (1UL << MSTATUS_MPIE_BIT)

#define SSTATUS_SPIE_BIT                   5
#define SSTATUS_SPIE                       (1UL << SSTATUS_SPIE_BIT)

// STATUS Floating-point System (FS)
#define MSTATUS_FS_BIT                     13
#define MSTATUS_FS_OFF                     (0UL << MSTATUS_FS_BIT)
#define MSTATUS_FS_INITIAL                 (1UL << MSTATUS_FS_BIT)
#define MSTATUS_FS_CLEAN                   (2UL << MSTATUS_FS_BIT)
#define MSTATUS_FS_DIRTY                   (3UL << MSTATUS_FS_BIT)

#define SSTATUS_FS_BIT                     13
#define SSTATUS_FS_OFF                     (0UL << SSTATUS_FS_BIT)
#define SSTATUS_FS_INITIAL                 (1UL << SSTATUS_FS_BIT)
#define SSTATUS_FS_CLEAN                   (2UL << SSTATUS_FS_BIT)
#define SSTATUS_FS_DIRTY                   (3UL << SSTATUS_FS_BIT)

// External Interrupts
#define MEIE_BIT                           11
#define MEIP_BIT                           MEIE_BIT
#define SEIE_BIT                           9
#define SEIP_BIT                           SEIE_BIT
#define MIE_MEIE                           (1UL << MEIE_BIT)
#define MIP_MEIP                           MIE_MEIE
#define MIE_SEIE                           (1UL << SEIE_BIT)
#define MIP_SEIP                           MIE_SEIE
#define SIE_SEIE                           MIE_SEIE
#define SIP_SEIP                           SIE_SEIE

// Timer Interrupts
#define MTIE_BIT                           7
#define MTIP_BIT                           MTIE_BIT
#define STIE_BIT                           5
#define STIP_BIT                           STIE_BIT
#define MIE_MTIE                           (1UL << MTIE_BIT)
#define MIP_MTIP                           MIE_MTIE
#define MIE_STIE                           (1UL << STIE_BIT)
#define MIP_STIP                           MIE_STIE
#define SIE_STIE                           MIE_STIE
#define SIP_STIP                           SIE_STIE

// Software Interrupts
#define MSIE_BIT                           3
#define MSIP_BIT                           MSIE_BIT
#define SSIE_BIT                           1
#define SSIP_BIT                           SSIE_BIT
#define MIE_MSIE                           (1UL << MSIE_BIT)
#define MIP_MSIP                           MIE_MSIE
#define MIE_SSIE                           (1UL << SSIE_BIT)
#define MIP_SSIP                           MIE_SSIE
#define SIE_SSIE                           MIE_SSIE
#define SIP_SSIP                           SIE_SSIE

// Machine Exception (MEX) codes
#define MEX_INSTR_ADDR_MISALIGN            (0)
#define MEX_INSTR_ACCESS_FAULT             (1)
#define MEX_ILLEGAL_INSTR                  (2)
#define MEX_BREAKPOINT                     (3)
#define MEX_LOAD_ADDR_MISALIGN             (4)
#define MEX_LOAD_ACCESS_FAULT              (5)
#define MEX_STORE_ACCESS_MISALIGN          (6)
#define MEX_STORE_ACCESS_FAULT             (7)
#define MEX_ECALL_UMODE                    (8)
#define MEX_ECALL_SMODE                    (9)
#define MEX_ECALL_MMODE                    (11)
#define MEX_INSTR_PAGE_FAULT               (12)
#define MEX_LOAD_PAGE_FAULT                (13)
#define MEX_STORE_PAGE_FAULT               (15)

// All exceptions delegation
#define MEDELEG_ALL                        (0xB1F7UL)

// The general purpose register ABI names
// Example: X7 is index 7 called the T2 register
#define XREG_ZERO                          (0)
#define XREG_RA                            (1)
#define XREG_SP                            (2)
#define XREG_GP                            (3)
#define XREG_TP                            (4)
#define XREG_T0                            (5)
#define XREG_T1                            (6)
#define XREG_T2                            (7)
#define XREG_S0                            (8)
#define XREG_S1                            (9)
#define XREG_A0                            (10)
#define XREG_A1                            (11)
#define XREG_A2                            (12)
#define XREG_A3                            (13)
#define XREG_A4                            (14)
#define XREG_A5                            (15)
#define XREG_A6                            (16)
#define XREG_A7                            (17)
#define XREG_S2                            (18)
#define XREG_S3                            (19)
#define XREG_S4                            (20)
#define XREG_S5                            (21)
#define XREG_S6                            (22)
#define XREG_S7                            (23)
#define XREG_S8                            (24)
#define XREG_S9                            (25)
#define XREG_S10                           (26)
#define XREG_S11                           (27)
#define XREG_T3                            (28)
#define XREG_T4                            (29)
#define XREG_T5                            (30)
#define XREG_T6                            (31)
#define XREG_NUM                           (32)

// Floating-point registers
#define FREG_T0                            (0)
#define FREG_T1                            (1)
#define FREG_T2                            (2)
#define FREG_T3                            (3)
#define FREG_T4                            (4)
#define FREG_T5                            (5)
#define FREG_T6                            (6)
#define FREG_T7                            (7)
#define FREG_S0                            (8)
#define FREG_S1                            (9)
#define FREG_A0                            (10)
#define FREG_A1                            (11)
#define FREG_A2                            (12)
#define FREG_A3                            (13)
#define FREG_A4                            (14)
#define FREG_A5                            (15)
#define FREG_A6                            (16)
#define FREG_A7                            (17)
#define FREG_S2                            (18)
#define FREG_S3                            (19)
#define FREG_S4                            (20)
#define FREG_S5                            (21)
#define FREG_S6                            (22)
#define FREG_S7                            (23)
#define FREG_S8                            (24)
#define FREG_S9                            (25)
#define FREG_S10                           (26)
#define FREG_S11                           (27)
#define FREG_T8                            (28)
#define FREG_T9                            (29)
#define FREG_T10                           (30)
#define FREG_T11                           (31)
#define FREG_NUM                           (32)

// Trap Causes
#define MCAUSE_IS_ASYNC(x)                 (((x) >> 63) & 1)
#define MCAUSE_NUM(x)                      ((x)&0xffUL)

#define SCAUSE_IS_ASYNC(x)                 (((x) >> 63) & 1)
#define SCAUSE_NUM(x)                      ((x)&0xffUL)

// Asynchronous Causes (interrupts)
#define CAUSE_SSIP                         1
#define CAUSE_MSIP                         3

#define CAUSE_STIP                         5
#define CAUSE_MTIP                         7

#define CAUSE_SEIP                         9
#define CAUSE_MEIP                         11

// Synchronous Causes (exceptions)
#define CAUSE_INSTRUCTION_ADDRESS_MISALIGN 0
#define CAUSE_INSTRUCTION_ACCESS_FAULT     1
#define CAUSE_ILLEGAL_INSTRUCTION          2
#define CAUSE_BREAKPOINT                   3
#define CAUSE_LOAD_ADDRESS_MISALIGNED      4
#define CAUSE_LOAD_ACCESS_FAULT            5
#define CAUSE_STORE_AMO_ADDRESS_MISALIGNED 6
#define CAUSE_STORE_AMO_ACCESS_FAULT       7
#define CAUSE_ECALL_U_MODE                 8
#define CAUSE_ECALL_S_MODE                 9
#define CAUSE_ECALL_M_MODE                 11
#define CAUSE_INSTRUCTION_PAGE_FAULT       12
#define CAUSE_LOAD_PAGE_FAULT              13
#define CAUSE_STORE_AMO_PAGE_FAULT         15

// Attributes to support non-returning (NORET) or naked functions
#define ATTR_NAKED_NORET                   __attribute__((naked, noreturn))
#define ATTR_NAKED                         __attribute__((naked))
#define ATTR_NORET                         __attribute__((noreturn))
